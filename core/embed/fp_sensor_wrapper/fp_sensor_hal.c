#include "fp_sensor_hal.h"

#include "systick.h"
#include "irq.h"
#include "spi.h"
#include "se_thd89.h"
#include "emmc.h"
#include "emmc_fs.h"
#include "debug_utils.h"
#include "util_macros.h"

#define fp_template_path "0:/fp_template.bin"
// const uint32_t fp_template_size = 1 * 1024 * 1024; // 1MB
const uint32_t fp_template_size = KB(20) + KB(4) + (KB(128) * 3); // assuming FP_MAX_USER_COUNT = 3
bool fp_template_inited = false;
static uint32_t emmcfs_processed = 0;

static fp_sensor_ll_interface_t fp_sensor_LLIF = {0};

#define DBG_LOG_BUS  0
#define DBG_LOG_DATA 0

// #undef DBG_PRINTF
// #undef DBG_BUF_DUMP
// #define DBG_PRINTF(fmt, ...)
// #define DBG_BUF_DUMP(data_p, len)

// gpio

void fp_sensor_gpio_ctrl(bool enable)
{
    if ( enable )
    {
        GPIO_InitTypeDef GPIO_InitStruct = {0};

        __HAL_RCC_GPIOB_CLK_ENABLE();

        // FP_IRQ      PB15
        GPIO_InitStruct.Pin = FP_IRQ_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = 0; // ignored
        HAL_GPIO_Init(FP_IRQ_PORT, &GPIO_InitStruct);

        // FP_RST      PB14
        GPIO_InitStruct.Pin = FP_RESET_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = 0; // ignored
        HAL_GPIO_Init(FP_RESET_PORT, &GPIO_InitStruct);

#if FP_USE_SOFTWARE_CS

        // RSTn        FP_RST      PB14
        GPIO_InitStruct.Pin = FP_CS_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = 0; // ignored
        HAL_GPIO_Init(FP_CS_PORT, &GPIO_InitStruct);

#endif
    }
    else
    {
        HAL_GPIO_DeInit(FP_IRQ_PORT, FP_IRQ_PIN);
        HAL_GPIO_DeInit(FP_RESET_PORT, FP_RESET_PIN);
#if FP_USE_SOFTWARE_CS
        HAL_GPIO_DeInit(FP_CS_PORT, FP_CS_PIN);
#endif
    }
}

void fp_sensor_gpio_chip_select(bool activate)
{
#if FP_USE_SOFTWARE_CS
    HAL_GPIO_WritePin(FP_CS_PORT, FP_CS_PIN, activate ? GPIO_PIN_RESET : GPIO_PIN_SET);

    // UNUSED(activate);
    // HAL_GPIO_WritePin(FP_CS_PORT, FP_CS_PIN, GPIO_PIN_RESET);
#endif
}

void fp_sensor_gpio_reset(bool activate)
{
    HAL_GPIO_WritePin(FP_RESET_PORT, FP_RESET_PIN, activate ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void fp_sensor_gpio_irq_ctrl(bool activate)
{
    EXTI_HandleTypeDef hexti = {0};
    EXTI_ConfigTypeDef pExtiConfig = {0};

    if ( activate )
    {
        NVIC_SetPriority(EXTI15_10_IRQn, IRQ_PRI_GPIO);
        __HAL_GPIO_EXTI_CLEAR_IT(FP_IRQ_PIN);
        pExtiConfig.Line = FP_EXTI_PORT;
        pExtiConfig.Mode = EXTI_MODE_INTERRUPT;
        pExtiConfig.Trigger = EXTI_TRIGGER_RISING;
        pExtiConfig.GPIOSel = FP_EXTI_PORT;
        HAL_EXTI_SetConfigLine(&hexti, &pExtiConfig);
    }
    else
    {
        __HAL_GPIO_EXTI_CLEAR_IT(FP_IRQ_PIN);
        pExtiConfig.Line = FP_EXTI_PORT;
        pExtiConfig.Mode = EXTI_MODE_NONE;
        pExtiConfig.GPIOSel = FP_EXTI_PORT;
        HAL_EXTI_SetConfigLine(&hexti, &pExtiConfig);
    }
}

bool fp_sensor_gpio_irq_status()
{
    return (HAL_GPIO_ReadPin(FP_IRQ_PORT, FP_IRQ_PIN) == GPIO_PIN_RESET);
}

// seq

void fp_sensor_seq_chip_reset()
{
    fp_sensor_gpio_reset(true);
    fp_sensor_LLIF.delay_ms(fp_sensor_LLIF.seq_delayMs_chipRst);
    fp_sensor_gpio_reset(false);
    fp_sensor_LLIF.delay_ms(fp_sensor_LLIF.seq_delayMs_chipRst);
}

void fp_sensor_seq_chip_select(bool activate)
{
    fp_sensor_gpio_chip_select(activate);
    fp_sensor_LLIF.delay_ms(fp_sensor_LLIF.seq_delayMs_chipSel);
}

// bus

static SPI_HandleTypeDef* spi_handle_fingerprint;

void fp_sensor_bus_ctrl(bool enable)
{
    if ( enable )
    {
        spi_init_by_device(SPI_FINGERPRINT);
        spi_handle_fingerprint = &spi_handles[spi_find_channel_by_device(SPI_FINGERPRINT)];
    }
    else
    {
        spi_handle_fingerprint = NULL;
        spi_deinit_by_device(SPI_FINGERPRINT);
    }
}

void fp_sensor_bus_write(uint8_t* buf, uint16_t size)
{
#if DBG_LOG_BUS
    DBG_PRINTF("%s: buf_w:\n", __func__);
    DBG_BUF_DUMP(buf, size);
#endif

    if ( spi_handle_fingerprint == NULL )
        return;

#if FP_USE_SOFTWARE_CS
    fp_sensor_seq_chip_select(true);
#endif

    HAL_SPI_Transmit(spi_handle_fingerprint, buf, size, 1000);

#if FP_USE_SOFTWARE_CS
    fp_sensor_seq_chip_select(false);
#endif
}

void fp_sensor_bus_read(uint8_t* buf, uint16_t size)
{

    if ( spi_handle_fingerprint == NULL )
        return;

#if FP_USE_SOFTWARE_CS
    fp_sensor_seq_chip_select(true);
#endif

    HAL_SPI_Receive(spi_handle_fingerprint, buf, size, 1000);

#if FP_USE_SOFTWARE_CS
    fp_sensor_seq_chip_select(false);
#endif

#if DBG_LOG_BUS
    DBG_PRINTF("%s: buf_r:\n", __func__);
    DBG_BUF_DUMP(buf, size);
#endif
}

void fp_sensor_bus_rw(uint8_t* buf_w, uint16_t len_w, uint8_t* buf_r, uint16_t len_r)
{

#if DBG_LOG_BUS
    DBG_PRINTF("%s: buf_w:\n", __func__);
    DBG_BUF_DUMP(buf_w, len_w);
#endif

    if ( spi_handle_fingerprint == NULL )
        return;

#if FP_USE_SOFTWARE_CS
    fp_sensor_seq_chip_select(true);
#endif

    uint8_t buf[len_w + len_r];
    memset(buf, 0x00, (len_w + len_r));
    memcpy(buf, buf_w, len_w);
    HAL_SPI_TransmitReceive(spi_handle_fingerprint, buf, buf, (len_w + len_r), 1000);
    memcpy(buf_r, buf + len_w, len_r);

#if FP_USE_SOFTWARE_CS
    fp_sensor_seq_chip_select(false);
#endif

#if DBG_LOG_BUS
    DBG_PRINTF("%s: buf_r=:\n", __func__);
    DBG_BUF_DUMP(buf_r, len_r);
#endif
}

// data

bool fp_sensor_data_init(bool as_new)
{
#if DBG_LOG_DATA
    DBG_PRINTF("%s: called, as_new=%s\n", __func__, (as_new ? "True" : "False"));
#endif

    EMMC_PATH_INFO pathinfo;
    if ( !emmc_fs_path_info(fp_template_path, &pathinfo) )
        return false;

    if ( as_new || !pathinfo.path_exist )
    {
        if ( pathinfo.path_exist )
        {
            emmc_fs_file_delete(fp_template_path);
        }
        if ( !emmc_fs_file_resize(
                 fp_template_path, fp_template_size
             ) ) // TODO: fixme why emmc_fs_file_resize not working
        {
#if DBG_LOG_DATA
            DBG_PRINTF("%s: new file create failed!\n", __func__);
#endif
            return false;
        }
#if DBG_LOG_DATA
        DBG_PRINTF("%s: new file created\n", __func__);
#endif

        // fill with 0xff as new created file has all 0x00
        uint32_t block_size = 64 * 1024;
        uint8_t FFs[block_size];
        memset(FFs, 0xff, block_size);
        for ( uint32_t i = 0; i < fp_template_size; i += block_size )
        {
            emmc_fs_file_write(fp_template_path, i, FFs, block_size, NULL, false, true);
        }
#if DBG_LOG_DATA
        DBG_PRINTF("%s: new file filled with 0xff\n", __func__);
#endif
    }

    fp_template_inited = true;

    return true;
}

bool fp_sensor_data_read(uint32_t offset, uint8_t* buf, uint32_t size)
{
#if DBG_LOG_DATA
    DBG_PRINTF("%s: called, offset=0x%08x, buf_p=0x%08x, size=0x%08x\n", __func__, offset, (uint32_t)buf, size);
#endif

    if ( !fp_template_inited )
    {
#if DBG_LOG_DATA
        DBG_PRINTF("%s: not initialized\n", __func__);
#endif
        return false;
    }

    if ( offset + size > fp_template_size )
    {
#if DBG_LOG_DATA
        DBG_PRINTF("%s: oversized request\n", __func__);
#endif
        return false;
    }

    if ( !emmc_fs_file_read(fp_template_path, offset, buf, size, &emmcfs_processed) )
    {
        return false;
    }

#if DBG_LOG_DATA
    DBG_BUF_DUMP(buf, size);
#endif

    return true;
}

bool fp_sensor_data_write(uint32_t offset, uint8_t* buf, uint32_t size)
{
#if DBG_LOG_DATA
    DBG_PRINTF("%s: called, offset=0x%08x, buf_p=0x%08x, size=0x%08x\n", __func__, offset, (uint32_t)buf, size);
#endif

    if ( !fp_template_inited )
    {
#if DBG_LOG_DATA
        DBG_PRINTF("%s: not initialized", __func__);
#endif
        return false;
    }

    if ( offset + size > fp_template_size )
    {
#if DBG_LOG_DATA
        DBG_PRINTF("%s: oversized request\n", __func__);
#endif
        return false;
    }

#if DBG_LOG_DATA
    DBG_BUF_DUMP(buf, size);
#endif

    if ( !emmc_fs_file_write(fp_template_path, offset, buf, size, &emmcfs_processed, false, true) )
    {
        return false;
    }

    return true;
}

bool fp_sensor_data_syncToSE()
{
#warning "Implement me!!"
#if DBG_LOG_DATA
    DBG_PRINTF("%s: called\n", __func__);
#endif

    return false;
}

// bool fp_sensor_data_slot_set(uint8_t id, uint8_t* buf, uint32_t size)
// {

// }

// bool fp_sensor_data_slot_get(uint8_t id, uint8_t* buf, uint32_t size)
// {

// }

// bool fp_sensor_data_slot_clear(int8_t id)
// {

// }

// bool fp_sensor_data_slot_syncToSE(int8_t id)
// {

// }

fp_sensor_ll_interface_t* fp_senrsor_get_LLIF(uint32_t delayMs_chipRst, uint32_t delayMs_chipSel)
{
    fp_sensor_LLIF.seq_delayMs_chipRst = delayMs_chipRst;
    fp_sensor_LLIF.seq_delayMs_chipSel = delayMs_chipSel;

    fp_sensor_LLIF.delay_us = dwt_delay_us;
    fp_sensor_LLIF.delay_ms = dwt_delay_ms;

    fp_sensor_LLIF.gpio_ctrl = fp_sensor_gpio_ctrl;
    fp_sensor_LLIF.gpio_chip_select = fp_sensor_gpio_chip_select;
    fp_sensor_LLIF.gpio_reset = fp_sensor_gpio_reset;
    fp_sensor_LLIF.gpio_irq_ctrl = fp_sensor_gpio_irq_ctrl;
    fp_sensor_LLIF.gpio_irq_status = fp_sensor_gpio_irq_status;

    fp_sensor_LLIF.seq_chip_reset = fp_sensor_seq_chip_reset;
    fp_sensor_LLIF.seq_chip_select = fp_sensor_seq_chip_select;

    fp_sensor_LLIF.bus_ctrl = fp_sensor_bus_ctrl;
    fp_sensor_LLIF.bus_read = fp_sensor_bus_read;
    fp_sensor_LLIF.bus_write = fp_sensor_bus_write;
    fp_sensor_LLIF.bus_rw = fp_sensor_bus_rw;

    EMMC_CardInfoTypeDef card_info = {0};
    emmc_get_card_info(&card_info);
    fp_sensor_LLIF.data_sector_size = card_info.LogBlockSize;
    fp_sensor_LLIF.data_init = fp_sensor_data_init;
    fp_sensor_LLIF.data_read = fp_sensor_data_read;
    fp_sensor_LLIF.data_write = fp_sensor_data_write;
    fp_sensor_LLIF.data_syncToSE = fp_sensor_data_syncToSE;

    // fp_sensor_LLIF.data_slot_get = fp_sensor_data_slot_get;
    // fp_sensor_LLIF.data_slot_set = fp_sensor_data_slot_set;
    // fp_sensor_LLIF.data_slot_clear = fp_sensor_data_slot_clear;
    // fp_sensor_LLIF.data_slot_syncToSE = fp_sensor_data_slot_syncToSE;

    return &fp_sensor_LLIF;
}
