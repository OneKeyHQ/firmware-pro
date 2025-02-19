#include STM32_HAL_H

#include <stdio.h>
#include <string.h>

#include "ble.h"
#include "common.h"
#include "dma_channel.h"
#include "irq.h"
#include "usart.h"

#define USART_TIMEOUT 0x100000

UART_HandleTypeDef uart;
UART_HandleTypeDef *huart = &uart;

// static DMA_HandleTypeDef hdma_tx;
static DMA_HandleTypeDef hdma_rx;

static bool uart_tx_done = false;

#define UART_PACKET_MAX_LEN 128
static uint8_t dma_uart_rev_buf[UART_PACKET_MAX_LEN]
    __attribute__((section(".sram3")));
// static uint8_t dma_uart_send_buf[UART_PACKET_MAX_LEN]
//     __attribute__((section(".sram3")));
uint32_t usart_fifo_len = 0;

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
  huart->Init.BaudRate = 115200;
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
  // hdma_tx.Instance = UARTx_TX_DMA_STREAM;

  // hdma_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  // hdma_tx.Init.Request = UARTx_TX_DMA_REQUEST;
  // hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  // hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  // hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
  // hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  // hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  // hdma_tx.Init.Mode = DMA_NORMAL;
  // hdma_tx.Init.Priority = DMA_PRIORITY_MEDIUM;

  // HAL_DMA_Init(&hdma_tx);

  // __HAL_LINKDMA(huart, hdmatx, hdma_tx);

  hdma_rx.Instance = UARTx_RX_DMA_STREAM;

  hdma_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  hdma_rx.Init.Request = UARTx_RX_DMA_REQUEST;
  hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_rx.Init.Mode = DMA_NORMAL;
  hdma_rx.Init.Priority = DMA_PRIORITY_MEDIUM;

  HAL_DMA_Init(&hdma_rx);

  __HAL_LINKDMA(huart, hdmarx, hdma_rx);

  /*##-4- Configure the NVIC for DMA #########################################*/
  NVIC_SetPriority(UARTx_DMA_RX_IRQn, IRQ_PRI_DMA);
  HAL_NVIC_EnableIRQ(UARTx_DMA_RX_IRQn);

  // NVIC_SetPriority(UARTx_DMA_TX_IRQn, IRQ_PRI_DMA);
  // HAL_NVIC_EnableIRQ(UARTx_DMA_TX_IRQn);

  NVIC_SetPriority(UART4_IRQn, IRQ_PRI_UART);
  HAL_NVIC_EnableIRQ(UART4_IRQn);

  __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
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
  HAL_UART_Transmit(huart, buf, len, 0xFFFF);
  // while (len > 0) {
  //   uart_tx_done = false;
  //   uint32_t send_len = len > UART_PACKET_MAX_LEN ? UART_PACKET_MAX_LEN :
  //   len; memcpy(dma_uart_send_buf, buf, send_len);
  //   HAL_UART_Transmit_DMA(huart, dma_uart_send_buf, send_len);
  //   uint32_t start = HAL_GetTick();
  //   while (!uart_tx_done) {
  //     if (HAL_GetTick() - start > 500) {
  //       return;
  //     }
  //     __WFI();
  //   }
  //   len -= send_len;
  //   buf += send_len;
  // }
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

static uint8_t calXor(uint8_t *buf, uint32_t len) {
  uint8_t tmp = 0;
  uint32_t i;
  for (i = 0; i < len; i++) {
    tmp ^= buf[i];
  }
  return tmp;
}

static void usart_rev_package_dma(uint8_t *buf, uint32_t len) {
  if (len < 5) {
    return;
  }
  uint32_t index = 0;
  uint8_t *p_header;
  while (len > 0) {
    if (buf[index] != 0xA5 || buf[index + 1] != 0x5A) {
      index++;
      len--;
      continue;
    }
    p_header = buf + index;
    index += 2;
    len -= 2;
    if (len < 2) {
      return;
    }
    // length include xor byte
    uint16_t data_len = (buf[index] << 8) + buf[index + 1];
    index += 2;
    len -= 2;
    if (len < data_len) {
      return;
    }
    index += data_len;
    len -= data_len;

    uint8_t xor = calXor(p_header, data_len + 3);
    if (buf[index - 1] != xor) {
      return;
    }
    fifo_write_no_overflow(&uart_fifo_in, p_header, data_len + 3);
  }
}

// void UART4_IRQHandler(void) {
//   volatile uint8_t data = 0;
//   (void)data;
//   if (__HAL_UART_GET_FLAG(huart, UART_FLAG_WUF)) {
//     __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_WUF);
//   }
//   if (__HAL_UART_GET_FLAG(huart, UART_FLAG_ORE) != 0) {
//     data = (uint8_t)(huart->Instance->RDR);
//     __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF);
//   }
//   if (__HAL_UART_GET_FLAG(huart, UART_FLAG_RXFNE) != 0) {
//     memset(dma_uart_rev_buf, 0x00, sizeof(dma_uart_rev_buf));
//     usart_rev_package(dma_uart_rev_buf);
//   }
// }

void UARTx_DMA_TX_IRQHandler(void) { HAL_DMA_IRQHandler(huart->hdmatx); }

void UARTx_DMA_RX_IRQHandler(void) { HAL_DMA_IRQHandler(huart->hdmarx); }

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  usart_rev_package_dma(dma_uart_rev_buf, sizeof(dma_uart_rev_buf));
  HAL_UART_Receive_DMA(huart, dma_uart_rev_buf, sizeof(dma_uart_rev_buf));
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) { uart_tx_done = true; }

void UART4_IRQHandler(void) {
  if (__HAL_UART_GET_FLAG(huart, UART_FLAG_WUF)) {
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_WUF);
  }
  if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)) {
    __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_IDLE);
    HAL_UART_Abort(huart);
    usart_fifo_len =
        sizeof(dma_uart_rev_buf) - __HAL_DMA_GET_COUNTER(huart->hdmarx);
    if (usart_fifo_len > 0) {
      usart_rev_package_dma(dma_uart_rev_buf, usart_fifo_len);
    }
    HAL_UART_Receive_DMA(huart, dma_uart_rev_buf, sizeof(dma_uart_rev_buf));
  } else {
    HAL_UART_IRQHandler(huart);
  }
}

void usart_print(const char *text, int text_len) {
  HAL_UART_Transmit(huart, (uint8_t *)text, text_len, 0xFFFF);
}
