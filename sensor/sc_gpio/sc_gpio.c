/**
 * @file     sc_gpio.c
 * @brief    GPIO控制接口实现
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include "sc_gpio.h"


#define SYSFS_GPIO_DIR "/sys/class/gpio"

#define MAX_GPIO_REC_BUF  64
#define MAX_GPIO_PIN_NUM  32


/**
* @brief  initialize gpio component
* @param  gpio gpio component
* @return 0	ok.
* @note echo gpio_num > sys/class/gpio/export.
*/
int sc_gpio_export(unsigned int gpio)
{
    int fd, len;
    char buf[MAX_GPIO_REC_BUF];

    fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
    if(fd < 0)
    {
        perror("gpio/export");
        return fd;
    }

    len = snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, len);
    close(fd);

    return 0;
}

/**
* @brief  uninitialize gpio component
* @param  gpio gpio component
* @return 0	ok.
* @note echo gpio_num > sys/class/gpio/unexport.
*/
int sc_gpio_unexport(unsigned int gpio)
{
    int fd, len;
    char buf[MAX_GPIO_REC_BUF];

    fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
    if(fd < 0)
    {
        perror("gpio/export");
        return fd;
    }

    len = snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, len);
    close(fd);
    return 0;
}

/**
* @brief  set gpio direction
* @param  gpio gpio component
* @param  dir 1 output 0 input
* @return 0	ok.
* @note echo in/out > sys/class/gpio/gpioXXX/direction.
*/
int sc_gpio_set_dir(unsigned int gpio, ENUM_GPIO_DIR dir)
{
    int fd, len;
    char buf[MAX_GPIO_REC_BUF];

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);
    if(len < 0)
    {
        perror("gpio/direction");
        return len;
    }

    fd = open(buf, O_WRONLY);
    if(fd < 0)
    {
        perror("gpio/direction");
        return fd;
    }

    if(dir)
    {
        write(fd, "out", 4);
    }
    else
    {
        write(fd, "in", 3);
    }

    close(fd);
    return 0;
}

/**
* @brief  set gpio high or low
* @param  gpio gpio component
* @param  value  0 set gpio low
				 1 set gpio high
* @return 0	ok.
* @note echo 1/0 > sys/class/gpio/gpioXXX/value.
*/
int sc_gpio_set_value(unsigned int gpio, unsigned int value)
{
    int fd, len;
    char buf[MAX_GPIO_REC_BUF];

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
    if(len < 0)
    {
        perror("gpio/set-value");
        return len;
    }

    fd = open(buf, O_WRONLY);
    if(fd < 0)
    {
        perror("gpio/set-value");
        return fd;
    }

    if(value)
    {
        write(fd, "1", 2);
    }
    else
    {
        write(fd, "0", 2);
    }

    close(fd);
    return 0;
}

/**
* @brief  get gpio value status
* @param  gpio gpio component
* @param  value  return gpio status
* @return 0	ok.
* @note cat sys/class/gpio/gpioXXX/value.
*/
int sc_gpio_get_value(unsigned int gpio, unsigned int *value)
{
    int fd, len;
    char buf[MAX_GPIO_REC_BUF];
    char ch;

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
    if(len < 0)
    {
        perror("gpio/get-value");
        return len;
    }

    fd = open(buf, O_RDONLY);
    if(fd < 0)
    {
        perror("gpio/get-value");
        return fd;
    }

    read(fd, &ch, 1);
    if(ch != '0')
    {
        *value = 1;
    }
    else
    {
        *value = 0;
    }

    close(fd);
    return 0;
}

/**
* @brief  convert gpio gourp, port, pin to gpio component
* @param  group gpio group
* @param  pin gpio pin
* @return gpio component.
* @note group0-3, pin0-31
*/
int sc_gpio_name_to_num(ENUM_GPIO_GROUP group, unsigned int pin)
{
    if(group >= GROUP_MAX)
    {
        printf("invalid group num!\n");
                return -1;
    }

    if(pin >= MAX_GPIO_PIN_NUM)
    {
        printf("invalid pin num!\n");
        return -1;
    }

    return (group * 32 + pin);
}
