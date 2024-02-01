#include "GD25Q64E.h"
#include STM32_HAL_H

#include "debug_utils.h"

#pragma GCC optimize("Og")

#define ExecuteCheck_ADV(func_call, expected_result, on_false) \
  {                                                            \
    int ret = (func_call);                                     \
    if (ret != (expected_result)) {                            \
      on_false                                                 \
    }                                                          \
  }

#define ExecuteCheck_QSPIFlash_ERROR_NONE(func_call) \
  ExecuteCheck_ADV(func_call, QSPIFlash_ERROR_NONE, { return ret; })

static QSPI_HandleTypeDef* hqspi;
static bool is_quad_enabled = false;

void GD25Q64E_GetFlashInfo(QSPIFlahs_Info_t* pInfo) {
  pInfo->FullID.raw = 0x1740c8;
  pInfo->JEDECID.raw = 0x16C8;
  pInfo->PageSize = GD25Q64E_PAGE_SIZE;
  pInfo->PagesCount = (GD25Q64E_FLASH_SIZE / pInfo->PageSize);
  pInfo->SectorSize = GD25Q64E_SECTOR_SIZE;
  pInfo->SectorCount = (GD25Q64E_FLASH_SIZE / pInfo->SectorSize);
  pInfo->BlockSize = GD25Q64E_BLOCK_SIZE;
  pInfo->BlockCount = (GD25Q64E_FLASH_SIZE / pInfo->BlockSize);
  pInfo->FlashSize = GD25Q64E_FLASH_SIZE;
}

void GD25Q64E_SetupInterface(QSPIFlash_t* flash_interface) {
  hqspi = NULL;

  memset(flash_interface, 0x00, sizeof(QSPIFlash_t));

  flash_interface->Init = GD25Q64E_Init;
  flash_interface->Deinit = GD25Q64E_Deinit;

  flash_interface->Reset = GD25Q64E_Reset;
  flash_interface->ChipErase = GD25Q64E_ChipErase;
  flash_interface->Erase = GD25Q64E_Erase;
  flash_interface->Read = GD25Q64E_Read;
  flash_interface->Write = GD25Q64E_Write;

  flash_interface->MemMapCtrl = GD25Q64E_MemoryMapCtrl;
  flash_interface->PowerCtrl = GD25Q64E_MemoryMapCtrl;
}

QSPIFlash_ErrorCode_t GD25Q64E_Init(QSPI_HandleTypeDef* Ctx) {
  hqspi = Ctx;
  QSPIFlahs_Info_t finfo;
  QSPIFlahs_Full_ID_t fid;
  QSPIFlahs_JEDEC_ID_t jid;
  GD25Q64E_StatusReg_t srs = {0};

  GD25Q64E_GetFlashInfo(&finfo);

  // check id
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_ReadID(&fid));
  if (finfo.FullID.raw != fid.raw) return QSPIFlash_ERROR_COMMAND;

  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_ReadJEDECID(&jid));
  if (finfo.JEDECID.raw != jid.raw) return QSPIFlash_ERROR_COMMAND;

  // reset
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_Reset());

  // enable quad
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_ReadStatusRegister(&srs));
  is_quad_enabled = srs.SR2.val.QE;
  if (!is_quad_enabled) {
    ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_QuadEnableCtrl(true));
    ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_ReadStatusRegister(&srs));
    is_quad_enabled = srs.SR2.val.QE;
  }

  // for debug
  // is_quad_enabled = false;

  return QSPIFlash_ERROR_NONE;
}

QSPIFlash_ErrorCode_t GD25Q64E_Deinit(void) {
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_Reset());

  // update QE status
  is_quad_enabled = false;

  hqspi = NULL;
  return QSPIFlash_ERROR_NONE;
}

QSPIFlash_ErrorCode_t GD25Q64E_Reset(void) {
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_ResetEnable());
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_ResetMemory());

  HAL_Delay(50);

  return QSPIFlash_ERROR_NONE;
}

QSPIFlash_ErrorCode_t GD25Q64E_Erase(uint32_t address, uint32_t len) {
  uint32_t erase_size = len;

  const uint32_t erase_size_sector_4k = 4 * 1024;
  const uint32_t erase_size_32k = 32 * 1024;
  const uint32_t erase_size_64k = 64 * 1024;

  if (len % erase_size_sector_4k != 0)
    return QSPIFlash_ERROR_USAGE;  // not aligned to minimal erase size;

  while (erase_size > 0) {
    if (erase_size >= erase_size_64k) {
      ExecuteCheck_QSPIFlash_ERROR_NONE(
          GD25Q64E_Erase_Internal(address, GD25Q64E_ERASE_64K));
      erase_size -= erase_size_64k;
      continue;
    } else if (erase_size >= erase_size_32k) {
      ExecuteCheck_QSPIFlash_ERROR_NONE(
          GD25Q64E_Erase_Internal(address, GD25Q64E_ERASE_32K));
      erase_size -= erase_size_32k;
      continue;
    } else if (erase_size >= erase_size_sector_4k) {
      ExecuteCheck_QSPIFlash_ERROR_NONE(
          GD25Q64E_Erase_Internal(address, GD25Q64E_ERASE_SECTOR));
      erase_size -= erase_size_sector_4k;
      continue;
    }
  }

  return QSPIFlash_ERROR_NONE;
}

QSPIFlash_ErrorCode_t GD25Q64E_Read(uint32_t address, uint8_t* buffer,
                                    uint32_t len) {
  // GD25Q64E_Read_t read_mode = is_quad_enabled ? GD25Q64E_READ_1A4D_FAST :
  // GD25Q64E_READ_1A1D_FAST;
  GD25Q64E_Read_t read_mode =
      is_quad_enabled ? GD25Q64E_READ_1A4D_FAST : GD25Q64E_READ_1A1D;

  ExecuteCheck_QSPIFlash_ERROR_NONE(
      GD25Q64E_Read_Internal(address, read_mode, buffer, len));

  return QSPIFlash_ERROR_NONE;
}

QSPIFlash_ErrorCode_t GD25Q64E_Write(uint32_t address, uint8_t* buffer,
                                     uint32_t len) {
  GD25Q64E_Write_t write_mode =
      is_quad_enabled ? GD25Q64E_WRITE_1A4D : GD25Q64E_WRITE_1A1D;

  uint32_t write_size = len;
  uint16_t batch_size;  // 256 do not fit in uint8_t
  while (write_size > 0) {
    batch_size = MIN(256, write_size);
    ExecuteCheck_QSPIFlash_ERROR_NONE(
        GD25Q64E_Write_Internal(address + (len - write_size), write_mode,
                                buffer + (len - write_size), batch_size));
    write_size -= batch_size;
  }

  return QSPIFlash_ERROR_NONE;
}

// ==================================
// Internals

// SR

QSPIFlash_ErrorCode_t GD25Q64E_ReadStatusRegister(GD25Q64E_StatusReg_t* srs) {
  QSPI_CommandTypeDef s_command = {0};

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DummyCycles = 0;
  s_command.DataMode = QSPI_DATA_1_LINE;
  s_command.NbData = 1;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  // SR1
  s_command.Instruction = GD25Q64E_CMD_READ_STATUS_REG_1;
  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  if (HAL_QSPI_Receive(hqspi, &(srs->SR1.raw),
                       HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPIFlash_ERROR_RECEIVE;
  }

  // SR2
  s_command.Instruction = GD25Q64E_CMD_READ_STATUS_REG_2;
  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  if (HAL_QSPI_Receive(hqspi, &(srs->SR2.raw),
                       HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPIFlash_ERROR_RECEIVE;
  }

  // SR3
  s_command.Instruction = GD25Q64E_CMD_READ_STATUS_REG_3;
  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  if (HAL_QSPI_Receive(hqspi, &(srs->SR3.raw),
                       HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPIFlash_ERROR_RECEIVE;
  }

  return QSPIFlash_ERROR_NONE;
}

QSPIFlash_ErrorCode_t GD25Q64E_WriteStatusRegister(GD25Q64E_StatusReg_t* srs,
                                                   bool write_nv) {
  QSPI_CommandTypeDef s_command = {0};

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_1_LINE;
  s_command.DummyCycles = 0;
  s_command.NbData = 1;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_WriteEnable());

  // SR1
  if (write_nv) {
    ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_WriteEnableStatusRegisterNV());
  }
  s_command.Instruction = GD25Q64E_CMD_WRITE_STATUS_REG_1;
  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }
  if (HAL_QSPI_Transmit(hqspi, &(srs->SR1.raw),
                        HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPIFlash_ERROR_TRANSMIT;
  }
  // wait bit unset
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_AutoPollStatusRegister(
      GD25Q64E_SR_WIP, false, HAL_QPSI_TIMEOUT_DEFAULT_VALUE));

  // SR2
  if (write_nv) {
    ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_WriteEnableStatusRegisterNV());
  }
  s_command.Instruction = GD25Q64E_CMD_WRITE_STATUS_REG_2;
  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }
  if (HAL_QSPI_Transmit(hqspi, &(srs->SR2.raw),
                        HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPIFlash_ERROR_TRANSMIT;
  }
  // wait bit unset
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_AutoPollStatusRegister(
      GD25Q64E_SR_WIP, false, HAL_QPSI_TIMEOUT_DEFAULT_VALUE));

  // SR3
  if (write_nv) {
    ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_WriteEnableStatusRegisterNV());
  }
  s_command.Instruction = GD25Q64E_CMD_WRITE_STATUS_REG_3;
  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }
  if (HAL_QSPI_Transmit(hqspi, &(srs->SR3.raw),
                        HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPIFlash_ERROR_TRANSMIT;
  }
  // wait bit unset
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_AutoPollStatusRegister(
      GD25Q64E_SR_WIP, false, HAL_QPSI_TIMEOUT_DEFAULT_VALUE));

  return QSPIFlash_ERROR_NONE;
}

QSPIFlash_ErrorCode_t GD25Q64E_AutoPollStatusRegister(uint8_t bit_num,
                                                      bool set_unset,
                                                      uint32_t timeout) {
  GD25Q64E_StatusReg_t srs = {0};
  uint32_t tick = HAL_GetTick();

  while (true) {
    if ((HAL_GetTick() - tick) > timeout) return QSPIFlash_ERROR_AUTOPOLLING;

    ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_ReadStatusRegister(&srs));

    if (set_unset) {
      // wait for set
      if ((srs.raw & (1 << bit_num)) == (1 << bit_num)) break;
    } else {
      // wait for unset
      if ((srs.raw | (~(1 << bit_num))) == (~(1 << bit_num))) break;
    }

    HAL_Delay(10);
  }

  return QSPIFlash_ERROR_NONE;
}

QSPIFlash_ErrorCode_t GD25Q64E_WriteEnableStatusRegisterNV() {
  QSPI_CommandTypeDef s_command = {0};

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = GD25Q64E_CMD_NV_SR_WRITE_ENABLE;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_NONE;
  s_command.DummyCycles = 0;
  s_command.NbData = 0;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  return QSPIFlash_ERROR_NONE;
}

// Reset

QSPIFlash_ErrorCode_t GD25Q64E_ResetEnable() {
  QSPI_CommandTypeDef s_command = {0};

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = GD25Q64E_CMD_RESET_ENABLE;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_NONE;
  s_command.DummyCycles = 0;
  s_command.NbData = 0;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  return QSPIFlash_ERROR_NONE;
}

QSPIFlash_ErrorCode_t GD25Q64E_ResetMemory() {
  QSPI_CommandTypeDef s_command = {0};

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = GD25Q64E_CMD_RESET;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_NONE;
  s_command.DummyCycles = 0;
  s_command.NbData = 0;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  return QSPIFlash_ERROR_NONE;
}

// WE

QSPIFlash_ErrorCode_t GD25Q64E_WriteEnable() {
  QSPI_CommandTypeDef s_command = {0};

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = GD25Q64E_CMD_WRITE_ENABLE;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_NONE;
  s_command.DummyCycles = 0;
  s_command.NbData = 0;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  // wait bit set
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_AutoPollStatusRegister(
      GD25Q64E_SR_WEL, true, HAL_QPSI_TIMEOUT_DEFAULT_VALUE));

  return QSPIFlash_ERROR_NONE;
}

QSPIFlash_ErrorCode_t GD25Q64E_WriteDisable() {
  QSPI_CommandTypeDef s_command = {0};
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = GD25Q64E_CMD_WRITE_DISABLE;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_NONE;
  s_command.DummyCycles = 0;
  s_command.NbData = 0;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  // wait bit unset
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_AutoPollStatusRegister(
      GD25Q64E_SR_WEL, false, HAL_QPSI_TIMEOUT_DEFAULT_VALUE));

  return QSPIFlash_ERROR_NONE;
}

// Controls

QSPIFlash_ErrorCode_t GD25Q64E_QuadEnableCtrl(bool quad) {
  GD25Q64E_StatusReg_t srs = {0};

  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_ReadStatusRegister(&srs));
  srs.raw |= (1 << GD25Q64E_SR_QE);
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_WriteStatusRegister(&srs, true));

  return QSPIFlash_ERROR_NONE;
}

QSPIFlash_ErrorCode_t GD25Q64E_ProgEraseSuspendCtrl(bool suspend) {
  QSPI_CommandTypeDef s_command = {0};

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = (suspend ? GD25Q64E_CMD_PROG_ERASE_SUSPEND
                                   : GD25Q64E_CMD_PROG_ERASE_RESUME);
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_NONE;
  s_command.DummyCycles = 0;
  s_command.NbData = 0;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  return QSPIFlash_ERROR_NONE;
}

QSPIFlash_ErrorCode_t GD25Q64E_DeepPowerDownCtrl(bool power_down) {
  QSPI_CommandTypeDef s_command = {0};

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction =
      (power_down ? GD25Q64E_CMD_ENTER_DEEP_POWER_DOWN
                  : GD25Q64E_CMD_RELEASE_FROM_DEEP_POWER_DOWN);
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_NONE;
  s_command.DummyCycles = 0;
  s_command.NbData = 0;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  return QSPIFlash_ERROR_NONE;
}

QSPIFlash_ErrorCode_t GD25Q64E_MemoryMapCtrl(bool memory_map) {
  // TODO: impl me!

  // QSPI_CommandTypeDef s_command = {0};
  // QSPI_MemoryMappedTypeDef s_mem_mapped_cfg;
  // switch ( interface_mode )
  // {
  // case GD25Q64E_SPI_MODE: /* 1-1-1 read commands */
  //     s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  //     s_command.Instruction = GD25Q64E_FAST_READ_4_BYTE_ADDR_CMD;
  //     s_command.AddressMode = QSPI_ADDRESS_1_LINE;
  //     s_command.DataMode = QSPI_DATA_1_LINE;

  //     break;
  // case GD25Q64E_SPI_2IO_MODE: /* 1-2-2 read commands */

  //     s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  //     s_command.Instruction = GD25Q64E_DUAL_INOUT_FAST_READ_4_BYTE_ADDR_CMD;
  //     s_command.AddressMode = QSPI_ADDRESS_2_LINES;
  //     s_command.DataMode = QSPI_DATA_2_LINES;

  //     break;

  // case GD25Q64E_SPI_4IO_MODE: /* 1-4-4 read commands */

  //     s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  //     s_command.Instruction = GD25Q64E_QUAD_INOUT_FAST_READ_4_BYTE_ADDR_CMD;
  //     s_command.AddressMode = QSPI_ADDRESS_4_LINES;
  //     s_command.DataMode = QSPI_DATA_4_LINES;

  //     break;

  // case GD25Q64E_QPI_MODE: /* 4-4-4 commands */
  //     s_command.InstructionMode = QSPI_INSTRUCTION_4_LINES;
  //     s_command.Instruction = GD25Q64E_QUAD_INOUT_FAST_READ_CMD;
  //     s_command.AddressMode = QSPI_ADDRESS_4_LINES;
  //     s_command.DataMode = QSPI_DATA_4_LINES;

  //     break;
  // }
  // /* Configure th>e command for the read instruction */
  // s_command.DummyCycles = GD25Q64E_DUMMY_CYCLES_READ;
  // s_command.AddressSize = QSPI_ADDRESS_32_BITS;
  // s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  // s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  // s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  // s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  // /* Configure the memory mapped mode */
  // s_mem_mapped_cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
  // s_mem_mapped_cfg.TimeOutPeriod = 0;

  // if ( HAL_QSPI_MemoryMapped(hqspi, &s_command, &s_mem_mapped_cfg) != HAL_OK
  // )
  // {
  //     return QSPIFlash_ERROR_MEMORYMAPPED;
  // }

  return QSPIFlash_ERROR_NONE;
}

// IDs

QSPIFlash_ErrorCode_t GD25Q64E_ReadJEDECID(QSPIFlahs_JEDEC_ID_t* IDs) {
  QSPI_CommandTypeDef s_command = {0};

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = GD25Q64E_CMD_READ_DEVICE_ID;
  s_command.AddressMode = QSPI_ADDRESS_1_LINE;
  s_command.AddressSize = QSPI_ADDRESS_24_BITS;
  s_command.Address = 0;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DummyCycles = 0;
  s_command.DataMode = QSPI_DATA_1_LINE;
  s_command.NbData = 2;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  if (HAL_QSPI_Receive(hqspi, (uint8_t*)IDs, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_RECEIVE;
  }

  return QSPIFlash_ERROR_NONE;
}

QSPIFlash_ErrorCode_t GD25Q64E_ReadID(QSPIFlahs_Full_ID_t* IDs) {
  QSPI_CommandTypeDef s_command = {0};

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = GD25Q64E_CMD_READ_ID;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DummyCycles = 0;
  s_command.DataMode = QSPI_DATA_1_LINE;
  s_command.NbData = 3;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  if (HAL_QSPI_Receive(hqspi, (uint8_t*)IDs, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_RECEIVE;
  }

  return QSPIFlash_ERROR_NONE;
}

QSPIFlash_ErrorCode_t GD25Q64E_ReadUinqueID(uint8_t* UID) {
  QSPI_CommandTypeDef s_command = {0};

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = GD25Q64E_CMD_READ_UID;
  s_command.AddressMode = QSPI_ADDRESS_1_LINE;
  s_command.AddressSize = QSPI_ADDRESS_24_BITS;
  s_command.Address = 0;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DummyCycles = 0;
  s_command.DataMode = QSPI_DATA_1_LINE;
  s_command.NbData = 16;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  if (HAL_QSPI_Receive(hqspi, (uint8_t*)UID, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_RECEIVE;
  }

  return QSPIFlash_ERROR_NONE;
}

// Erase

QSPIFlash_ErrorCode_t GD25Q64E_ChipErase() {
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_WriteEnable());

  QSPI_CommandTypeDef s_command = {0};
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = GD25Q64E_CMD_ERASE_CHIP;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_NONE;
  s_command.DummyCycles = 0;
  s_command.NbData = 0;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  // wait bit unset
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_AutoPollStatusRegister(
      GD25Q64E_SR_WIP, false, HAL_QPSI_TIMEOUT_DEFAULT_VALUE));

  return QSPIFlash_ERROR_NONE;
}

QSPIFlash_ErrorCode_t GD25Q64E_Erase_Internal(uint32_t address,
                                              GD25Q64E_Erase_t mode) {
  QSPI_CommandTypeDef s_command = {0};

  switch (mode) {
    case GD25Q64E_ERASE_SECTOR:
      s_command.Instruction = GD25Q64E_CMD_SECTOR_ERASE;
      break;

    case GD25Q64E_ERASE_32K:
      s_command.Instruction = GD25Q64E_CMD_ERASE_32K;
      break;

    case GD25Q64E_ERASE_64K:
      s_command.Instruction = GD25Q64E_CMD_ERASE_64K;
      break;
    default:
      return QSPIFlash_ERROR_COMMAND;
  }

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.AddressMode = QSPI_ADDRESS_1_LINE;
  s_command.AddressSize = QSPI_ADDRESS_24_BITS;
  s_command.Address = address;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_NONE;
  s_command.DummyCycles = 0;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  // enable write
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_WriteEnable());

  // erase
  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  // wait bit unset
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_AutoPollStatusRegister(
      GD25Q64E_SR_WIP, false, HAL_QPSI_TIMEOUT_DEFAULT_VALUE));

  return QSPIFlash_ERROR_NONE;
}

// Read

QSPIFlash_ErrorCode_t GD25Q64E_Read_Internal(uint32_t address,
                                             GD25Q64E_Read_t mode,
                                             uint8_t* buffer, uint32_t len) {
  QSPI_CommandTypeDef s_command = {0};

  switch (mode) {
    case GD25Q64E_READ_1A1D:
      s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      s_command.Instruction = GD25Q64E_CMD_READ;
      s_command.AddressMode = QSPI_ADDRESS_1_LINE;
      s_command.DataMode = QSPI_DATA_1_LINE;
      s_command.DummyCycles = 0;
      s_command.SIOOMode = QSPI_SIOO_INST_ONLY_FIRST_CMD;
      break;
    case GD25Q64E_READ_1A1D_FAST:
      s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      s_command.Instruction = GD25Q64E_CMD_FAST_READ;
      s_command.AddressMode = QSPI_ADDRESS_1_LINE;
      s_command.DataMode = QSPI_DATA_1_LINE;
      s_command.DummyCycles = 8;
      s_command.SIOOMode = QSPI_SIOO_INST_ONLY_FIRST_CMD;
      break;
    case GD25Q64E_READ_1A2D_FAST:
      s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      s_command.Instruction = GD25Q64E_CMD_DUAL_OUT_FAST_READ;
      s_command.AddressMode = QSPI_ADDRESS_1_LINE;
      s_command.DataMode = QSPI_DATA_2_LINES;
      s_command.DummyCycles = 8;
      s_command.SIOOMode = QSPI_SIOO_INST_ONLY_FIRST_CMD;
      break;
    case GD25Q64E_READ_1A4D_FAST:
      s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      s_command.Instruction = GD25Q64E_CMD_QUAD_OUT_FAST_READ;
      s_command.AddressMode = QSPI_ADDRESS_1_LINE;
      s_command.DataMode = QSPI_DATA_4_LINES;
      s_command.DummyCycles = 8;
      s_command.SIOOMode = QSPI_SIOO_INST_ONLY_FIRST_CMD;
      break;
    case GD25Q64E_READ_2A2D_FAST:
      s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      s_command.Instruction = GD25Q64E_CMD_DUAL_INOUT_FAST_READ;
      s_command.AddressMode = QSPI_ADDRESS_2_LINES;
      s_command.DataMode = QSPI_DATA_2_LINES;
      s_command.DummyCycles = GD25Q64E_DUMMY_CYCLES_READ_2A2D_FAST;
      s_command.SIOOMode = QSPI_SIOO_INST_ONLY_FIRST_CMD;
      s_command.AlternateBytes = GD25Q64E_CONTINUOUS_READ_MAGIC;
      s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
      s_command.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
      break;
    case GD25Q64E_READ_4A4D_FAST:
      s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      s_command.Instruction = GD25Q64E_CMD_QUAD_INOUT_FAST_READ;
      s_command.AddressMode = QSPI_ADDRESS_4_LINES;
      s_command.DataMode = QSPI_DATA_4_LINES;
      s_command.DummyCycles = GD25Q64E_DUMMY_CYCLES_READ_4A4D_FAST;
      s_command.SIOOMode = QSPI_SIOO_INST_ONLY_FIRST_CMD;
      s_command.AlternateBytes = GD25Q64E_CONTINUOUS_READ_MAGIC;
      s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
      s_command.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
      break;

    default:
      return QSPIFlash_ERROR_COMMAND;
  }

  s_command.Address = address;
  s_command.AddressSize = QSPI_ADDRESS_24_BITS;
  s_command.NbData = len;

  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  if (HAL_QSPI_Receive(hqspi, buffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_RECEIVE;
  }

  return QSPIFlash_ERROR_NONE;
}

// Write

QSPIFlash_ErrorCode_t GD25Q64E_Write_Internal(uint32_t address,
                                              GD25Q64E_Write_t mode,
                                              uint8_t* buffer, uint32_t len) {
  QSPI_CommandTypeDef s_command = {0};
  switch (mode) {
    case GD25Q64E_WRITE_1A1D:
      s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      s_command.Instruction = GD25Q64E_CMD_PAGE_PROG;
      s_command.AddressMode = QSPI_ADDRESS_1_LINE;
      s_command.DataMode = QSPI_DATA_1_LINE;
      break;
    case GD25Q64E_WRITE_1A4D:
      s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      s_command.Instruction = GD25Q64E_CMD_QUAD_IN_FAST_PROG;
      s_command.AddressMode = QSPI_ADDRESS_1_LINE;
      s_command.DataMode = QSPI_DATA_4_LINES;
      break;

    default:
      return QSPIFlash_ERROR_COMMAND;
  }

  s_command.Address = address;
  s_command.AddressSize = QSPI_ADDRESS_24_BITS;
  s_command.NbData = len;
  s_command.DummyCycles = 0;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  // enable write
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_WriteEnable());

  // write
  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  if (HAL_QSPI_Transmit(hqspi, buffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_TRANSMIT;
  }

  // wait bit unset
  ExecuteCheck_QSPIFlash_ERROR_NONE(GD25Q64E_AutoPollStatusRegister(
      GD25Q64E_SR_WIP, false, HAL_QPSI_TIMEOUT_DEFAULT_VALUE));
  return QSPIFlash_ERROR_NONE;
}

#ifdef WIP

/**
 * @brief  Read SECTOR PROTECTION Block register value.
 *         SPI; 1-0-1
 * @param  hqspi Component object pointer
 * @param  interface_mode Interface mode
 * @param  SPBRegister pointer to SPBRegister value
 * @retval QSPI memory status
 */
QSPIFlash_ErrorCode_t GD25Q64E_ReadSPBLockRegister(uint8_t* SPBRegister) {
  QSPI_CommandTypeDef s_command = {0};

  /* Initialize the reading of SPB lock register command */
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = GD25Q64E_READ_SECTOR_PROTECTION_CMD;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DummyCycles = GD25Q64E_DUMMY_CYCLES_CONFIG_CMD;
  s_command.DataMode = QSPI_DATA_1_LINE;
  s_command.NbData = 1;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  /* Configure the command */
  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(hqspi, SPBRegister, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return QSPIFlash_ERROR_RECEIVE;
  }

  return QSPIFlash_ERROR_NONE;
}

#endif