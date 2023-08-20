/*
* Company:	smart
* Author: 	Smart Software Team
* Date:	2020/11/14
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "postProcess.h"

int entry_index(int h, int w, int c, int byteUnit, output_tensor &pTensorInfo)
{
	/*
	int n = location / (layer_w*layer_h);
	int loc = location % (layer_w*layer_h);
	return (n*layer_w*layer_h*(4 + 80 + 1) + entry* layer_w*layer_h + loc);
	*/
	int tensor_k_size_norm = pTensorInfo.u32KSizeNorm;
	int tensor_k_ddr_step = pTensorInfo.u32KStep;
	int tensor_row_ddr_step = pTensorInfo.u32RowStep;
	int tensor_k_norm_num = pTensorInfo.u32KNormNum;
	int tensor_last = pTensorInfo.u32KSizeLast;
	//int tensor_last_valid = pTensorInfo.u32OriChannel -tensor_k_norm_num*tensor_k_size_norm;

	int AddressOffset = (c / tensor_k_size_norm)*tensor_k_ddr_step + h*tensor_row_ddr_step;
	int memoryMigration = (c <tensor_k_norm_num*tensor_k_size_norm) ? \
		(AddressOffset + c%tensor_k_size_norm*byteUnit + w*tensor_k_size_norm*byteUnit) : (AddressOffset + (c - tensor_k_norm_num*tensor_k_size_norm)*byteUnit + w*tensor_last*byteUnit);
	return memoryMigration / byteUnit;
}

int chw_entry_index(int h, int w, int c, output_tensor &pTensorInfo)
{
	/*
	int n = location / (layer_w*layer_h);
	int loc = location % (layer_w*layer_h);
	return (n*layer_w*layer_h*(4 + 80 + 1) + entry* layer_w*layer_h + loc);
	*/
	int tensorWidth = pTensorInfo.u32Width;
	int tensorHeight = pTensorInfo.u32Height;

	int memoryMigration = c*tensorWidth*tensorHeight+h*tensorWidth + w;
	return memoryMigration;
}

// int getImgID(char *name)
// {
//      	if(name == NULL)
// 		return 0;

//      	int len = strlen(name);

//         char *pStart = name;
//         char *pEnd = name+len-1;

//         char *pNumStart = NULL;
//         char *pNumEnd = NULL;

//         while(pEnd > pStart) {
//                 if(*pEnd >='0' && *pEnd<='9') {
//                     pNumEnd = pEnd;
//                     break;
//                 }

//                 pEnd--;
//         }

//         if(pNumEnd == NULL) {
//                 return 0;
//         }

//         while(pEnd >=pStart) {
//                 if(*pEnd < '0' || *pEnd >'9')
//                         break;
//                 pEnd--;
//         }

//         pNumStart = pEnd+1;

//         char buf[64];
// 	    if((unsigned int)(pNumEnd-pNumStart+1) >= sizeof(buf))
// 		   return 0;

//         memset(buf, 0, sizeof(buf));
//         memcpy(buf,pNumStart, pNumEnd-pNumStart+1);

//         int val = atoi(buf);

// 	return val;
// }
