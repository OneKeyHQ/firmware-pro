#include STM32_HAL_H

#include <stdio.h>
#include <string.h>

#include "ble.h"
#include "common.h"
#include "dma_channel.h"
#include "irq.h"
#include "usart.h"

#define BAUD_RATE 115200
#define UART_ONE_BIT_TIME_US (1000000 / BAUD_RATE)
// max timeout is 0xffffffff * UART_ONE_BIT_TIME_US
#define UART_TIMEOUT_MS (2)

UART_HandleTypeDef uart;
UART_HandleTypeDef *huart = &uart;

static DMA_HandleTypeDef hdma_tx;
static DMA_HandleTypeDef hdma_rx;

static bool uart_tx_done = false;

#define UART_PACKET_MAX_LEN 256
static uint8_t dma_uart_rev_buf[UART_PACKET_MAX_LEN]
    __attribute__((section(".sram3")));
static uint8_t dma_uart_send_buf[UART_PACKET_MAX_LEN]
    __attribute__((section(".sram3")));
static uint32_t usart_dma_rx_read_pos = 0;

uint8_t uart_data_in[UART_BUF_MAX_LEN];

trans_fifo uart_fifo_in = {.p_buf = uart_data_in,
                           .buf_size = UART_BUF_MAX_LEN,
                           .over_pre = false,
                           .read_pos = 0,
                           .write_pos = 0,
                           .lock_pos = 0};

void ble_usart_init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  HAL_SYSCFG_AnalogSwitchConfig(SYSCFG_SWITCH_PA0, SYSCFG_SWITCH_PA0_CLOSE);
  HAL_SYSCFG_AnalogSwitchConfig(SYSCFG_SWITCH_PA1, SYSCFG_SWITCH_PA1_CLOSE);

  __HAL_RCC_UART4_FORCE_RESET();
  __HAL_RCC_UART4_RELEASE_RESET();

  __HAL_RCC_UART4_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  __HAL_RCC_DMA1_FORCE_RESET();
  __HAL_RCC_DMA1_RELEASE_RESET();
  __HAL_RCC_DMA1_CLK_ENABLE();

  // UART4: PA0_C(TX), PA1_C(RX)
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  huart->Instance = UART4;
  huart->Init.BaudRate = BAUD_RATE;
  huart->Init.WordLength = UART_WORDLENGTH_8B;
  huart->Init.StopBits = UART_STOPBITS_1;
  huart->Init.Parity = UART_PARITY_NONE;
  huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart->Init.Mode = UART_MODE_TX_RX;
  huart->Init.OverSampling = UART_OVERSAMPLING_16;
  huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart->Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(huart) != HAL_OK) {
    ensure(secfalse, "uart init failed");
  }

  // Configure DMA
  hdma_tx.Instance = UARTx_TX_DMA_STREAM;

  hdma_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  hdma_tx.Init.Request = UARTx_TX_DMA_REQUEST;
  hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_tx.Init.Mode = DMA_NORMAL;
  hdma_tx.Init.Priority = DMA_PRIORITY_MEDIUM;

  HAL_DMA_Init(&hdma_tx);
  __HAL_LINKDMA(huart, hdmatx, hdma_tx);

  hdma_rx.Instance = UARTx_RX_DMA_STREAM;

  hdma_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  hdma_rx.Init.Request = UARTx_RX_DMA_REQUEST;
  hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_rx.Init.Mode = DMA_CIRCULAR;
  hdma_rx.Init.Priority = DMA_PRIORITY_MEDIUM;

  HAL_DMA_Init(&hdma_rx);
  __HAL_LINKDMA(huart, hdmarx, hdma_rx);

  // clear all on going tx/rx
  HAL_UART_Abort(huart);

  /*##-4- Configure the NVIC for DMA #########################################*/
  NVIC_SetPriority(UARTx_DMA_RX_IRQn, IRQ_PRI_DMA);
  HAL_NVIC_ClearPendingIRQ(UARTx_DMA_RX_IRQn);
  HAL_NVIC_EnableIRQ(UARTx_DMA_RX_IRQn);

  NVIC_SetPriority(UARTx_DMA_TX_IRQn, IRQ_PRI_DMA);
  HAL_NVIC_ClearPendingIRQ(UARTx_DMA_TX_IRQn);
  HAL_NVIC_EnableIRQ(UARTx_DMA_TX_IRQn);

  NVIC_SetPriority(UART4_IRQn, IRQ_PRI_UART);
  HAL_NVIC_ClearPendingIRQ(UART4_IRQn);
  HAL_NVIC_EnableIRQ(UART4_IRQn);

  HAL_UART_EnableReceiverTimeout(huart);
  HAL_UART_ReceiverTimeout_Config(
      huart, (UART_TIMEOUT_MS * 1000) / UART_ONE_BIT_TIME_US);
  __HAL_UART_ENABLE_IT(huart, UART_IT_RTO);
  usart_dma_rx_read_pos = 0;
  HAL_UART_Receive_DMA(huart, dma_uart_rev_buf, sizeof(dma_uart_rev_buf));
}

void usart_enable_stop_wup(void) {
  UART_WakeUpTypeDef WakeUpSelection;

  WakeUpSelection.WakeUpEvent = UART_WAKEUP_ON_STARTBIT;
  HAL_UARTEx_StopModeWakeUpSourceConfig(huart, WakeUpSelection);
  __HAL_UART_ENABLE_IT(huart, UART_IT_WUF);
  HAL_UARTEx_EnableStopMode(huart);
}

void usart_disable_stop_wup(void) {
  HAL_UARTEx_DisableStopMode(huart);
  __HAL_UART_DISABLE_IT(huart, UART_IT_WUF);
}

void ble_usart_send_byte(uint8_t data) {
  HAL_UART_Transmit(huart, &data, 1, 0xFFFF);
}

void ble_usart_send(uint8_t *buf, uint32_t len) {
  while (len > 0) {
    uart_tx_done = false;
    uint32_t send_len = len > UART_PACKET_MAX_LEN ? UART_PACKET_MAX_LEN : len;
    memcpy(dma_uart_send_buf, buf, send_len);
    if (HAL_UART_Transmit_DMA(huart, dma_uart_send_buf, send_len) != HAL_OK) {
      return;
    }
    uint32_t start = HAL_GetTick();
    while (!uart_tx_done) {
      if (HAL_GetTick() - start > 500) {
        return;
      }
      __WFI();
    }
    len -= send_len;
    buf += send_len;
  }
}

bool ble_read_byte(uint8_t *buf) {
  if (HAL_UART_Receive(huart, buf, 1, 50) == HAL_OK) {
    return true;
  }
  return false;
}

secbool ble_usart_can_read(void) {
  if (fifo_lockdata_len(&uart_fifo_in)) {
    return sectrue;
  } else {
    return secfalse;
  }
}

void ble_usart_irq_ctrl(bool enable) {
  if (enable) {
    HAL_NVIC_EnableIRQ(UART4_IRQn);
    HAL_UART_Abort(huart);
    usart_dma_rx_read_pos = 0;
    HAL_UART_Receive_DMA(huart, dma_uart_rev_buf, sizeof(dma_uart_rev_buf));
  } else {
    HAL_UART_Abort(huart);
    HAL_NVIC_DisableIRQ(UART4_IRQn);
  }
}

uint32_t ble_usart_read(uint8_t *buf, uint32_t lenth) {
  uint32_t len = 0;
  fifo_read_peek(&uart_fifo_in, buf, 4);

  len = (buf[2] << 8) + buf[3];

  fifo_read_lock(&uart_fifo_in, buf, len + 3);
  return len + 3;
}

static void usart_rx_dma_process(void) {
  uint32_t write_pos =
      sizeof(dma_uart_rev_buf) - __HAL_DMA_GET_COUNTER(huart->hdmarx);
  if (write_pos == usart_dma_rx_read_pos) {
    return;
  }

  uint32_t available_len;
  if (write_pos > usart_dma_rx_read_pos) {
    available_len = write_pos - usart_dma_rx_read_pos;
  } else {
    available_len =
        sizeof(dma_uart_rev_buf) - usart_dma_rx_read_pos + write_pos;
  }

  uint32_t processed_len = 0;
  uint32_t current_read_pos = usart_dma_rx_read_pos;
  const uint32_t buf_len = sizeof(dma_uart_rev_buf);

  while (processed_len < available_len) {
    uint32_t remaining = available_len - processed_len;

    if (remaining < 2) {
      break;
    }

    uint8_t b0 = dma_uart_rev_buf[current_read_pos % buf_len];
    uint8_t b1 = dma_uart_rev_buf[(current_read_pos + 1) % buf_len];
    if (b0 != 0xA5 || b1 != 0x5A) {
      current_read_pos = (current_read_pos + 1) % buf_len;
      processed_len += 1;
      continue;
    }

    if (remaining < 4) {
      break;
    }

    uint16_t data_len =
        (dma_uart_rev_buf[(current_read_pos + 2) % buf_len] << 8) +
        dma_uart_rev_buf[(current_read_pos + 3) % buf_len];

    // Packet format: [0xA5][0x5A][LH][LL][payload...][XOR]
    // data_len = len(payload) + len(XOR)
    uint32_t packet_total_len = data_len + 4;  // Including header
    uint32_t packet_store_len = data_len + 3;  // Excluding XOR byte

    // Validate packet length
    if (packet_total_len < 5 || packet_total_len > buf_len) {
      current_read_pos = (current_read_pos + 1) % buf_len;
      processed_len += 1;
      continue;
    }

    if (remaining < packet_total_len) {
      break;
    }

    uint8_t xor = 0;
    uint32_t first_part_len = buf_len - current_read_pos;
    if (first_part_len > packet_store_len) {
      first_part_len = packet_store_len;
    }
    uint32_t second_part_len = packet_store_len - first_part_len;
    for (uint32_t i = 0; i < first_part_len; i++) {
      xor ^= dma_uart_rev_buf[current_read_pos + i];
    }
    for (uint32_t i = 0; i < second_part_len; i++) {
      xor ^= dma_uart_rev_buf[i];
    }
    if (xor !=
        dma_uart_rev_buf[(current_read_pos + packet_total_len - 1) % buf_len]) {
      current_read_pos = (current_read_pos + 1) % buf_len;
      processed_len += 1;
      continue;
    }

    for (uint32_t i = 0; i < first_part_len; i++) {
      fifo_put_no_overflow(&uart_fifo_in,
                           dma_uart_rev_buf[current_read_pos + i]);
    }
    for (uint32_t i = 0; i < second_part_len; i++) {
      fifo_put_no_overflow(&uart_fifo_in, dma_uart_rev_buf[i]);
    }

    fifo_lockpos_set(&uart_fifo_in);

    current_read_pos = (current_read_pos + packet_total_len) % buf_len;
    processed_len += packet_total_len;
  }

  usart_dma_rx_read_pos = current_read_pos;
}

void UARTx_DMA_TX_IRQHandler(void) { HAL_DMA_IRQHandler(huart->hdmatx); }

void UARTx_DMA_RX_IRQHandler(void) { HAL_DMA_IRQHandler(huart->hdmarx); }

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  usart_rx_dma_process();
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
  usart_rx_dma_process();
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) { uart_tx_done = true; }

void UART4_IRQHandler(void) {
  if (__HAL_UART_GET_FLAG(huart, UART_FLAG_WUF)) {
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_WUF);
  }
  if (__HAL_UART_GET_FLAG(huart, UART_FLAG_RTOF)) {
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_RTOF);
    usart_rx_dma_process();
  }
  HAL_UART_IRQHandler(huart);
}

void usart_print(const char *text, int text_len) {
  HAL_UART_Transmit(huart, (uint8_t *)text, text_len, 0xFFFF);
}
