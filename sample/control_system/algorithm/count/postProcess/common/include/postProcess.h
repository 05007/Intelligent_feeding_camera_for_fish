/* 
 *
 *  Created on: Jan 27, 2021
 *      Author: Smart Software Team
 */
#ifndef __POST_PROCESS_H__
#define __POST_PROCESS_H__

#include "sc_npu.h"

typedef NPU_TENSOR_S output_tensor;

typedef struct {
	unsigned int classId;
	float confidence;
}classification_output;

typedef struct{
	float x, y, w, h;
}box;

typedef struct{
	box bbox;
	int classes;
	float* prob;
	float* mask;
	float objectness;
	int sort_class;
}detection;

typedef struct {
	float x;
	float y;
	float w;
	float h;
	unsigned int classId;
	float confidence;
}detection_output;

int entry_index(int h, int w, int c, int byteUnit, output_tensor &pTensorInfo);

int chw_entry_index(int h, int w, int c, output_tensor &pTensorInfo);

// int getImgID(char *name);

#endif /* __POST_PROCESS_H__ */
