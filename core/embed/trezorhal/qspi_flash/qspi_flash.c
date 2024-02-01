#include <memory.h>
#include <stdbool.h>
#include <stdint.h>

#include STM32_HAL_H
#include "irq.h"
#include "qspi_flash.h"
#include "secbool.h"

#include "debug_utils.h"

#include "GD25Q64E.h"

#pragma GCC optimize("Og")

// #define QSPI_FLASH_SIZE 23
// #define QSPI_SECTOR_SIZE (64 * 1024)
// #define QSPI_PAGE_SIZE 256
// #define QSPI_END_ADDR (1 << QSPI_FLASH_SIZE)
// #define QSPI_FLASH_SIZES (8 * 1024 * 1024)

// macros
#define ExecuteCheck_ADV(func_call, expected_result, on_false) \
  {                                                            \
    int ret = (func_call);                                     \
    if (ret != (expected_result)) {                            \
      on_false                                                 \
    }                                                          \
  }

#define ExecuteCheck_QSPIFlash_ERROR_NONE_bool(func_call) \
  ExecuteCheck_ADV(func_call, QSPIFlash_ERROR_NONE, { return false; })
#define ExecuteCheck_bool(func_call) \
  ExecuteCheck_ADV(func_call, true, { return false; })

#define QSPI_FLASH_ENSURE_READY()                                   \
  ExecuteCheck_bool((is_qspi_initialized && (qspi_flash != NULL) && \
                     (qspi_flash_info != NULL)))
#define SPI_FLASH_MEM_MAP_WRAPPER(expr_block)                \
  {                                                          \
    bool was_memory_mapped = is_memory_mapped;               \
    if (was_memory_mapped) qspi_flash_set_memory_map(false); \
    expr_block;                                              \
    if (was_memory_mapped) qspi_flash_set_memory_map(true);  \
  }

// instance
QSPIFlash_t* qspi_flash = NULL;
QSPIFlahs_Info_t* qspi_flash_info = NULL;
// internal vars
static QSPI_HandleTypeDef QSPIHandle;
static QSPIFlash_t QSPIFlash;
static QSPIFlahs_Info_t QSPIFlashInfo;
static bool is_qspi_initialized = false;
static bool is_memory_mapped = false;

// functions

static void qspi_io_init() {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /**QUADSPI GPIO Configuration
  PF10     ------> QUADSPI_CLK
  PG6     ------> QUADSPI_BK1_NCS
  PF8     ------> QUADSPI_BK1_IO0
  PF9     ------> QUADSPI_BK1_IO1
  PF7     ------> QUADSPI_BK1_IO2
  PF6     ------> QUADSPI_BK1_IO3
  */

  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
}

static bool qspi_init() {
  __HAL_RCC_QSPI_FORCE_RESET();
  __HAL_RCC_QSPI_RELEASE_RESET();

  __HAL_RCC_QSPI_CLK_DISABLE();
  __HAL_RCC_QSPI_CLK_ENABLE();

  QSPIHandle = (QSPI_HandleTypeDef){0};

  QSPIHandle.Instance = QUADSPI;
  QSPIHandle.Init.ClockPrescaler = 1;
  QSPIHandle.Init.FifoThreshold = 4;
  // QSPIHandle.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
  QSPIHandle.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
  QSPIHandle.Init.FlashSize = POSITION_VAL(GD25Q64E_FLASH_SIZE);
  QSPIHandle.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_8_CYCLE;
  QSPIHandle.Init.ClockMode = QSPI_CLOCK_MODE_0;
  QSPIHandle.Init.FlashID = QSPI_FLASH_ID_1;
  QSPIHandle.Init.DualFlash = QSPI_DUALFLASH_DISABLE;

  HAL_QSPI_DeInit(&QSPIHandle);
  if (HAL_QSPI_Init(&QSPIHandle) != HAL_OK) {
    return false;
  }

  // NVIC_SetPriority (IRQn_Type IRQn, uint32_t priority)
  NVIC_SetPriority(QUADSPI_IRQn, IRQ_PRI_QSPI);

  // HAL_NVIC_SetPriority(QUADSPI_IRQn, 0x0F, 0);
  // HAL_NVIC_SetPriority(QUADSPI_IRQn, IRQ_PRI_QSPI, 0);

  HAL_NVIC_EnableIRQ(QUADSPI_IRQn);

  return true;
}

static bool qspi_deinit() {
  HAL_NVIC_DisableIRQ(QUADSPI_IRQn);

  __HAL_RCC_QSPI_FORCE_RESET();
  __HAL_RCC_QSPI_RELEASE_RESET();

  __HAL_RCC_QSPI_CLK_DISABLE();

  HAL_QSPI_DeInit(&QSPIHandle);
  QSPIHandle = (QSPI_HandleTypeDef){0};

  return true;
}

bool qspi_flash_init() {
  qspi_io_init();
  ExecuteCheck_bool(qspi_init());
  is_qspi_initialized = true;
  is_memory_mapped = false;
  qspi_flash = NULL;
  qspi_flash_info = NULL;

  return true;
}

bool qspi_flash_deinit() {
  ExecuteCheck_bool(qspi_deinit());
  is_qspi_initialized = false;
  is_memory_mapped = false;
  qspi_flash = NULL;
  qspi_flash_info = NULL;

  return true;
}

bool qspi_flash_probe(void) {
  ExecuteCheck_bool(is_qspi_initialized);

  qspi_flash = NULL;
  qspi_flash_info = NULL;

  // GD25Q64E
  GD25Q64E_SetupInterface(&QSPIFlash);
  if (QSPIFlash.Init(&QSPIHandle) == QSPIFlash_ERROR_NONE) {
    GD25Q64E_GetFlashInfo(&QSPIFlashInfo);
    qspi_flash_info = &QSPIFlashInfo;
    qspi_flash = &QSPIFlash;
    return true;
  }

  // GD25Q64C
  // GD25Q64C_SetupInterface(&QSPIFlash, &QSPIHandle);
  // W25Q64
  // W25Q64_SetupInterface(&QSPIFlash, &QSPIHandle);

  return false;
}

bool qspi_flash_test() {
  QSPI_FLASH_ENSURE_READY();

  uint8_t flash_data[4096] = {0};
  uint8_t test_data[4096] = {0};
  for (uint32_t i = 0; i < sizeof(test_data); i++) {
    test_data[i] = i;
  }

  ExecuteCheck_QSPIFlash_ERROR_NONE_bool(QSPIFlash.Erase(0, 4096));
  dbgprintf_Wait("Erase done!");
  ExecuteCheck_QSPIFlash_ERROR_NONE_bool(QSPIFlash.Write(0, test_data, 4096));
  dbgprintf_Wait("Write done!");
  ExecuteCheck_QSPIFlash_ERROR_NONE_bool(QSPIFlash.Read(0, flash_data, 4096));
  dbgprintf_Wait("Read done!");

  return (memcmp(flash_data, test_data, 4096) == 0);
}

bool qspi_flash_set_memory_map(bool mapped) {
  QSPI_FLASH_ENSURE_READY();
  ExecuteCheck_QSPIFlash_ERROR_NONE_bool(QSPIFlash.MemMapCtrl(mapped));
  is_memory_mapped = mapped;

  return true;
}

bool qspi_flash_erase_chip() {
  QSPI_FLASH_ENSURE_READY();
  SPI_FLASH_MEM_MAP_WRAPPER(
      { ExecuteCheck_QSPIFlash_ERROR_NONE_bool(QSPIFlash.ChipErase()); });

  return true;
}

bool qspi_flash_erase(uint32_t address, uint32_t len) {
  QSPI_FLASH_ENSURE_READY();
  SPI_FLASH_MEM_MAP_WRAPPER({
    ExecuteCheck_QSPIFlash_ERROR_NONE_bool(QSPIFlash.Erase(address, len));
  });

  return true;
}

bool qspi_flash_read_buffer(uint8_t* data, uint32_t address, uint32_t len) {
  QSPI_FLASH_ENSURE_READY();
  SPI_FLASH_MEM_MAP_WRAPPER({
    ExecuteCheck_QSPIFlash_ERROR_NONE_bool(QSPIFlash.Read(address, data, len));
  });

  return true;
}

bool qspi_flash_write_buffer(uint8_t* data, uint32_t address, uint32_t len) {
  QSPI_FLASH_ENSURE_READY();
  SPI_FLASH_MEM_MAP_WRAPPER({
    ExecuteCheck_QSPIFlash_ERROR_NONE_bool(QSPIFlash.Write(address, data, len));
  });

  return true;
}
