/**
 * @file     imx307_cmos.c
 * @brief    SONY IMX307 SENSOR控制接口
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2021-11-15 创建文件
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


#if !defined(__IMX307_CMOS_C_)
#define __IMX307_CMOS_C_

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


#define IMX307_ID 307


/****************************************************************************
 * global variables                                                         *
 ****************************************************************************/
ISP_SNS_STATE_S *g_pastImx307[ISP_MAX_PIPE_NUM] = {SC_NULL};

#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_pastImx307[dev])
#define SENSOR_SET_CTX(dev, pstCtx) (g_pastImx307[dev] = pstCtx)
#define SENSOR_RESET_CTX(dev)       (g_pastImx307[dev] = SC_NULL)

ISP_SNS_COMMBUS_U g_aunImx307BusInfo[ISP_MAX_PIPE_NUM] =
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

static SENSOR_STATE_S g_astimx307State[ISP_MAX_PIPE_NUM] = {{0}};

static SC_TUNING_BIN_FILE_S g_acTuningBinName[ISP_MAX_PIPE_NUM] =
{
    [0] = {
              .acLinearBinName = "/local/tuning/imx307/imx307_tuning.bin",
              .acWdrBinName    = "/local/tuning/imx307/imx307_tuning_hdr.bin"
          },

    [1] = {
              .acLinearBinName = "/local/tuning/imx307/imx307_tuning_night.bin",
              .acWdrBinName    = "/local/tuning/imx307/imx307_tuning_hdr.bin"
          },
    [2] = {
              .acLinearBinName = "/local/tuning/imx307/imx307_tuning.bin",
              .acWdrBinName    = "/local/tuning/imx307/imx307_tuning_hdr.bin"
          },

    [3] = {
              .acLinearBinName = "/local/tuning/imx307/imx307_tuning.bin",
              .acWdrBinName    = "/local/tuning/imx307/imx307_tuning_hdr.bin"
          },
};



/****************************************************************************
 * extern                                                                   *
 ****************************************************************************/
extern const unsigned int imx307_i2c_addr;
extern unsigned int imx307_addr_byte;
extern unsigned int imx307_data_byte;

extern int  imx307_init(VI_PIPE ViPipe);
extern void imx307_exit(VI_PIPE ViPipe);
extern void imx307_standby(VI_PIPE ViPipe);
extern void imx307_restart(VI_PIPE ViPipe);
extern void imx307_default_reg_init(VI_PIPE ViPipe);
extern int  imx307_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  imx307_read_register(VI_PIPE ViPipe, int addr);
extern int  imx307_flip_off_mirror_off(VI_PIPE ViPipe);
extern int  imx307_flip_on_mirror_off(VI_PIPE ViPipe);
extern int  imx307_flip_off_mirror_on(VI_PIPE ViPipe);
extern int  imx307_flip_on_mirror_on(VI_PIPE ViPipe);



/****************************************************************************
 * local variables                                                          *
 ****************************************************************************/
/* Sensor register address */
#define IMX307_SHS1_ADDR       (0x3020)
#define IMX307_SHS2_ADDR       (0x3024)
#define IMX307_GAIN_ADDR       (0x3014)
#define IMX307_SGAIN_ADDR      (0x30f2)
#define IMX307_HCG_ADDR        (0x3009)
#define IMX307_VMAX_ADDR       (0x3018)
#define IMX307_HMAX_ADDR       (0x301c)
#define IMX307_RHS1_ADDR       (0x3030)
#define IMX307_RHS2_ADDR       (0x3034)
#define IMX307_Y_OUT_SIZE_ADDR (0x3418)

#define SENSOR_FULL_LINES_MAX          (0x3FFFF)
#define SENSOR_FULL_LINES_MAX_2TO1_WDR (0x966)  /* considering the YOUT_SIZE and bad frame */

#define SENSOR_INCREASE_LINES       (1) /* make real fps less than stand fps because NVR require */
#define SENSOR_VMAX_1080P30_LINEAR  (1125 + SENSOR_INCREASE_LINES)
#define SENSOR_VMAX_1080P60TO30_WDR (1219 + SENSOR_INCREASE_LINES)

#define SENSOR_1080P_30FPS_LINEAR_MODE  (1)
#define SENSOR_1080P_30FPS_2T1_WDR_MODE (2)

#define SENSOR_RES_IS_1080P(w, h) ((w) <= 1920 && (h) <= 1080)
#define SENSOR_RES_IS_WDR(w, h)   ((w) <= 1952 && (h) <= 2678)

#define HIGHER_4BITS(x) (((x) & 0x30000) >> 16)
#define HIGH_8BITS(x)   (((x) & 0xff00) >> 8)
#define LOW_8BITS(x)    ((x) & 0x00ff)



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
            pstAeSnsDft->u32MaxAgain   = 2886024;
            pstAeSnsDft->u32MinAgain   = 1024;
            pstAeSnsDft->u32MaxDgain   = 2886024;
            pstAeSnsDft->u32MinDgain   = 1024;
            pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd - 2;
            pstAeSnsDft->u32MinIntTime = 1;

            break;
        }

        case WDR_MODE_2To1_LINE:
        {
            /* Wdr mode */
            pstAeSnsDft->u32MaxAgain   = 2886024;
            pstAeSnsDft->u32MinAgain   = 1024;
            pstAeSnsDft->u32MaxDgain   = 2886024;
            pstAeSnsDft->u32MinDgain   = 1024;
            pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd - 2;
            pstAeSnsDft->u32MinIntTime = 2;

            pstAeSnsDft->u32FullLinesShort = 461;
            pstAeSnsDft->u32LFMaxShortTime = 461;
            pstAeSnsDft->u32LFMinExposure  = 2;
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
        case SENSOR_1080P_30FPS_2T1_WDR_MODE:
        {
            if((f32Fps <= pstAeSnsDft->f32MaxFps) && (f32Fps >= 15.22))
            {
                u32VMAX = SENSOR_VMAX_1080P60TO30_WDR * pstAeSnsDft->f32MaxFps / DIV_0_TO_1_FLOAT(f32Fps);
            }
            else
            {
                printf("Not support Fps: %f\n", f32Fps);
                return;
            }

            u32VMAX = (u32VMAX > SENSOR_FULL_LINES_MAX_2TO1_WDR) ? SENSOR_FULL_LINES_MAX_2TO1_WDR : u32VMAX;
            break;
        }

        case SENSOR_1080P_30FPS_LINEAR_MODE:
        {
            if((f32Fps <= pstAeSnsDft->f32MaxFps) && (f32Fps >= 0.119))
            {
                u32VMAX = SENSOR_VMAX_1080P30_LINEAR * pstAeSnsDft->f32MaxFps / DIV_0_TO_1_FLOAT(f32Fps);
            }
            else
            {
                printf("Not support Fps: %f\n", f32Fps);
                return;
            }

            u32VMAX = (u32VMAX > SENSOR_FULL_LINES_MAX) ? SENSOR_FULL_LINES_MAX : u32VMAX;
            break;
        }

        default:
        {
            return;
        }
    }

    if(WDR_MODE_NONE == pstSnsState->enWDRMode)
    {
        pstSnsState->astRegsInfo[0].astI2cData[5].u32Data = LOW_8BITS(u32VMAX);
        pstSnsState->astRegsInfo[0].astI2cData[6].u32Data = HIGH_8BITS(u32VMAX);
        pstSnsState->astRegsInfo[0].astI2cData[7].u32Data = HIGHER_4BITS(u32VMAX);
    }
    else
    {
        pstSnsState->astRegsInfo[0].astI2cData[9].u32Data  = LOW_8BITS(u32VMAX);
        pstSnsState->astRegsInfo[0].astI2cData[10].u32Data = HIGH_8BITS(u32VMAX);
        pstSnsState->astRegsInfo[0].astI2cData[11].u32Data = HIGHER_4BITS(u32VMAX);
    }

    if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
    {
        pstSnsState->u32FLStd   = u32VMAX * 2 + 80;
        pstSnsState->u32FLShort = pstAeSnsDft->u32FullLinesShort;
        //printf("gu32FullLinesStd:%d\n",pstSnsState->u32FLStd);

        /*
            RHS1 limitation:
            2n + 5
            RHS1 <= FSC - BRL*2 -21
            (2 * VMAX_IMX307_1080P30_WDR - 2 * gu32BRL - 21) - (((2 * VMAX_IMX307_1080P30_WDR - 2 * 1109 - 21) - 5) %2)
        */
        g_astimx307State[ViPipe].u32RHS1_MAX = (u32VMAX - g_astimx307State[ViPipe].u32BRL) * 2 - 21;
    }
    else
    {
        pstSnsState->u32FLStd = u32VMAX;
    }

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

    if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
    {
        u32FullLines = (u32FullLines > 2 * SENSOR_FULL_LINES_MAX_2TO1_WDR) ? 2 * SENSOR_FULL_LINES_MAX_2TO1_WDR : u32FullLines;
        pstSnsState->au32FL[0] = (u32FullLines >> 1) << 1;
        g_astimx307State[ViPipe].u32RHS1_MAX = pstSnsState->au32FL[0] - g_astimx307State[ViPipe].u32BRL * 2 - 21;
    }
    else
    {
        u32FullLines = (u32FullLines > SENSOR_FULL_LINES_MAX) ? SENSOR_FULL_LINES_MAX : u32FullLines;
        pstSnsState->au32FL[0] = u32FullLines;
    }

    if(WDR_MODE_NONE == pstSnsState->enWDRMode)
    {
        pstSnsState->astRegsInfo[0].astI2cData[5].u32Data = LOW_8BITS(pstSnsState->au32FL[0]);
        pstSnsState->astRegsInfo[0].astI2cData[6].u32Data = HIGH_8BITS(pstSnsState->au32FL[0]);
        pstSnsState->astRegsInfo[0].astI2cData[7].u32Data = HIGHER_4BITS(pstSnsState->au32FL[0]);
    }
    else if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
    {
        pstSnsState->astRegsInfo[0].astI2cData[8].u32Data = LOW_8BITS((pstSnsState->au32FL[0] >> 1));
        pstSnsState->astRegsInfo[0].astI2cData[9].u32Data = HIGH_8BITS((pstSnsState->au32FL[0] >> 1));
        pstSnsState->astRegsInfo[0].astI2cData[10].u32Data = HIGHER_4BITS((pstSnsState->au32FL[0] >> 1));
    }
    else
    {
    }

    pstAeSnsDft->u32FullLines  = pstSnsState->au32FL[0];
    pstAeSnsDft->u32MaxIntTime = pstSnsState->au32FL[0] - 2;

    return;
}

/* while isp notify ae to update sensor regs, ae call these funcs. */
static SC_VOID cmos_inttime_update(VI_PIPE ViPipe, SC_U32 u32IntTime)
{
    static SC_BOOL bFirst[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = 1};

    static SC_U32 u32ShortIntTime[ISP_MAX_PIPE_NUM]  = {0};
    static SC_U32 u32LongIntTime[ISP_MAX_PIPE_NUM]   = {0};

    static SC_S32 s32SHS1[ISP_MAX_PIPE_NUM]  = {0};
    static SC_S32 s32SHS2[ISP_MAX_PIPE_NUM]  = {0};

    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SC_S32 u32Value = 0;

    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
    {
        u32IntTime = u32IntTime * 2;
        if(bFirst[ViPipe])
        {
            /* short exposure */
            pstSnsState->au32WDRIntTime[0] = u32IntTime;
            u32ShortIntTime[ViPipe]        = u32IntTime;
            s32SHS1[ViPipe]                = pstSnsState->u32FLShort - u32ShortIntTime[ViPipe];
            //printf("sns[%d]: shtInt=%d, FLS=%d, SHS1=%d\n", ViPipe, u32ShortIntTime[ViPipe], pstSnsState->u32FLShort, s32SHS1[ViPipe]);
            if(s32SHS1[ViPipe] <= 2)
            {
                s32SHS1[ViPipe] = 2;
            }
            else if(s32SHS1[ViPipe] >= pstSnsState->u32FLShort - 2)
            {
                s32SHS1[ViPipe] = pstSnsState->u32FLShort - 2;
            }

            bFirst[ViPipe] = SC_FALSE;
        }
        else
        {
            /* long exposure */
            pstSnsState->au32WDRIntTime[1] = u32IntTime;
            u32LongIntTime[ViPipe]         = u32IntTime;
            s32SHS2[ViPipe]                = pstSnsState->au32FL[1] - u32LongIntTime[ViPipe];
            //printf("sns[%d]: longInt=%d, FL1=%d, SHS2=%d\n", ViPipe, u32LongIntTime[ViPipe], pstSnsState->au32FL[1], s32SHS2[ViPipe]);
            if(s32SHS2[ViPipe] <= pstSnsState->u32FLShort + 2)
            {
                /* This is not same as datasheet with >= RSH1 + 2 */
                s32SHS2[ViPipe] = pstSnsState->u32FLShort + 2;
            }
            else if(s32SHS2[ViPipe] >= pstSnsState->au32FL[1] - 2)
            {
                s32SHS2[ViPipe] =  pstSnsState->au32FL[1] - 2;
            }

            pstSnsState->astRegsInfo[0].astI2cData[0].u32Data = LOW_8BITS(s32SHS2[ViPipe]);
            pstSnsState->astRegsInfo[0].astI2cData[1].u32Data = HIGH_8BITS(s32SHS2[ViPipe]);
            pstSnsState->astRegsInfo[0].astI2cData[2].u32Data = HIGHER_4BITS(s32SHS2[ViPipe]);

            pstSnsState->astRegsInfo[0].astI2cData[6].u32Data = LOW_8BITS(s32SHS1[ViPipe]);
            pstSnsState->astRegsInfo[0].astI2cData[7].u32Data = HIGH_8BITS(s32SHS1[ViPipe]);
            pstSnsState->astRegsInfo[0].astI2cData[8].u32Data = HIGHER_4BITS(s32SHS1[ViPipe]);

            bFirst[ViPipe] = SC_TRUE;
        }
    }
    else
    {
        u32Value = pstSnsState->au32FL[0] - u32IntTime - 1;
        //printf("sns[%d]: ln=%d, u32Value=%u\n", ViPipe, u32IntTime, u32Value);
        pstSnsState->astRegsInfo[0].astI2cData[0].u32Data = LOW_8BITS(u32Value);
        pstSnsState->astRegsInfo[0].astI2cData[1].u32Data = HIGH_8BITS(u32Value);
        pstSnsState->astRegsInfo[0].astI2cData[2].u32Data = HIGHER_4BITS(u32Value);
        bFirst[ViPipe] = SC_TRUE;
    }

    return;
}

static SC_U32 gain_table[262] =
{
       1024,    1059,    1097,    1135,    1175,    1217,    1259,    1304,    1349,    1397,
       1446,    1497,    1549,    1604,    1660,    1719,    1779,    1842,    1906,    1973,
       2043,    2048,    2119,    2194,    2271,    2351,    2434,    2519,    2608,    2699,
       2794,    2892,    2994,    3099,    3208,    3321,    3438,    3559,    3684,    3813,
       3947,    4086,    4229,    4378,    4532,    4691,    4856,    5027,    5203,    5386,
       5576,    5772,    5974,    6184,    6402,    6627,    6860,    7101,    7350,    7609,
       7876,    8153,    8439,    8736,    9043,    9361,    9690,   10030,   10383,   10748,
      11125,   11516,   11921,   12340,   12774,   13222,   13687,   14168,   14666,   15182,
      15715,   16267,   16839,   17431,   18043,   18677,   19334,   20013,   20717,   21445,
      22198,   22978,   23786,   24622,   25487,   26383,   27310,   28270,   29263,   30292,
      31356,   32458,   33599,   34780,   36002,   37267,   38577,   39932,   41336,   42788,
      44292,   45849,   47460,   49128,   50854,   52641,   54491,   56406,   58388,   60440,
      62564,   64763,   67039,   69395,   71833,   74358,   76971,   79676,   82476,   85374,
      88375,   91480,   94695,   98023,  101468,  105034,  108725,  112545,  116501,  120595,
     124833,  129220,  133761,  138461,  143327,  148364,  153578,  158975,  164562,  170345,
     176331,  182528,  188942,  195582,  202455,  209570,  216935,  224558,  232450,  240619,
     249074,  257827,  266888,  276267,  285976,  296026,  306429,  317197,  328344,  339883,
     351827,  364191,  376990,  390238,  403952,  418147,  432842,  448053,  463799,  480098,
     496969,  514434,  532512,  551226,  570597,  590649,  611406,  632892,  655133,  678156,
     701988,  726657,  752194,  778627,  805990,  834314,  863634,  893984,  925400,  957921,
     991585, 1026431, 1062502, 1099841, 1138491, 1178500, 1219916, 1262786, 1307163, 1353100,
    1400651, 1449872, 1500824, 1553566, 1608162, 1664676, 1723177, 1783733, 1846417, 1911304,
    1978472, 2048000, 2119971, 2194471, 2271590, 2351418, 2434052, 2519590, 2608134, 2699789,
    2794666, 2892876, 2994538, 3099773, 3208706, 3321467, 3438190, 3559016, 3684087, 3813554,
    3947571, 4086297, 4229898, 4378546, 4532417, 4691696, 4856573, 5027243, 5203912, 5386788,
    5576092, 5772048, 5974890, 6184861, 6402210, 6627198, 6860092, 7101170, 7350721, 7609041,
    7876439, 8153234
};

static SC_VOID cmos_again_calc_table(VI_PIPE ViPipe, SC_U32 *pu32AgainLin, SC_U32 *pu32AgainDb)
{
    SC_S32 i        = 0;
    SC_U32 u32HCGOn = 0;

    CMOS_CHECK_POINTER_VOID(pu32AgainLin);
    CMOS_CHECK_POINTER_VOID(pu32AgainDb);

    if(*pu32AgainLin >= 65536)
    {
        /* Reg0x3009[7:0] bit[4] HCG */
        g_astimx307State[ViPipe].u8Hcg |= 0x10;
        u32HCGOn = 1;
    }

    if(*pu32AgainLin < 32768)
    {
        g_astimx307State[ViPipe].u8Hcg &= ~0x10;
        u32HCGOn = 0;
    }

    if(u32HCGOn)
    {
        *pu32AgainLin = *pu32AgainLin / 2;
    }

    if(*pu32AgainLin >= gain_table[230])
    {
        *pu32AgainLin = gain_table[230];
        *pu32AgainDb  = 230;
        return;
    }

    for(i = 1; i < 231; i++)
    {
        if(*pu32AgainLin < gain_table[i])
        {
            *pu32AgainLin = gain_table[i - 1];
            *pu32AgainDb  = i - 1;
            break;
        }
    }

    return;
}

static SC_VOID cmos_dgain_calc_table(VI_PIPE ViPipe, SC_U32 *pu32DgainLin, SC_U32 *pu32DgainDb)
{
    SC_S32 i        = 0;
    SC_U32 u32HCGOn = 0;

    CMOS_CHECK_POINTER_VOID(pu32DgainLin);
    CMOS_CHECK_POINTER_VOID(pu32DgainDb);

    if(*pu32DgainLin >= 65536)
    {
        /* Reg0x3009[7:0] bit[4] HCG */
        g_astimx307State[ViPipe].u8Hcg |= 0x10;
        u32HCGOn = 1;
    }

    if(*pu32DgainLin < 32768)
    {
        g_astimx307State[ViPipe].u8Hcg &= ~0x10;
        u32HCGOn = 0;
    }

    if(u32HCGOn)
    {
        *pu32DgainLin = *pu32DgainLin / 2;
    }

    if(*pu32DgainLin >= gain_table[230])
    {
        *pu32DgainLin = gain_table[230];
        *pu32DgainDb  = 230;
        return;
    }

    for(i = 1; i < 231; i++)
    {
        if(*pu32DgainLin < gain_table[i])
        {
            *pu32DgainLin = gain_table[i - 1];
            *pu32DgainDb  = i - 1;
            break;
        }
    }

    return;
}

static SC_VOID cmos_gains_update(VI_PIPE ViPipe, SC_U32 u32Again, SC_U32 u32Dgain)
{
    static SC_BOOL bFirst[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = 1};

    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SC_U32 u32HCG = g_astimx307State[ViPipe].u8Hcg;
    SC_U32 u32Tmp = 0;

    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    //printf("sns pp=%d, ag=%d, dg=%d, hcg=0x%x\n", ViPipe, u32Again, u32Dgain, u32HCG);
    u32Tmp = u32Again + u32Dgain;
    if(pstSnsState->enWDRMode == WDR_MODE_2To1_LINE)
    {
        if(bFirst[ViPipe])
        {
            /* short gain */
            bFirst[ViPipe] = SC_FALSE;
            pstSnsState->astRegsInfo[0].astI2cData[4].u32Data = LOW_8BITS(u32Tmp);
            pstSnsState->astRegsInfo[0].astI2cData[5].u32Data = LOW_8BITS(u32HCG);
        }
        else
        {
            /* long gain */
            bFirst[ViPipe] = SC_TRUE;
            pstSnsState->astRegsInfo[0].astI2cData[3].u32Data = LOW_8BITS(u32Tmp);
            pstSnsState->astRegsInfo[0].astI2cData[5].u32Data = LOW_8BITS(u32HCG);
        }
    }
    else if(pstSnsState->enWDRMode == WDR_MODE_NONE)
    {
        //printf("sns pp=%d, gn=%d, hcg=0x%x\n", ViPipe, u32Tmp, u32HCG);
        pstSnsState->astRegsInfo[0].astI2cData[3].u32Data = LOW_8BITS(u32Tmp);
        pstSnsState->astRegsInfo[0].astI2cData[4].u32Data = LOW_8BITS(u32HCG);
    }

    return;
}

static SC_VOID cmos_get_inttime_max(VI_PIPE ViPipe, SC_U16 u16ManRatioEnable, SC_U32 *au32Ratio,
    SC_U32 *au32IntTimeMax, SC_U32 *au32IntTimeMin, SC_U32 *pu32LFMaxIntTime)
{
    SC_U32 u32IntTimeMaxTmp0    = 0;
    SC_U32 u32IntTimeMaxTmp     = 0;
    SC_U32 u32RatioTmp          = 0x40;
    SC_U32 u32ShortTimeMinLimit = 0;

    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    CMOS_CHECK_POINTER_VOID(au32Ratio);
    CMOS_CHECK_POINTER_VOID(au32IntTimeMax);
    CMOS_CHECK_POINTER_VOID(au32IntTimeMin);
    CMOS_CHECK_POINTER_VOID(pu32LFMaxIntTime);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    u32ShortTimeMinLimit = 2;
    if(pstSnsState->enWDRMode == WDR_MODE_2To1_LINE)
    {
        /*  limitation for DOL 2t1

            SHS1 limitation:
            2 or more
            RHS1 - 2 or less

            SHS2 limitation:
            RHS1 + 2 or more
            FSC - 2 or less

            RHS1 Limitation
            2n + 5 (n = 0,1,2...)
            RHS1 <= FSC - BRL * 2 - 21

            short exposure time = RHS1 - (SHS1 + 1) <= RHS1 - 3
            long exposure time = FSC - (SHS2 + 1) <= FSC - (RHS1 + 3)
            ExposureShort + ExposureLong <= FSC - 6
            short exposure time <= (FSC - 6) / (ratio + 1)
        */
        if(ISP_FSWDR_LONG_FRAME_MODE == genFSWDRMode[ViPipe])
        {
            u32IntTimeMaxTmp0 = pstSnsState->au32FL[1] - 6 - pstSnsState->au32WDRIntTime[0];
            u32IntTimeMaxTmp  = pstSnsState->au32FL[0] - 10;
            u32IntTimeMaxTmp  = (u32IntTimeMaxTmp0 < u32IntTimeMaxTmp) ? u32IntTimeMaxTmp0 : u32IntTimeMaxTmp;
            au32IntTimeMax[0] = u32IntTimeMaxTmp;
            au32IntTimeMin[0] = u32ShortTimeMinLimit;
            return;
        }
        else
        {
            u32IntTimeMaxTmp0 = ((pstSnsState->au32FL[1] - 6 - pstSnsState->au32WDRIntTime[0]) * 0x40)  / DIV_0_TO_1(au32Ratio[0]);
            u32IntTimeMaxTmp  = ((pstSnsState->au32FL[0] - 6) * 0x40)  / DIV_0_TO_1(au32Ratio[0] + 0x40);
            u32IntTimeMaxTmp  = (u32IntTimeMaxTmp0 < u32IntTimeMaxTmp) ? u32IntTimeMaxTmp0 : u32IntTimeMaxTmp;
            u32IntTimeMaxTmp  = (u32IntTimeMaxTmp > (g_astimx307State[ViPipe].u32RHS1_MAX - 3)) ? (g_astimx307State[ViPipe].u32RHS1_MAX - 3) : u32IntTimeMaxTmp;
            u32IntTimeMaxTmp  = (0 == u32IntTimeMaxTmp) ? 1 : u32IntTimeMaxTmp;
        }
    }
    else
    {
    }

    if(u32IntTimeMaxTmp >= u32ShortTimeMinLimit)
    {
        if(IS_LINE_WDR_MODE(pstSnsState->enWDRMode))
        {
            au32IntTimeMax[0] = u32IntTimeMaxTmp;
            au32IntTimeMax[1] = au32IntTimeMax[0] * au32Ratio[0] >> 6;
            au32IntTimeMax[2] = au32IntTimeMax[1] * au32Ratio[1] >> 6;
            au32IntTimeMax[3] = au32IntTimeMax[2] * au32Ratio[2] >> 6;
            au32IntTimeMin[0] = u32ShortTimeMinLimit;
            au32IntTimeMin[1] = au32IntTimeMin[0] * au32Ratio[0] >> 6;
            au32IntTimeMin[2] = au32IntTimeMin[1] * au32Ratio[1] >> 6;
            au32IntTimeMin[3] = au32IntTimeMin[2] * au32Ratio[2] >> 6;
        }
        else
        {
        }
    }
    else
    {
        if(1 == u16ManRatioEnable)
        {
            printf("Manual ExpRatio is too large!\n");
            return;
        }
        else
        {
            u32IntTimeMaxTmp = u32ShortTimeMinLimit;
            if(pstSnsState->enWDRMode == WDR_MODE_2To1_LINE)
            {
                u32RatioTmp       = 0xFFF;
                au32IntTimeMax[0] = u32IntTimeMaxTmp;
                au32IntTimeMax[1] = au32IntTimeMax[0] * u32RatioTmp >> 6;
            }
            else
            {
            }

            au32IntTimeMin[0] = au32IntTimeMax[0];
            au32IntTimeMin[1] = au32IntTimeMax[1];
            au32IntTimeMin[2] = au32IntTimeMax[2];
            au32IntTimeMin[3] = au32IntTimeMax[3];
        }
    }

    return;
}

/* Only used in LINE_WDR mode */
static SC_VOID cmos_ae_fswdr_attr_set(VI_PIPE ViPipe, AE_FSWDR_ATTR_S *pstAeFSWDRAttr)
{
    CMOS_CHECK_POINTER_VOID(pstAeFSWDRAttr);

    genFSWDRMode[ViPipe]      = pstAeFSWDRAttr->enFSWDRMode;
    gu32MaxTimeGetCnt[ViPipe] = 0;
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
    pstExpFuncs->pfn_cmos_dgain_calc_table   = cmos_dgain_calc_table;
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

        case WDR_MODE_2To1_LINE:
        {
            strncpy(pstDef->acTuningPraBinName, g_acTuningBinName[ViPipe].acWdrBinName, TUNING_BIN_FILENAME_LEN);
            break;
        }
    }

    if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
    {
        pstDef->stWdrSwitchAttr.au32ExpRatio[0] = 0x40;
        pstDef->stWdrSwitchAttr.au32ExpRatio[1] = 0x40;
        pstDef->stWdrSwitchAttr.au32ExpRatio[2] = 0x40;
        pstDef->stWdrSwitchAttr.au32ShortOffset = 475;
        pstDef->stWdrSwitchAttr.au32MidOffset   = 475;
        pstDef->stWdrSwitchAttr.au32LongOffset  = 14;
    }

    pstDef->stSensorMode.u32SensorID  = IMX307_ID;
    pstDef->stSensorMode.u8SensorMode = pstSnsState->u8ImgMode;
    memcpy(&pstDef->stDngColorParam, &g_stDngColorParam, sizeof(ISP_CMOS_DNG_COLORPARAM_S));

    switch(pstSnsState->u8ImgMode)
    {
        default:
        case SENSOR_1080P_30FPS_LINEAR_MODE:
        {
            pstDef->stSensorMode.stDngRawFormat.u8BitsPerSample = 12;
            pstDef->stSensorMode.stDngRawFormat.u32WhiteLevel   = 4095;
            break;
        }

        case SENSOR_1080P_30FPS_2T1_WDR_MODE:
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

    SC_U32 u32FullLines_5Fps  = 0;
    SC_U32 u32MaxIntTime_5Fps = 0;

    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
    {
        return;
    }
    else
    {
        if(SENSOR_1080P_30FPS_LINEAR_MODE == pstSnsState->u8ImgMode)
        {
            u32FullLines_5Fps = (SENSOR_VMAX_1080P30_LINEAR * 30) / 5;
        }
        else
        {
            return;
        }
    }

    u32MaxIntTime_5Fps = 4;
    if(bEnable)
    {
        /* setup for ISP pixel calibration mode */
        imx307_write_register(ViPipe, IMX307_GAIN_ADDR, 0x00);

        imx307_write_register(ViPipe, IMX307_VMAX_ADDR, u32FullLines_5Fps & 0xFF);
        imx307_write_register(ViPipe, IMX307_VMAX_ADDR + 1, (u32FullLines_5Fps & 0xFF00) >> 8);
        imx307_write_register(ViPipe, IMX307_VMAX_ADDR + 2, (u32FullLines_5Fps & 0x30000) >> 16);

        imx307_write_register(ViPipe, IMX307_SHS1_ADDR, u32MaxIntTime_5Fps & 0xFF);
        imx307_write_register(ViPipe, IMX307_SHS1_ADDR + 1,  (u32MaxIntTime_5Fps & 0xFF00) >> 8);
        imx307_write_register(ViPipe, IMX307_SHS1_ADDR + 2, (u32MaxIntTime_5Fps & 0x30000) >> 16);
    }
    else
    {
        /* setup for ISP 'normal mode' */
        pstSnsState->u32FLStd = (pstSnsState->u32FLStd > 0x1FFFF) ? 0x1FFFF : pstSnsState->u32FLStd;
        imx307_write_register (ViPipe, IMX307_VMAX_ADDR, pstSnsState->u32FLStd & 0xFF);
        imx307_write_register (ViPipe, IMX307_VMAX_ADDR + 1, (pstSnsState->u32FLStd & 0xFF00) >> 8);
        imx307_write_register(ViPipe, IMX307_VMAX_ADDR + 2, (pstSnsState->u32FLStd & 0x30000) >> 16);
        pstSnsState->bSyncInit = SC_FALSE;
    }

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

        case WDR_MODE_2To1_LINE:
        {
            pstSnsState->enWDRMode = WDR_MODE_2To1_LINE;
            printf("cmos_set_wdr_mode: 2to1 line WDR 1080p mode(60fps->30fps)\n");
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
        pstSnsState->astRegsInfo[0].unComBus.s8I2cDev   = g_aunImx307BusInfo[ViPipe].s8I2cDev;
        pstSnsState->astRegsInfo[0].u8Cfg2ValidDelayMax = 2;
        pstSnsState->astRegsInfo[0].u32RegNum           = 8;

        if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
        {
            pstSnsState->astRegsInfo[0].u32RegNum += 4;
            pstSnsState->astRegsInfo[0].u8Cfg2ValidDelayMax = 2;
        }

        for(i = 0; i < pstSnsState->astRegsInfo[0].u32RegNum; i++)
        {
            pstSnsState->astRegsInfo[0].astI2cData[i].bUpdate        = SC_TRUE;
            pstSnsState->astRegsInfo[0].astI2cData[i].u8DevAddr      = imx307_i2c_addr;
            pstSnsState->astRegsInfo[0].astI2cData[i].u32AddrByteNum = imx307_addr_byte;
            pstSnsState->astRegsInfo[0].astI2cData[i].u32DataByteNum = imx307_data_byte;
        }

        /* Linear Mode Regs */
        pstSnsState->astRegsInfo[0].astI2cData[0].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[0].u32RegAddr    = IMX307_SHS1_ADDR;
        pstSnsState->astRegsInfo[0].astI2cData[1].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[1].u32RegAddr    = IMX307_SHS1_ADDR + 1;
        pstSnsState->astRegsInfo[0].astI2cData[2].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[2].u32RegAddr    = IMX307_SHS1_ADDR + 2;

        /* Make shutter and gain effective at the same time */
        pstSnsState->astRegsInfo[0].astI2cData[3].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[3].u32RegAddr    = IMX307_GAIN_ADDR;
        pstSnsState->astRegsInfo[0].astI2cData[4].u8DelayFrmNum = 1;
        pstSnsState->astRegsInfo[0].astI2cData[4].u32RegAddr    = IMX307_HCG_ADDR;

        pstSnsState->astRegsInfo[0].astI2cData[5].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[5].u32RegAddr    = IMX307_VMAX_ADDR;
        pstSnsState->astRegsInfo[0].astI2cData[6].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[6].u32RegAddr    = IMX307_VMAX_ADDR + 1;
        pstSnsState->astRegsInfo[0].astI2cData[7].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[7].u32RegAddr    = IMX307_VMAX_ADDR + 2;

        /* DOL 2t1 Mode Regs */
        if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
        {
            pstSnsState->astRegsInfo[0].astI2cData[0].u8DelayFrmNum = 0;
            pstSnsState->astRegsInfo[0].astI2cData[1].u8DelayFrmNum = 0;
            pstSnsState->astRegsInfo[0].astI2cData[2].u8DelayFrmNum = 0;

            pstSnsState->astRegsInfo[0].astI2cData[3].u8DelayFrmNum = 0;
            pstSnsState->astRegsInfo[0].astI2cData[3].u32RegAddr    = IMX307_GAIN_ADDR;
            pstSnsState->astRegsInfo[0].astI2cData[4].u8DelayFrmNum = 0;
            pstSnsState->astRegsInfo[0].astI2cData[4].u32RegAddr    = IMX307_SGAIN_ADDR;
            pstSnsState->astRegsInfo[0].astI2cData[5].u8DelayFrmNum = 1;
            pstSnsState->astRegsInfo[0].astI2cData[5].u32RegAddr    = IMX307_HCG_ADDR;

            pstSnsState->astRegsInfo[0].astI2cData[6].u8DelayFrmNum = 0;
            pstSnsState->astRegsInfo[0].astI2cData[6].u32RegAddr    = IMX307_SHS2_ADDR;
            pstSnsState->astRegsInfo[0].astI2cData[7].u8DelayFrmNum = 0;
            pstSnsState->astRegsInfo[0].astI2cData[7].u32RegAddr    = IMX307_SHS2_ADDR + 1;
            pstSnsState->astRegsInfo[0].astI2cData[8].u8DelayFrmNum = 0;
            pstSnsState->astRegsInfo[0].astI2cData[8].u32RegAddr    = IMX307_SHS2_ADDR + 2;

            pstSnsState->astRegsInfo[0].astI2cData[9].u8DelayFrmNum  = 1;
            pstSnsState->astRegsInfo[0].astI2cData[9].u32RegAddr     = IMX307_VMAX_ADDR;
            pstSnsState->astRegsInfo[0].astI2cData[10].u8DelayFrmNum = 1;
            pstSnsState->astRegsInfo[0].astI2cData[10].u32RegAddr    = IMX307_VMAX_ADDR + 1;
            pstSnsState->astRegsInfo[0].astI2cData[11].u8DelayFrmNum = 1;
            pstSnsState->astRegsInfo[0].astI2cData[11].u32RegAddr    = IMX307_VMAX_ADDR + 2;
        }

        pstSnsState->bSyncInit = SC_TRUE;
    }
    else
    {
        for(i = 0; i < pstSnsState->astRegsInfo[0].u32RegNum; i++)
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
    }

    memcpy(pstSnsRegsInfo, &pstSnsState->astRegsInfo[0], sizeof(ISP_SNS_REGS_INFO_S));
    memcpy(&pstSnsState->astRegsInfo[1], &pstSnsState->astRegsInfo[0], sizeof(ISP_SNS_REGS_INFO_S));

    pstSnsState->au32FL[1] = pstSnsState->au32FL[0];

    /* Set real register for aec */
    imx307_default_reg_init(ViPipe);

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
            g_astimx307State[ViPipe].u8Hcg = 0x2;
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
        else if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
        {
            g_astimx307State[ViPipe].u8Hcg = 0x1;
            if(SENSOR_RES_IS_1080P(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height))
            {
                u8SensorImageMode               = SENSOR_1080P_30FPS_2T1_WDR_MODE;
                pstSnsState->u32FLStd           = SENSOR_VMAX_1080P60TO30_WDR * 2;
                g_astimx307State[ViPipe].u32BRL = 1109;
            }
            else if(SENSOR_RES_IS_WDR(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height))
            {
                u8SensorImageMode               = SENSOR_1080P_30FPS_2T1_WDR_MODE;
                pstSnsState->u32FLStd           = SENSOR_VMAX_1080P60TO30_WDR * 2 + 340;
                g_astimx307State[ViPipe].u32BRL = 1109;
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
    else
    {
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

static SC_VOID imx307_global_init(VI_PIPE ViPipe)
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
            ret = imx307_flip_off_mirror_off(ViPipe);
            break;
        }

        case ISP_SNS_MIRROR:
        {
            ret = imx307_flip_off_mirror_on(ViPipe);
            break;
        }

        case ISP_SNS_FLIP:
        {
            ret = imx307_flip_on_mirror_off(ViPipe);
            break;
        }

        case ISP_SNS_MIRROR_FLIP:
        {
            ret = imx307_flip_on_mirror_on(ViPipe);
            break;
        }

        default:
        {
        break;
        }
    }

    return ret;
}

static SC_S32 cmos_imx307_ctl(VI_PIPE ViPipe, ISP_CMOS_SENSOR_CTL *pSensorCtl)
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

static SC_S32 cmos_init_imx307_exp_function(ISP_SENSOR_EXP_FUNC_S *pstSensorExpFunc)
{
    CMOS_CHECK_POINTER(pstSensorExpFunc);

    memset(pstSensorExpFunc, 0, sizeof(ISP_SENSOR_EXP_FUNC_S));
    pstSensorExpFunc->pfn_cmos_sensor_init        = imx307_init;
    pstSensorExpFunc->pfn_cmos_sensor_exit        = imx307_exit;
    pstSensorExpFunc->pfn_cmos_sensor_global_init = imx307_global_init;
    pstSensorExpFunc->pfn_cmos_set_image_mode     = cmos_set_image_mode;
    pstSensorExpFunc->pfn_cmos_set_wdr_mode       = cmos_set_wdr_mode;

    pstSensorExpFunc->pfn_cmos_get_isp_default  = cmos_get_isp_default;
    pstSensorExpFunc->pfn_cmos_set_pixel_detect = cmos_set_pixel_detect;
    pstSensorExpFunc->pfn_cmos_get_sns_reg_info = cmos_get_sns_regs_info;

    pstSensorExpFunc->pfn_cmos_sns_power_on  = cmos_power_on;
    pstSensorExpFunc->pfn_cmos_sns_power_off = cmos_power_off;
    pstSensorExpFunc->pfn_cmos_sns_ctl       = cmos_imx307_ctl;

    return SC_SUCCESS;
}


/****************************************************************************
 * callback structure                                                       *
 ****************************************************************************/
static SC_S32 imx307_set_bus_info(VI_PIPE ViPipe, ISP_SNS_COMMBUS_U unSNSBusInfo)
{
    g_aunImx307BusInfo[ViPipe].s8I2cDev = unSNSBusInfo.s8I2cDev;

    return SC_SUCCESS;
}

static SC_S32 imx307_ctx_init(VI_PIPE ViPipe)
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

static SC_VOID imx307_ctx_exit(VI_PIPE ViPipe)
{
    ISP_SNS_STATE_S *pastSnsStateCtx = SC_NULL;

    SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);
    SENSOR_FREE(pastSnsStateCtx);
    SENSOR_RESET_CTX(ViPipe);
}

static SC_S32 imx307_register_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    SC_S32 s32Ret = SC_FAILURE;

    ISP_SENSOR_REGISTER_S stIspRegister;
    AE_SENSOR_REGISTER_S  stAeRegister;
    AWB_SENSOR_REGISTER_S stAwbRegister;
    ISP_SNS_ATTR_INFO_S   stSnsAttrInfo;

    CMOS_CHECK_POINTER(pstAeLib);
    CMOS_CHECK_POINTER(pstAwbLib);

    s32Ret = imx307_ctx_init(ViPipe);
    if(SC_SUCCESS != s32Ret)
    {
        return SC_FAILURE;
    }

    stSnsAttrInfo.eSensorId = IMX307_ID;

    s32Ret  = cmos_init_imx307_exp_function(&stIspRegister.stSnsExp);
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

static SC_S32 imx307_unregister_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    SC_S32 s32Ret = SC_FAILURE;

    CMOS_CHECK_POINTER(pstAeLib);
    CMOS_CHECK_POINTER(pstAwbLib);

    s32Ret = SC_MPI_ISP_SensorUnRegCallBack(ViPipe, IMX307_ID);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor unregister callback function failed!\n");
        return s32Ret;
    }

    s32Ret = SC_MPI_AE_SensorUnRegCallBack(ViPipe, pstAeLib, IMX307_ID);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor unregister callback function to ae lib failed!\n");
        return s32Ret;
    }

    s32Ret = SC_MPI_AWB_SensorUnRegCallBack(ViPipe, pstAwbLib, IMX307_ID);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor unregister callback function to awb lib failed!\n");
        return s32Ret;
    }

    imx307_ctx_exit(ViPipe);

    return SC_SUCCESS;
}

static SC_S32 imx307_set_init(VI_PIPE ViPipe, ISP_INIT_ATTR_S *pstInitAttr)
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

ISP_SNS_OBJ_S stSnsImx307Obj =
{
    .pfnRegisterCallback    = imx307_register_callback,
    .pfnUnRegisterCallback  = imx307_unregister_callback,
    .pfnStandby             = imx307_standby,
    .pfnRestart             = imx307_restart,
    .pfnMirrorFlip          = SC_NULL,
    .pfnWriteReg            = imx307_write_register,
    .pfnReadReg             = imx307_read_register,
    .pfnSetBusInfo          = imx307_set_bus_info,
    .pfnSetInit             = imx307_set_init
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __IMX307_CMOS_C_ */
