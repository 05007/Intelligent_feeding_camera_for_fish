#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

#include "common.h"
#include "opencv2/opencv.hpp"
#include "sample_comm.h"
#include "sample_comm_npu.h"
using namespace std;
using namespace cv;

cv::Mat preprocess_img(cv::Mat &img, int INPUT_W, int INPUT_H) {
    int w, h, x, y;
    float r_w = INPUT_W / (img.cols * 1.0);
    float r_h = INPUT_H / (img.rows * 1.0);
    if (r_h > r_w) {
        w = INPUT_W;
        h = r_w * img.rows;
        x = 0;
        y = (INPUT_H - h) / 2;
    } else {
        w = r_h * img.cols;
        h = INPUT_H;
        x = (INPUT_W - w) / 2;
        y = 0;
    }
    cv::Mat re(h, w, CV_8UC3);
    cv::resize(img, re, re.size(), 0, 0, cv::INTER_LINEAR);
    cv::Mat out(INPUT_H, INPUT_W, CV_8UC3, cv::Scalar(128, 128, 128));
    re.copyTo(out(cv::Rect(x, y, re.cols, re.rows)));
    return out;
}

void transform(const int &ih, const int &iw, const int &oh, const int &ow, float& xmin, float& ymin, float& xmax, float& ymax) {
	float scale = std::min(static_cast<float>(ow) / static_cast<float>(iw), static_cast<float>(oh) / static_cast<float>(ih));
	int nh = static_cast<int>(scale * static_cast<float>(ih));
	int nw = static_cast<int>(scale * static_cast<float>(iw));
	int dh = (oh - nh) / 2;
	int dw = (ow - nw) / 2;
	xmin = (xmin - dw) / scale;
	ymin = (ymin - dh) / scale;
	xmax = (xmax - dw) / scale;
	ymax = (ymax - dh) / scale;
}

static int get_image_file_type(char * pchFileName)
{
    char * pchExt = NULL;
    int s32Ret = 0;

    if(!pchFileName)
    {
        s32Ret = INVALID_IMG_FILE;
        return s32Ret;
    }

    //ignore "." ".." and any non-image files
    if(strcasecmp(pchFileName, ".") == 0 || strcasecmp(pchFileName, "..") == 0)
    {
        s32Ret = INVALID_IMG_FILE;
        return s32Ret;
    }

    pchExt = strrchr(pchFileName, '.');
    if(!pchExt || pchExt == pchFileName)
    {
        s32Ret = INVALID_IMG_FILE;
        return s32Ret;
    }

    //convert to lowercase
   // pchTmp = pchExt;
   // for(; *pchTmp; ++pchTmp) *pchTmp = tolower(*pchTmp);

    if(strcasecmp(pchExt, ".rgb") == 0)
    {
        s32Ret = RGB_RAW_DATA_FILE;
    }
    else if(strcasecmp(pchExt, ".bgr") == 0)
    {
        s32Ret = BGR_RAW_DATA_FILE;
    }
    else if(strcasecmp(pchExt, ".rgbd") == 0)
    {
        s32Ret = RGBD_RAW_DATA_FILE;
    }
    else if(strcasecmp(pchExt, ".jpg") == 0 || strcasecmp(pchExt, ".bmp") == 0 || strcasecmp(pchExt, ".png") == 0)
    {
        s32Ret = JPG_BMP_PNG_IMG_FILE;
    }
    else
    {
        s32Ret = INVALID_IMG_FILE;
    }

    //Need to add RGBD or RGB interleave format later..

    return s32Ret;
}

//For local image test. Need to add RGBD or RGBIntlv format later.
cv::Mat get_jpg_image(char * pchFileName, NPU_IMAGE_S * pstImg, char* inputTensorName, int width, int height)
{
    int s32FileSize = 0;
    char * pBuf = NULL;

    pstImg->u32InputNum = 1;
    //strcpy(pstImg->astInputImg[0].achTensorName, inputTensorName);
    //pstImg->astInputImg[0].bPreIfcProcess = SC_TRUE;

	cv::Mat sample_resized;
	//read image and resize it, TBD
	cv::Mat src_img = cv::imread(pchFileName, cv::IMREAD_ANYCOLOR);
	src_img = preprocess_img(src_img, width, height);
	if(src_img.empty())
	{
	   printf("Imread file %s failed!\r\n", pchFileName);
	   return cv::Mat();
	}
	cv::Size input_geometry = cv::Size(width, height);
	if(src_img.size() != input_geometry)
		cv::resize(src_img, sample_resized, input_geometry);
	else
		sample_resized = src_img;

	unsigned short u16Stride = ALIGNED_256B(sample_resized.cols);
	s32FileSize = u16Stride * sample_resized.rows * sample_resized.channels();
	pBuf = (char*)malloc(s32FileSize);
	if(pBuf == NULL)
	{
		printf("Malloc %d bytes failed.\n", s32FileSize);
		return cv::Mat();
	}

	cv::Mat channels[3];
	cv::split(sample_resized, channels);
	char *pchR = (char *)channels[2].data;
	char *pchG = (char *)channels[1].data;
	char *pchB = (char *)channels[0].data;

	for(int i = 0;i< sample_resized.rows;i++)
	{
		for(int j = 0;j< sample_resized.cols;j++)
		{
			pBuf[i*u16Stride + j] 				  = pchR[i*sample_resized.cols + j];
			pBuf[(i+sample_resized.rows)*u16Stride + j]    = pchG[i*sample_resized.cols + j];
			pBuf[(i+sample_resized.rows*2)*u16Stride + j]  = pchB[i*sample_resized.cols + j];
		}
		for(int j = sample_resized.cols;j< u16Stride;j++)
		{
			pBuf[i*u16Stride + j] 				  = 0;
			pBuf[(i+sample_resized.rows)*u16Stride + j]    = 0;
			pBuf[(i+sample_resized.rows*2)*u16Stride + j]  = 0;
		}
	}

	pstImg->astInputImg[0].stImages[0].enType= SVP_IMG_RGB;
	pstImg->astInputImg[0].stImages[0].au64VirAddr[0] = (uintptr_t)pBuf;
	pstImg->astInputImg[0].stImages[0].au64PhyAddr[0] = 0;
	pstImg->astInputImg[0].stImages[0].au64PhyAddr[1] = 0;
	pstImg->astInputImg[0].stImages[0].au64PhyAddr[2] = 0;
	pstImg->astInputImg[0].stImages[0].au64PhyAddr[3] = 0;


    printf("Get image %s..\n", pchFileName);
    return sample_resized;
}

//For local image test. Need to add RGBD or RGBIntlv format later.
int get_local_image(char * pchFileName, NPU_IMAGE_S * pstImg, char* inputTensorName, int width, int height)
{
    unsigned int u32FileType = 0;
    int s32FileSize = 0;
    char * pBuf = NULL;

    pstImg->u32InputNum = 1;
    //strcpy(pstImg->astInputImg[0].achTensorName, inputTensorName);
    //pstImg->astInputImg[0].bPreIfcProcess = SC_TRUE;

    u32FileType = get_image_file_type(pchFileName);
    if(u32FileType == RGB_RAW_DATA_FILE || u32FileType == BGR_RAW_DATA_FILE || u32FileType == RGBD_RAW_DATA_FILE)
    {
        ifstream file(pchFileName, ios::in|ios::binary|ios::ate);
        cout<<"1"<<endl;
        if(file.is_open())
        {
            s32FileSize = file.tellg();
            file.seekg(0, ios::beg);
            pBuf = NULL;
            pBuf = (char*)malloc(s32FileSize);
            if(pBuf == NULL)
            {
                printf("Malloc %d bytes failed.\n", s32FileSize);
                return -1;
            }

            file.read(pBuf, s32FileSize);
            file.close();

            if(u32FileType == RGBD_RAW_DATA_FILE)
            {
                pstImg->astInputImg[0].stImages[0].enType = SVP_IMG_RGBD; //Indicates that there's D data
            }
            else
            {
                pstImg->astInputImg[0].stImages[0].enType = SVP_IMG_RGB;
            }

            //No need to set w/h/s since ifc.json will set these parameters.
            pstImg->astInputImg[0].stImages[0].au64PhyAddr[0] = 0;
            pstImg->astInputImg[0].stImages[0].au64VirAddr[0] = (uintptr_t)pBuf;
        }
    }
    else if(u32FileType == JPG_BMP_PNG_IMG_FILE)
    {
        cv::Mat sample_resized;
        //read image and resize it, TBD
        cv::Mat src_img = cv::imread(pchFileName, cv::IMREAD_ANYCOLOR);
        // cout<<src_img<<endl;
        src_img = preprocess_img(src_img, width, height);
        if(src_img.empty())
        {
           printf("Imread file %s failed!\r\n", pchFileName);
           return -1;
        }
        cv::Size input_geometry = cv::Size(width, height);
        if(src_img.size() != input_geometry)
            cv::resize(src_img, sample_resized, input_geometry);
        else
            sample_resized = src_img;

		unsigned short u16Stride = ALIGNED_256B(sample_resized.cols);
        s32FileSize = u16Stride * sample_resized.rows * sample_resized.channels();
        pBuf = (char*)malloc(s32FileSize);
		if(pBuf == NULL)
        {
            printf("Malloc %d bytes failed.\n", s32FileSize);
            return -1;
        }
        cout<<1<<endl;
        int ret;
        ret = SC_MPI_SYS_MmzAlloc(&pstImg->astInputImg[0].stImages[0].au64PhyAddr[0], (SC_VOID **)&pstImg->astInputImg[0].stImages[0].au64VirAddr[0], NULL, SC_NULL, s32FileSize);
        if (ret != SC_SUCCESS)
        {
            printf("Mmz Alloc fail,Error(%#x)\n", ret);
            return -1;
        }
        cout<<2<<endl;
        // auto pBuf=&pstImg->astInputImg[0].stImages[0].au64VirAddr[0];
        cv::Mat channels[3];
        cv::split(sample_resized, channels);
        char *pchR = (char *)channels[2].data;
		char *pchG = (char *)channels[1].data;
		char *pchB = (char *)channels[0].data;
        cout<<3<<endl;
		for(int i = 0;i< sample_resized.rows;i++)
		{
			for(int j = 0;j< sample_resized.cols;j++)
			{
				pBuf[i*u16Stride + j] 				  = pchR[i*sample_resized.cols + j];
				pBuf[(i+sample_resized.rows)*u16Stride + j]    = pchG[i*sample_resized.cols + j];
				pBuf[(i+sample_resized.rows*2)*u16Stride + j]  = pchB[i*sample_resized.cols + j];
			}
			for(int j = sample_resized.cols;j< u16Stride;j++)
			{
				pBuf[i*u16Stride + j] 				  = 0;
				pBuf[(i+sample_resized.rows)*u16Stride + j]    = 0;
				pBuf[(i+sample_resized.rows*2)*u16Stride + j]  = 0;
			}
    	}
        cout<<4<<endl;
        //add
        pstImg->astInputImg[0].u32BatchNum = 1;
        pstImg->astInputImg[0].stImages[0].u32Height = height;
        pstImg->astInputImg[0].stImages[0].u32Width = width;
        //end
        pstImg->astInputImg[0].stImages[0].enType= SVP_IMG_RGB;
        pstImg->astInputImg[0].stImages[0].au64VirAddr[0] = (uintptr_t)pBuf;
        // pstImg->astInputImg[0].stImages[0].au64PhyAddr[0] = 0;
        // pstImg->astInputImg[0].stImages[0].au64PhyAddr[1] = 0;
        // pstImg->astInputImg[0].stImages[0].au64PhyAddr[2] = 0;
        // pstImg->astInputImg[0].stImages[0].au64PhyAddr[3] = 0;
    }
    else
    {
        printf("Invalid file type: %d\n", u32FileType);
        return -1;
    }

    printf("Get image %s..\n", pchFileName);
    return 0;
}
