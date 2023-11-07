#include "nfc.h"

PN532 pn532;
SPI_HandleTypeDef hspi6;

static char MX_SPI6_Init(void){
    char ret=0;

    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI6;
    PeriphClkInitStruct.Spi6ClockSelection = RCC_SPI6CLKSOURCE_HSE;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    /* Peripheral clock enable */
    __HAL_RCC_SPI6_CLK_ENABLE();

    __HAL_RCC_GPIOG_CLK_ENABLE();
    /**SPI6 GPIO Configuration
    PG12     ------> SPI6_MISO
    PG14     ------> SPI6_MOSI
    PG13     ------> SPI6_SCK
    */
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_14|GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI6;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    hspi6.Instance = SPI6;
    hspi6.Init.Mode = SPI_MODE_MASTER;
    hspi6.Init.Direction = SPI_DIRECTION_2LINES;
    hspi6.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi6.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi6.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi6.Init.NSS = SPI_NSS_SOFT;
    hspi6.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hspi6.Init.FirstBit = SPI_FIRSTBIT_LSB;
    hspi6.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi6.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi6.Init.CRCPolynomial = 0x0;
    hspi6.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    hspi6.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    hspi6.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    hspi6.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    hspi6.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    hspi6.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    hspi6.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    hspi6.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    hspi6.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    hspi6.Init.IOSwap = SPI_IO_SWAP_DISABLE;
    if(HAL_SPI_Init(&hspi6)!=HAL_OK)ret=1;

    return ret;
}

int PN532_Reset(void) {
    // TODO: not implemented
    return PN532_STATUS_OK;
}

static char spi_rw(uint8_t* data, uint8_t count) {
    char ret=PN532_STATUS_OK;
    SET_NFC_SPI_CS_L;
    HAL_Delay(1);
    if(HAL_SPI_TransmitReceive(&hspi6, data, data, count, _SPI_TIMEOUT)!=HAL_OK)
        ret=PN532_STATUS_ERROR;
    HAL_Delay(1);
    SET_NFC_SPI_CS_H;
    return ret;
}

int PN532_SPI_ReadData(uint8_t* data, uint16_t count) {
    int ret=PN532_STATUS_OK;
    uint8_t frame[count + 1];
    frame[0] = _SPI_DATAREAD;
    HAL_Delay(5);
    ret=spi_rw(frame, count + 1);
    for (uint8_t i = 0; i < count; i++) {
        data[i] = frame[i + 1];
    }
    return ret;
}

int PN532_SPI_WriteData(uint8_t *data, uint16_t count) {
    int ret=PN532_STATUS_OK;
    uint8_t frame[count + 1];
    frame[0] = _SPI_DATAWRITE;
    for (uint8_t i = 0; i < count; i++) {
        frame[i + 1] = data[i];
    }
    ret=spi_rw(frame, count + 1);
    return ret;
}

bool PN532_SPI_WaitReady(uint32_t timeout) {
    uint8_t status[] = {_SPI_STATREAD, 0x00};
    uint32_t tickstart = HAL_GetTick();
    while (HAL_GetTick() - tickstart < timeout) {
        HAL_Delay(10);
        spi_rw(status, sizeof(status));
        if (status[1] == _SPI_READY) {
            return true;
        } else {
            HAL_Delay(5);
        }
    }
    return false;
}

int PN532_SPI_Wakeup(void) {
    // Send any special commands/data to wake up PN532
    uint8_t data[] = {0x00};
    spi_rw(data, 1);
    return PN532_STATUS_OK;
}

void PN532_Log(const char* log) {
    display_printf("%s\n", log);
}

char nfc_init(void){

    char ret=0;
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOJ_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_PULLDOWN;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStructure.Pin = GPIO_PIN_4;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_PULLDOWN;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStructure.Pin = GPIO_PIN_4;
    HAL_GPIO_Init(GPIOJ, &GPIO_InitStructure);

    SET_NFC_SPI_CS_H;

    ret=MX_SPI6_Init();

    // init the pn532 functions
    pn532.reset =  PN532_Reset;
    pn532.read_data = PN532_SPI_ReadData;
    pn532.write_data = PN532_SPI_WriteData;
    pn532.wait_ready = PN532_SPI_WaitReady;
    pn532.wakeup = PN532_SPI_Wakeup;
    pn532.log = PN532_Log;

    // hardware wakeup
    pn532.wakeup();

    return ret;
}