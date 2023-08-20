/**
 * @file     sc_mipi.h
 * @brief    MIPI操作相关的宏、枚举和结构体类型定义
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

#ifndef __SC_MIPI_H__
#define __SC_MIPI_H__

typedef unsigned int combo_dev_t;

#define MIPI_LANE_NUM           8

#define WDR_VC_NUM              4
#define SYNC_CODE_NUM           4

#define MIPI_RX_MAX_DEV_NUM     5
#define CMOS_MAX_DEV_NUM        2

#define SNS_MAX_CLK_SOURCE_NUM  3
#define SNS_MAX_RST_SOURCE_NUM  3

typedef enum
{
    LANE_DIVIDE_MODE_0 = 0,
    LANE_DIVIDE_MODE_1 = 1,
    LANE_DIVIDE_MODE_2 = 2,
    LANE_DIVIDE_MODE_3 = 3,
    LANE_DIVIDE_MODE_4 = 4,
    LANE_DIVIDE_MODE_5 = 5,
    LANE_DIVIDE_MODE_6 = 6,
    LANE_DIVIDE_MODE_BUTT
} LANE_DIVIDE_MODE_E;

typedef enum
{
    INPUT_MODE_MIPI    = 0x0, /* MIPI */
    INPUT_MODE_SUBLVDS = 0x1, /* SUB_LVDS */
    INPUT_MODE_LVDS    = 0x2, /* LVDS */
    INPUT_MODE_HISPI   = 0x3, /* HISPI */
    INPUT_MODE_SLVS    = 0x4, /* SLVS */
    INPUT_MODE_CMOS    = 0x5, /* CMOS */
    INPUT_MODE_BT601   = 0x6, /* BT601 */
    INPUT_MODE_BT656   = 0x7, /* BT656 */
    INPUT_MODE_BT1120  = 0x8, /* BT1120 */
    INPUT_MODE_BYPASS  = 0x9, /* MIPI Bypass */

    INPUT_MODE_BUTT
} INPUT_MODE_E;

typedef enum
{
    MIPI_DATA_RATE_X1 = 0, /* output 1 pixel per clock */
    MIPI_DATA_RATE_X2 = 1, /* output 2 pixel per clock */
    MIPI_DATA_RATE_BUTT
} MIPI_DATA_RATE_E;

typedef struct
{
    int x;
    int y;

    unsigned int width;
    unsigned int height;
} img_rect_t;

typedef struct
{
    unsigned int width;
    unsigned int height;
} img_size_t;

typedef enum
{
    DATA_TYPE_RAW_8BIT = 0,
    DATA_TYPE_RAW_10BIT,
    DATA_TYPE_RAW_12BIT,
    DATA_TYPE_RAW_14BIT,
    DATA_TYPE_RAW_16BIT,
    DATA_TYPE_YUV420_8BIT_NORMAL,
    DATA_TYPE_YUV420_8BIT_LEGACY,
    DATA_TYPE_YUV422_8BIT,
    DATA_TYPE_BUTT
} DATA_TYPE_E;

/* MIPI D_PHY WDR MODE defines */
typedef enum
{
    SC_MIPI_WDR_MODE_NONE = 0x0,
    SC_MIPI_WDR_MODE_VC   = 0x1, /* Virtual Channel */
    SC_MIPI_WDR_MODE_DT   = 0x2, /* Data Type */
    SC_MIPI_WDR_MODE_DOL  = 0x3, /* DOL Mode */
    SC_MIPI_WDR_MODE_BUTT
} MIPI_WDR_MODE_E;

typedef struct
{
    unsigned int timing_mode; /* 0:auto, 1:manual */
    unsigned int hsa;
    unsigned int hbp;
    unsigned int hsd;
} mipi_ipi_timing_t;

typedef struct
{
    DATA_TYPE_E     input_data_type; /* Data type: 8/10/12/14/16 bit */
    MIPI_WDR_MODE_E wdr_mode;        /* MIPI WDR mode */

    short lane_id[MIPI_LANE_NUM];    /* lane_id: -1 - disable */

    unsigned int mipi_pix_clk;
    unsigned int settle_count;

    union
    {
        short data_type[WDR_VC_NUM]; /* used by the SC_MIPI_WDR_MODE_DT */
    };

    mipi_ipi_timing_t ipi_timing;
} mipi_dev_attr_t;

typedef enum
{
    SC_WDR_MODE_NONE   = 0x0,
    SC_WDR_MODE_2F     = 0x1,
    SC_WDR_MODE_3F     = 0x2,
    SC_WDR_MODE_4F     = 0x3,
    SC_WDR_MODE_DOL_2F = 0x4,
    SC_WDR_MODE_DOL_3F = 0x5,
    SC_WDR_MODE_DOL_4F = 0x6,
    SC_WDR_MODE_BUTT
} SC_WDR_MODE_E;

typedef enum
{
    MCLK_SRC_SENSOR0 = 64,
    MCLK_SRC_SENSOR1 = 65,
    MCLK_SRC_SENSOR2 = 66,
    MCLK_SRC_SENSOR3 = 67,
    MCLK_SRC_TYPE_BUTT
} SC_MCLK_SRC_TYPE_E;

typedef enum
{
    SNS_MCLK_24MHZ     = 24000,
    SNS_MCLK_27MHZ     = 27000,
    SNS_MCLK_37_125MHZ = 37125,
    SNS_MCLK_74_25MHZ  = 74250,
    SNS_MCLK_MODE_BUTT
} SC_SNS_MCLK_MODE_E;

typedef struct
{
    int reset_gpio[2];  /* reset_gpio[0]:group,  reset_gpio[1]:pin */
    int power_gpio[2];  /* power_gpio[0]:group,  power_gpio[1]:pin */
    int common_gpio[2]; /* common_gpio[0]:group, common_gpio[1]:pin */
    int mclk_src;       /* MCLK source(SC_MCLK_SRC_TYPE_E) */
    int mclk_k;         /* Sensor input clock(SC_SNS_MCLK_MODE_E) */
} dev_power_attr_t;

typedef struct
{
    int timing_mode; /* 0:auto, 1:manual */
    int de_delay;
    int htotal;
    int hstart;
} vif_line_buffer_timing_t;

typedef struct
{
    combo_dev_t  devno;      /* Device number */
    INPUT_MODE_E input_mode; /* Input mode: MIPI/LVDS/SUBLVDS/SC_SPI/DC */
    int          data_rate;  /* Sensor mipi out data lane rate M eg:450 */
    img_rect_t   img_rect;   /* MIPI Rx device crop area (corresponding to the oringnal sensor input image size) */

    union
    {
        mipi_dev_attr_t mipi_attr;
    };

    dev_power_attr_t      dev_power_attr;
    vif_line_buffer_timing_t line_timing;
} SC_COMBO_DEV_ATTR_S;

#endif /* __SC_MIPI_H__ */
