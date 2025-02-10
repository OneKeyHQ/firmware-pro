#ifndef _DMA_CHANNEL_H_
#define _DMA_CHANNEL_H_

// ST BLE UART4 DMA
#define UARTx_RX_DMA_STREAM DMA1_Stream0
#define UARTx_TX_DMA_STREAM DMA1_Stream1

#define UARTx_RX_DMA_REQUEST DMA_REQUEST_UART4_RX
#define UARTx_TX_DMA_REQUEST DMA_REQUEST_UART4_TX

#define UARTx_DMA_RX_IRQn DMA1_Stream0_IRQn
#define UARTx_DMA_RX_IRQHandler DMA1_Stream0_IRQHandler

#define UARTx_DMA_TX_IRQn DMA1_Stream1_IRQn
#define UARTx_DMA_TX_IRQHandler DMA1_Stream1_IRQHandler

// ST BLE SPI2 DMA
/* Definition for SPIx's DMA */
#define SPIx_TX_DMA_STREAM DMA1_Stream3
#define SPIx_RX_DMA_STREAM DMA1_Stream2

#define SPIx_TX_DMA_REQUEST DMA_REQUEST_SPI2_TX
#define SPIx_RX_DMA_REQUEST DMA_REQUEST_SPI2_RX

/* Definition for SPIx's NVIC */
#define SPIx_DMA_TX_IRQn DMA1_Stream3_IRQn
#define SPIx_DMA_RX_IRQn DMA1_Stream2_IRQn

#define SPIx_DMA_TX_IRQHandler DMA1_Stream3_IRQHandler
#define SPIx_DMA_RX_IRQHandler DMA1_Stream2_IRQHandler

#endif
