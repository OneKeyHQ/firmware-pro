#pragma once

#include <stdint.h>
#include <stdbool.h>

// ---------------------- Defines ----------------------

// ---------------------- Interface ----------------------

// ---------------------- HAL ----------------------

// ---------------------- Frame ----------------------
// Config
// #define PN532_SUPPORT_EXTENDED_INFO_FRAME

// Header Footer
#define PN532_PREAMBLE   (0x00)
#define PN532_STARTCODE1 (0x00)
#define PN532_STARTCODE2 (0xFF)
#define PN532_POSTAMBLE  (0x00)

// Direction
#define PN532_HOSTTOPN532 (0xD4)
#define PN532_PN532TOHOST (0xD5)

// Frame Length
// usual case, 255 byte data with normal info frame
#define PN532_FRAME_NORMAL_INFO_MAX_LENGTH (0x00ff + 7) // 262 bytes
// extended case, 65535 byte data with exended info frame
#define PN532_FRAME_EXTENDED_INFO_MAX_LENGTH (0xffff + 10)

#ifndef PN532_SUPPORT_EXTENDED_INFO_FRAME
  #define PN532_FRAME_MAX_LENGTH PN532_FRAME_NORMAL_INFO_MAX_LENGTH
#else
  #define PN532_FRAME_MAX_LENGTH PN532_FRAME_EXTENDED_INFO_MAX_LENGTH
#endif

typedef enum
{
    PN532_FRAME_NORMAL_INFO = 0,
    PN532_FRAME_EXTENDED_INFO,
    PN532_FRAME_ACK,
    PN532_FRAME_NACK,
    PN532_FRAME_ERROR,
} PN532_FRAME_TYPE;

// Frame Container - static allocation (no malloc in embedded HAL)
// Max data size = 265 bytes: the chip's real packet-data ceiling
// (UM0701-02 sec 7.4.7 Fig 109: TFI + CMD + Tg + 262-byte payload), reached
// only via extended info frames - a normal frame LEN caps packet data at 255.
#define PN532_FRAME_DATA_MAX 265

// Raw wire size of the largest frame either side can send: extended info
// frame overhead is 10 bytes (preamble, start code, FF FF, LENM/LENL, LCS,
// DCS, postamble) around the 265-byte packet data. The chip answers with an
// extended frame whenever the response exceeds a normal frame, regardless of
// what the host sent, so RX buffers must always be this size.
#define PN532_FRAME_RAW_MAX (PN532_FRAME_DATA_MAX + 10)

typedef enum
{
    PN532_SPI_STATREAD = 0x02,
    PN532_SPI_DATAWRITE = 0x01,
    PN532_SPI_DATAREAD = 0x03,
    PN532_SPI_READY = 0x01,
} PN532_FRAME_TYPE_SPI;

typedef enum
{
    PN532_COMMAND_DIAGNOSE = 0x00,
    PN532_COMMAND_GETFIRMWAREVERSION = 0x02,
    PN532_COMMAND_GETGENERALSTATUS = 0x04,
    PN532_COMMAND_READREGISTER = 0x06,
    PN532_COMMAND_WRITEREGISTER = 0x08,
    PN532_COMMAND_READGPIO = 0x0C,
    PN532_COMMAND_WRITEGPIO = 0x0E,
    PN532_COMMAND_SETSERIALBAUDRATE = 0x10,
    PN532_COMMAND_SETPARAMETERS = 0x12,
    PN532_COMMAND_SAMCONFIGURATION = 0x14,
    PN532_COMMAND_POWERDOWN = 0x16,

    PN532_COMMAND_RFCONFIGURATION = 0x32,
    PN532_COMMAND_RFREGULATIONTEST = 0x58,

    PN532_COMMAND_INJUMPFORDEP = 0x56,
    PN532_COMMAND_INJUMPFORPSL = 0x46,
    PN532_COMMAND_INLISTPASSIVETARGET = 0x4A,
    PN532_COMMAND_INATR = 0x50,
    PN532_COMMAND_INPSL = 0x4E,
    PN532_COMMAND_INDATAEXCHANGE = 0x40,
    PN532_COMMAND_INCOMMUNICATETHRU = 0x42,
    PN532_COMMAND_INDESELECT = 0x44,
    PN532_COMMAND_INRELEASE = 0x52,
    PN532_COMMAND_INSELECT = 0x54,
    PN532_COMMAND_INAUTOPOLL = 0x60,

    PN532_COMMAND_TGINITASTARGET = 0x8C,
    PN532_COMMAND_TGSETGENERALBYTES = 0x92,
    PN532_COMMAND_TGGETDATA = 0x86,
    PN532_COMMAND_TGSETDATA = 0x8E,
    PN532_COMMAND_TGSETMETADATA = 0x94,
    PN532_COMMAND_TGGETINITIATORCOMMAND = 0x88,
    PN532_COMMAND_TGRESPONSETOINITIATOR = 0x90,
    PN532_COMMAND_TGGETTARGETSTATUS = 0x8A,
} PN532_COMMAND;

typedef struct
{
    PN532_FRAME_TYPE type;
    uint16_t length;
    uint8_t data[PN532_FRAME_DATA_MAX];
} PN532_FRAME;

// ---------------------- Functions ----------------------
// Covers typical ISO-DEP operations including FWT extensions. 2000 ms gives
// the PN532 firmware room to complete anti-collision + ATS.
#define PN532_COMMAND_TIMEOUT 2000

// Miscellaneous Functions
// Diagnose

// GetFirmwareVersion

// GetGeneralStatus
#define PN532_GENERALSTATUS_MAX_TG 2

// ReadRegister
// WriteRegister

// ReadGPIO
// WriteGPIO

// SetSerialBaudRate
// SetParameters
#define PN532_PARAM_NAD_USED            (1U << 0)
#define PN532_PARAM_DID_USED            (1U << 1)
#define PN532_PARAM_AUTOMATIC_ATR_RES   (1U << 2)
#define PN532_PARAM_AUTOMATIC_RATS      (1U << 4)
#define PN532_PARAM_ISO14443_4_PICC     (1U << 5)
#define PN532_PARAM_REMOVE_PREPOSTAMBLE (1U << 6)

// SAMConfiguration
typedef enum
{
    PN532_SAM_Normal = 0x01,
    PN532_SAM_VirtualCard = 0x02,
    PN532_SAM_WiredCard = 0x03,
    PN532_SAM_DualCard = 0x04
} PN532_SAM_MODE;
#define PN532_SAM_CONFIG_BUFF_SIZE 1 + 1 + 1 // Mode, Timeout, IRQ

// PowerDown
#define PN532_POWERDOWN_WAKEUP_I2C      (1U << 7)
#define PN532_POWERDOWN_WAKEUP_GPIO     (1U << 6)
#define PN532_POWERDOWN_WAKEUP_SPI      (1U << 5)
#define PN532_POWERDOWN_WAKEUP_HSU      (1U << 4)
#define PN532_POWERDOWN_WAKEUP_RF_LEVEL (1U << 3)
#define PN532_POWERDOWN_WAKEUP_INT1     (1U << 1)
#define PN532_POWERDOWN_WAKEUP_INT0     (1U << 0)

// RF communication Functions
// RFConfiguration
#define PN532_RFConfiguration_BUFF_SIZE 1 + 11 // CfgItem type, largest CfgItem data
typedef enum
{
    PN532_RFConfiguration_CfgItem_RFField = 0x01,
    PN532_RFConfiguration_CfgItem_VariousTimings = 0x02,
    PN532_RFConfiguration_CfgItem_MaxRtyCOM = 0x04,
    PN532_RFConfiguration_CfgItem_MaxRetries = 0x05,
    PN532_RFConfiguration_CfgItem_AnalogSettings_106k_typeA = 0x0a,
    PN532_RFConfiguration_CfgItem_AnalogSettings_212k_424k = 0x0b,
    PN532_RFConfiguration_CfgItem_AnalogSettings_typeB = 0x0c,
    PN532_RFConfiguration_CfgItem_AnalogSettings_212k_424k_848k_iso4443dash4 = 0x0d,
} PN532_RFConfiguration_CfgItem;

// Initiator Functions
#define PN532_NFCID3_LEN 10
// InListPassiveTarget
#define PN532_InListPassiveTarget_BUFF_SIZE PN532_FRAME_MAX_LENGTH // use max since it can't be predicted
// Upper bound on per-target data from InListPassiveTarget: the longest is
// typeA (NFCID + SAK + ATS) which UM0701-02 section 7.3.5 caps around 32 B;
// 48 B leaves headroom for maximum ATS and forward compatibility.
#define PN532_InListPassiveTarget_DATA_BUFF_SIZE  48
#define PN532_InListPassiveTarget_BrTy_106k_typeA 0x00
typedef struct
{
    uint8_t MaxTg;
} PN532_InListPassiveTarget_Params;
typedef struct
{
    uint8_t NbTg;
    uint8_t TargetData_len;
    uint8_t TargetData[PN532_InListPassiveTarget_DATA_BUFF_SIZE];
} PN532_InListPassiveTarget_Results;
// InDataExchange
#define PN532_DATA_MAX_LEN             262
#define PN532_InDataExchange_BUFF_SIZE (1 + PN532_DATA_MAX_LEN) // Tg/Status + DataOut/DataIn

// ---------------------- Misc ----------------------
// GPIO
#define PN532_GPIO_VALIDATIONBIT (0x80)
#define PN532_GPIO_P30           (0)
#define PN532_GPIO_P31           (1)
#define PN532_GPIO_P32           (2)
#define PN532_GPIO_P33           (3)
#define PN532_GPIO_P34           (4)
#define PN532_GPIO_P35           (5)

// ---------------------- Registers ----------------------
// CIU register (long address form for Read/WriteRegister), used for the RF gain
// boost after activation.
#define PN532_REG_CIU_RFCfg 0x6316
