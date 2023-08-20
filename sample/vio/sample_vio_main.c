/*
 * @file     sample_vio_main.c
 * @brief    用户示例程序。用于将vin的数据通过 vo显示.
 * @version  1.0.1
 * @since    1.0.0
 * @author
 */

/*
 ********************************************************************************************************
 * Copyright 2021, BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
 *             ALL RIGHTS RESERVED
 * Permission is hereby granted to licensees of BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
 * to use or abstract this computer program for the sole purpose of implementing a product based on
 * BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. No other rights to reproduce, use,
 * or disseminate this computer program,whether in part or in whole, are granted.
 * BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. makes no representation or warranties
 * with respect to the performance of this computer program, and specifically disclaims
 * any responsibility for any damages, special or consequential, connected with the use of this program.
 * For details, see http://www.sgitg.sgcc.com.cn/
 ********************************************************************************************************
 */

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sc_common.h"
#include "sample_vio.h"

/******************************************************************************
* function : show usage
******************************************************************************/
SC_VOID SAMPLE_VIO_VoInterface_Usage(SC_VOID)
{
    printf("intf:\n");
    printf("\t 0) vo HDMI output.\n");
    printf("\t 1) vo BT1120 output, default.\n");
}

void SAMPLE_VIO_Usage(char *sPrgNm)
{
    printf("Usage : %s <index> <intf>\n", sPrgNm);
    printf("index:\n");
    printf("\t 0)VI(Online) - VPSS(Offline) - VENC/VO.\n");
    printf("\t 1)VI(Offline)- VPSS(Offline) - VENC/VO.\n");
    printf("\t 3)VI(Online)- VPSS(Offline) - VO.  Double chn.\n");
    printf("\t 4)Resolute Ratio Switch.\n");
    printf("\t 9)90/180/270 Rotate.\n");
    printf("\t 12)Vio vi chn1 resolution switch.\n");
    printf("\t 13)Vio vi crop.\n");
    printf("\t 14)Vio vi raw.\n");
    printf("\t 15)Vio vi osd.\n");
    printf("\t 16)Vio vi cover.\n");
    printf("\t 17)Vio vi mosaic.\n");
    printf("\t 18)Vio vi two sensor.\n");
    printf("\t 19)Vio vi hdr switch.\n");
    printf("\t 20)Vio vi two chn.\n");
    printf("\t 21)Vio vi raw stream open close.\n");
    printf("\t 22)Vio vi get bgr.\n");
    printf("\t 23)Vio vi send vo, alg.\n");
    printf("\t 24)Vio vi four sensor.\n");
    printf("\t 25)Vio vi sensor switch.\n");
    printf("\t 26)Vio vi get frm venc snap.\n");

    printf("intf:\n");
    printf("\t 0) vo HDMI output, default.\n");
    printf("\t 1) vo BT1120 output.\n");
    return;
}

/******************************************************************************
* function    : main()
* Description : main
******************************************************************************/
int main(int argc, char *argv[])
{
    SC_S32 s32Ret = SC_FAILURE;
    SC_S32 s32Index;
    SC_U32 u32VoIntfType = 0;

    if (argc < 2)
    {
        SAMPLE_VIO_Usage(argv[0]);
        return SC_FAILURE;
    }

    signal(SIGINT, SAMPLE_VIO_HandleSig);
    signal(SIGTERM, SAMPLE_VIO_HandleSig);

    if ((argc > 2) && (1 == atoi(argv[2])))
    {
        u32VoIntfType = 1;
    }

    SAMPLE_VIO_MsgInit();

    s32Index = atoi(argv[1]);
    switch (s32Index)
    {
    case 0:
        s32Ret = SAMPLE_VIO_ViOnlineVpssOfflineRoute(u32VoIntfType);
        break;

    case 1:
        s32Ret = SAMPLE_VIO_ViOfflineVpssOfflineRoute(u32VoIntfType);
        break;

    case 3:
        s32Ret = SAMPLE_VIO_ViDoubleChnRoute(u32VoIntfType);
        break;

    case 4:
        s32Ret = SAMPLE_VIO_ViResoSwitch(u32VoIntfType);
        break;

    case 9:
        s32Ret = SAMPLE_VIO_ViRotate(u32VoIntfType);
        break;

    case 12:
        s32Ret = SAMPLE_VIO_VioViChn1ResoSwitch(u32VoIntfType);
        break;

    case 13:
        s32Ret = SAMPLE_VIO_ViCrop(u32VoIntfType);
        break;

    case 14:
        s32Ret = SAMPLE_VIO_ViRaw(u32VoIntfType);
        break;

    case 15:
        s32Ret = SAMPLE_VIO_ViOsd(u32VoIntfType);
        break;

    case 16:
        s32Ret = SAMPLE_VIO_ViCover(u32VoIntfType);
        break;

    case 17:
        s32Ret = SAMPLE_VIO_ViMosaic(u32VoIntfType);
        break;

    case 18:
        s32Ret = SAMPLE_VIO_Vi2Sensor(u32VoIntfType);
        break;

    case 19:
        s32Ret = SAMPLE_VIO_ViHdrSwitch(u32VoIntfType);
        break;

    case 20:
        s32Ret = SAMPLE_VIO_Vi2Chn(u32VoIntfType);
        break;

    case 21:
        s32Ret = SAMPLE_VIO_ViOpenCloseRaw(u32VoIntfType);
        break;

    case 22:
        s32Ret = SAMPLE_VIO_ViGetBgr(u32VoIntfType);
        break;

    case 23:
        s32Ret = SAMPLE_VIO_ViAlgRect(u32VoIntfType);
        break;

    case 24:
        s32Ret = SAMPLE_VIO_Vi4Sensor(u32VoIntfType);
        break;

    case 25:
        s32Ret = SAMPLE_VIO_ViSensorSwitch(u32VoIntfType);
        break;

    case 26:
        s32Ret = SAMPLE_VIO_ViOnlineVencSnap(u32VoIntfType);
        break;
    default:
        SAMPLE_PRT("the index %d is invaild!\n", s32Index);
        SAMPLE_VIO_Usage(argv[0]);
        SAMPLE_VIO_MsgExit();
        return SC_FAILURE;
    }

    if (SC_SUCCESS == s32Ret)
    {
        SAMPLE_PRT("sample_vio exit success!\n");
    }
    else
    {
        SAMPLE_PRT("sample_vio exit abnormally!\n");
    }

    SAMPLE_VIO_MsgExit();

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
