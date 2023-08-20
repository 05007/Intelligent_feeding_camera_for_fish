/*
 * run_classfication.cpp
 *
 *  Created on: Dec 29, 2020
 *      Author: Smart Software Team
 */

#include "run_solo.hpp"

// #define OUTPUT_INTEGER  // OUTPUT_FLOAT

// #define COUNT_WIDTH 416
// #define COUNT_HEIGHT 416
#define COUNT_WIDTH 1600
#define COUNT_HEIGHT 1200
// #define COUNT_WIDTH 400
// #define COUNT_HEIGHT 300
using namespace std;
SC_VOID SAMPLE_VOU_SYS_Exit(void)
{
    SC_MPI_VO_Exit();
    SC_MPI_SYS_Exit();
    SC_MPI_VB_Exit();
}

#define ALIGN_BACK(x, a)        ((a) * (((x + a -1) / (a))))
#define SAMPLE_CHECK_RET(express,name)\
    do {\
        SC_S32 Ret;\
        Ret = express;\
        if (Ret != SC_SUCCESS) {\
            printf("%s failed at %s : LINE: %d with %#x!\n",name, __FUNCTION__,__LINE__,Ret);\
            SAMPLE_VOU_SYS_Exit();\
            return Ret;\
            }\
        }while(0)

typedef struct
{
    int left;
    int top;
    int right;
    int bottom;
}PointRect_S;
SC_S32 SAMPLE_VO_PlanToSemi(SC_U8 *pY, SC_S32 yStride,
    SC_U8 *pU, SC_S32 uStride,
    SC_U8 *pV, SC_S32 vStride,
    SC_S32 picWidth, SC_S32 picHeight, PIXEL_FORMAT_E enPixFrm)
{
    SC_S32 i;
    SC_U8 *pTmpU, *ptu;
    SC_U8 *pTmpV, *ptv;

    SC_S32 s32HafW = uStride >> 1 ;
    SC_S32 s32HafH;

    if(enPixFrm == PIXEL_FORMAT_YVU_SEMIPLANAR_422)
    {
        s32HafH = picHeight;
    }
    else
    {
        s32HafH = picHeight >> 1 ;
    }

    SC_S32 s32Size = s32HafW * s32HafH;

    pTmpU = (SC_U8 *)malloc( s32Size );
    ptu = pTmpU;
    pTmpV = (SC_U8 *)malloc( s32Size );
    ptv = pTmpV;

    memcpy(pTmpU, pU, s32Size);
    memcpy(pTmpV, pV, s32Size);

    for(i = 0; i<s32Size >> 1; i++)
    {
        *pU++ = *pTmpV++;
        *pU++ = *pTmpU++;

    }
    for(i = 0; i<s32Size >> 1; i++)
    {
        *pV++ = *pTmpV++;
        *pV++ = *pTmpU++;
    }

    free( ptu );
    free( ptv );

    return SC_SUCCESS;
}

SC_S32  Mat_TO_NPUIMG(cv::Mat cv_img, SAMPLE_SVP_NPU_PARAM_S *pstNpuParam,NPU_IMAGE_S *pstImg)
{
    SC_U32 u32Width = pstImg->astInputImg[0].stImages[0].u32Width;
    SC_U32 u32Height = pstImg->astInputImg[0].stImages[0].u32Height;
    cv_img = preprocess_img(cv_img, u32Width, u32Height);
    cv::Mat sample_resized;
    //read image and resize it, TBD
    cv::Size input_geometry = cv::Size(u32Width, u32Height);
    if(cv_img.size() != input_geometry)
        cv::resize(cv_img, sample_resized, input_geometry);
    else
        sample_resized = cv_img;
    cv::Mat channels[3];
    cv::split(sample_resized, channels);

    unsigned short u16Stride = ALIGNED_256B(u32Width);
    SC_CHAR *pBuf = (SC_CHAR *)((SC_U32)pstNpuParam->PictMem.u64VirAddr);
    SC_CHAR *pchR = (SC_CHAR *)channels[2].data;
    SC_CHAR *pchG = (SC_CHAR *)channels[1].data;
    SC_CHAR *pchB = (SC_CHAR *)channels[0].data;

    for(int i = 0; i < u32Height; i++)
    {
        for(int j = 0; j < u32Width; j++)
        {
            pBuf[i * u16Stride + j]                 = pchR[i * u32Width + j];
            pBuf[(i + u32Height)*u16Stride + j]    = pchG[i * u32Width + j];
            pBuf[(i + u32Height * 2)*u16Stride + j]  = pchB[i * u32Width + j];
        }
        for(int j = u32Width; j < u16Stride; j++)
        {
            pBuf[i * u16Stride + j]                 = 0;
            pBuf[(i + u32Height)*u16Stride + j]    = 0;
            pBuf[(i + u32Height * 2)*u16Stride + j]  = 0;
        }
    }

    pstImg->astInputImg[0].stImages[0].au64VirAddr[0] = (uintptr_t)(pstNpuParam->PictMem.u64VirAddr);
    pstImg->astInputImg[0].stImages[0].au64PhyAddr[0] = (uintptr_t)(pstNpuParam->PictMem.u64PhyAddr);
    pstImg->astInputImg[0].stImages[0].au64PhyAddr[1] = (uintptr_t)(pstNpuParam->PictMem.u64PhyAddr + u16Stride *
            u32Height);
    pstImg->astInputImg[0].stImages[0].au64PhyAddr[2] = (uintptr_t)(pstNpuParam->PictMem.u64PhyAddr + 2 * u16Stride *
            u32Height);
    pstImg->astInputImg[0].stImages[0].au64PhyAddr[3] = 0;


    return SC_SUCCESS;
}
SC_S32 MAT_RGB_TO_YUV420P(cv::Mat cv_img,SC_U8 *pY, SC_U8 *pU, SC_U8 *pV,
    SC_U32 width, SC_U32 height,SC_U32 stride, SC_U32 stride2){
        //read image and resize it, TBD
        if(cv_img.empty())
        {
           return -1;
        }
        SC_U8 *pDst_y=pY , *pDst_u=pU, *pDst_v=pV;
        cv::Mat sample_resized;
        cv::Size input_geometry = cv::Size(width, height);
        if(cv_img.size() != input_geometry)
            cv::resize(cv_img, sample_resized, input_geometry);
        else
            sample_resized = cv_img;

        cv::Mat channels[3];
        cv::split(sample_resized, channels);
        SC_CHAR *pchR = (SC_CHAR *)channels[2].data;
        SC_CHAR *pchG = (SC_CHAR *)channels[1].data;
        SC_CHAR *pchB = (SC_CHAR *)channels[0].data;
        // RGB 转 YUV420P
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                int index = i * width + j;
                unsigned char r = pchR[index];
                unsigned char g = pchG[index];
                unsigned char b = pchB[index];
                pDst_y[index] = (SC_U8)(0.299 * r + 0.587 * g + 0.114 * b);
                pDst_u[(i >> 1) * (width >> 1) + (j >> 1)] = (SC_U8)(-0.169 * r - 0.331 * g + 0.5 * b + 128);
                pDst_v[(i >> 1) * (width >> 1) + (j >> 1)]  = (SC_U8)(0.5 * r - 0.419 * g - 0.081 * b + 128);
            }
        }
        return 0;
    }

SC_S32 SAMPLE_VOU_SYS_Init(void)
{
    VB_CONFIG_S stVbConf = {0};
    SC_U32      u32BlkSize;

    SC_MPI_VO_Exit();

    SC_MPI_SYS_Exit();

    SC_MPI_VB_Exit();

    stVbConf.u32MaxPoolCnt = 2;

    u32BlkSize = COMMON_GetPicBufferSize(1920, 1080, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 6;

    u32BlkSize = COMMON_GetPicBufferSize(720, 576, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 6;

    if(SAMPLE_COMM_SYS_Init(&stVbConf)==SC_SUCCESS)
        cout<<"SAMPLE_COMM_SYS_Init"<<endl;
    return SC_SUCCESS;
}

//classification
int run_solo( char *pcModelName, char * pcSrcFile){
    // SC_CHAR *pcSrcFile = "./data/npu_image/yuv420p/dog_bike_car.yuv";
    // SC_CHAR *pcModelName = "./data/npu_model/detection/yolov3_caffe_yuv420p.npubin";
    SAMPLE_SVP_NPU_MODEL_S                   s_stYolov3Model = {0};
    SAMPLE_SVP_NPU_PARAM_S                   s_stYolov3NpuParam = {0};
    SC_S32 s32Ret = SC_FAILURE;

    if(access(pcSrcFile, F_OK) != 0)
    {
        cout<<"Error,pcSrcFile not exist!"<<endl;
        return -1;
    }

    if(access(pcModelName, F_OK) != 0)
    {
        cout<<"Error,pcModelName not exist !"<<endl;
        return -1;
    }

    s_stYolov3Model.stModelConf.enCbMode = SVP_NPU_NO_CB;
    s_stYolov3Model.stModelConf.enPriority = SVP_NPU_PRIORITY_NORMAL;

    /*Yolov3 Load model*/
    s32Ret = SAMPLE_COMM_SVP_NPU_LoadModel(pcModelName, &s_stYolov3Model);
    if(SC_SUCCESS != s32Ret){
        cout<<"Error,SAMPLE_COMM_SVP_NPU_LoadModel failed!"<<endl;
        return -1;
    }

    /*Yolov3 parameter initialization*/
    s32Ret = SAMPLE_COMM_SVP_NPU_ParamInit(&s_stYolov3Model, &s_stYolov3NpuParam);
    if(SC_SUCCESS != s32Ret){
        cout<<"Error,SAMPLE_SVP_NPU_Cnn_ParamInit failed!"<<endl;
        return -1;
    }
    /*Yolov3 src data*/
    NPU_IMAGE_S yolov3_image;
    yolov3_image.u32InputNum = 1;
    yolov3_image.astInputImg[0].u32BatchNum = 1;
    yolov3_image.astInputImg[0].stImages[0].enType = SVP_IMG_RGB;
    yolov3_image.astInputImg[0].stImages[0].u32Height = COUNT_HEIGHT;
    yolov3_image.astInputImg[0].stImages[0].u32Width = COUNT_WIDTH;
    cv::Mat src_img = cv::imread(pcSrcFile, cv::IMREAD_ANYCOLOR);
    
    if(src_img.empty())
    {
        printf("Imread file %s failed!\r\n", pcSrcFile);
        return -1;
    }
    s32Ret = Mat_TO_NPUIMG(src_img, &s_stYolov3NpuParam, &yolov3_image);
    if(SC_SUCCESS != s32Ret){
        cout<<"Error,SAMPLE_SVP_NPU_FillSrcData failed!"<<endl;
        return -1;
    }
    /*NPU process*/
/*NPU process*/
    SVP_NPU_HANDLE hSvpNpuHandle = 0;
    s32Ret =  SC_MPI_SVP_NPU_Forward(&hSvpNpuHandle, &yolov3_image, &s_stYolov3NpuParam.stIntMem, &s_stYolov3Model.stModel,
    &s_stYolov3NpuParam.stOutMem, SC_TRUE);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SC_MPI_SVP_NPU_Forward err 0x%x!\n", s32Ret);
        return -1;
    }

    /*Software process and Get result*/
    NPU_TENSOR_S outTensor = s_stYolov3Model.stModel.astDstTensor[0];

    vector<classification_output> output;

    // SC_UINTPTR_T virtAddr = stoutMem.u64VirAddr;
    SC_UINTPTR_T virtAddr = s_stYolov3NpuParam.stOutMem.u64VirAddr;
    
    ///////////////////
    output_tensor *pTensor = &outTensor;

    int oriChannels = pTensor->u32OriChannels;
    char* buffQuantCurrentChar=NULL;
    short* buffQuantCurrentShort=NULL;


    __attribute__((__unused__)) int last_tensorBank=0;
    __attribute__((__unused__)) int last_tensorBankOffset=0;

    int offsetQuant=0;
    double sum = 0;

    for(int n = 0;n < 1; n++){
        output_tensor tensor = pTensor[n];
        int channel = tensor.u32OriChannels;
        int width = tensor.u32Width;
        int height = tensor.u32Height;
        int precision=tensor.u32Precision;
        int tensor_size=tensor.u32Size;
        int bits=precision/8;
        // cout<<bits<<endl;

        if(precision==8){
            buffQuantCurrentChar = (char*)((char*)virtAddr+offsetQuant);
        }else{
            buffQuantCurrentShort = (short*)((short*)virtAddr+offsetQuant);
        }

        offsetQuant+=tensor_size*bits;

        last_tensorBank=tensor.u32Bank;
        last_tensorBankOffset=tensor.u32Offset;
        for (int h = 0; h < height; h++)
        {
            for (int w = 0; w < width; w++)
            {
                for (int c = 0; c < channel; c++)
                {
                    if(precision==8){
                        int index_transform_quant = entry_index(h, w, c, sizeof(char), tensor);
                        double quanVal = (buffQuantCurrentChar[index_transform_quant] - tensor.s32ZeroPoint)*tensor.dScaleFactor;
                        sum += quanVal;
                    }else{
                        int index_transform_quant = entry_index(h, w, c, sizeof(short), tensor);
                        double quanVal = (buffQuantCurrentShort[index_transform_quant] - tensor.s32ZeroPoint)*tensor.dScaleFactor;
                        sum += quanVal;
                    }
                }
            }
        }
    }
    if(sum<0) sum=0;
    //std::cout << "sum : " << sum << std::endl;
    // uart_log(uart1,"img_sum: "+to_string(sum));
    // std::cout << "vo_start" << std::endl;
    // //====================vo_test====================
    // SC_U32 i = 0;
    // SC_S32 VoDev = 0;
    // SC_S32 VoLayer = 0;
    // SC_S32 VoChnNum = 1;
    // SC_U32 DisplayBufLen = 0;
    // VO_PUB_ATTR_S stPubAttr;
    // VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    // SIZE_S  stFrameSize;
    // SIZE_S  stDevSize = {1920, 1080};
    // VO_CHN_ATTR_S astChnAttr[VO_MAX_CHN_NUM];

    // SAMPLE_CHECK_RET(SAMPLE_VOU_SYS_Init(), "SAMPLE_VOU_SYS_Init");
    // cout<<1<<endl;
    // stPubAttr.u32BgColor = 0x808080;
    // stPubAttr.enIntfType = VO_INTF_HDMI;
    // stPubAttr.enIntfSync = VO_OUTPUT_1080P60;
    // cout<<2<<endl;
    // SAMPLE_CHECK_RET(SC_MPI_VO_SetPubAttr(VoDev, &stPubAttr), "SC_MPI_VO_SetPubAttr");
    // cout<<3<<endl;
    // SAMPLE_CHECK_RET(SC_MPI_VO_GetPubAttr(VoDev, &stPubAttr), "SC_MPI_VO_GetPubAttr");
    // cout<<4<<endl;
    // #if 0
    // /* ENABLE VO DEV */
    // SAMPLE_CHECK_RET(SC_MPI_VO_Enable(VoDev), "SC_MPI_VO_Enable");

    // SAMPLE_CHECK_RET(SC_MPI_VO_Disable(VoDev), "SC_MPI_VO_Disable");

    // SAMPLE_CHECK_RET(SC_MPI_VO_SetPubAttr(VoDev, &stPubAttr), "SC_MPI_VO_SetPubAttr");
    // #endif

    // /* ENABLE VO DEV */
    // SAMPLE_CHECK_RET(SC_MPI_VO_Enable(VoDev), "SC_MPI_VO_Enable");

    // /* SET VO DISPLAY BUFFER LENGTH */
    // DisplayBufLen = 3;
    // SAMPLE_CHECK_RET(SC_MPI_VO_SetDisplayBufLen(VoDev, DisplayBufLen), "SC_MPI_VO_SetDisplayBufLen");

    // SAMPLE_CHECK_RET(SC_MPI_VO_GetDisplayBufLen(VoDev, &DisplayBufLen), "SC_MPI_VO_GetDisplayBufLen");

    // SAMPLE_VO_GetUserLayerAttr(&stLayerAttr, &stDevSize);
    // stLayerAttr.u32DispFrmRt = 30;

    // SAMPLE_CHECK_RET(SC_MPI_VO_SetVideoLayerAttr(VoLayer, &stLayerAttr), "SC_MPI_VO_SetVideoLayerAttr");

    // SAMPLE_CHECK_RET(SC_MPI_VO_GetVideoLayerAttr(VoLayer, &stLayerAttr), "SC_MPI_VO_GetVideoLayerAttr");

    // #if 0
    // /* ENABLE VO LAYER */
    // SAMPLE_CHECK_RET(SC_MPI_VO_EnableVideoLayer(VoLayer), "SC_MPI_VO_EnableVideoLayer");

    // SAMPLE_CHECK_RET(SC_MPI_VO_DisableVideoLayer(VoLayer), "SC_MPI_VO_DisableVideoLayer");

    // SAMPLE_CHECK_RET(SC_MPI_VO_SetVideoLayerAttr(VoLayer, &stLayerAttr), "SC_MPI_VO_SetVideoLayerAttr");
    // #endif

    // /* ENABLE VO LAYER */
    // SAMPLE_CHECK_RET(SC_MPI_VO_EnableVideoLayer(VoLayer), "SC_MPI_VO_EnableVideoLayer");

    // /* SET AND ENABLE VO CHN */
    // SAMPLE_VO_GetUserChnAttr(astChnAttr, &stDevSize, VoChnNum);

    // for (i = 0; i < VoChnNum; i++)
    // {
    //     SAMPLE_CHECK_RET(SC_MPI_VO_SetChnAttr(VoLayer, i, &astChnAttr[i]), "SC_MPI_VO_SetChnAttr");

    //     SAMPLE_CHECK_RET(SC_MPI_VO_GetChnAttr(VoLayer, i, &astChnAttr[i]), "SC_MPI_VO_GetChnAttr");

    //     #if 0
    //     SAMPLE_CHECK_RET(SC_MPI_VO_EnableChn(VoLayer, i), "SC_MPI_VO_EnableChn");

    //     SAMPLE_CHECK_RET(SC_MPI_VO_DisableChn(VoLayer, i), "SC_MPI_VO_DisableChn");

    //     SAMPLE_CHECK_RET(SC_MPI_VO_SetChnAttr(VoLayer, i, &astChnAttr[i]), "SC_MPI_VO_SetChnAttr");
    //     #endif

    //     SAMPLE_CHECK_RET(SC_MPI_VO_EnableChn(VoLayer, i), "SC_MPI_VO_EnableChn");

    // }
    // cout<<5<<endl;
    // stFrameSize.u32Width = stDevSize.u32Width;
    // stFrameSize.u32Height = stDevSize.u32Height;

    // VB_BLK hBlkHdl;
    // SC_U32 u32Size;
    // SC_U32 u32SrcWidth;
    // SC_U32 u32SrcHeight;
    // VB_POOL Pool;
    // VIDEO_FRAME_INFO_S stUserFrame;
    // VB_POOL_CONFIG_S stVbPoolCfg;
    // SC_U32 u32LumaSize = 0;
    // SC_U32 u32ChromaSize = 0;
    // struct timeval tv_frame;
    // SC_S32 pre_tvdiff = 0;
    // memset(&stUserFrame, 0x0, sizeof(VIDEO_FRAME_INFO_S));

    // u32SrcWidth = stDevSize.u32Width;
    // u32SrcHeight = stDevSize.u32Height;

    // u32Size = ALIGN_BACK(u32SrcWidth, 512) * u32SrcHeight * 3;
    // memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));
    // stVbPoolCfg.u64BlkSize = u32Size;
    // stVbPoolCfg.u32BlkCnt = 20;
    // stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
    // Pool = SC_MPI_VB_CreatePool(&stVbPoolCfg);
    // if (Pool == VB_INVALID_POOLID)
    // {
    //     printf("Maybe you not call sys init\n");
    //     return SC_NULL;
    // }
    // cout<<6<<endl;
    // stUserFrame.stVFrame.enField = VIDEO_FIELD_FRAME;
    // stUserFrame.stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    // stUserFrame.stVFrame.enPixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    // stUserFrame.stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    // stUserFrame.stVFrame.enColorGamut = COLOR_GAMUT_BT709;
    // stUserFrame.stVFrame.u32Width = u32SrcWidth;
    // stUserFrame.stVFrame.u32Height = u32SrcHeight;
    // stUserFrame.stVFrame.u32Stride[0] = ALIGN_BACK(u32SrcWidth, 512);
    // stUserFrame.stVFrame.u32Stride[1] = ALIGN_BACK(u32SrcWidth, 512);
    // stUserFrame.stVFrame.u32Stride[2] = ALIGN_BACK(u32SrcWidth, 512);
    // stUserFrame.stVFrame.u32TimeRef = 0;
    // stUserFrame.stVFrame.u64PTS = 0;
    // stUserFrame.stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;

    // u32LumaSize =  stUserFrame.stVFrame.u32Stride[0] * u32SrcHeight;
    
    // u32ChromaSize =  stUserFrame.stVFrame.u32Stride[0] * u32SrcHeight / 4;
    

    // while (1)
    // {
    //     usleep(100);
        
    //     //pre_tvdiff = (time_diff(&tv_frame) + pre_tvdiff);
    //     //printf("time_diff(&tv_frame):%d pre_tvdiff:%d\n", time_diff(&tv_frame), pre_tvdiff);

    //     hBlkHdl = SC_MPI_VB_GetBlock(Pool, u32Size, NULL);
    //     if (hBlkHdl == VB_INVALID_HANDLE)
    //     {
    //         printf("[VOU_MST_File2VO] get vb fail!!!\n");
    //         sleep(1);
    //         continue;
    //     }

    //     stUserFrame.u32PoolId = SC_MPI_VB_Handle2PoolId(hBlkHdl);
    //     stUserFrame.stVFrame.u64PhyAddr[0] = SC_MPI_VB_Handle2PhysAddr( hBlkHdl );
    //     stUserFrame.stVFrame.u64PhyAddr[1] = stUserFrame.stVFrame.u64PhyAddr[0] + u32LumaSize;
    //     stUserFrame.stVFrame.u64PhyAddr[2] = stUserFrame.stVFrame.u64PhyAddr[1] + u32ChromaSize;

    //     stUserFrame.stVFrame.u64VirAddr[0] = (SC_UL)SC_MPI_SYS_Mmap(stUserFrame.stVFrame.u64PhyAddr[0], u32Size);
    //     stUserFrame.stVFrame.u64VirAddr[1] = (SC_UL)(stUserFrame.stVFrame.u64VirAddr[0]) + u32LumaSize;
    //     stUserFrame.stVFrame.u64VirAddr[2] = (SC_UL)(stUserFrame.stVFrame.u64VirAddr[1]) + u32ChromaSize;
    //     s32Ret = MAT_RGB_TO_YUV420P(src_img,(SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[0],
    //                     (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[1], (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[2],
    //                     stUserFrame.stVFrame.u32Width, stUserFrame.stVFrame.u32Height,
    //                     stUserFrame.stVFrame.u32Stride[0], stUserFrame.stVFrame.u32Stride[1] >> 1);
    //     if(s32Ret == SC_SUCCESS)
    //     {
    //         SAMPLE_VO_PlanToSemi( (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[0], stUserFrame.stVFrame.u32Stride[0],
    //             (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[1], stUserFrame.stVFrame.u32Stride[1],
    //             (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[2], stUserFrame.stVFrame.u32Stride[1],
    //             stUserFrame.stVFrame.u32Width,    stUserFrame.stVFrame.u32Height,
    //             stUserFrame.stVFrame.enPixelFormat);
    //     }
    //     else{
    //         cout<<"error MAT_RGB_TO_YUV420P"<<endl;
    //         break;
    //     }
    //     stUserFrame.stVFrame.u64PTS += 40000;
    //     stUserFrame.stVFrame.u32TimeRef += 40000;
    //     for (i = 0; i < VoChnNum; i++)
    //     {
    //         s32Ret = SC_MPI_VO_SendFrame(VoLayer, i, &stUserFrame, 0);
    //     }
    //     SC_MPI_VB_ReleaseBlock(hBlkHdl);
    //     SC_MPI_SYS_Munmap((SC_VOID *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[0], u32Size);
    // }
    //====================vo_test====================

    s32Ret = SAMPLE_COMM_SVP_NPU_ParamDeinit(&s_stYolov3NpuParam);
    s32Ret = SAMPLE_COMM_SVP_NPU_UnloadModel(&s_stYolov3Model);
    return s32Ret;
}

int run_solo_2( char *npuFile, char * imgFile){
	SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VO_LAYER           voLayer = 0;
    PointRect_S        rect[16]    = {};
    VIDEO_FRAME_INFO_S stFrameInfo ={};

    /*config vi*/
    memset(&stViConfig, 0, sizeof(stViConfig));
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.stFps.s32SrcFrameRate = 30;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.stFps.s32DstFrameRate = 15;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 6;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 3;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }
	SAMPLE_SVP_NPU_MODEL_S                   s_stSsdModel = {0};
    SAMPLE_SVP_NPU_PARAM_S                   s_stSsdNpuParam = {0};
	SC_VOID *pResult = SC_NULL;
    SC_S32   s32ResultNum;

    NPU_IMAGE_S ssd_image;
    ssd_image.u32InputNum = 1;
    ssd_image.astInputImg[0].u32BatchNum = 1;
    ssd_image.astInputImg[0].stImages[0].enType = SVP_IMG_RGB;
    ssd_image.astInputImg[0].stImages[0].u32Height = COUNT_HEIGHT;
    ssd_image.astInputImg[0].stImages[0].u32Width = COUNT_WIDTH;

    IVE_IMAGE_S stFrm = {};

    IVE_SRC_IMAGE_S resize_image = {0};
    resize_image.u32Width = COUNT_WIDTH;
    resize_image.u32Height = COUNT_HEIGHT;
    resize_image.enType = IVE_IMAGE_TYPE_U8C3_PLANAR;
    resize_image.au32Stride[0] =ALIGNED_256B(resize_image.u32Width);
    resize_image.au32Stride[1] =ALIGNED_256B(resize_image.u32Width);
    resize_image.au32Stride[2] =ALIGNED_256B(resize_image.u32Width);


    SC_U32 u32Size = resize_image.au32Stride[0] * resize_image.u32Height * 3;
    s32Ret = SC_MPI_SYS_MmzAlloc(&resize_image.au64PhyAddr[0], (SC_VOID **)&resize_image.au64VirAddr[0], NULL, SC_NULL, u32Size);
    /*start vi*/
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    stVoConfig.enVoIntfType = VO_INTF_HDMI;
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    s_stSsdModel.stModelConf.enCbMode = SVP_NPU_NO_CB;
    s_stSsdModel.stModelConf.enPriority = SVP_NPU_PRIORITY_NORMAL;

    /*SSD Load model*/
    s32Ret = SAMPLE_COMM_SVP_NPU_LoadModel(npuFile, &s_stSsdModel);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("NPU load model err(%#x)\n", s32Ret);
        goto EXIT1;
    }

    /*SSD parameter initialization*/
    s32Ret = SAMPLE_COMM_SVP_NPU_ParamInit(&s_stSsdModel, &s_stSsdNpuParam);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("NPU param init err(%#x)\n", s32Ret);
        goto EXIT1;
    }

    // #define SSD_WIDTH 300
    // #define SSD_HEIGHT 300
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
        goto EXIT1;
    }
    resize_image.au32Stride[1] = resize_image.au32Stride[0];
    resize_image.au64VirAddr[1] = resize_image.au64VirAddr[0] + resize_image.au32Stride[0] * resize_image.u32Height;
    resize_image.au64PhyAddr[1] = resize_image.au64PhyAddr[0] + resize_image.au32Stride[0] * resize_image.u32Height;
    resize_image.au32Stride[2] = resize_image.au32Stride[1];
    resize_image.au64VirAddr[2] = resize_image.au64VirAddr[1] + resize_image.au32Stride[1] * resize_image.u32Height;
    resize_image.au64PhyAddr[2] = resize_image.au64PhyAddr[1] + resize_image.au32Stride[1] * resize_image.u32Height;
    while(1)
    {
        memset(&stFrameInfo, 0, sizeof(VIDEO_FRAME_INFO_S));
        s32Ret = SC_MPI_VI_GetChnFrame(ViPipe, ViChn, &stFrameInfo, -1);
        if(SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VI_GetFrame errno = %#x \n", s32Ret);
            goto EXIT2;
        }

        s32Ret = yv12_2_rgb(&stFrameInfo, &stFrm);
        if(s32Ret)
        {
            SAMPLE_PRT("yv12_2_rgb failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT1;
        }

        IVE_HANDLE hIve;
        IVE_RESIZE_CTRL_S stIveCtrl;
        stIveCtrl.u16Num = 1;
        SC_BOOL bInstance = SC_TRUE;
        s32Ret = SC_MPI_IVE_Resize(&hIve, &stFrm, &resize_image, &stIveCtrl, bInstance);
        if(s32Ret != SC_SUCCESS)
        {
            printf("SC_MPI_IVE_csc err 0x%x!\n", s32Ret);
            continue;
        }

        SC_BOOL bBlock = SC_TRUE;
        SC_BOOL bFinish;
        SC_S32 ret = SC_MPI_IVE_Query(hIve, &bFinish, bBlock);
        while (SC_ERR_IVE_QUERY_TIMEOUT == ret)
        {
            usleep(100);
            ret = SC_MPI_IVE_Query(hIve, &bFinish, bBlock);
        }

        free_vi_frm_bgr(&stFrm);

        ssd_image.astInputImg[0].stImages[0].au64PhyAddr[0] = resize_image.au64PhyAddr[0];
        ssd_image.astInputImg[0].stImages[0].au64PhyAddr[1] = resize_image.au64PhyAddr[1];
        ssd_image.astInputImg[0].stImages[0].au64PhyAddr[2] = resize_image.au64PhyAddr[2];

        ssd_image.astInputImg[0].stImages[0].au64VirAddr[1] = resize_image.au64VirAddr[0];
        ssd_image.astInputImg[0].stImages[0].au64VirAddr[2] = resize_image.au64VirAddr[1];
        ssd_image.astInputImg[0].stImages[0].au64VirAddr[3] = resize_image.au64VirAddr[2];



        /*NPU process*/
       SVP_NPU_HANDLE hSvpNpuHandle = 0;
       s32Ret =  SC_MPI_SVP_NPU_Forward(&hSvpNpuHandle, &ssd_image, &s_stSsdNpuParam.stIntMem, &s_stSsdModel.stModel,
        &s_stSsdNpuParam.stOutMem, SC_TRUE);
        if(s32Ret != SC_SUCCESS)
        {
            printf("SC_MPI_SVP_NPU_Forward err 0x%x!\n", s32Ret);
            continue;
        }

       /*Software process and Get result*/
        NPU_TENSOR_S outTensor = s_stSsdModel.stModel.astDstTensor[0];

		vector<classification_output> output;

		// SC_UINTPTR_T virtAddr = stoutMem.u64VirAddr;
		SC_UINTPTR_T virtAddr = s_stSsdNpuParam.stOutMem.u64VirAddr;
		
		///////////////////
		output_tensor *pTensor = &outTensor;

		int oriChannels = pTensor->u32OriChannels;
		
		vector<double> classiferVector;
		int point_number=0;

		char* buffQuantCurrentChar=NULL;
		short* buffQuantCurrentShort=NULL;


		__attribute__((__unused__)) int last_tensorBank=0;
		__attribute__((__unused__)) int last_tensorBankOffset=0;

		int offsetQuant=0;
		double sum = 0;

		for(int n = 0;n < 1; n++){
			output_tensor tensor = pTensor[n];
			int channel = tensor.u32OriChannels;
			int width = tensor.u32Width;
			int height = tensor.u32Height;
			int precision = tensor.u32Precision;
			int tensor_size = tensor.u32Size;
			int bits = precision/8;
			// cout<<bits<<endl;

			if(precision==8){
				buffQuantCurrentChar = (char*)((char*)virtAddr+offsetQuant);
			}else{
				buffQuantCurrentShort = (short*)((short*)virtAddr+offsetQuant);
			}

			offsetQuant+=tensor_size*bits;

			last_tensorBank=tensor.u32Bank;
			last_tensorBankOffset=tensor.u32Offset;

			point_number+=width*height*channel;
			for (int h = 0; h < height; h++)
			{
				for (int w = 0; w < width; w++)
				{
					for (int c = 0; c < channel; c++)
					{
						if(precision==8){
							int index_transform_quant = entry_index(h, w, c, sizeof(char), tensor);
							double quanVal = (buffQuantCurrentChar[index_transform_quant] - tensor.s32ZeroPoint)*tensor.dScaleFactor;
							sum += quanVal;
						}else{
							int index_transform_quant = entry_index(h, w, c, sizeof(short), tensor);
							double quanVal = (buffQuantCurrentShort[index_transform_quant] - tensor.s32ZeroPoint)*tensor.dScaleFactor;
							sum += quanVal;
						}
					}
				}
			}

		}
        msg_log(msg_common,__func__,"sum : " + to_string(sum));

        s32Ret = SC_MPI_VO_SendFrame(voLayer, VoChn, &stFrameInfo, -1);
        if(s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VO_SendFrame failed!\n");
            goto EXIT2;
        }

        SC_MPI_VI_ReleaseChnFrame(ViPipe, ViChn, &stFrameInfo);

        if(s32Ret)
        {
            break;
        }
    }

    PAUSE();

    
    s32Ret = SAMPLE_COMM_SVP_NPU_ParamDeinit(&s_stSsdNpuParam);
    s32Ret = SAMPLE_COMM_SVP_NPU_UnloadModel(&s_stSsdModel);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
	free_vi_frm_bgr(&resize_image);
    SAMPLE_COMM_SYS_Exit();
	return s32Ret;
}

/******************************************************************************
* function : Dcnn print
******************************************************************************/
SC_S32 GET_SVP_NPU_Detection_Result(SAMPLE_SVP_NPU_DETECTION_OUTPUT *pResult, SC_S32 s32ResultNum)
{
    string yolo_msg;
    SC_FLOAT center_x,center_y;
    if(0 == s32ResultNum)
    {
        // msg_log(msg_error,__func__,"SAMPLE_SVP_NPU_YOLOV3_PrintResult is NULL\n");
        return 0;
    }
    else
    {
        if(SC_NULL == pResult)
        {
            //msg_log(msg_error,__func__,"pResult is NULL");
            return SC_FAILURE;
        }
        msg_log(msg_common,__func__,"+++++++++++++++++++++++++");
        for(int i = 0; i < s32ResultNum; ++ i)
        {
            center_x=(pResult[i].x+pResult[i].w)/2.0;
            center_y=(pResult[i].y+pResult[i].h)/2.0;
            yolo_msg="ClassID:"+to_string(pResult[i].classId)+" x:"+to_string(pResult[i].x)+" y:"+to_string(pResult[i].y)
                +" w:"+to_string(pResult[i].w)+" h:"+to_string(pResult[i].h)+" confidence:"+to_string(pResult[i].confidence)
                +"c_x"+to_string(center_x)+"c_y"+to_string(center_y)+"\n";
            msg_log(msg_common,__func__,yolo_msg);
        }
    }

    return SC_SUCCESS;
}

int run_solo_4( char *npuFile, char * imgFile){
    msg_log(msg_tip,__func__,"Start flow detection!!");
    int f=1;
	SAMPLE_SVP_NPU_MODEL_S                   s_stSsdModel = {0};
    SAMPLE_SVP_NPU_PARAM_S                   s_stSsdNpuParam = {0};
    SC_S32 s32Ret; 
    s_stSsdModel.stModelConf.enCbMode = SVP_NPU_NO_CB;
    s_stSsdModel.stModelConf.enPriority = SVP_NPU_PRIORITY_NORMAL;
    cv::Mat frame;
    cv::VideoCapture cap(0);
    Trajectory_solving* trajectory = new Trajectory_solving(3);
    // 打开默认的摄像头（设备编号为 0）
    if (!cap.isOpened()) {
        std::cerr << "Failed to open camera!" << std::endl;
        goto EXIT;
    }
    /*SSD Load model*/
    s32Ret = SAMPLE_COMM_SVP_NPU_LoadModel(npuFile, &s_stSsdModel);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("NPU load model err(%#x)\n", s32Ret);
        goto EXIT1;
    }

    /*SSD parameter initialization*/
    s32Ret = SAMPLE_COMM_SVP_NPU_ParamInit(&s_stSsdModel, &s_stSsdNpuParam);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("NPU param init err(%#x)\n", s32Ret);
        goto EXIT2;
    }
    // #define SSD_WIDTH 300
    // #define SSD_HEIGHT 300

    while(1)
    {
	// 从摄像头读取一帧数据
	    cap >> frame;
        if(frame.empty()) {
            msg_log(msg_error,__func__,"The input image is empty, please judge whether the camera is normal!");
            sleep(1);
            continue;
        }
        NPU_IMAGE_S yolov3_image;
        yolov3_image.u32InputNum = 1;
        yolov3_image.astInputImg[0].u32BatchNum = 1;
        yolov3_image.astInputImg[0].stImages[0].enType = SVP_IMG_RGB;
        yolov3_image.astInputImg[0].stImages[0].u32Height = 416;
        yolov3_image.astInputImg[0].stImages[0].u32Width = 416;
        s32Ret = Mat_TO_NPUIMG(frame, &s_stSsdNpuParam, &yolov3_image);
        if(SC_SUCCESS != s32Ret){
            cout<<"Error,SAMPLE_SVP_NPU_FillSrcData failed!"<<endl;
            return -1;
        }
        /*NPU process*/
       SVP_NPU_HANDLE hSvpNpuHandle = 0;
       s32Ret =  SC_MPI_SVP_NPU_Forward(&hSvpNpuHandle, &yolov3_image, &s_stSsdNpuParam.stIntMem, &s_stSsdModel.stModel,
        &s_stSsdNpuParam.stOutMem, SC_TRUE);
        if(s32Ret != SC_SUCCESS)
        {
            msg_log(msg_error,__func__,"SC_MPI_SVP_NPU_Forward err 0x%x!"+to_string(s32Ret));
            continue;
        }

        // msg_log(msg_common,__func__,"get_yolov3_result:");
        /*Software process and Get result*/
        SC_VOID *pResult = SC_NULL;
        SC_S32   s32ResultNum;
        SAMPLE_SVP_NPU_YOLOVX_GetResult(s_stSsdModel.stModel.astDstTensor,s_stSsdModel.stModel.u16DstNum,
            (uintptr_t)s_stSsdNpuParam.stOutMem.u64VirAddr, &pResult, &s32ResultNum, 3,
            yolov3_image.astInputImg[0].stImages[0].u32Width, yolov3_image.astInputImg[0].stImages[0].u32Height);
        // msg_log(msg_common,__func__,"Yolov3 result:");
        SAMPLE_SVP_NPU_DETECTION_OUTPUT *yolo_result=(SAMPLE_SVP_NPU_DETECTION_OUTPUT *)pResult;
        sc_s32 people_nums=0;
        SC_FLOAT center_x,center_y;
        // s32Ret = GET_SVP_NPU_Detection_Result((SAMPLE_SVP_NPU_DETECTION_OUTPUT *)pResult, s32ResultNum);
        if( 0 == s32ResultNum || yolo_result==SC_NULL){
            trajectory->loss_frame();
        }
        else{
            for(int i = 0; i < s32ResultNum; ++ i){
                if(yolo_result[i].classId==0 && yolo_result[i].confidence > 0.5){ //结果为人且置信度大于0.5
                    center_x=(yolo_result[i].x+yolo_result[i].w)/2.0;
                    center_y=(yolo_result[i].y+yolo_result[i].h)/2.0;
                    ++people_nums;
                }
            }
            if(people_nums > 0){
                trajectory->next_frame(center_x,center_y,people_nums);
            }else trajectory->loss_frame();
        }
    }

    PAUSE();
   
EXIT2:
    s32Ret = SAMPLE_COMM_SVP_NPU_ParamDeinit(&s_stSsdNpuParam);
EXIT1:
    s32Ret = SAMPLE_COMM_SVP_NPU_UnloadModel(&s_stSsdModel);
EXIT:
     // 释放摄像头资源
    cap.release();
	return s32Ret;
}

int run_solo_5( char *npuFile, char * imgFile){

	SAMPLE_SVP_NPU_MODEL_S                   s_stSsdModel = {0};
    SAMPLE_SVP_NPU_PARAM_S                   s_stSsdNpuParam = {0};
    SC_S32 s32Ret; 
    s_stSsdModel.stModelConf.enCbMode = SVP_NPU_NO_CB;
    s_stSsdModel.stModelConf.enPriority = SVP_NPU_PRIORITY_NORMAL;

    cv::Mat frame;
    cv::VideoCapture cap(0);
    // 打开默认的摄像头（设备编号为 0）
    if (!cap.isOpened()) {
        std::cerr << "Failed to open camera!" << std::endl;
        goto EXIT;
    }
    /*SSD Load model*/
    s32Ret = SAMPLE_COMM_SVP_NPU_LoadModel(npuFile, &s_stSsdModel);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("NPU load model err(%#x)\n", s32Ret);
        goto EXIT1;
    }

    /*SSD parameter initialization*/
    s32Ret = SAMPLE_COMM_SVP_NPU_ParamInit(&s_stSsdModel, &s_stSsdNpuParam);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("NPU param init err(%#x)\n", s32Ret);
        goto EXIT2;
    }
   
    
    // #define SSD_WIDTH 300
    // #define SSD_HEIGHT 300
    while(1)
    {
	// 从摄像头读取一帧数据
	    cap >> frame;
        NPU_IMAGE_S yolov3_image;
        yolov3_image.u32InputNum = 1;
        yolov3_image.astInputImg[0].u32BatchNum = 1;
        yolov3_image.astInputImg[0].stImages[0].enType = SVP_IMG_RGB;
        yolov3_image.astInputImg[0].stImages[0].u32Height = COUNT_HEIGHT;
        yolov3_image.astInputImg[0].stImages[0].u32Width = COUNT_WIDTH;

        s32Ret = Mat_TO_NPUIMG(frame, &s_stSsdNpuParam, &yolov3_image);
        if(SC_SUCCESS != s32Ret){
            cout<<"Error,SAMPLE_SVP_NPU_FillSrcData failed!"<<endl;
            return -1;
        }
        /*NPU process*/
       SVP_NPU_HANDLE hSvpNpuHandle = 0;
       s32Ret =  SC_MPI_SVP_NPU_Forward(&hSvpNpuHandle, &yolov3_image, &s_stSsdNpuParam.stIntMem, &s_stSsdModel.stModel,
        &s_stSsdNpuParam.stOutMem, SC_TRUE);
        if(s32Ret != SC_SUCCESS)
        {
            msg_log(msg_error,__func__,"SC_MPI_SVP_NPU_Forward err 0x%x!"+to_string(s32Ret));
            continue;
        }
               /*Software process and Get result*/
        NPU_TENSOR_S outTensor = s_stSsdModel.stModel.astDstTensor[0];

		vector<classification_output> output;

		// SC_UINTPTR_T virtAddr = stoutMem.u64VirAddr;
		SC_UINTPTR_T virtAddr = s_stSsdNpuParam.stOutMem.u64VirAddr;
		
		///////////////////
		output_tensor *pTensor = &outTensor;

		int oriChannels = pTensor->u32OriChannels;
		
		vector<double> classiferVector;
		int point_number=0;

		char* buffQuantCurrentChar=NULL;
		short* buffQuantCurrentShort=NULL;


		__attribute__((__unused__)) int last_tensorBank=0;
		__attribute__((__unused__)) int last_tensorBankOffset=0;

		int offsetQuant=0;
		double sum = 0;

		for(int n = 0;n < 1; n++){
			output_tensor tensor = pTensor[n];
			int channel = tensor.u32OriChannels;
			int width = tensor.u32Width;
			int height = tensor.u32Height;
			int precision = tensor.u32Precision;
			int tensor_size = tensor.u32Size;
			int bits = precision/8;
			// cout<<bits<<endl;

			if(precision==8){
				buffQuantCurrentChar = (char*)((char*)virtAddr+offsetQuant);
			}else{
				buffQuantCurrentShort = (short*)((short*)virtAddr+offsetQuant);
			}

			offsetQuant+=tensor_size*bits;

			last_tensorBank=tensor.u32Bank;
			last_tensorBankOffset=tensor.u32Offset;

			point_number+=width*height*channel;
			for (int h = 0; h < height; h++)
			{
				for (int w = 0; w < width; w++)
				{
					for (int c = 0; c < channel; c++)
					{
						if(precision==8){
							int index_transform_quant = entry_index(h, w, c, sizeof(char), tensor);
							double quanVal = (buffQuantCurrentChar[index_transform_quant] - tensor.s32ZeroPoint)*tensor.dScaleFactor;
							sum += quanVal;
						}else{
							int index_transform_quant = entry_index(h, w, c, sizeof(short), tensor);
							double quanVal = (buffQuantCurrentShort[index_transform_quant] - tensor.s32ZeroPoint)*tensor.dScaleFactor;
							sum += quanVal;
						}
					}
				}
			}

		}
        if(sum<0) sum=0;
        msg_log(msg_common,__func__,"sum : " + to_string(sum));
        // uart_log(uart1,"sum : " + to_string(sum));
        sleep(30);
    }
    PAUSE();
   
EXIT2:
    s32Ret = SAMPLE_COMM_SVP_NPU_ParamDeinit(&s_stSsdNpuParam);
EXIT1:
    s32Ret = SAMPLE_COMM_SVP_NPU_UnloadModel(&s_stSsdModel);
EXIT:
     // 释放摄像头资源
    cap.release();
	return s32Ret;
}
