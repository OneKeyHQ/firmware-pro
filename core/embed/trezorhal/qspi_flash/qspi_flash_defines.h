#ifndef _QSPI_FLASH_DEFINES_
#define _QSPI_FLASH_DEFINES_

#include <stdbool.h>
#include <stdint.h>

#define JOIN_EXPR(a, b, c) a##_##b##_##c
// regex ->(JOIN_EXPR\((.*), (.*), (.*)\),)
// replace -> $1 // $2_$3_$4

typedef enum {
  JOIN_EXPR(QSPIFlash, Interface, 1I_1A_1D) =
      0U,                                     // QSPIFlash_Interface_1I_1A_1D
  JOIN_EXPR(QSPIFlash, Interface, 1I_1A_2D),  // QSPIFlash_Interface_1I_1A_2D
  JOIN_EXPR(QSPIFlash, Interface, 1I_1A_4D),  // QSPIFlash_Interface_1I_1A_4D
  JOIN_EXPR(QSPIFlash, Interface, 1I_2A_2D),  // QSPIFlash_Interface_1I_2A_2D
  JOIN_EXPR(QSPIFlash, Interface, 1I_4A_4D),  // QSPIFlash_Interface_1I_4A_4D
  JOIN_EXPR(QSPIFlash, Interface, 4I_4A_4D),  // QSPIFlash_Interface_4I_4A_4D
} QSPIFlash_Interface_t;

typedef enum {
  JOIN_EXPR(QSPIFlash, ERROR, NONE) = 0,        // QSPIFlash_ERROR_NONE
  JOIN_EXPR(QSPIFlash, ERROR, USAGE) = -1,      // QSPIFlash_ERROR_USAGE
  JOIN_EXPR(QSPIFlash, ERROR, COMMAND) = -10,   // QSPIFlash_ERROR_COMMAND
  JOIN_EXPR(QSPIFlash, ERROR, TRANSMIT) = -11,  // QSPIFlash_ERROR_TRANSMIT
  JOIN_EXPR(QSPIFlash, ERROR, RECEIVE) = -12,   // QSPIFlash_ERROR_RECEIVE
  JOIN_EXPR(QSPIFlash, ERROR, AUTOPOLLING) =
      -13,  // QSPIFlash_ERROR_AUTOPOLLING
            // JOIN_EXPR(QSPIFlash, ERROR, MEMORYMAPPED) = -14, //
            // QSPIFlash_ERROR_MEMORYMAPPED
} QSPIFlash_ErrorCode_t;

typedef union {
  uint16_t raw;
  struct __attribute__((packed)) {
    uint8_t MID;
    uint8_t DID;  // CID
  } ids;
} QSPIFlahs_JEDEC_ID_t;

typedef union __attribute__((packed)) {
  uint32_t raw : 24;
  struct __attribute__((packed)) {
    uint8_t MID;
    uint8_t TID;
    uint8_t CID;
  } ids;
} QSPIFlahs_Full_ID_t;

typedef struct {
  QSPIFlahs_Full_ID_t FullID;
  QSPIFlahs_JEDEC_ID_t JEDECID;
  uint32_t PageSize;
  uint32_t PagesCount;
  uint32_t SectorSize;
  uint32_t SectorCount;
  uint32_t BlockSize;
  uint32_t BlockCount;
  uint32_t FlashSize;
} QSPIFlahs_Info_t;

typedef struct {
  QSPIFlash_ErrorCode_t (*Init)(QSPI_HandleTypeDef* Ctx);
  QSPIFlash_ErrorCode_t (*Deinit)(void);
  QSPIFlash_ErrorCode_t (*Reset)(void);
  QSPIFlash_ErrorCode_t (*ChipErase)(void);
  QSPIFlash_ErrorCode_t (*Erase)(uint32_t address, uint32_t len);
  QSPIFlash_ErrorCode_t (*Read)(uint32_t address, uint8_t* buffer,
                                uint32_t len);
  QSPIFlash_ErrorCode_t (*Write)(uint32_t address, uint8_t* buffer,
                                 uint32_t len);
  QSPIFlash_ErrorCode_t (*MemMapCtrl)(bool);
  QSPIFlash_ErrorCode_t (*PowerCtrl)(bool);
} QSPIFlash_t;

#endif  //_QSPI_FLASH_DEFINES_