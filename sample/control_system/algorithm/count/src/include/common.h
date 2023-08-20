#ifndef __COMMON_H
#define __COMMON_H

#include "sc_npu.h"
#include "opencv2/opencv.hpp"

#define RGB_RAW_DATA_FILE     1
#define BGR_RAW_DATA_FILE     2
#define JPG_BMP_PNG_IMG_FILE  3
#define RGBD_RAW_DATA_FILE    4
#define INVALID_IMG_FILE      (-1)

#define ALIGNED_256B(x) ((x)%256 == 0 ? (x) : (((x)/256 + 1) * 256))

cv::Mat preprocess_img(cv::Mat &img, int INPUT_W, int INPUT_H);
void transform(const int &ih, const int &iw, const int &oh, const int &ow, float& xmin, float& ymin, float& xmax, float& ymax);

int get_local_image(char * pchFileName, NPU_IMAGE_S * pstImg, char* inputTensorName, int width, int height);
cv::Mat get_jpg_image(char * pchFileName, NPU_IMAGE_S * pstImg, char* inputTensorName, int width, int height);

#endif
