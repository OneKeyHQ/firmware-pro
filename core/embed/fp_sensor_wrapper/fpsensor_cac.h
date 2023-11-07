#ifndef FPSENSOR_CAC_H
#define FPSENSOR_CAC_H
#include <stdint.h>
typedef struct fpsensor_adc fpsensor_adc_t;

typedef enum fpsensor_finger_type
{
    NORMAL_FINGER = 0,
    WET_FINGER,
    DRY_FINGER,
    VERY_WET_FINGER,
    VERY_DRY_FINGER,
} fpsensor_finger_type_t;

/*
 * Function:     fpsensor_cac_init
 * Description:  cac初始化
 * Input:        adc - 当前采图所使用的adc值
 * Output:       无
 * Return:       无
 * Others:       未使用
 */
void fpsensor_cac_init(fpsensor_adc_t* adc);

/*
* Function:     fpsensor_cac
* Description:  cac算法实现，根据当前指纹图像数据和adc值计算下一次采图adc值
* Input:        image  - 指纹图像数据
                height - 指纹图像高
                width  - 指纹图像宽
                adc_   - 当前采图所使用的adc值
* Output:       无
* Return:       无
* Others:       未使用
*/
uint8_t fpsensor_cac(uint8_t* image, int32_t height, int32_t width, fpsensor_adc_t* adc_);

/*
* Function:     fpsensor_cac2
* Description:  cac算法实现，根据当前指纹图像数据和adc值计算下一次采图adc值
* Input:        image  - 指纹图像数据
                height - 指纹图像高
                width  - 指纹图像宽
                adc_   - 当前采图所使用的adc值
* Output:       adc_   - cac调整后的adc值
* Return:       无
* Others:       采图驱动中使用该函数进行cac计算
*/
uint8_t fpsensor_cac2(uint8_t* image, int32_t height, int32_t width, fpsensor_adc_t* adc_);

/*
* Function:     fpsensor_run_cac
* Description:  多次采集图像进行cac计算直到采集图像符合要求，或达到最大调整次数
* Input:        buffer - 数据缓冲区用于配置传感器寄存器或存储图像数据
                length - 缓冲区大小
                adc    - 当前采图所使用的adc值
* Output:       adc    - cac调整后最终输出的adc值
* Return:       无
* Others:       无
*/
uint8_t fpsensor_run_cac(uint8_t* buffer, uint32_t length, fpsensor_adc_t* adc);

#ifdef CAC_TEST_MULTI_IMAGE
extern uint8_t flag;
#endif

#ifdef CAC_TEST_UP_PARAMETER
typedef struct fpsensor_adc_paralog
{
    uint32_t count;
    fpsensor_adc_t** paras;
} fpsensor_adc_paralog_t;

extern fpsensor_adc_paralog_t adc_paralog;
#endif
#endif
