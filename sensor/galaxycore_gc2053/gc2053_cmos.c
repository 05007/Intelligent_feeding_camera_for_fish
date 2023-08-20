/**
 * @file     gc2053_cmos.c
 * @brief    GALAXYCORE GC2053 SENSOR控制接口
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2022-02-08 创建文件
 */
/************************************************************
 *@note
    Copyright 2021, BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
                   ALL RIGHTS RESERVED
    Permission is hereby granted to licensees of  BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. to use
    or abstract this computer program for the sole purpose of implementing a product based on BEIJIING SMARTCHIP
    MICROELECTRONICS TECHNOLOGY CO., LTD. No other rights to reproduce, use, or disseminate this computer program,
    whether in part or in whole, are granted. BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. makes no
    representation or warranties with respect to the performance of this computer program, and specifically disclaims
    any responsibility for any damages, special or consequential, connected with the use of this program.
　　For details, see http://www.sgitg.sgcc.com.cn/
**********************************************************/


#if !defined(__GC2053_CMOS_C_)
#define __GC2053_CMOS_C_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "sc_gpio.h"
#include "sc_comm_sns.h"
#include "sc_comm_video.h"
#include "sc_sns_ctrl.h"
#include "mpi_isp.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


#define GC2053_ID 2053


/****************************************************************************
 * global variables                                                         *
 ****************************************************************************/
ISP_SNS_STATE_S *g_pastGc2053[ISP_MAX_PIPE_NUM] = {SC_NULL};

#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_pastGc2053[dev])
#define SENSOR_SET_CTX(dev, pstCtx) (g_pastGc2053[dev] = pstCtx)
#define SENSOR_RESET_CTX(dev)       (g_pastGc2053[dev] = SC_NULL)

ISP_SNS_COMMBUS_U g_aunGc2053BusInfo[ISP_MAX_PIPE_NUM] =
{
    [0] = { .s8I2cDev = 1},
    [1 ... ISP_MAX_PIPE_NUM - 1] = { .s8I2cDev = -1}
};

static ISP_FSWDR_MODE_E genFSWDRMode[ISP_MAX_PIPE_NUM] =
{
    [0 ... ISP_MAX_PIPE_NUM - 1] = ISP_FSWDR_NORMAL_MODE
};

static SC_U32 gu32MaxTimeGetCnt[ISP_MAX_PIPE_NUM]   = {0};
static SC_U32 g_au32InitExposure[ISP_MAX_PIPE_NUM]  = {0};
static SC_U32 g_au32LinesPer500ms[ISP_MAX_PIPE_NUM] = {0};
static SC_U16 g_au16InitWBGain[ISP_MAX_PIPE_NUM][3] = {{0}};
static SC_U16 g_au16SampleRgain[ISP_MAX_PIPE_NUM]   = {0};
static SC_U16 g_au16SampleBgain[ISP_MAX_PIPE_NUM]   = {0};

static SC_TUNING_BIN_FILE_S g_acTuningBinName[ISP_MAX_PIPE_NUM] =
{
    [0] = {
              .acLinearBinName = "/local/tuning/gc2053/gc2053_tuning.bin",
          },

    [1] = {
              .acLinearBinName = "/local/tuning/gc2053/gc2053_tuning_night.bin",
          },
};



/****************************************************************************
 * extern                                                                   *
 ****************************************************************************/
extern const unsigned int gc2053_i2c_addr;
extern unsigned int gc2053_addr_byte;
extern unsigned int gc2053_data_byte;

extern int  gc2053_init(VI_PIPE ViPipe);
extern void gc2053_exit(VI_PIPE ViPipe);
extern void gc2053_standby(VI_PIPE ViPipe);
extern void gc2053_restart(VI_PIPE ViPipe);
extern void gc2053_default_reg_init(VI_PIPE ViPipe);
extern int  gc2053_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  gc2053_read_register(VI_PIPE ViPipe, int addr);
extern int  gc2053_flip_off_mirror_off(VI_PIPE ViPipe);
extern int  gc2053_flip_on_mirror_off(VI_PIPE ViPipe);
extern int  gc2053_flip_off_mirror_on(VI_PIPE ViPipe);
extern int  gc2053_flip_on_mirror_on(VI_PIPE ViPipe);



/****************************************************************************
 * local variables                                                          *
 ****************************************************************************/
/* Sensor register address */
#define GC2053_EXPTIME_ADDR_H      (0x03) // Shutter-time[13:8]
#define GC2053_EXPTIME_ADDR_L      (0x04) // Shutter-time[7:0]

#define GC2053_AGAIN_ADDR_H        (0xb4) // Analog-gain[9:8]
#define GC2053_AGAIN_ADDR_L        (0xb3) // Analog-gain[7:0]

#define GC2053_DGAIN_ADDR_H        (0xb9) // digital-gain[11:6]
#define GC2053_DGAIN_ADDR_L        (0xb8) // digital-gain[5:0]

#define GC2053_AUTO_PREGAIN_ADDR_H (0xb1) // auto-pregain-gain[9:6]
#define GC2053_AUTO_PREGAIN_ADDR_L (0xb2) // auto-pregain-gain[5:0]

#define GC2053_VMAX_ADDR_H         (0x41) // Vmax[13:8]
#define GC2053_VMAX_ADDR_L         (0x42) // Vmax[7:0]
#define GC2053_STATUS_ADDR         (0x90) // slow shutter via framerate or exptime

#define SENSOR_FULL_LINES_MAX          (0x3FFF)
#define SENSOR_INCREASE_LINES          (0)
#define SENSOR_VMAX_1080P30_LINEAR     (1125 + SENSOR_INCREASE_LINES)
#define SENSOR_1080P_30FPS_LINEAR_MODE (0)

#define SENSOR_RES_IS_1080P(w, h) ((w) <= 1920 && (h) <= 1080)
#define HIGH_8BITS(x)             (((x) & 0xff00) >> 8)
#define LOW_8BITS(x)              ((x) & 0x00ff)



static SC_S32 cmos_power_on(VI_PIPE ViPipe, dev_power_attr_t *p_dev_power_attr)
{
    int power_gpio = 0;
    int reset_gpio = 0;

    power_gpio = sc_gpio_name_to_num(p_dev_power_attr->power_gpio[0],
                                     p_dev_power_attr->power_gpio[1]);

    reset_gpio = sc_gpio_name_to_num(p_dev_power_attr->reset_gpio[0],
                                     p_dev_power_attr->reset_gpio[1]);

    printf("cmos_power_on: power_gpio=%d, reset_gpio=%d\n", power_gpio, reset_gpio);
    sc_gpio_export(power_gpio);
    sc_gpio_export(reset_gpio);

    /* set the gpio dir to out */
    sc_gpio_set_dir(power_gpio, 1);
    sc_gpio_set_dir(reset_gpio, 1);

    /* set the gpio to low */
    sc_gpio_set_value(power_gpio, 0);
    sc_gpio_set_value(reset_gpio, 0);

    /* give some delay, set the gpio to low */
    usleep(10000);
    sc_gpio_set_value(power_gpio, 1);
    usleep(10000);
    sc_gpio_set_value(reset_gpio, 1);
    usleep(10000);

    return SC_SUCCESS;
}

static SC_S32 cmos_power_off(VI_PIPE ViPipe, dev_power_attr_t *p_dev_power_attr)
{
    int power_gpio = 0;
    int reset_gpio = 0;

    power_gpio = sc_gpio_name_to_num(p_dev_power_attr->power_gpio[0],
                                     p_dev_power_attr->power_gpio[1]);

    reset_gpio = sc_gpio_name_to_num(p_dev_power_attr->reset_gpio[0],
                                     p_dev_power_attr->reset_gpio[1]);

    printf("cmos_power_off: power_gpio=%d, reset_gpio=%d\n", power_gpio, reset_gpio);
    sc_gpio_export(power_gpio);
    sc_gpio_export(reset_gpio);

    /* set the gpio dir to out */
    sc_gpio_set_dir(power_gpio, 1);
    sc_gpio_set_dir(reset_gpio, 1);

    /* set the gpio to low */
    sc_gpio_set_value(power_gpio, 0);
    sc_gpio_set_value(reset_gpio, 0);

    return SC_SUCCESS;
}

static SC_S32 cmos_get_ae_default(VI_PIPE ViPipe, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    CMOS_CHECK_POINTER(pstAeSnsDft);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    pstAeSnsDft->f32Fps          = 30;
    pstAeSnsDft->f32MaxFps       = 30;
    pstAeSnsDft->u32FullLinesStd = pstSnsState->u32FLStd;
    pstAeSnsDft->u32FlickerFreq  = 50 * 256;
    pstAeSnsDft->u32FullLinesMax = SENSOR_FULL_LINES_MAX;
    pstAeSnsDft->u32Hmax         = 2200;

    if(g_au32LinesPer500ms[ViPipe] == 0)
    {
        pstAeSnsDft->u32LinesPer500ms = pstSnsState->u32FLStd * pstAeSnsDft->f32Fps / 2;
    }
    else
    {
        pstAeSnsDft->u32LinesPer500ms = g_au32LinesPer500ms[ViPipe];
    }

    switch(pstSnsState->enWDRMode)
    {
        default:
        case WDR_MODE_NONE:
        {
            /* Linear mode */
            pstAeSnsDft->u32MaxAgain   = 123568;
            pstAeSnsDft->u32MinAgain   = 1024;
            pstAeSnsDft->u32MaxDgain   = 123568;
            pstAeSnsDft->u32MinDgain   = 1024;
            pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd - 2;
            pstAeSnsDft->u32MinIntTime = 3;

            break;
        }
    }

    return SC_SUCCESS;
}

static SC_VOID cmos_fps_set(VI_PIPE ViPipe, SC_FLOAT f32Fps, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SC_U32 u32VMAX = SENSOR_VMAX_1080P30_LINEAR;

    CMOS_CHECK_POINTER_VOID(pstAeSnsDft);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    switch(pstSnsState->u8ImgMode)
    {
        case SENSOR_1080P_30FPS_LINEAR_MODE:
        {
            if((f32Fps <= pstAeSnsDft->f32MaxFps) && (f32Fps >= 2.06))
            {
                u32VMAX = SENSOR_VMAX_1080P30_LINEAR * pstAeSnsDft->f32MaxFps / DIV_0_TO_1_FLOAT(f32Fps);
            }
            else
            {
                printf("Not support Fps: %f\n", f32Fps);
                return;
            }

            break;
        }

        default:
        {
            return;
        }
    }

    u32VMAX               = (u32VMAX > SENSOR_FULL_LINES_MAX) ? SENSOR_FULL_LINES_MAX : u32VMAX;
    pstSnsState->u32FLStd = u32VMAX;
    pstSnsState->astRegsInfo[0].astI2cData[8].u32Data  = 0x0;
    pstSnsState->astRegsInfo[0].astI2cData[9].u32Data  = LOW_8BITS(u32VMAX);
    pstSnsState->astRegsInfo[0].astI2cData[10].u32Data = HIGH_8BITS(u32VMAX);

    pstAeSnsDft->f32Fps           = f32Fps;
    pstAeSnsDft->u32LinesPer500ms = pstSnsState->u32FLStd * f32Fps / 2;
    pstAeSnsDft->u32FullLinesStd  = pstSnsState->u32FLStd;
    pstAeSnsDft->u32MaxIntTime    = pstSnsState->u32FLStd - 2;
    pstSnsState->au32FL[0]        = pstSnsState->u32FLStd;
    pstAeSnsDft->u32FullLines     = pstSnsState->au32FL[0];

    return;
}

static SC_VOID cmos_slow_framerate_set(VI_PIPE ViPipe, SC_U32 u32FullLines, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    CMOS_CHECK_POINTER_VOID(pstAeSnsDft);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    u32FullLines = (u32FullLines > SENSOR_FULL_LINES_MAX) ? SENSOR_FULL_LINES_MAX : u32FullLines;
    pstSnsState->au32FL[0] = u32FullLines;

    pstSnsState->astRegsInfo[0].astI2cData[8].u32Data  = 0x0;
    pstSnsState->astRegsInfo[0].astI2cData[9].u32Data  = LOW_8BITS(pstSnsState->au32FL[0]);
    pstSnsState->astRegsInfo[0].astI2cData[10].u32Data = HIGH_8BITS(pstSnsState->au32FL[0]);

    pstAeSnsDft->u32FullLines  = pstSnsState->au32FL[0];
    pstAeSnsDft->u32MaxIntTime = pstSnsState->au32FL[0] - 2;

    return;
}

/* while isp notify ae to update sensor regs, ae call these funcs. */
static SC_VOID cmos_inttime_update(VI_PIPE ViPipe, SC_U32 u32IntTime)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SC_S32 u32Value = 0;

    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    u32Value = (u32IntTime > SENSOR_FULL_LINES_MAX) ? SENSOR_FULL_LINES_MAX : u32IntTime;
    //printf("sns[%d]: ln=%d, u32Value=%u\n", ViPipe, u32IntTime, u32Value);
    pstSnsState->astRegsInfo[0].astI2cData[0].u32Data = LOW_8BITS(u32Value);
    pstSnsState->astRegsInfo[0].astI2cData[1].u32Data = HIGH_8BITS(u32Value);
    return;
}

static SC_U32 regValTable[29][4] =
{
    /* 0xb4 0xb3 0xb8 0xb9 */
    {0x00, 0x00, 0x01, 0x00},
    {0x00, 0x10, 0x01, 0x0c},
    {0x00, 0x20, 0x01, 0x1b},
    {0x00, 0x30, 0x01, 0x2c},
    {0x00, 0x40, 0x01, 0x3f},
    {0x00, 0x50, 0x02, 0x16},
    {0x00, 0x60, 0x02, 0x35},
    {0x00, 0x70, 0x03, 0x16},
    {0x00, 0x80, 0x04, 0x02},
    {0x00, 0x90, 0x04, 0x31},
    {0x00, 0xa0, 0x05, 0x32},
    {0x00, 0xb0, 0x06, 0x35},
    {0x00, 0xc0, 0x08, 0x04},
    {0x00, 0x5a, 0x09, 0x19},
    {0x00, 0x83, 0x0b, 0x0f},
    {0x00, 0x93, 0x0d, 0x12},
    {0x00, 0x84, 0x10, 0x00},
    {0x00, 0x94, 0x12, 0x3a},
    {0x01, 0x2c, 0x1a, 0x02},
    {0x01, 0x3c, 0x1b, 0x20},
    {0x00, 0x8c, 0x20, 0x0f},
    {0x00, 0x9c, 0x26, 0x07},
    {0x02, 0x64, 0x36, 0x21},
    {0x02, 0x74, 0x37, 0x3a},
    {0x00, 0xc6, 0x3d, 0x02},
    {0x00, 0xdc, 0x3f, 0x3f},
    {0x02, 0x85, 0x3f, 0x3f},
    {0x02, 0x95, 0x3f, 0x3f},
    {0x00, 0xce, 0x3f, 0x3f},
};

static SC_U32 analog_gain_table[29] =
{
     1024, 1230, 1440, 1730, 2032, 2380, 2880, 3460, 4080, 4800,
     5776, 6760, 8064, 9500, 11552, 13600, 16132, 18912, 22528, 27036,
    32340, 38256, 45600, 53912, 63768, 76880, 92300, 108904, 123568,
};

static SC_U32 g_au32DGainVal[ISP_MAX_PIPE_NUM];

static SC_VOID cmos_again_calc_table(VI_PIPE ViPipe, SC_U32 *pu32AgainLin, SC_U32 *pu32AgainDb)
{
    SC_S32 again = 0;
    SC_S32 i     = 0;

    CMOS_CHECK_POINTER_VOID(pu32AgainLin);
    CMOS_CHECK_POINTER_VOID(pu32AgainDb);

    again = *pu32AgainLin;
    if(again >= analog_gain_table[28])
    {
        *pu32AgainLin = analog_gain_table[28];
        *pu32AgainDb  = 28;

        g_au32DGainVal[ViPipe] = again * 64 / analog_gain_table[28];
    }
    else
    {
        for(i = 1; i < 29; i++)
        {
            if(again < analog_gain_table[i])
            {
                *pu32AgainLin = analog_gain_table[i - 1];
                *pu32AgainDb  = i - 1;

                g_au32DGainVal[ViPipe] = again * 64 / analog_gain_table[i - 1];
                break;
            }
        }
    }

    return;
}

static SC_VOID cmos_gains_update(VI_PIPE ViPipe, SC_U32 u32Again, SC_U32 u32Dgain)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SC_U8 u8DgainHigh = 1;
    SC_U8 u8DgainLow  = 0;

    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    //printf("sns pp=%d, ag=%d, dg=%d\n", ViPipe, u32Again, u32Dgain);
    u8DgainHigh = (g_au32DGainVal[ViPipe] >> 6) & 0x0f;
    u8DgainLow  = (g_au32DGainVal[ViPipe] & 0x3f) << 2;
    pstSnsState->astRegsInfo[0].astI2cData[2].u32Data = regValTable[u32Again][0];
    pstSnsState->astRegsInfo[0].astI2cData[3].u32Data = regValTable[u32Again][1];
    pstSnsState->astRegsInfo[0].astI2cData[4].u32Data = regValTable[u32Again][2];
    pstSnsState->astRegsInfo[0].astI2cData[5].u32Data = regValTable[u32Again][3];
    pstSnsState->astRegsInfo[0].astI2cData[6].u32Data = u8DgainHigh;
    pstSnsState->astRegsInfo[0].astI2cData[7].u32Data = u8DgainLow;
    //printf("sns pp=%d, dg=%d, dgRegH=%d, dgRegL=%d\n", ViPipe, g_au32DGainVal[ViPipe], u8DgainHigh, u8DgainLow);

    return;
}

static SC_VOID cmos_get_inttime_max(VI_PIPE ViPipe, SC_U16 u16ManRatioEnable, SC_U32 *au32Ratio,
    SC_U32 *au32IntTimeMax, SC_U32 *au32IntTimeMin, SC_U32 *pu32LFMaxIntTime)
{
    return;
}

/* Only used in LINE_WDR mode */
static SC_VOID cmos_ae_fswdr_attr_set(VI_PIPE ViPipe, AE_FSWDR_ATTR_S *pstAeFSWDRAttr)
{
    CMOS_CHECK_POINTER_VOID(pstAeFSWDRAttr);

    genFSWDRMode[ViPipe]      = pstAeFSWDRAttr->enFSWDRMode;
    gu32MaxTimeGetCnt[ViPipe] = 0;

    return;
}

static SC_S32 cmos_init_ae_exp_function(AE_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    CMOS_CHECK_POINTER(pstExpFuncs);

    memset(pstExpFuncs, 0, sizeof(AE_SENSOR_EXP_FUNC_S));

    pstExpFuncs->pfn_cmos_get_ae_default     = cmos_get_ae_default;
    pstExpFuncs->pfn_cmos_fps_set            = cmos_fps_set;
    pstExpFuncs->pfn_cmos_slow_framerate_set = cmos_slow_framerate_set;
    pstExpFuncs->pfn_cmos_inttime_update     = cmos_inttime_update;
    pstExpFuncs->pfn_cmos_gains_update       = cmos_gains_update;
    pstExpFuncs->pfn_cmos_again_calc_table   = cmos_again_calc_table;
    pstExpFuncs->pfn_cmos_dgain_calc_table   = SC_NULL;
    pstExpFuncs->pfn_cmos_get_inttime_max    = cmos_get_inttime_max;
    pstExpFuncs->pfn_cmos_ae_fswdr_attr_set  = cmos_ae_fswdr_attr_set;

    return SC_SUCCESS;
}

/* Rgain and Bgain of the golden sample */
#define GOLDEN_RGAIN 0
#define GOLDEN_BGAIN 0
static SC_S32 cmos_get_awb_default(VI_PIPE ViPipe, AWB_SENSOR_DEFAULT_S *pstAwbSnsDft)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    CMOS_CHECK_POINTER(pstAwbSnsDft);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    memset(pstAwbSnsDft, 0, sizeof(AWB_SENSOR_DEFAULT_S));
    pstAwbSnsDft->u16GoldenRgain = GOLDEN_RGAIN;
    pstAwbSnsDft->u16GoldenBgain = GOLDEN_BGAIN;

    pstAwbSnsDft->u16SampleRgain = g_au16SampleRgain[ViPipe];
    pstAwbSnsDft->u16SampleBgain = g_au16SampleBgain[ViPipe];

    return SC_SUCCESS;
}

static SC_S32 cmos_init_awb_exp_function(AWB_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    CMOS_CHECK_POINTER(pstExpFuncs);

    memset(pstExpFuncs, 0, sizeof(AWB_SENSOR_EXP_FUNC_S));

    pstExpFuncs->pfn_cmos_get_awb_default = cmos_get_awb_default;

    return SC_SUCCESS;
}

static ISP_CMOS_DNG_COLORPARAM_S g_stDngColorParam =
{
    {378, 256, 430},
    {439, 256, 439}
};

static SC_S32 cmos_get_isp_default(VI_PIPE ViPipe, ISP_CMOS_DEFAULT_S *pstDef)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    CMOS_CHECK_POINTER(pstDef);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    memset(pstDef, 0, sizeof(ISP_CMOS_DEFAULT_S));
    switch(pstSnsState->enWDRMode)
    {
        default:
        case WDR_MODE_NONE:
        {
            strncpy(pstDef->acTuningPraBinName, g_acTuningBinName[ViPipe].acLinearBinName, TUNING_BIN_FILENAME_LEN);
            break;
        }
    }

    pstDef->stSensorMode.u32SensorID  = GC2053_ID;
    pstDef->stSensorMode.u8SensorMode = pstSnsState->u8ImgMode;
    memcpy(&pstDef->stDngColorParam, &g_stDngColorParam, sizeof(ISP_CMOS_DNG_COLORPARAM_S));

    switch(pstSnsState->u8ImgMode)
    {
        default:
        case SENSOR_1080P_30FPS_LINEAR_MODE:
        {
            pstDef->stSensorMode.stDngRawFormat.u8BitsPerSample = 10;
            pstDef->stSensorMode.stDngRawFormat.u32WhiteLevel   = 1023;
            break;
        }
    }

    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleH.u32Denominator = 1;
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleH.u32Numerator   = 1;
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleV.u32Denominator = 1;
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleV.u32Numerator   = 1;
    pstDef->stSensorMode.stDngRawFormat.stCfaRepeatPatternDim.u16RepeatPatternDimRows = 2;
    pstDef->stSensorMode.stDngRawFormat.stCfaRepeatPatternDim.u16RepeatPatternDimCols = 2;
    pstDef->stSensorMode.stDngRawFormat.stBlcRepeatDim.u16BlcRepeatRows = 2;
    pstDef->stSensorMode.stDngRawFormat.stBlcRepeatDim.u16BlcRepeatCols = 2;
    pstDef->stSensorMode.stDngRawFormat.enCfaLayout         = CFALAYOUT_TYPE_RECTANGULAR;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPlaneColor[0] = 0;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPlaneColor[1] = 1;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPlaneColor[2] = 2;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[0] = 0;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[1] = 1;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[2] = 1;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[3] = 2;
    pstDef->stSensorMode.bValidDngRawFormat = SC_TRUE;

    return SC_SUCCESS;
}

static SC_VOID cmos_set_pixel_detect(VI_PIPE ViPipe, SC_BOOL bEnable)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    return;
}

static SC_S32 cmos_set_wdr_mode(VI_PIPE ViPipe, SC_U8 u8Mode)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    pstSnsState->bSyncInit = SC_FALSE;

    switch(u8Mode)
    {
        case WDR_MODE_NONE:
        {
            pstSnsState->enWDRMode = WDR_MODE_NONE;
            printf("cmos_set_wdr_mode: linear mode\n");
            break;
        }

        default:
        {
            printf("NOT support this mode!\n");
            return SC_FAILURE;
        }
    }

    memset(pstSnsState->au32WDRIntTime, 0, sizeof(pstSnsState->au32WDRIntTime));
    return SC_SUCCESS;
}

static SC_S32 cmos_get_sns_regs_info(VI_PIPE ViPipe, ISP_SNS_REGS_INFO_S *pstSnsRegsInfo)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SC_S32 i = 0;

    CMOS_CHECK_POINTER(pstSnsRegsInfo);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    if((SC_FALSE == pstSnsState->bSyncInit) || (SC_FALSE == pstSnsRegsInfo->bConfig))
    {
        pstSnsState->astRegsInfo[0].enSnsType           = ISP_SNS_I2C_TYPE;
        pstSnsState->astRegsInfo[0].unComBus.s8I2cDev   = g_aunGc2053BusInfo[ViPipe].s8I2cDev;
        pstSnsState->astRegsInfo[0].u8Cfg2ValidDelayMax = 3;
        pstSnsState->astRegsInfo[0].u32RegNum           = 11;

        for(i = 0; i < pstSnsState->astRegsInfo[0].u32RegNum; i++)
        {
            pstSnsState->astRegsInfo[0].astI2cData[i].bUpdate        = SC_TRUE;
            pstSnsState->astRegsInfo[0].astI2cData[i].u8DevAddr      = gc2053_i2c_addr;
            pstSnsState->astRegsInfo[0].astI2cData[i].u32AddrByteNum = gc2053_addr_byte;
            pstSnsState->astRegsInfo[0].astI2cData[i].u32DataByteNum = gc2053_data_byte;
        }

        pstSnsState->astRegsInfo[0].astI2cData[0].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[0].u32RegAddr     = GC2053_EXPTIME_ADDR_L;
        pstSnsState->astRegsInfo[0].astI2cData[1].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[1].u32RegAddr     = GC2053_EXPTIME_ADDR_H;

        pstSnsState->astRegsInfo[0].astI2cData[2].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[2].u32RegAddr     = GC2053_AGAIN_ADDR_H;
        pstSnsState->astRegsInfo[0].astI2cData[3].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[3].u32RegAddr     = GC2053_AGAIN_ADDR_L;

        pstSnsState->astRegsInfo[0].astI2cData[4].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[4].u32RegAddr     = GC2053_DGAIN_ADDR_L;
        pstSnsState->astRegsInfo[0].astI2cData[5].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[5].u32RegAddr     = GC2053_DGAIN_ADDR_H;

        pstSnsState->astRegsInfo[0].astI2cData[6].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[6].u32RegAddr     = GC2053_AUTO_PREGAIN_ADDR_H;
        pstSnsState->astRegsInfo[0].astI2cData[7].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[7].u32RegAddr     = GC2053_AUTO_PREGAIN_ADDR_L;

        pstSnsState->astRegsInfo[0].astI2cData[8].u8DelayFrmNum  = 0;
        pstSnsState->astRegsInfo[0].astI2cData[8].u32RegAddr     = GC2053_STATUS_ADDR;

        pstSnsState->astRegsInfo[0].astI2cData[9].u8DelayFrmNum  = 0;
        pstSnsState->astRegsInfo[0].astI2cData[9].u32RegAddr     = GC2053_VMAX_ADDR_L;
        pstSnsState->astRegsInfo[0].astI2cData[10].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[10].u32RegAddr    = GC2053_VMAX_ADDR_H;

        pstSnsState->bSyncInit = SC_TRUE;
    }
    else
    {
        for(i = 0; i < pstSnsState->astRegsInfo[0].u32RegNum - 2; i++)
        {
            if(pstSnsState->astRegsInfo[0].astI2cData[i].u32Data == pstSnsState->astRegsInfo[1].astI2cData[i].u32Data)
            {
                pstSnsState->astRegsInfo[0].astI2cData[i].bUpdate = SC_FALSE;
            }
            else
            {
                pstSnsState->astRegsInfo[0].astI2cData[i].bUpdate = SC_TRUE;
            }
        }

        if((pstSnsState->astRegsInfo[0].astI2cData[6].bUpdate == SC_TRUE)
            || (pstSnsState->astRegsInfo[0].astI2cData[7].bUpdate == SC_TRUE))
        {
            pstSnsState->astRegsInfo[0].astI2cData[6].bUpdate = SC_TRUE;
            pstSnsState->astRegsInfo[0].astI2cData[7].bUpdate = SC_TRUE;
        }

        pstSnsState->astRegsInfo[0].astI2cData[9].bUpdate  = SC_TRUE;
        pstSnsState->astRegsInfo[0].astI2cData[10].bUpdate = SC_TRUE;
    }

    pstSnsRegsInfo->bConfig = SC_FALSE;
    memcpy(pstSnsRegsInfo, &pstSnsState->astRegsInfo[0], sizeof(ISP_SNS_REGS_INFO_S));
    memcpy(&pstSnsState->astRegsInfo[1], &pstSnsState->astRegsInfo[0], sizeof(ISP_SNS_REGS_INFO_S));

    pstSnsState->au32FL[1] = pstSnsState->au32FL[0];

    /* Set real register for aec */
    gc2053_default_reg_init(ViPipe);

    return SC_SUCCESS;
}

static SC_S32 cmos_set_image_mode(VI_PIPE ViPipe, ISP_CMOS_SENSOR_IMAGE_MODE_S *pstSensorImageMode)
{
    SC_U8 u8SensorImageMode = 0;
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    CMOS_CHECK_POINTER(pstSensorImageMode);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    u8SensorImageMode = pstSnsState->u8ImgMode;
    pstSnsState->bSyncInit = SC_FALSE;

    if(pstSensorImageMode->f32Fps <= 30)
    {
        if(WDR_MODE_NONE == pstSnsState->enWDRMode)
        {
            if(SENSOR_RES_IS_1080P(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height))
            {
                u8SensorImageMode     = SENSOR_1080P_30FPS_LINEAR_MODE;
                pstSnsState->u32FLStd = SENSOR_VMAX_1080P30_LINEAR;
            }
            else
            {
                printf("%d: Not support! Width:%d, Height:%d, ImageMode:%d, SensorState:%d\n",
                    __LINE__, pstSensorImageMode->u16Width, pstSensorImageMode->u16Height, u8SensorImageMode,
                    pstSnsState->enWDRMode);
                return SC_FAILURE;
            }
        }
        else
        {
            printf("%d: Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
                __LINE__, pstSensorImageMode->u16Width, pstSensorImageMode->u16Height, pstSensorImageMode->f32Fps,
                pstSnsState->enWDRMode);
            return SC_FAILURE;
        }
    }

    if((SC_TRUE == pstSnsState->bInit) && (u8SensorImageMode == pstSnsState->u8ImgMode))
    {
        /* Don't need to switch SensorImageMode */
        return SC_FAILURE;
    }

    pstSnsState->u8ImgMode = u8SensorImageMode;
    pstSnsState->au32FL[0] = pstSnsState->u32FLStd;
    pstSnsState->au32FL[1] = pstSnsState->au32FL[0];

    return SC_SUCCESS;
}

static SC_VOID gc2053_global_init(VI_PIPE ViPipe)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    pstSnsState->bInit     = SC_FALSE;
    pstSnsState->bSyncInit = SC_FALSE;
    pstSnsState->u8ImgMode = SENSOR_1080P_30FPS_LINEAR_MODE;
    pstSnsState->enWDRMode = WDR_MODE_NONE;
    pstSnsState->u32FLStd  = SENSOR_VMAX_1080P30_LINEAR;
    pstSnsState->au32FL[0] = SENSOR_VMAX_1080P30_LINEAR;
    pstSnsState->au32FL[1] = SENSOR_VMAX_1080P30_LINEAR;

    memset(&pstSnsState->astRegsInfo[0], 0, sizeof(ISP_SNS_REGS_INFO_S));
    memset(&pstSnsState->astRegsInfo[1], 0, sizeof(ISP_SNS_REGS_INFO_S));
}

static SC_S32 cmos_set_mirror_flip(VI_PIPE ViPipe, int mode)
{
    SC_S32 ret = 0;

    printf("cmos_set_mirror_flip: mode=%d\n", mode);
    switch(mode)
    {
        case ISP_SNS_NORMAL:
        {
            ret = gc2053_flip_off_mirror_off(ViPipe);
            break;
        }

        case ISP_SNS_MIRROR:
        {
            ret = gc2053_flip_off_mirror_on(ViPipe);
            break;
        }

        case ISP_SNS_FLIP:
        {
            ret = gc2053_flip_on_mirror_off(ViPipe);
            break;
        }

        case ISP_SNS_MIRROR_FLIP:
        {
            ret = gc2053_flip_on_mirror_on(ViPipe);
            break;
        }

        default:
        {
        break;
        }
    }

    return ret;
}

static SC_S32 cmos_gc2053_ctl(VI_PIPE ViPipe, ISP_CMOS_SENSOR_CTL *pSensorCtl)
{
    SC_U8  u8CtlCode = 0;
    SC_S32 ret       = 0;
    SC_S32 mode      = 0;

    if(!pSensorCtl)
    {
         printf("sensor ctl failed!\n");
         return SC_ERR_ISP_INVALID_ADDR;
    }

    mode      = *(SC_S32 *)pSensorCtl->pCtlData;
    u8CtlCode = pSensorCtl->u8CtlCode;
    switch(u8CtlCode)
    {
        case SNS_CB_EVENT_FLIP_MIRROR:
        {
            ret = cmos_set_mirror_flip(ViPipe, mode);
            break;
        }

        default:
        {
            printf("error ctl code\n");
            ret = SC_FAILURE;
            break;
        }
    }

    return ret;
}

static SC_S32 cmos_init_gc2053_exp_function(ISP_SENSOR_EXP_FUNC_S *pstSensorExpFunc)
{
    CMOS_CHECK_POINTER(pstSensorExpFunc);

    memset(pstSensorExpFunc, 0, sizeof(ISP_SENSOR_EXP_FUNC_S));
    pstSensorExpFunc->pfn_cmos_sensor_init        = gc2053_init;
    pstSensorExpFunc->pfn_cmos_sensor_exit        = gc2053_exit;
    pstSensorExpFunc->pfn_cmos_sensor_global_init = gc2053_global_init;
    pstSensorExpFunc->pfn_cmos_set_image_mode     = cmos_set_image_mode;
    pstSensorExpFunc->pfn_cmos_set_wdr_mode       = cmos_set_wdr_mode;

    pstSensorExpFunc->pfn_cmos_get_isp_default  = cmos_get_isp_default;
    pstSensorExpFunc->pfn_cmos_set_pixel_detect = cmos_set_pixel_detect;
    pstSensorExpFunc->pfn_cmos_get_sns_reg_info = cmos_get_sns_regs_info;

    pstSensorExpFunc->pfn_cmos_sns_power_on  = cmos_power_on;
    pstSensorExpFunc->pfn_cmos_sns_power_off = cmos_power_off;
    pstSensorExpFunc->pfn_cmos_sns_ctl       = cmos_gc2053_ctl;

    return SC_SUCCESS;
}


/****************************************************************************
 * callback structure                                                       *
 ****************************************************************************/
static SC_S32 gc2053_set_bus_info(VI_PIPE ViPipe, ISP_SNS_COMMBUS_U unSNSBusInfo)
{
    g_aunGc2053BusInfo[ViPipe].s8I2cDev = unSNSBusInfo.s8I2cDev;

    return SC_SUCCESS;
}

static SC_S32 gc2053_ctx_init(VI_PIPE ViPipe)
{
    ISP_SNS_STATE_S *pastSnsStateCtx = SC_NULL;

    SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);

    if(SC_NULL == pastSnsStateCtx)
    {
        pastSnsStateCtx = (ISP_SNS_STATE_S *)malloc(sizeof(ISP_SNS_STATE_S));
        if(SC_NULL == pastSnsStateCtx)
        {
            printf("Isp[%d] SnsCtx malloc memory failed!\n", ViPipe);
            return SC_ERR_ISP_NOMEM;
        }
    }

    memset(pastSnsStateCtx, 0, sizeof(ISP_SNS_STATE_S));

    SENSOR_SET_CTX(ViPipe, pastSnsStateCtx);

    return SC_SUCCESS;
}

static SC_VOID gc2053_ctx_exit(VI_PIPE ViPipe)
{
    ISP_SNS_STATE_S *pastSnsStateCtx = SC_NULL;

    SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);
    SENSOR_FREE(pastSnsStateCtx);
    SENSOR_RESET_CTX(ViPipe);
}

static SC_S32 gc2053_register_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    SC_S32 s32Ret = SC_FAILURE;

    ISP_SENSOR_REGISTER_S stIspRegister;
    AE_SENSOR_REGISTER_S  stAeRegister;
    AWB_SENSOR_REGISTER_S stAwbRegister;
    ISP_SNS_ATTR_INFO_S   stSnsAttrInfo;

    CMOS_CHECK_POINTER(pstAeLib);
    CMOS_CHECK_POINTER(pstAwbLib);

    s32Ret = gc2053_ctx_init(ViPipe);
    if(SC_SUCCESS != s32Ret)
    {
        return SC_FAILURE;
    }

    stSnsAttrInfo.eSensorId = GC2053_ID;

    s32Ret  = cmos_init_gc2053_exp_function(&stIspRegister.stSnsExp);
    s32Ret |= SC_MPI_ISP_SensorRegCallBack(ViPipe, &stSnsAttrInfo, &stIspRegister);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor register callback function failed!\n");
        return s32Ret;
    }

    s32Ret  = cmos_init_ae_exp_function(&stAeRegister.stSnsExp);
    s32Ret |= SC_MPI_AE_SensorRegCallBack(ViPipe, pstAeLib, &stSnsAttrInfo, &stAeRegister);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor register callback function to ae lib failed!\n");
        return s32Ret;
    }

    s32Ret  = cmos_init_awb_exp_function(&stAwbRegister.stSnsExp);
    s32Ret |= SC_MPI_AWB_SensorRegCallBack(ViPipe, pstAwbLib, &stSnsAttrInfo, &stAwbRegister);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor register callback function to awb lib failed!\n");
        return s32Ret;
    }

    return SC_SUCCESS;
}

static SC_S32 gc2053_unregister_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    SC_S32 s32Ret = SC_FAILURE;

    CMOS_CHECK_POINTER(pstAeLib);
    CMOS_CHECK_POINTER(pstAwbLib);

    s32Ret = SC_MPI_ISP_SensorUnRegCallBack(ViPipe, GC2053_ID);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor unregister callback function failed!\n");
        return s32Ret;
    }

    s32Ret = SC_MPI_AE_SensorUnRegCallBack(ViPipe, pstAeLib, GC2053_ID);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor unregister callback function to ae lib failed!\n");
        return s32Ret;
    }

    s32Ret = SC_MPI_AWB_SensorUnRegCallBack(ViPipe, pstAwbLib, GC2053_ID);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor unregister callback function to awb lib failed!\n");
        return s32Ret;
    }

    gc2053_ctx_exit(ViPipe);

    return SC_SUCCESS;
}

static SC_S32 gc2053_set_init(VI_PIPE ViPipe, ISP_INIT_ATTR_S *pstInitAttr)
{
    CMOS_CHECK_POINTER(pstInitAttr);

    g_au32InitExposure[ViPipe]  = pstInitAttr->u32Exposure;
    g_au32LinesPer500ms[ViPipe] = pstInitAttr->u32LinesPer500ms;
    g_au16InitWBGain[ViPipe][0] = pstInitAttr->u16WBRgain;
    g_au16InitWBGain[ViPipe][1] = pstInitAttr->u16WBGgain;
    g_au16InitWBGain[ViPipe][2] = pstInitAttr->u16WBBgain;
    g_au16SampleRgain[ViPipe]   = pstInitAttr->u16SampleRgain;
    g_au16SampleBgain[ViPipe]   = pstInitAttr->u16SampleBgain;

    return SC_SUCCESS;
}

ISP_SNS_OBJ_S stSnsGc2053Obj =
{
    .pfnRegisterCallback    = gc2053_register_callback,
    .pfnUnRegisterCallback  = gc2053_unregister_callback,
    .pfnStandby             = gc2053_standby,
    .pfnRestart             = gc2053_restart,
    .pfnMirrorFlip          = SC_NULL,
    .pfnWriteReg            = gc2053_write_register,
    .pfnReadReg             = gc2053_read_register,
    .pfnSetBusInfo          = gc2053_set_bus_info,
    .pfnSetInit             = gc2053_set_init
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __GC2053_CMOS_C_ */
