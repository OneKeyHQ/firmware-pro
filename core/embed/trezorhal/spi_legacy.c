#include STM32_HAL_H

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "irq.h"
#include "spi_legacy.h"
#include "timer.h"

SPI_HandleTypeDef spi;
static DMA_HandleTypeDef hdma_tx;
static DMA_HandleTypeDef hdma_rx;

// SRAM3 32K
static uint8_t dma_recv_buf[SPI_BUF_MAX_IN_LEN] __attribute__((section(".sram3")));
static uint8_t dma_send_buf[SPI_BUF_MAX_OUT_LEN] __attribute__((section(".sram3")));

static volatile int32_t spi_rx_event = 0;
static volatile int32_t spi_tx_event = 0;
static volatile int32_t spi_abort_event = 0;
static volatile bool spi_data_dir_in = true;
static volatile bool spi_data_received = false;
ChannelType host_channel = CHANNEL_NULL;

uint8_t spi_data_in[SPI_BUF_MAX_IN_LEN];
uint8_t spi_data_out[SPI_BUF_MAX_OUT_LEN];

#define DATA_HEADER_SIZE      3
#define DATA_FIDO_HEADER_SIZE 3

typedef enum
{
    SPI_STATE_IDLE,
    SPI_STATE_FIDO_HEADER,
    SPI_STATE_FIDO_DATA,
    SPI_STATE_ABORTED,
} SPI_State;

SPI_State spi_state = SPI_STATE_IDLE;

trans_fifo spi_fifo_in = {
    .p_buf = spi_data_in,
    .buf_size = SPI_BUF_MAX_IN_LEN,
    .over_pre = false,
    .read_pos = 0,
    .write_pos = 0,
    .lock_pos = 0};

secbool spi_can_write(void)
{
    if ( spi_tx_event == 0 )
        return sectrue;
    else
        return secfalse;
}

int32_t wait_spi_rx_event(int32_t timeout)
{
    int32_t tickstart = HAL_GetTick();

    while ( spi_rx_event == 1 )
    {
        if ( (HAL_GetTick() - tickstart) > timeout )
        {
            return -1;
        }
    }
    return 0;
}

int32_t wait_spi_tx_event(int32_t timeout)
{
    int32_t tickstart = HAL_GetTick();

    while ( spi_tx_event == 1 )
    {
        if ( (HAL_GetTick() - tickstart) > timeout )
        {
            spi_tx_event = 0;
            return -1;
        }
    }
    return 0;
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef* hspi)
{
    ST_BLE_STATUS_IO_BUSY();
    if ( spi_rx_event )
    {
        spi_rx_event = 0;
    }
    if ( !fifo_write_no_overflow(&spi_fifo_in, dma_recv_buf, hspi->RxXferSize) )
    {
        memset(dma_recv_buf, 0, hspi->RxXferSize);
    }

    HAL_SPI_Receive_DMA(&spi, dma_recv_buf, sizeof(dma_recv_buf));
    ST_BLE_STATUS_IO_IDLE();
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi)
{
    if ( spi_tx_event )
    {
        spi_tx_event = 0;
    }

    HAL_SPI_Receive_DMA(&spi, dma_recv_buf, sizeof(dma_recv_buf));
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef* hspi)
{
    ensure(secfalse, "spi ovr err");
}

void HAL_SPI_AbortCpltCallback(SPI_HandleTypeDef* hspi)
{
    spi_abort_event = 0;
}

void SPI2_IRQHandler(void)
{
    HAL_SPI_IRQHandler(&spi);
}

int32_t spi_slave_init()
{
    GPIO_InitTypeDef gpio;

    __HAL_RCC_DMA1_CLK_ENABLE();

    __HAL_RCC_SPI2_FORCE_RESET();
    __HAL_RCC_SPI2_RELEASE_RESET();
    __HAL_RCC_SPI2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOK_CLK_ENABLE();

    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Pin = GPIO_PIN_5 | GPIO_PIN_6;
    // set pin before config port direction to avoid unwanted reset to bluetooth chip
    HAL_GPIO_WritePin(GPIOK, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOK, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_GPIO_Init(GPIOK, &gpio);

    // SPI2: PA11(NSS),PA9(SCK)
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF5_SPI2;
    gpio.Pin = GPIO_PIN_12 | GPIO_PIN_11;
    HAL_GPIO_Init(GPIOA, &gpio);

    // SPI2: PC2(MISO), PC3(MOSI)
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF5_SPI2;
    gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    HAL_GPIO_Init(GPIOC, &gpio);

    spi.Instance = SPI2;
    spi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    spi.Init.Direction = SPI_DIRECTION_2LINES;
    spi.Init.CLKPhase = SPI_PHASE_1EDGE;
    spi.Init.CLKPolarity = SPI_POLARITY_LOW;
    spi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    spi.Init.CRCPolynomial = 7;
    spi.Init.DataSize = SPI_DATASIZE_8BIT;
    spi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    spi.Init.NSS = SPI_NSS_HARD_INPUT;
    spi.Init.TIMode = SPI_TIMODE_DISABLE;
    spi.Init.Mode = SPI_MODE_SLAVE;
    spi.Init.FifoThreshold = SPI_FIFO_THRESHOLD_16DATA;

    if ( HAL_OK != HAL_SPI_Init(&spi) )
    {
        return -1;
    }

    /*##-3- Configure the DMA ##################################################*/
    /* Configure the DMA handler for Transmission process */
    hdma_tx.Instance = SPIx_TX_DMA_STREAM;
    hdma_tx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_tx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_1QUARTERFULL;
    hdma_tx.Init.MemBurst = DMA_MBURST_SINGLE;
    hdma_tx.Init.PeriphBurst = DMA_PBURST_SINGLE;
    hdma_tx.Init.Request = SPIx_TX_DMA_REQUEST;
    hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_tx.Init.Mode = DMA_NORMAL;
    hdma_tx.Init.Priority = DMA_PRIORITY_LOW;

    HAL_DMA_Init(&hdma_tx);

    /* Associate the initialized DMA handle to the the SPI handle */
    __HAL_LINKDMA(&spi, hdmatx, hdma_tx);

    /* Configure the DMA handler for Transmission process */
    hdma_rx.Instance = SPIx_RX_DMA_STREAM;

    hdma_rx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_rx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_1QUARTERFULL;
    hdma_rx.Init.MemBurst = DMA_MBURST_SINGLE;
    hdma_rx.Init.PeriphBurst = DMA_PBURST_SINGLE;
    hdma_rx.Init.Request = SPIx_RX_DMA_REQUEST;
    hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_rx.Init.Mode = DMA_NORMAL;
    hdma_rx.Init.Priority = DMA_PRIORITY_HIGH;

    HAL_DMA_Init(&hdma_rx);

    /* Associate the initialized DMA handle to the the SPI handle */
    __HAL_LINKDMA(&spi, hdmarx, hdma_rx);

    /*##-4- Configure the NVIC for DMA #########################################*/
    /* NVIC configuration for DMA transfer complete interrupt (SPI1_TX) */
    NVIC_SetPriority(SPIx_DMA_TX_IRQn, IRQ_PRI_DMA);
    HAL_NVIC_EnableIRQ(SPIx_DMA_TX_IRQn);

    /* NVIC configuration for DMA transfer complete interrupt (SPI1_RX) */
    NVIC_SetPriority(SPIx_DMA_RX_IRQn, IRQ_PRI_DMA);
    HAL_NVIC_EnableIRQ(SPIx_DMA_RX_IRQn);

    NVIC_SetPriority(SPI2_IRQn, IRQ_PRI_SPI);
    HAL_NVIC_EnableIRQ(SPI2_IRQn);

    EXTI_HandleTypeDef hexti = {0};

    EXTI_ConfigTypeDef pExtiConfig;
    pExtiConfig.Line = EXTI_LINE_11;
    pExtiConfig.Mode = EXTI_MODE_INTERRUPT;
    pExtiConfig.Trigger = EXTI_TRIGGER_RISING;
    pExtiConfig.GPIOSel = EXTI_GPIOA;

    HAL_EXTI_SetConfigLine(&hexti, &pExtiConfig);

    NVIC_SetPriority(EXTI15_10_IRQn, IRQ_PRI_GPIO);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    memset(dma_recv_buf, 0, sizeof(dma_recv_buf));
    spi_rx_event = 1;
    ST_BLE_STATUS_IO_IDLE();
    /* start SPI receive */
    if ( HAL_SPI_Receive_DMA(&spi, dma_recv_buf, sizeof(dma_recv_buf)) != HAL_OK )
    {
        return -1;
    }

    return 0;
}

void spi_disable_cs_irq(void)
{
    EXTI_HandleTypeDef hexti = {0};
    EXTI_ConfigTypeDef pExtiConfig;
    pExtiConfig.Line = EXTI_LINE_11;
    pExtiConfig.Mode = EXTI_MODE_NONE;
    pExtiConfig.GPIOSel = EXTI_GPIOA;
    HAL_EXTI_SetConfigLine(&hexti, &pExtiConfig);
}

int32_t spi_slave_send(uint8_t* buf, uint32_t size, int32_t timeout)
{
    uint32_t msg_size;
    int32_t ret = 0;
    msg_size = size; //< SPI_PKG_SIZE ? SPI_PKG_SIZE : size
    memcpy(dma_send_buf, buf, msg_size);

    spi_abort_event = 1;
    spi_data_dir_in = false;
    if ( HAL_SPI_Abort_IT(&spi) != HAL_OK )
    {
        return 0;
    }
    while ( spi_abort_event )
        ;
    if ( HAL_SPI_Transmit_DMA(&spi, dma_send_buf, msg_size) != HAL_OK )
    {
        goto END;
    }

    ST_BLE_STATUS_IO_BUSY();

    spi_tx_event = 1;

    if ( wait_spi_tx_event(timeout) != 0 )
    {
        goto END;
    }
    ret = msg_size;
END:
    if ( ret == 0 )
    {
        HAL_SPI_Abort(&spi);
        HAL_SPI_Receive_DMA(&spi, dma_recv_buf, sizeof(dma_recv_buf));
    }
    ST_BLE_STATUS_IO_IDLE();
    spi_data_dir_in = true;

    return ret;
}

uint32_t _spi_slave_poll_ex(uint8_t* buf, bool fido)
{
    volatile uint32_t total_len, len;
    uint8_t header[3];

    if ( buf == NULL )
        return 0;

    total_len = fifo_lockdata_len(&spi_fifo_in);
    if ( total_len == 0 )
    {
        return 0;
    }

    fifo_read_peek(&spi_fifo_in, header, sizeof(header));

    if ( memcmp(header, "fid", 3) != 0 && header[0] != '?' )
    {
        fifo_flush(&spi_fifo_in);
        return 0;
    }

    if ( fido )
    {
        if ( memcmp(header, "fid", 3) == 0 )
        {
            fifo_read_lock(&spi_fifo_in, header, sizeof(header));
            // read len
            fifo_read_lock(&spi_fifo_in, header, 2);
            len = header[0] << 8 | header[1];
            fifo_read_lock(&spi_fifo_in, buf, total_len - 5);
            if ( total_len - 5 < len )
            {
                return 0;
            }
            return len;
        }
    }
    else
    {
        if ( header[0] == '?' )
        {
            len = total_len > SPI_PKG_SIZE ? SPI_PKG_SIZE : total_len;
            return fifo_read_lock(&spi_fifo_in, buf, len);
        }
#if BOOT_ONLY
        else
        {
            fifo_flush(&spi_fifo_in);
        }
#endif
    }

    return 0;
}

uint32_t spi_slave_poll(uint8_t* buf)
{
    return _spi_slave_poll_ex(buf, false);
}

uint32_t spi_slave_poll_fido(uint8_t* buf)
{
    return _spi_slave_poll_ex(buf, true);
}

uint32_t spi_read_retry(uint8_t* buf)
{
    spi_rx_event = 1;

    for ( int retry = 0;; retry++ )
    {
        int r = wait_spi_rx_event(500);
        if ( r == -1 )
        { // reading failed
            if ( r == -1 && retry < 10 )
            {
                // only timeout => let's try again
            }
            else
            {
                // error
                error_shutdown("Error reading", "from SPI.", "Try to", "reset.");
            }
        }

        if ( r == 0 )
        {
            return spi_slave_poll(buf);
        }
    }
}

uint32_t spi_read_blocking(uint8_t* buf, int timeout)
{
    spi_rx_event = 1;

    // check if there already some data
    int r = spi_slave_poll(buf);

    // yes, retrun
    if ( r != 0 )
    {
        return r;
    }

    // no, try read with timeout
    switch ( wait_spi_rx_event(timeout) )
    {
    case 0:
        return spi_slave_poll(buf);
        break;
    case -1:
    default:
        return 0;
        break;
    }
}

void SPIx_DMA_RX_IRQHandler(void)
{
    HAL_DMA_IRQHandler(spi.hdmarx);
}

void SPIx_DMA_TX_IRQHandler(void)
{
    HAL_DMA_IRQHandler(spi.hdmatx);
}

void spi_cs_irq_handler(void)
{
    if ( HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11) == GPIO_PIN_RESET )
    {
        return;
    }

    if ( spi_data_dir_in )
    {
        HAL_SPI_Abort(&spi);
        uint16_t recv_len = sizeof(dma_recv_buf) - __HAL_DMA_GET_COUNTER(spi.hdmarx);
        if ( recv_len > 0 )
        {
            ST_BLE_STATUS_IO_BUSY();
            fifo_write_no_overflow(&spi_fifo_in, dma_recv_buf, recv_len);
            HAL_SPI_Receive_DMA(&spi, dma_recv_buf, sizeof(dma_recv_buf));
            ST_BLE_STATUS_IO_IDLE();
            spi_rx_event = 0;
            return;
        }

        HAL_SPI_Receive_DMA(&spi, dma_recv_buf, sizeof(dma_recv_buf));
    }
}
