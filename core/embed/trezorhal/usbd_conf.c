// clang-format off

/*
 * This file is part of the Micro Python project, http://micropython.org/
 */

/**
  ******************************************************************************
  * @file    USB_Device/CDC_Standalone/Src/usbd_conf.c
  * @author  MCD Application Team
  * @version V1.4.0
  * @date    17-February-2017
  * @brief   This file implements the USB Device library callbacks and MSP
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include STM32_HAL_H
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usb.h"
#include "irq.h"
#include "supervise.h"
#include "systemview.h"
#include "common.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#ifdef USE_USB_FS
static PCD_HandleTypeDef pcd_fs_handle;
#endif
#ifdef USE_USB_HS
static PCD_HandleTypeDef pcd_hs_handle;
#endif

volatile bool usb_connect_state = false;
volatile bool data_in_done = false;
static uint8_t data_in_ep = 0xff;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
                       PCD BSP Routines
*******************************************************************************/
/**
  * @brief  Initializes the PCD MSP.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
  GPIO_InitTypeDef  GPIO_InitStruct;

  if(hpcd->Instance == USB_OTG_FS)
  {
#if defined(STM32H747xx)
    uint32_t pin_alternate = GPIO_AF10_OTG2_FS;
#else
    uint32_t pin_alternate = GPIO_AF10_OTG_FS;
#endif
    /* Configure USB FS GPIOs */
    __HAL_RCC_GPIOA_CLK_ENABLE();
 
    /* Configure DM DP Pins */
    GPIO_InitStruct.Pin = (GPIO_PIN_11 | GPIO_PIN_12);
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = pin_alternate;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* Configure VBUS Pin */
#if defined(MICROPY_HW_USB_VBUS_DETECT_PIN)
    // USB VBUS detect pin is always A9
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

    /* Configure ID pin */
#if defined(MICROPY_HW_USB_OTG_ID_PIN)
    // USB ID pin is always A10
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Alternate = pin_alternate;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

    /* Enable USB FS Clocks */
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

    /* Set USBFS Interrupt priority */
    svc_setpriority(OTG_FS_IRQn, IRQ_PRI_OTG_FS);

    /* Enable USBFS Interrupt */
    svc_enableIRQ(OTG_FS_IRQn);
  }
#if defined(USE_USB_HS)
  else if(hpcd->Instance == USB_OTG_HS)
  {

    /* Configure USB HS GPIOs */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    /* CLK */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_HS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* D0 */
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_HS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* D1 D2 D3 D4 D5 D6 D7 */
    GPIO_InitStruct.Pin = GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_5 |\
                          GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_HS;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* STP */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_HS;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* NXT */
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_HS;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    /* DIR */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_HS;
    HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

    /* Enable USB HS Clocks */
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
    __HAL_RCC_USB_OTG_HS_ULPI_CLK_ENABLE();
  

    /* Set USBHS Interrupt to the lowest priority */
    svc_setpriority(OTG_HS_IRQn, IRQ_PRI_OTG_HS);

    /* Enable USBHS Interrupt */
    svc_enableIRQ(OTG_HS_IRQn);
  }
#endif  // USE_USB_HS
}
/**
  * @brief  DeInitializes the PCD MSP.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_MspDeInit(PCD_HandleTypeDef *hpcd)
{
  if(hpcd->Instance == USB_OTG_FS)
  {
    /* Disable USB FS Clocks */
    __HAL_RCC_USB_OTG_FS_CLK_DISABLE();
    __HAL_RCC_SYSCFG_CLK_DISABLE();
  }
  #if defined(USE_USB_HS)
  else if(hpcd->Instance == USB_OTG_HS)
  {
    /* Disable USB FS Clocks */
    __HAL_RCC_USB_OTG_HS_CLK_DISABLE();
    __HAL_RCC_SYSCFG_CLK_DISABLE();
  }
  #endif
}

/*******************************************************************************
                       LL Driver Callbacks (PCD -> USB Device Library)
*******************************************************************************/


/**
  * @brief  Setup stage callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_SetupStage(hpcd->pData, (uint8_t *)hpcd->Setup);
  usb_connect_state = true;
}

/**
  * @brief  Data Out stage callback.
  * @param  hpcd: PCD handle
  * @param  epnum: Endpoint Number
  * @retval None
  */
void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
  USBD_LL_DataOutStage(hpcd->pData, epnum, hpcd->OUT_ep[epnum].xfer_buff);
}

/**
  * @brief  Data In stage callback.
  * @param  hpcd: PCD handle
  * @param  epnum: Endpoint Number
  * @retval None
  */
void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
  USBD_LL_DataInStage(hpcd->pData, epnum, hpcd->IN_ep[epnum].xfer_buff);
  if(data_in_ep == epnum){
    data_in_done = true;
  }
}

/**
  * @brief  SOF callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_SOF(hpcd->pData);
}

/**
  * @brief  Reset callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_SpeedTypeDef speed = USBD_SPEED_FULL;

  /* Set USB Current Speed */
  switch(hpcd->Init.speed)
  {
  case PCD_SPEED_HIGH:
    speed = USBD_SPEED_HIGH;
    break;

  case PCD_SPEED_FULL:
    speed = USBD_SPEED_FULL;
    break;

	default:
    speed = USBD_SPEED_FULL;
    break;
  }
  USBD_LL_SetSpeed(hpcd->pData, speed);

  /* Reset Device */
  USBD_LL_Reset(hpcd->pData);
}

/**
  * @brief  Suspend callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_Suspend(hpcd->pData);
  usb_connect_state = false;
}

/**
  * @brief  Resume callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_Resume(hpcd->pData);
}

/**
  * @brief  ISOC Out Incomplete callback.
  * @param  hpcd: PCD handle
  * @param  epnum: Endpoint Number
  * @retval None
  */
void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
  USBD_LL_IsoOUTIncomplete(hpcd->pData, epnum);
}

/**
  * @brief  ISOC In Incomplete callback.
  * @param  hpcd: PCD handle
  * @param  epnum: Endpoint Number
  * @retval None
  */
void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
  USBD_LL_IsoINIncomplete(hpcd->pData, epnum);
}

/**
  * @brief  Connect callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_DevConnected(hpcd->pData);
  usb_connect_state = true;
}

/**
  * @brief  Disconnect callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_DevDisconnected(hpcd->pData);
  usb_connect_state = false;
}

/*******************************************************************************
                       LL Driver Interface (USB Device Library --> PCD)
*******************************************************************************/
/**
  * @brief  Initializes the Low Level portion of the Device driver.
  * @param  pdev: Device handle
  * @retval USBD Status
  */
USBD_StatusTypeDef  USBD_LL_Init (USBD_HandleTypeDef *pdev)
{
#if defined(USE_USB_FS)
  // Trezor 1 uses the OTG_FS peripheral
  if (pdev->id == USB_PHY_FS_ID) {
    /*Set LL Driver parameters */
    pcd_fs_handle.Instance = USB_OTG_FS;
    pcd_fs_handle.Init.dev_endpoints = 4;
    pcd_fs_handle.Init.use_dedicated_ep1 = 0;
    pcd_fs_handle.Init.ep0_mps = 0x40;
    pcd_fs_handle.Init.dma_enable = 0;
    pcd_fs_handle.Init.low_power_enable = 0;
    pcd_fs_handle.Init.phy_itface = PCD_PHY_EMBEDDED;
    pcd_fs_handle.Init.Sof_enable = 1;
    pcd_fs_handle.Init.speed = PCD_SPEED_FULL;
    pcd_fs_handle.Init.vbus_sensing_enable = 0; // No VBUS Sensing on USB0
    /* Link The driver to the stack */
    pcd_fs_handle.pData = pdev;
    pdev->pData = &pcd_fs_handle;
    /*Initialize LL Driver */
    HAL_PCD_Init(&pcd_fs_handle);
    // the OTG_FS peripheral has a dedicated 1.25KiB data RAM from which we
    // allocate an area for each transmit FIFO and the single shared receive FIFO.
    // the configuration is in terms of 32-bit words, so we have 320 32-bit words
    // in this dedicated 1.25KiB data RAM to use. see section 6.3.8 in UM1021 and 29.13 in RM0033.
    // USB packets that we deal with are 64 bytes in size which equates to 16 32-bit words.
    // we size the transmit FIFO's equally and give the rest of the space to the receive FIFO.
    const uint16_t transmit_fifo_size = 48; // 48 = 16 * 3 meaning that we give 3 packets of space for each transmit fifo
    const uint16_t receive_fifo_zie = 128; // 128 = 320 - 4 * 48
    HAL_PCDEx_SetRxFiFo(&pcd_fs_handle, receive_fifo_zie);
    for (uint16_t i = 0; i < 4; i++) {
      HAL_PCDEx_SetTxFiFo(&pcd_fs_handle, i, transmit_fifo_size);
    }
  }
#endif
#if defined(USE_USB_HS)
  // Trezor T uses the OTG_HS peripheral
  if (pdev->id == USB_PHY_HS_ID) {

      /* Set LL Driver parameters */
      pcd_hs_handle.Instance = USB_OTG_HS;
      pcd_hs_handle.Init.dev_endpoints = 6;
      pcd_hs_handle.Init.use_dedicated_ep1 = 0;
      pcd_hs_handle.Init.ep0_mps = 0x40;
      pcd_hs_handle.Init.dma_enable = 0;
      pcd_hs_handle.Init.low_power_enable = 0;
      pcd_hs_handle.Init.phy_itface = PCD_PHY_ULPI;
      pcd_hs_handle.Init.Sof_enable = 1;
      pcd_hs_handle.Init.speed = PCD_SPEED_HIGH;
     
    // Trezor T hardware has PB13 connected to HS_VBUS
    // but we leave vbus sensing disabled because
    // we don't use it for anything. the device is a bus powered peripheral.
    pcd_hs_handle.Init.vbus_sensing_enable = 0;
    /* Link The driver to the stack */
    pcd_hs_handle.pData = pdev;
    pdev->pData = &pcd_hs_handle;
    /* Initialize LL Driver */
    HAL_PCD_Init(&pcd_hs_handle);
    // the OTG_HS peripheral has a dedicated 4KiB data RAM from which we
    // allocate an area for each transmit FIFO and the single shared receive FIFO.
    // the configuration is in terms of 32-bit words, so we have 1024 32-bit words
    // in this dedicated 4KiB data RAM to use. see section 35.10.1 and 34.11 in RM0090.
    // the reference to section 34.11 is for the OTG_FS device, but the FIFO architecture
    // diagram seems to apply similarly to the FIFO in the OTG_HS that we are using.
    // USB packets that we deal with are 64 bytes in size which equates to 16 32-bit words.
    // we size the transmit FIFO's equally and give the rest of the space to the receive FIFO.
    const uint16_t transmit_fifo_size = 144; // 144 = 16 * 9 meaning that we give 9 packets of space for each transmit fifo
    const uint16_t receive_fifo_zie = 160; // 160 = 1024 - 6 * 144 section 35.10.1 details what some of this is used for besides storing packets
    HAL_PCDEx_SetRxFiFo(&pcd_hs_handle, receive_fifo_zie);
    for (uint16_t i = 0; i < 6; i++) {
      HAL_PCDEx_SetTxFiFo(&pcd_hs_handle, i, transmit_fifo_size);
    }
  }
#endif
  return USBD_OK;
}

/**
  * @brief  De-Initializes the Low Level portion of the Device driver.
  * @param  pdev: Device handle
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev)
{
  HAL_PCD_DeInit(pdev->pData);
  return USBD_OK;
}

/**
  * @brief  Starts the Low Level portion of the Device driver.
  * @param  pdev: Device handle
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev)
{
  HAL_PCD_Start(pdev->pData);
  return USBD_OK;
}

/**
  * @brief  Stops the Low Level portion of the Device driver.
  * @param  pdev: Device handle
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev)
{
  HAL_PCD_Stop(pdev->pData);
  return USBD_OK;
}

/**
  * @brief  Opens an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @param  ep_type: Endpoint Type
  * @param  ep_mps: Endpoint Max Packet Size
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev,
                                  uint8_t  ep_addr,
                                  uint8_t  ep_type,
                                  uint16_t ep_mps)
{
  HAL_PCD_EP_Open(pdev->pData, ep_addr, ep_mps, ep_type);
  return USBD_OK;
}

/**
  * @brief  Closes an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  HAL_PCD_EP_Close(pdev->pData, ep_addr);
  return USBD_OK;
}

/**
  * @brief  Flushes an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  HAL_PCD_EP_Flush(pdev->pData, ep_addr);
  return USBD_OK;
}

/**
  * @brief  Sets a Stall condition on an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  HAL_PCD_EP_SetStall(pdev->pData, ep_addr);
  return USBD_OK;
}

/**
  * @brief  Clears a Stall condition on an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  HAL_PCD_EP_ClrStall(pdev->pData, ep_addr);
  return USBD_OK;
}

/**
  * @brief  Returns Stall condition.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval Stall (1: yes, 0: No)
  */
uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  PCD_HandleTypeDef *hpcd = pdev->pData;

  if((ep_addr & 0x80) == 0x80)
  {
    return hpcd->IN_ep[ep_addr & 0x7F].is_stall;
  }
  else
  {
    return hpcd->OUT_ep[ep_addr & 0x7F].is_stall;
  }
}

/**
  * @brief  Assigns an USB address to the device
  * @param  pdev: Device handle
  * @param  dev_addr: USB address
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t dev_addr)
{
  HAL_PCD_SetAddress(pdev->pData, dev_addr);
  return USBD_OK;
}

/**
  * @brief  Transmits data over an endpoint
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @param  pbuf: Pointer to data to be sent
  * @param  size: Data size
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev,
                                    uint8_t  ep_addr,
                                    uint8_t  *pbuf,
                                    uint16_t  size)
{
  HAL_PCD_EP_Transmit(pdev->pData, ep_addr, pbuf, size);  
  return USBD_OK;
}

/**
  * @brief  Prepares an endpoint for reception
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @param  pbuf:pointer to data to be received
  * @param  size: data size
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev,
                                          uint8_t  ep_addr,
                                          uint8_t  *pbuf,
                                          uint16_t  size)
{
  HAL_PCD_EP_Receive(pdev->pData, ep_addr, pbuf, size);
  return USBD_OK;
}

/**
  * @brief  Returns the last transferred packet size.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval Received Data Size
  */
uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t  ep_addr)
{
  return HAL_PCD_EP_GetRxCount(pdev->pData, ep_addr);
}

/**
  * @brief  Delay routine for the USB Device Library
  * @param  Delay: Delay in ms
  * @retval None
  */
void  USBD_LL_Delay(uint32_t Delay)
{
  HAL_Delay(Delay);
}

/*******************************************************************************
                       IRQ Handlers
*******************************************************************************/

/**
  * @brief  This function handles USB-On-The-Go FS/HS global interrupt request.
  * @param  None
  * @retval None
  */
#if defined(USE_USB_FS)
void OTG_FS_IRQHandler(void) {
    SEGGER_SYSVIEW_RecordEnterISR();
    IRQ_ENTER(OTG_FS_IRQn);
    if (pcd_fs_handle.Instance) {
        HAL_PCD_IRQHandler(&pcd_fs_handle);
    }
    IRQ_EXIT(OTG_FS_IRQn);
    SEGGER_SYSVIEW_RecordExitISR();
}
#endif
#if defined(USE_USB_HS)
void OTG_HS_IRQHandler(void) {
    SEGGER_SYSVIEW_RecordEnterISR();
    IRQ_ENTER(OTG_HS_IRQn);
    if (pcd_hs_handle.Instance) {
        HAL_PCD_IRQHandler(&pcd_hs_handle);
    }
    IRQ_EXIT(OTG_HS_IRQn);
    SEGGER_SYSVIEW_RecordExitISR();
}
#endif

/**
  * @brief  This function handles USB OTG Common FS/HS Wakeup functions.
  * @param  *pcd_handle for FS or HS
  * @retval None
  */
#if defined(USE_USB_FS) || defined(USE_USB_HS)
static void OTG_CMD_WKUP_Handler(PCD_HandleTypeDef *pcd_handle) {
    if (!(pcd_handle->Init.low_power_enable)) {
        return;
    }

    /* Reset SLEEPDEEP bit of Cortex System Control Register */
    SCB->SCR &= (uint32_t) ~((uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));

    /* Configures system clock after wake-up from STOP: enable HSE, PLL and select
    PLL as system clock source (HSE and PLL are disabled in STOP mode) */

    __HAL_RCC_HSE_CONFIG(RCC_HSE_ON);

    /* Wait till HSE is ready */
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) == RESET) {}

    /* Enable the main PLL. */
    __HAL_RCC_PLL_ENABLE();

    /* Wait till PLL is ready */
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY) == RESET) {}

    /* Select PLL as SYSCLK */
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_SYSCLKSOURCE_PLLCLK);
#if defined(STM32H747xx)
    while (__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_CFGR_SWS_PLL1) {}
#else
     while (__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_CFGR_SWS_PLL) {}
#endif
    /* ungate PHY clock */
    __HAL_PCD_UNGATE_PHYCLOCK(pcd_handle);
}
#endif

/**
  * @brief  This function handles USB OTG FS/HS Wakeup IRQ Handler.
  * @param  None
  * @retval None
  */
#if defined(USE_USB_FS)
void OTG_FS_WKUP_IRQHandler(void) {
    IRQ_ENTER(OTG_FS_WKUP_IRQn);
    if (pcd_fs_handle.Instance) {
        OTG_CMD_WKUP_Handler(&pcd_fs_handle);
    }
    /* Clear EXTI pending Bit*/
    __HAL_USB_OTG_FS_WAKEUP_EXTI_CLEAR_FLAG();

    IRQ_EXIT(OTG_FS_WKUP_IRQn);
}
#endif
#if defined(USE_USB_HS)
void OTG_HS_WKUP_IRQHandler(void) {
    IRQ_ENTER(OTG_HS_WKUP_IRQn);
    if (pcd_hs_handle.Instance) {
        OTG_CMD_WKUP_Handler(&pcd_hs_handle);
    }
    /* Clear EXTI pending Bit*/
#if defined(STM32H747xx)
    EXTI->PR1 = (USB_OTG_HS_WAKEUP_EXTI_LINE)
#else
    __HAL_USB_HS_EXTI_CLEAR_FLAG();
#endif
    IRQ_EXIT(OTG_HS_WKUP_IRQn);
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
