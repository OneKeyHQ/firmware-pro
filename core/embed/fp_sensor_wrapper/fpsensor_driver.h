#ifndef __FPSENSOR_DRIVER_H
#define __FPSENSOR_DRIVER_H

#define FPSENSOR_TYPE_7152

#include "fpsensor_cac.h" //CAC相关内容不要改，千万不要改
#include "fpsensor_platform.h"
#include "fpsensor_regs.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NULL
  #define NULL ((void*)0)
#endif
#ifndef TRUE
  #define TRUE 1
#endif
#ifndef FALSE
  #define FALSE 0
#endif

#if defined(FPSENSOR_TYPE_7152) || defined(FPSENSOR_TYPE_7153)
  #define FPSENSOR_IMAGE_HEIGHT 112u
  #define FPSENSOR_IMAGE_WIDTH  88u
#endif

// #define FPSENSOR_DEBUG
#if defined(FPSENSOR_DEBUG)
  #define LOG printf
#else
  #define LOG(a, ...)
#endif

typedef enum
{
    FPSENSOR_OK = 0x00,
    FPSENSOR_SPI_ERROR = 0x01,
    FPSENSOR_TIMER_ERROR = 0x02,
    FPSENSOR_GPIO_ERROR = 0x04,
    FPSENSOR_BUFFER_ERROR = 0x08,
    FPSENSOR_TIMEOUT_ERROR = 0x10,
    FPSENSOR_NOFNGR_ERROR = 0x20
} fpsensor_error_t;

#define SLEEP_MODE_CLK 16000

typedef struct fpsensor_iamge
{
    int32_t height;
    int32_t width;
    uint8_t* data;
    int32_t tagLength;
    uint8_t* tag; // Chipone fingerprint algorithm will verify this item.
                  // Algorithm works when it's matched.
} fpsensor_image_t;

typedef struct fpsensor_adc
{
    uint8_t shift;
    uint8_t gain;
    uint8_t pixel;
    uint8_t et1;
} fpsensor_adc_t;

typedef struct fpsensor_rd_img_session
{
    uint32_t length;
    uint8_t* buffer;
    uint8_t* temp;
    uint32_t dataLen; // valid data length
    uint32_t sum;
    uint8_t count;
    uint8_t stop;
} fpsensor_rd_img_session_t;

extern fpsensor_adc_t adc_fixed;

int8_t* fpsensor_driver_version(void);
/*
 * Function:     fpsensor_init
 * Description:  传感器寄存器初始化
 * Input:        无
 * Output:       无
 * Return:       SPI数据传输状态
 * Others:       无
 */
uint8_t fpsensor_init(void);

/*
 * Function:     fpsensor_set_test
 * Description:  传感器开启内部测试模式
 * Input:        无
 * Output:       无
 * Return:       SPI数据传输状态
 * Others:       测试模式下读取图像数据为棋盘格图
 */
uint8_t fpsensor_set_test(uint8_t forward);

/*
 * Function:     fpsensor_get_HWID
 * Description:  读取传感器ID
 * Input:        无
 * Output:       hwid - 读取到的传感器ID
 * Return:       SPI数据传输状态
 * Others:       无
 */
uint8_t fpsensor_get_HWID(uint8_t hwid[2]);

/*
 * Function:     fpsensor_print_VID
 * Description:  读取CST传感器VID并打印
 * Input:        无
 * Output:       无
 * Return:       SPI数据传输状态
 * Others:       用于驱动调试阶段。
 */
uint8_t fpsensor_get_VID(uint8_t vid[3]);

/*
* Function:     fpsensor_adc_init
* Description:  配置adc全局变量adc_fixed
* Input:        shift - adc寄存器shift值
                gain  - adc寄存器gian值
                pixel - adc寄存器pixel值
                et1   - adc寄存器et1值(已过时)
* Output:       无
* Return:       SPI数据传输状态
* Others:       无
*/
uint8_t fpsensor_adc_init(uint8_t shift, uint8_t gain, uint8_t pixel, uint8_t et1);

/*
* Function:     fpsensor_read_image
* Description:  读指纹图像数据
* Input:        length - 缓冲区大小，其值需要大于等于图像宽*高 + 2
                timeout_seconds - 等待手指超时时间，单位为秒
* Output:       buffer - 指纹图像数据
* Return:       SPI数据传输状态
* Others:       无
*/
uint8_t fpsensor_read_image(
    uint8_t* buffer, uint32_t length,
    uint32_t timeout_seconds
); // please ensure the buffer length is not less
   // than IMAGE_HEIGHT*IMAGE_WIDTH+2

/*
 * Function:     fpsensor_finger_status_statistics
 * Description:  读取指纹按压或抬起状态，覆盖关键点个数
 * Input:        u8fpStatus - 01-按下 02-抬起
 * Output:       无
 * Return:       指纹按压覆盖关键点个数
 * Others:       无
 */
uint8_t fpsensor_finger_status_statistics(void);

/*
 * Function:     fpsensor_finger_down
 * Description:  判断手指是否按下，指纹按压覆盖关键点个数大于9个可以采图
 * Input:        无
 * Output:       无
 * Return:       1 手指按下可采图，0 手指未按压或按压面积小
 * Others:       无
 */
uint8_t fpsensor_finger_down(void);

/*
 * Function:     fpsensor_finger_off
 * Description:  判断手指是否离开
 * Input:        无
 * Output:       无
 * Return:       1 手指已离开传感器表面，0 手指未离开
 * Others:       无
 */
uint8_t fpsensor_finger_off(void);

/*
 * Function:     fpsensor_active_sleep_mode
 * Description:  传感器进入休眠模式
 * Input:        fngerDectPeriod - 休眠时定时唤醒扫描指纹的周期，单位毫秒
 * Output:       无
 * Return:       SPI数据传输状态
 * Others:       无
 */
uint8_t fpsensor_active_sleep_mode(int32_t fngerDectPeriod);

/*
 * Function:     fpsensor_active_sleep_for_test
 * Description:  传感器进入休眠模式 测试使用
 * Input:        fngerDectPeriod - 休眠时定时唤醒扫描指纹的周期，单位毫秒
 * Output:       无
 * Return:       SPI数据传输状态
 * Others:       无
 */
uint8_t fpsensor_active_sleep_for_test(int32_t fngerDectPeriod);

/*
 * Function:     fpsensor_active_deep_sleep_mode
 * Description:  传感器进入深睡觉模式
 * Input:        无
 * Output:       无
 * Return:       SPI数据传输状态
 * Others:       无
 */
uint8_t fpsensor_active_deep_sleep_mode(void);

/*
 * Function:     fpsensor_active_idle_mode
 * Description:  传感器进入空闲模式
 * Input:        无
 * Output:       无
 * Return:       SPI数据传输状态
 * Others:       无
 */
uint8_t fpsensor_active_idle_mode(void);

/*
 * Function:     fpsensor_active_quary_finger_mode
 * Description:  传感器进入检测手指模式
 * Input:        无
 * Output:       无
 * Return:       SPI数据传输状态
 * Others:       无
 */
uint8_t fpsensor_active_quary_finger_mode(void);

/*
 * Function:     fpsensor_read_irq_with_clear
 * Description:  读取传感器中断寄存器的值，并清除中断
 * Input:        length    - 缓冲区大小
 * Output:       buffer[1] - 中断寄存器值
 * Return:       SPI数据传输状态
 * Others:       无
 */
uint8_t fpsensor_read_irq_no_clear(uint8_t* buffer, uint32_t length);
uint8_t fpsensor_read_irq_with_clear(uint8_t* buffer, uint32_t length);

/*
* Function:     fpsesor_read_testpattern
* Description:  读取测试棋盘格图像
* Input:        pu8bufimage - 图像空间
                length - 空间大小
                                                                u8forward - 方向
* Output:       棋盘格数据
* Return:       SPI数据传输状态
* Others:       无
*/
uint8_t fpsesor_read_testpattern(uint8_t* pu8bufimage, uint32_t length, uint8_t u8forward);

/*
* Function:     fpsensor_set_config_param
* Description:  设置驱动库参数
* Input:
                u32FingerStatusThres - 检测手指阈值
                u16FingerArea - 手指按压检测面积
* Output:       无
* Return:       SPI数据传输状态
* Others:       无
*/
uint8_t fpsensor_set_config_param(uint32_t u32FingerStatusThres, uint16_t u16FingerArea);

#endif
