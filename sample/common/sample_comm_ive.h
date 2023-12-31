#ifndef __SAMPLE_COMM_IVE_H__
#define __SAMPLE_COMM_IVE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "sc_debug.h"
#include "sc_comm_ive.h"
#include "mpi_ive.h"
#include "sample_comm.h"

#define VIDEO_WIDTH 352
#define VIDEO_HEIGHT 288
#define IVE_ALIGN 16
#define IVE_CHAR_CALW 8
#define IVE_CHAR_CALH 8
#define IVE_CHAR_NUM (IVE_CHAR_CALW *IVE_CHAR_CALH)
#define IVE_FILE_NAME_LEN 256
#define IVE_RECT_NUM   20
#define VPSS_CHN_NUM 2

#define SAMPLE_ALIGN_BACK(x, a)     ((a) * (((x) / (a))))

typedef struct scSAMPLE_IVE_SWITCH_S
{
    SC_BOOL bVenc;
    SC_BOOL bVo;
} SAMPLE_IVE_SWITCH_S;

typedef struct scSAMPLE_IVE_RECT_S
{
    POINT_S astPoint[4];
} SAMPLE_IVE_RECT_S;

typedef struct scSAMPLE_RECT_ARRAY_S
{
    SC_U16 u16Num;
    SAMPLE_IVE_RECT_S astRect[IVE_RECT_NUM];
} SAMPLE_RECT_ARRAY_S;

typedef struct scIVE_LINEAR_DATA_S
{
    SC_S32 s32LinearNum;
    SC_S32 s32ThreshNum;
    POINT_S *pstLinearPoint;
} IVE_LINEAR_DATA_S;

typedef struct scSAMPLE_IVE_DRAW_RECT_MSG_S
{
    VIDEO_FRAME_INFO_S stFrameInfo;
    SAMPLE_RECT_ARRAY_S stRegion;
} SAMPLE_IVE_DRAW_RECT_MSG_S;

//free mmz
#define IVE_MMZ_FREE(phy,vir)\
    do{\
        if ((0 != (phy)) && (0 != (vir)))\
        {\
            SC_MPI_SYS_MmzFree((phy),(SC_VOID *)(SC_UL)(vir));\
            (phy) = 0;\
            (vir) = 0;\
        }\
    }while(0)

#define IVE_CLOSE_FILE(fp)\
    do{\
        if (NULL != (fp))\
        {\
            fclose((fp));\
            (fp) = NULL;\
        }\
    }while(0)

#define SAMPLE_PAUSE()\
    do {\
        printf("---------------press Enter key to exit!---------------\n");\
        (void)getchar();\
    } while (0)
#define SAMPLE_CHECK_EXPR_RET(expr, ret, fmt...)\
do\
{\
    if(expr)\
    {\
        SAMPLE_PRT(fmt);\
        return (ret);\
    }\
}while(0)
#define SAMPLE_CHECK_EXPR_GOTO(expr, label, fmt...)\
do\
{\
    if(expr)\
    {\
        SAMPLE_PRT(fmt);\
        goto label;\
    }\
}while(0)

#define SAMPLE_COMM_IVE_CONVERT_64BIT_ADDR(Type,Addr) (Type*)(SC_UL)(Addr)

/******************************************************************************
* function : Mpi init
******************************************************************************/
SC_VOID SAMPLE_COMM_IVE_CheckIveMpiInit(SC_VOID);
/******************************************************************************
* function : Mpi exit
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_IveMpiExit(SC_VOID);
/******************************************************************************
* function :Read file
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_ReadFile(IVE_IMAGE_S *pstImg, FILE *pFp);
/******************************************************************************
* function :Write file
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_WriteFile(IVE_IMAGE_S *pstImg, FILE *pFp);
/******************************************************************************
* function :Calc stride
******************************************************************************/
SC_U16 SAMPLE_COMM_IVE_CalcStride(SC_U32 u32Width, SC_U8 u8Align);
/******************************************************************************
* function : Copy blob to rect
******************************************************************************/
SC_VOID SAMPLE_COMM_IVE_BlobToRect(IVE_CCBLOB_S *pstBlob, SAMPLE_RECT_ARRAY_S *pstRect,
    SC_U16 u16RectMaxNum, SC_U16 u16AreaThrStep,
    SC_U32 u32SrcWidth, SC_U32 u32SrcHeight,
    SC_U32 u32DstWidth, SC_U32 u32DstHeight);
/******************************************************************************
* function : Create ive image
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_CreateImage(IVE_IMAGE_S *pstImg, IVE_IMAGE_TYPE_E enType,
    SC_U32 u32Width, SC_U32 u32Height);
/******************************************************************************
* function : Create memory info
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_CreateMemInfo(IVE_MEM_INFO_S *pstMemInfo, SC_U32 u32Size);
/******************************************************************************
* function : Create ive image by cached
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_CreateImageByCached(IVE_IMAGE_S *pstImg,
    IVE_IMAGE_TYPE_E enType, SC_U32 u32Width, SC_U32 u32Height);
/******************************************************************************
* function : Create IVE_DATA_S
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_CreateData(IVE_DATA_S *pstData, SC_U32 u32Width, SC_U32 u32Height);
/******************************************************************************
* function : Init Vb
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_VbInit(PIC_SIZE_E *paenSize, SIZE_S *pastSize, SC_U32 u32VpssChnNum);
/******************************************************************************
* function : Dma frame info to  ive image
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_DmaImage(VIDEO_FRAME_INFO_S *pstFrameInfo, IVE_DST_IMAGE_S *pstDst, SC_BOOL bInstant);

/******************************************************************************
* function : Call vgs to fill rect
******************************************************************************/
SC_S32 SAMPLE_COMM_VGS_FillRect(VIDEO_FRAME_INFO_S *pstFrmInfo, SAMPLE_RECT_ARRAY_S *pstRect, SC_U32 u32Color);

/******************************************************************************
* function : Start Vpss
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_StartVpss(SIZE_S *pastSize, SC_U32 u32VpssChnNum);
/******************************************************************************
* function : Stop Vpss
******************************************************************************/
SC_VOID SAMPLE_COMM_IVE_StopVpss(SC_U32 u32VpssChnNum);
/******************************************************************************
* function : Start Vo
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_StartVo(SC_VOID);
/******************************************************************************
* function : Stop Vo
******************************************************************************/
SC_VOID SAMPLE_COMM_IVE_StopVo(SC_VOID);
/******************************************************************************
* function : Start Vi/Vpss/Venc/Vo
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_StartViVpssVencVo(SAMPLE_VI_CONFIG_S *pstViConfig, SAMPLE_IVE_SWITCH_S *pstSwitch,
    PIC_SIZE_E *penExtPicSize);
/******************************************************************************
* function : Stop Vi/Vpss/Venc/Vo
******************************************************************************/
SC_VOID SAMPLE_COMM_IVE_StopViVpssVencVo(SAMPLE_VI_CONFIG_S *pstViConfig, SAMPLE_IVE_SWITCH_S *pstSwitch);

#endif

