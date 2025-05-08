#include "lvgl.h"

#include "jpeg_dma.h"

#include STM32_HAL_H

#include "ff.h"
#include "irq.h"

#define CHUNK_SIZE_OUT ((uint32_t)(64 * 1024))
#define MAX_JPEG_SIZE (1024 * 1024)

JPEG_HandleTypeDef JPEG_Handle;
JPEG_ConfTypeDef JPEG_Info;

volatile uint32_t Jpeg_HWDecodingEnd = 0, Jpeg_HWDecodingError = 0;

uint32_t FrameBufferAddress;

uint8_t *g_inputJpegBuffer = NULL;
uint32_t g_inputJpegSize = 0;
uint32_t g_inputJpegOffset = 0;

uint8_t *g_outputBuffer = NULL;

typedef struct {
  FIL fatfs_file;
  lv_fs_file_t lvgl_file;
  uint8_t mode;
} file_operation_t;

file_operation_t file_operation = {
    .mode = JPEG_FILE_LVGL,
};

void jpeg_decode_file_operation(uint8_t mode) { file_operation.mode = mode; }

int jpeg_decode_file_open(const char *path) {
  if (file_operation.mode == JPEG_FILE_LVGL) {
    return lv_fs_open(&file_operation.lvgl_file, path, LV_FS_MODE_RD);
  } else if (file_operation.mode == JPEG_FILE_FATFS) {
    return f_open(&file_operation.fatfs_file, path, FA_READ);
  }
  return -1;
}

int jpeg_decode_file_size(uint32_t *size) {
  if (file_operation.mode == JPEG_FILE_LVGL) {
    uint32_t curr_pos = 0;
    uint32_t file_size = 0;
    lv_fs_tell(&file_operation.lvgl_file, &curr_pos);
    if (lv_fs_seek(&file_operation.lvgl_file, 0, LV_FS_SEEK_END) !=
        LV_FS_RES_OK) {
      return -1;
    }
    lv_fs_tell(&file_operation.lvgl_file, &file_size);
    lv_fs_seek(&file_operation.lvgl_file, curr_pos, LV_FS_SEEK_SET);
    *size = file_size;
    return 0;
  } else if (file_operation.mode == JPEG_FILE_FATFS) {
    *size = f_size(&file_operation.fatfs_file);
    return 0;
  }
  return -1;
}

int jpeg_decode_file_read(uint8_t *buf, uint32_t len, uint32_t *read_len) {
  if (file_operation.mode == JPEG_FILE_LVGL) {
    return lv_fs_read(&file_operation.lvgl_file, buf, len, read_len);
  } else if (file_operation.mode == JPEG_FILE_FATFS) {
    return f_read(&file_operation.fatfs_file, buf, len, (UINT *)read_len);
  }
  return -1;
}

void jpeg_decode_file_close(void) {
  if (file_operation.mode == JPEG_FILE_LVGL) {
    lv_fs_close(&file_operation.lvgl_file);
  } else if (file_operation.mode == JPEG_FILE_FATFS) {
    f_close(&file_operation.fatfs_file);
  }
}
int jpeg_get_decode_state(void) { return Jpeg_HWDecodingEnd; }

int jpeg_get_decode_error(void) { return Jpeg_HWDecodingError; }

void HAL_JPEG_MspInit(JPEG_HandleTypeDef *hjpeg) {
  static MDMA_HandleTypeDef hmdmaIn;
  static MDMA_HandleTypeDef hmdmaOut;

  /* Enable JPEG clock */
  __HAL_RCC_JPGDECEN_CLK_ENABLE();

  /* Enable MDMA clock */
  __HAL_RCC_MDMA_CLK_ENABLE();

  NVIC_SetPriority(JPEG_IRQn, IRQ_PRI_JPEG);
  HAL_NVIC_EnableIRQ(JPEG_IRQn);

  /* Input MDMA */
  /* Set the parameters to be configured */
  hmdmaIn.Init.Priority = MDMA_PRIORITY_HIGH;
  hmdmaIn.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
  hmdmaIn.Init.SourceInc = MDMA_SRC_INC_BYTE;
  hmdmaIn.Init.DestinationInc = MDMA_DEST_INC_DISABLE;
  hmdmaIn.Init.SourceDataSize = MDMA_SRC_DATASIZE_BYTE;
  hmdmaIn.Init.DestDataSize = MDMA_DEST_DATASIZE_WORD;
  hmdmaIn.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
  hmdmaIn.Init.SourceBurst = MDMA_SOURCE_BURST_32BEATS;
  hmdmaIn.Init.DestBurst = MDMA_DEST_BURST_16BEATS;
  hmdmaIn.Init.SourceBlockAddressOffset = 0;
  hmdmaIn.Init.DestBlockAddressOffset = 0;

  /*Using JPEG Input FIFO Threshold as a trigger for the MDMA*/
  hmdmaIn.Init.Request =
      MDMA_REQUEST_JPEG_INFIFO_TH; /* Set the MDMA HW trigger to JPEG Input FIFO
                                      Threshold flag*/
  hmdmaIn.Init.TransferTriggerMode = MDMA_BUFFER_TRANSFER;
  hmdmaIn.Init.BufferTransferLength =
      32; /*Set the MDMA buffer size to the JPEG FIFO threshold size i.e 32
             bytes (8 words)*/

  hmdmaIn.Instance = MDMA_Channel7;

  /* Associate the DMA handle */
  __HAL_LINKDMA(hjpeg, hdmain, hmdmaIn);

  /* DeInitialize the DMA Stream */
  HAL_MDMA_DeInit(&hmdmaIn);
  /* Initialize the DMA stream */
  HAL_MDMA_Init(&hmdmaIn);

  /* Output MDMA */
  /* Set the parameters to be configured */
  hmdmaOut.Init.Priority = MDMA_PRIORITY_VERY_HIGH;
  hmdmaOut.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
  hmdmaOut.Init.SourceInc = MDMA_SRC_INC_DISABLE;
  hmdmaOut.Init.DestinationInc = MDMA_DEST_INC_BYTE;
  hmdmaOut.Init.SourceDataSize = MDMA_SRC_DATASIZE_WORD;
  hmdmaOut.Init.DestDataSize = MDMA_DEST_DATASIZE_BYTE;
  hmdmaOut.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
  hmdmaOut.Init.SourceBurst = MDMA_SOURCE_BURST_32BEATS;
  hmdmaOut.Init.DestBurst = MDMA_DEST_BURST_32BEATS;
  hmdmaOut.Init.SourceBlockAddressOffset = 0;
  hmdmaOut.Init.DestBlockAddressOffset = 0;

  /*Using JPEG Output FIFO Threshold as a trigger for the MDMA*/
  hmdmaOut.Init.Request =
      MDMA_REQUEST_JPEG_OUTFIFO_TH; /* Set the MDMA HW trigger to JPEG Output
                                       FIFO Threshold flag*/
  hmdmaOut.Init.TransferTriggerMode = MDMA_BUFFER_TRANSFER;
  hmdmaOut.Init.BufferTransferLength =
      32; /*Set the MDMA buffer size to the JPEG FIFO threshold size i.e 32
             bytes (8 words)*/

  hmdmaOut.Instance = MDMA_Channel6;
  /* DeInitialize the DMA Stream */
  HAL_MDMA_DeInit(&hmdmaOut);
  /* Initialize the DMA stream */
  HAL_MDMA_Init(&hmdmaOut);

  /* Associate the DMA handle */
  __HAL_LINKDMA(hjpeg, hdmaout, hmdmaOut);

  // HAL_NVIC_SetPriority(MDMA_IRQn, 0x08, 0x00);
  NVIC_SetPriority(MDMA_IRQn, IRQ_PRI_DMA);
  HAL_NVIC_EnableIRQ(MDMA_IRQn);
}

void HAL_JPEG_MspDeInit(JPEG_HandleTypeDef *hjpeg) {
  HAL_NVIC_DisableIRQ(MDMA_IRQn);

  /* DeInitialize the MDMA Stream */
  HAL_MDMA_DeInit(hjpeg->hdmain);

  /* DeInitialize the MDMA Stream */
  HAL_MDMA_DeInit(hjpeg->hdmaout);
}

void JPEG_IRQHandler(void) { HAL_JPEG_IRQHandler(&JPEG_Handle); }

void MDMA_IRQHandler() {
  HAL_MDMA_IRQHandler(JPEG_Handle.hdmain);
  HAL_MDMA_IRQHandler(JPEG_Handle.hdmaout);
}

void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg,
                              uint32_t NbDecodedData) {
  g_inputJpegOffset += NbDecodedData;
  if (g_inputJpegOffset < g_inputJpegSize) {
    HAL_JPEG_ConfigInputBuffer(hjpeg, g_inputJpegBuffer + g_inputJpegOffset,
                               g_inputJpegSize - g_inputJpegOffset);
  }
}

void HAL_JPEG_DataReadyCallback(JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut,
                                uint32_t OutDataLength) {
  /* Update JPEG encoder output buffer address*/
  g_outputBuffer += OutDataLength;

  HAL_JPEG_ConfigOutputBuffer(hjpeg, g_outputBuffer, CHUNK_SIZE_OUT);
}

void HAL_JPEG_InfoReadyCallback(JPEG_HandleTypeDef *hjpeg,
                                JPEG_ConfTypeDef *pInfo) {}

void HAL_JPEG_ErrorCallback(JPEG_HandleTypeDef *hjpeg) {
  Jpeg_HWDecodingError = 1;
}

void HAL_JPEG_DecodeCpltCallback(JPEG_HandleTypeDef *hjpeg) {
  Jpeg_HWDecodingEnd = 1;
}

void jpeg_init(void) {
  JPEG_Handle.Instance = JPEG;
  HAL_JPEG_Init(&JPEG_Handle);
}

void jpeg_decode_init(uint32_t address) {
  Jpeg_HWDecodingEnd = 0;
  Jpeg_HWDecodingError = 0;
  g_outputBuffer = (uint8_t *)address;
}

int jpeg_decode_start(const char *path) {
  if (jpeg_decode_file_open(path) != 0) {
    return -1;
  }
  uint32_t file_size = 0;
  if (jpeg_decode_file_size(&file_size) != 0) {
    jpeg_decode_file_close();
    return -1;
  }
  if (file_size > MAX_JPEG_SIZE || file_size == 0) {
    jpeg_decode_file_close();
    return -1;
  }

  g_inputJpegBuffer = lodepng_malloc(file_size);
  g_inputJpegOffset = 0;
  uint32_t state = disable_irq();
  if (jpeg_decode_file_read(g_inputJpegBuffer, file_size, &g_inputJpegSize) ==
      0) {
    enable_irq(state);
  } else {
    enable_irq(state);
    lodepng_free(g_inputJpegBuffer);
    return -1;
  }
  if (g_inputJpegSize != file_size) {
    lodepng_free(g_inputJpegBuffer);
    return -1;
  }
  /* Start JPEG decoding with DMA method */
  HAL_JPEG_Decode_DMA(&JPEG_Handle, g_inputJpegBuffer, g_inputJpegSize,
                      g_outputBuffer, CHUNK_SIZE_OUT);

  uint32_t time_started = HAL_GetTick();
  while ((jpeg_get_decode_state() == 0) && (jpeg_get_decode_error() == 0)) {
    if (HAL_GetTick() - time_started > 500) Jpeg_HWDecodingError = 1;
  }
  lodepng_free(g_inputJpegBuffer);
  if (Jpeg_HWDecodingError) {
    return -2;
  }
  return 0;
}

void jpeg_decode_info(uint32_t *width, uint32_t *height,
                      uint32_t *subsampling) {
  HAL_JPEG_GetInfo(&JPEG_Handle, &JPEG_Info);
  *width = JPEG_Info.ImageWidth;
  *height = JPEG_Info.ImageHeight;
  *subsampling = JPEG_Info.ChromaSubsampling;
}

#include "mipi_lcd.h"
#include "sdram.h"

int jped_decode(char *path, uint32_t address) {
  jpeg_decode_init(address);
  jpeg_decode_file_operation(JPEG_FILE_FATFS);

  if (jpeg_decode_start(path) == 0) {
    HAL_JPEG_GetInfo(&JPEG_Handle, &JPEG_Info);

    dma2d_copy_ycbcr_to_rgb((uint32_t *)address, (uint32_t *)lcd_get_src_addr(),
                            JPEG_Info.ImageWidth, JPEG_Info.ImageHeight,
                            JPEG_Info.ChromaSubsampling);
    return 0;
  }
  return -1;
}
