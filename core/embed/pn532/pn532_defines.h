#ifndef _PN532_DEFINES_
#define _PN532_DEFINES_

#include <stdint.h>
#include <stdbool.h>

// ---------------------- Defines ----------------------

// ---------------------- Interface ----------------------

// ---------------------- HAL ----------------------
typedef enum
{
    PN532_SPI_STATREAD = 0x02,
    PN532_SPI_DATAWRITE = 0x01,
    PN532_SPI_DATAREAD = 0x03,
    PN532_SPI_READY = 0x01,
} PN532_FRAME_TYPE_SPI;

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
#define PN532_FRAME_NORMAL_INFO_MAX_LENGTH 0x00ff + 7
// extended case, 65535 byte data with exended info frame
#define PN532_FRAME_EXTENDED_INFO_MAX_LENGTH 0xffff + 10

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

// Frame Container (since we have no vector in C)
// lifecycle: pack -> use -> discard(free)
typedef struct
{
    PN532_FRAME_TYPE type;
    uint16_t length;
    uint8_t data[];
} PN532_FRAME;

// ---------------------- Functions ----------------------
#define PN532_COMMAND_TIMEOUT 0xff

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

// Miscellaneous Functions
// Diagnose
typedef enum
{
    PN532_DIAG_CommunicationLineTest = 0x00,
    PN532_DIAG_ROMTest = 0x01,
    PN532_DIAG_RAMTest = 0x02,
    PN532_DIAG_PollingTestToTarget = 0x04,
    PN532_DIAG_EchoBackTest = 0x05,
    PN532_DIAG_AttentionRequestTest = 0x06,
    PN532_DIAG_SelfAntenaTest = 0x07
} PN532_DIAG;
#define PN532_DIAG_BUFF_SIZE 1 + 262 // NumTst, Max Param

// GetFirmwareVersion
#define PN532_FW_VER_BUFF_SIZE 1 + 1 + 1 + 1 // IC, VER, Rev, Support
typedef struct __attribute__((__packed__))
{
    uint8_t IC;
    uint8_t Ver;
    uint8_t Rev;
    uint8_t Support;
} PN532_FW_VER;

// GetGeneralStatus

// ReadRegister
// WriteRegister

// ReadGPIO
// WriteGPIO

// SetSerialBaudRate
// SetParameters

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

// RF communication Functions
// RFConfiguration
// RFRegulationTest

// Initiator Functions
// InJumpForDEP
// InJumpForPSL
// InListPassiveTarget
// + max buffer calculate
/*
InitiatorData:
106k_typeA 12 bytes
212k/424k 5 bytes
106k_typeB 2 bytes
106k_Jewel 0 bytes
TargetData:
106k_typeA unknown // ATS size????
212k/424k 21 bytes
106k_typeB unknown // ATTRIB_RES size???
106k_Jewel 7 bytes
*/
#define PN532_InListPassiveTarget_BUFF_SIZE      PN532_FRAME_MAX_LENGTH // use max since it can't be predicted
#define PN532_InListPassiveTarget_DATA_BUFF_SIZE 64                     // this is a guessed value!
typedef enum
{
    PN532_InListPassiveTarget_BrTy_106k_typeA = 0x00,
    PN532_InListPassiveTarget_BrTy_212k = 0x01,
    PN532_InListPassiveTarget_BrTy_424k = 0x02,
    PN532_InListPassiveTarget_BrTy_106k_typeB = 0x03,
    PN532_InListPassiveTarget_BrTy_106k_Jewel = 0x04
} PN532_InListPassiveTarget_BrTy;
typedef struct
{
    uint8_t MaxTg;
    PN532_InListPassiveTarget_BrTy BrTy;
    uint8_t InitiatorData_len;
    uint8_t InitiatorData[PN532_InListPassiveTarget_DATA_BUFF_SIZE];
} PN532_InListPassiveTarget_Params;
typedef struct
{
    uint8_t NbTg;
    PN532_InListPassiveTarget_BrTy BrTy;
    uint8_t TargetData_len;
    uint8_t TargetData[PN532_InListPassiveTarget_DATA_BUFF_SIZE];
} PN532_InListPassiveTarget_Results;
// InATR
// InPSL
// InDataExchange
#define PN532_InDataExchange_BUFF_SIZE 1 + 262 // Tg/Status + DataOut/DataIn
// InCommunicateThru
// InDeselect
// InRelease
// InSelect
// InAutoPoll

// Target Functions
// TgInitAsTarget
// TgSetGeneralBytes
// TgGetData
// TgSetData
// TgSetMetaData
// TgGetInitiatorCommand
// TgResponseToInitiator
// TgGetTargetStatus

// ---------------------- Misc ----------------------
// GPIO
#define PN532_GPIO_VALIDATIONBIT (0x80)
#define PN532_GPIO_P30           (0)
#define PN532_GPIO_P31           (1)
#define PN532_GPIO_P32           (2)
#define PN532_GPIO_P33           (3)
#define PN532_GPIO_P34           (4)
#define PN532_GPIO_P35           (5)

#endif //_PN532_DEFINES_