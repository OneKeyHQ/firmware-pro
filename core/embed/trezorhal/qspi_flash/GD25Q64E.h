#ifndef GD25Q64E_H
#define GD25Q64E_H

#include <memory.h>
#include <stdbool.h>
#include <stdint.h>

#include STM32_HAL_H

#include "qspi_flash_defines.h"

#define GD25Q64E_ENUM_ITEM(prefix, name) JOIN_EXPR(GD25Q64E, prefix, name)
// to build comments use following regex
// match -> (GD25Q64E_ENUM_ITEM\((.*), (.*)\),)
// output -> $1 // GD25Q64E_$2_$3

// size
#define GD25Q64E_PAGE_SIZE 256
#define GD25Q64E_SECTOR_SIZE 4096
#define GD25Q64E_BLOCK_SIZE 65536
#define GD25Q64E_FLASH_SIZE 0x8000000

#define GD25Q64E_DUMMY_CYCLES_CONFIG_BIT 0
#if GD25Q64E_DUMMY_CYCLES_CONFIG_BIT == 0
#define GD25Q64E_DUMMY_CYCLES_READ_2A2D_FAST 4U
#define GD25Q64E_DUMMY_CYCLES_READ_4A4D_FAST 6U
#else
#define GD25Q64E_DUMMY_CYCLES_READ_2A2D_FAST 8U
#define GD25Q64E_DUMMY_CYCLES_READ_4A4D_FAST 10U
#endif

#define GD25Q64E_CONTINUOUS_READ_MAGIC 0b00100000

// TODO: fix this
//  #define GD25Q64E_CHIP_ERASE_MAX_TIME      460000
//  #define GD25Q64E_SECTOR_ERASE_MAX_TIME    1000
//  #define GD25Q64E_SUBSECTOR_ERASE_MAX_TIME 400

typedef enum {
  GD25Q64E_ENUM_ITEM(SR, WIP) = 0U,     // GD25Q64E_SR_WIP
  GD25Q64E_ENUM_ITEM(SR, WEL),          // GD25Q64E_SR_WEL
  GD25Q64E_ENUM_ITEM(SR, BP0),          // GD25Q64E_SR_BP0
  GD25Q64E_ENUM_ITEM(SR, BP1),          // GD25Q64E_SR_BP1
  GD25Q64E_ENUM_ITEM(SR, BP2),          // GD25Q64E_SR_BP2
  GD25Q64E_ENUM_ITEM(SR, BP3),          // GD25Q64E_SR_BP3
  GD25Q64E_ENUM_ITEM(SR, BP4),          // GD25Q64E_SR_BP4
  GD25Q64E_ENUM_ITEM(SR, SRP0),         // GD25Q64E_SR_SRP0
  GD25Q64E_ENUM_ITEM(SR, SRP1),         // GD25Q64E_SR_SRP1
  GD25Q64E_ENUM_ITEM(SR, QE),           // GD25Q64E_SR_QE
  GD25Q64E_ENUM_ITEM(SR, SUS2),         // GD25Q64E_SR_SUS2
  GD25Q64E_ENUM_ITEM(SR, LB1),          // GD25Q64E_SR_LB1
  GD25Q64E_ENUM_ITEM(SR, LB2),          // GD25Q64E_SR_LB2
  GD25Q64E_ENUM_ITEM(SR, LB3),          // GD25Q64E_SR_LB3
  GD25Q64E_ENUM_ITEM(SR, CMP),          // GD25Q64E_SR_CMP
  GD25Q64E_ENUM_ITEM(SR, SUS1),         // GD25Q64E_SR_SUS1
  GD25Q64E_ENUM_ITEM(SR, DC),           // GD25Q64E_SR_DC
  GD25Q64E_ENUM_ITEM(SR, RESERVED_17),  // GD25Q64E_SR_RESERVED_17
  GD25Q64E_ENUM_ITEM(SR, RESERVED_18),  // GD25Q64E_SR_RESERVED_18
  GD25Q64E_ENUM_ITEM(SR, RESERVED_19),  // GD25Q64E_SR_RESERVED_19
  GD25Q64E_ENUM_ITEM(SR, RESERVED_20),  // GD25Q64E_SR_RESERVED_20
  GD25Q64E_ENUM_ITEM(SR, DRV0),         // GD25Q64E_SR_DRV0
  GD25Q64E_ENUM_ITEM(SR, DRV1),         // GD25Q64E_SR_DRV1
  GD25Q64E_ENUM_ITEM(SR, RESERVED_23),  // GD25Q64E_SR_RESERVED_23
} GD25Q64E_StatusRegBit_t;

typedef union {
  uint32_t raw : 24;
  struct __attribute__((packed)) {
    union {
      struct {
        uint8_t WIP : 1;
        uint8_t WEL : 1;
        uint8_t BP0 : 1;
        uint8_t BP1 : 1;
        uint8_t BP2 : 1;
        uint8_t BP3 : 1;
        uint8_t BP4 : 1;
        uint8_t SRP0 : 1;
      } val;
      uint8_t raw;
    } SR1;
    union {
      struct {
        uint8_t SRP1 : 1;
        uint8_t QE : 1;
        uint8_t SUS2 : 1;
        uint8_t LB1 : 1;
        uint8_t LB2 : 1;
        uint8_t LB3 : 1;
        uint8_t CMP : 1;
        uint8_t SUS1 : 1;
      } val;
      uint8_t raw;
    } SR2;
    union {
      struct {
        uint8_t DC : 1;
        uint8_t RESERVED_17 : 1;
        uint8_t RESERVED_18 : 1;
        uint8_t RESERVED_19 : 1;
        uint8_t RESERVED_20 : 1;
        uint8_t DRV0 : 1;
        uint8_t DRV1 : 1;
        uint8_t RESERVED_23 : 1;
      } val;
      uint8_t raw;
    } SR3;
  };
} GD25Q64E_StatusReg_t;

typedef enum {
  GD25Q64E_ENUM_ITEM(ERASE, SECTOR) = 0U,  // GD25Q64E_ERASE_SECTOR
  GD25Q64E_ENUM_ITEM(ERASE, 32K),          // GD25Q64E_ERASE_32K
  GD25Q64E_ENUM_ITEM(ERASE, 64K),          // GD25Q64E_ERASE_64K
} GD25Q64E_Erase_t;

typedef enum {
  GD25Q64E_ENUM_ITEM(READ, 1A1D) = 0U,  // GD25Q64E_READ_1A1D
  GD25Q64E_ENUM_ITEM(READ, 1A1D_FAST),  // GD25Q64E_READ_1A1D_FAST
  GD25Q64E_ENUM_ITEM(READ, 1A2D_FAST),  // GD25Q64E_READ_1A2D_FAST
  GD25Q64E_ENUM_ITEM(READ, 1A4D_FAST),  // GD25Q64E_READ_1A4D_FAST
  GD25Q64E_ENUM_ITEM(READ, 2A2D_FAST),  // GD25Q64E_READ_2A2D_FAST
  GD25Q64E_ENUM_ITEM(READ, 4A4D_FAST),  // GD25Q64E_READ_4A4D_FAST

} GD25Q64E_Read_t;

typedef enum {
  GD25Q64E_ENUM_ITEM(WRITE, 1A1D) = 0U,  // GD25Q64E_WRITE_1A1D
  GD25Q64E_ENUM_ITEM(WRITE, 1A4D),       // GD25Q64E_WRITE_1A4D
} GD25Q64E_Write_t;

typedef enum {
  GD25Q64E_ENUM_ITEM(CMD, DUMMY) = 0U,            // GD25Q64E_CMD_DUMMY
  GD25Q64E_ENUM_ITEM(CMD, WRITE_ENABLE) = 0x06,   // GD25Q64E_CMD_WRITE_ENABLE
  GD25Q64E_ENUM_ITEM(CMD, WRITE_DISABLE) = 0x04,  // GD25Q64E_CMD_WRITE_DISABLE
  GD25Q64E_ENUM_ITEM(CMD, READ_STATUS_REG_1) =
      0x05,  // GD25Q64E_CMD_READ_STATUS_REG_1
  GD25Q64E_ENUM_ITEM(CMD, READ_STATUS_REG_2) =
      0x35,  // GD25Q64E_CMD_READ_STATUS_REG_2
  GD25Q64E_ENUM_ITEM(CMD, READ_STATUS_REG_3) =
      0x15,  // GD25Q64E_CMD_READ_STATUS_REG_3
  GD25Q64E_ENUM_ITEM(CMD, WRITE_STATUS_REG_1) =
      0x01,  // GD25Q64E_CMD_WRITE_STATUS_REG_1
  GD25Q64E_ENUM_ITEM(CMD, WRITE_STATUS_REG_2) =
      0x31,  // GD25Q64E_CMD_WRITE_STATUS_REG_2
  GD25Q64E_ENUM_ITEM(CMD, WRITE_STATUS_REG_3) =
      0x11,  // GD25Q64E_CMD_WRITE_STATUS_REG_3
  GD25Q64E_ENUM_ITEM(CMD, NV_SR_WRITE_ENABLE) =
      0x50,                                   // GD25Q64E_CMD_NV_SR_WRITE_ENABLE
  GD25Q64E_ENUM_ITEM(CMD, READ) = 0x03,       // GD25Q64E_CMD_READ
  GD25Q64E_ENUM_ITEM(CMD, FAST_READ) = 0x0B,  // GD25Q64E_CMD_FAST_READ
  GD25Q64E_ENUM_ITEM(CMD, DUAL_OUT_FAST_READ) =
      0x3B,  // GD25Q64E_CMD_DUAL_OUT_FAST_READ
  GD25Q64E_ENUM_ITEM(CMD, QUAD_OUT_FAST_READ) =
      0x6B,  // GD25Q64E_CMD_QUAD_OUT_FAST_READ
  GD25Q64E_ENUM_ITEM(CMD, DUAL_INOUT_FAST_READ) =
      0xBB,  // GD25Q64E_CMD_DUAL_INOUT_FAST_READ
  GD25Q64E_ENUM_ITEM(CMD, QUAD_INOUT_FAST_READ) =
      0xEB,  // GD25Q64E_CMD_QUAD_INOUT_FAST_READ
  GD25Q64E_ENUM_ITEM(CMD, BURST_WITH_WRAP) =
      0x77,                                   // GD25Q64E_CMD_BURST_WITH_WRAP
  GD25Q64E_ENUM_ITEM(CMD, PAGE_PROG) = 0x02,  // GD25Q64E_CMD_PAGE_PROG
  GD25Q64E_ENUM_ITEM(CMD, QUAD_IN_FAST_PROG) =
      0x32,  // GD25Q64E_CMD_QUAD_IN_FAST_PROG
  GD25Q64E_ENUM_ITEM(CMD, SECTOR_ERASE) = 0x20,  // GD25Q64E_CMD_SECTOR_ERASE
  GD25Q64E_ENUM_ITEM(CMD, ERASE_32K) = 0x52,     // GD25Q64E_CMD_ERASE_32K
  GD25Q64E_ENUM_ITEM(CMD, ERASE_64K) = 0xD8,     // GD25Q64E_CMD_ERASE_64K
  GD25Q64E_ENUM_ITEM(CMD, ERASE_CHIP) = 0xC7,    // GD25Q64E_CMD_ERASE_CHIP
  GD25Q64E_ENUM_ITEM(CMD, ENTER_DEEP_POWER_DOWN) =
      0xB9,  // GD25Q64E_CMD_ENTER_DEEP_POWER_DOWN
  GD25Q64E_ENUM_ITEM(CMD, RELEASE_FROM_DEEP_POWER_DOWN) =
      0xAB,  // GD25Q64E_CMD_RELEASE_FROM_DEEP_POWER_DOWN
  GD25Q64E_ENUM_ITEM(CMD, READ_DEVICE_ID) =
      0x90,                                  // GD25Q64E_CMD_READ_DEVICE_ID
  GD25Q64E_ENUM_ITEM(CMD, READ_ID) = 0x9F,   // GD25Q64E_CMD_READ_ID
  GD25Q64E_ENUM_ITEM(CMD, READ_UID) = 0x4B,  // GD25Q64E_CMD_READ_UID
  GD25Q64E_ENUM_ITEM(CMD, PROG_ERASE_SUSPEND) =
      0x75,  // GD25Q64E_CMD_PROG_ERASE_SUSPEND
  GD25Q64E_ENUM_ITEM(CMD, PROG_ERASE_RESUME) =
      0x7A,  // GD25Q64E_CMD_PROG_ERASE_RESUME
  GD25Q64E_ENUM_ITEM(CMD, ERASE_SECURITY_REGS) =
      0x44,  // GD25Q64E_CMD_ERASE_SECURITY_REGS
  GD25Q64E_ENUM_ITEM(CMD, WRITE_SECURITY_REGS) =
      0x42,  // GD25Q64E_CMD_WRITE_SECURITY_REGS
  GD25Q64E_ENUM_ITEM(CMD, READ_SECURITY_REGS) =
      0x48,  // GD25Q64E_CMD_READ_SECURITY_REGS
  GD25Q64E_ENUM_ITEM(CMD, RESET_ENABLE) = 0x66,  // GD25Q64E_CMD_RESET_ENABLE
  GD25Q64E_ENUM_ITEM(CMD, RESET) = 0x99,         // GD25Q64E_CMD_RESET
  GD25Q64E_ENUM_ITEM(CMD, READ_SERIAL_FLASH_DISCO_PARAM) =
      0x5A,  // GD25Q64E_CMD_READ_SERIAL_FLASH_DISCO_PARAM

} GD25Q64E_Command_t;

void GD25Q64E_GetFlashInfo(QSPIFlahs_Info_t* pInfo);
void GD25Q64E_SetupInterface(QSPIFlash_t* flash_interface);

QSPIFlash_ErrorCode_t GD25Q64E_Init(QSPI_HandleTypeDef* Ctx);
QSPIFlash_ErrorCode_t GD25Q64E_Deinit(void);
QSPIFlash_ErrorCode_t GD25Q64E_Reset(void);
QSPIFlash_ErrorCode_t GD25Q64E_Test(void);
QSPIFlash_ErrorCode_t GD25Q64E_Erase(uint32_t address, uint32_t len);
QSPIFlash_ErrorCode_t GD25Q64E_Read(uint32_t address, uint8_t* buffer,
                                    uint32_t len);
QSPIFlash_ErrorCode_t GD25Q64E_Write(uint32_t address, uint8_t* buffer,
                                     uint32_t len);

QSPIFlash_ErrorCode_t GD25Q64E_ReadStatusRegister(GD25Q64E_StatusReg_t* srs);
QSPIFlash_ErrorCode_t GD25Q64E_WriteStatusRegister(GD25Q64E_StatusReg_t* srs,
                                                   bool write_nv);
QSPIFlash_ErrorCode_t GD25Q64E_AutoPollStatusRegister(uint8_t bit_num,
                                                      bool set_unset,
                                                      uint32_t timeout);
QSPIFlash_ErrorCode_t GD25Q64E_WriteEnableStatusRegisterNV();

QSPIFlash_ErrorCode_t GD25Q64E_ResetEnable();
QSPIFlash_ErrorCode_t GD25Q64E_ResetMemory();

QSPIFlash_ErrorCode_t GD25Q64E_WriteEnable();
QSPIFlash_ErrorCode_t GD25Q64E_WriteDisable();

QSPIFlash_ErrorCode_t GD25Q64E_QuadEnableCtrl(bool quad);
QSPIFlash_ErrorCode_t GD25Q64E_ProgEraseSuspendCtrl(bool suspend);
QSPIFlash_ErrorCode_t GD25Q64E_DeepPowerDownCtrl(bool power_down);
QSPIFlash_ErrorCode_t GD25Q64E_MemoryMapCtrl(bool memory_map);

QSPIFlash_ErrorCode_t GD25Q64E_ReadJEDECID(
    QSPIFlahs_JEDEC_ID_t* IDs);                                   // 2 bytes
QSPIFlash_ErrorCode_t GD25Q64E_ReadID(QSPIFlahs_Full_ID_t* IDs);  // 3 bytes
QSPIFlash_ErrorCode_t GD25Q64E_ReadUinqueID(uint8_t* UID);        // 16 bytes

QSPIFlash_ErrorCode_t GD25Q64E_ChipErase();
QSPIFlash_ErrorCode_t GD25Q64E_Erase_Internal(uint32_t address,
                                              GD25Q64E_Erase_t mode);

QSPIFlash_ErrorCode_t GD25Q64E_Read_Internal(uint32_t address,
                                             GD25Q64E_Read_t mode,
                                             uint8_t* buffer, uint32_t len);

QSPIFlash_ErrorCode_t GD25Q64E_Write_Internal(uint32_t address,
                                              GD25Q64E_Write_t mode,
                                              uint8_t* buffer, uint32_t len);

#endif /* __GD25Q64E_H */
