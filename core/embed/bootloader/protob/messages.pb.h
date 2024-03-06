/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.5 */

#ifndef PB_MESSAGES_PB_H_INCLUDED
#define PB_MESSAGES_PB_H_INCLUDED
#include <pb.h>

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Enum definitions */
typedef enum _MessageType { 
    MessageType_MessageType_Initialize = 0, 
    MessageType_MessageType_Ping = 1, 
    MessageType_MessageType_Success = 2, 
    MessageType_MessageType_Failure = 3, 
    MessageType_MessageType_WipeDevice = 5, 
    MessageType_MessageType_FirmwareErase = 6, 
    MessageType_MessageType_FirmwareUpload = 7, 
    MessageType_MessageType_FirmwareRequest = 8, 
    MessageType_MessageType_FirmwareErase_ex = 16, 
    MessageType_MessageType_Features = 17, 
    MessageType_MessageType_ButtonRequest = 26, 
    MessageType_MessageType_ButtonAck = 27, 
    MessageType_MessageType_GetFeatures = 55, 
    MessageType_MessageType_DeviceInfoSettings = 10001, 
    MessageType_MessageType_GetDeviceInfo = 10002, 
    MessageType_MessageType_DeviceInfo = 10003, 
    MessageType_MessageType_ReadSEPublicKey = 10004, 
    MessageType_MessageType_SEPublicKey = 10005, 
    MessageType_MessageType_WriteSEPublicCert = 10006, 
    MessageType_MessageType_ReadSEPublicCert = 10007, 
    MessageType_MessageType_SEPublicCert = 10008, 
    MessageType_MessageType_SESignMessage = 10012, 
    MessageType_MessageType_SEMessageSignature = 10013, 
    MessageType_MessageType_Reboot = 30000, 
    MessageType_MessageType_FirmwareUpdateEmmc = 30001, 
    MessageType_MessageType_EmmcFixPermission = 30100, 
    MessageType_MessageType_EmmcPath = 30101, 
    MessageType_MessageType_EmmcPathInfo = 30102, 
    MessageType_MessageType_EmmcFile = 30103, 
    MessageType_MessageType_EmmcFileRead = 30104, 
    MessageType_MessageType_EmmcFileWrite = 30105, 
    MessageType_MessageType_EmmcFileDelete = 30106, 
    MessageType_MessageType_EmmcDir = 30107, 
    MessageType_MessageType_EmmcDirList = 30108, 
    MessageType_MessageType_EmmcDirMake = 30109, 
    MessageType_MessageType_EmmcDirRemove = 30110 
} MessageType;

typedef enum _RebootType { 
    RebootType_Normal = 0, 
    RebootType_Boardloader = 1, 
    RebootType_BootLoader = 2 
} RebootType;

typedef enum _OneKeyDeviceType { 
    OneKeyDeviceType_CLASSIC = 0, 
    OneKeyDeviceType_CLASSIC1S = 1, 
    OneKeyDeviceType_MINI = 2, 
    OneKeyDeviceType_TOUCH = 3, 
    OneKeyDeviceType_TOUCH_PRO = 5 
} OneKeyDeviceType;

typedef enum _OneKeySeType { 
    OneKeySeType_THD89 = 0, 
    OneKeySeType_SE608A = 1 
} OneKeySeType;

typedef enum _FailureType { 
    FailureType_Failure_UnexpectedMessage = 1, 
    FailureType_Failure_DataError = 3, 
    FailureType_Failure_ActionCancelled = 4, 
    FailureType_Failure_ProcessError = 9 
} FailureType;

typedef enum _ButtonRequestType { 
    ButtonRequestType_ButtonRequest_Other = 1 
} ButtonRequestType;

/* Struct definitions */
typedef struct _ButtonAck { 
    char dummy_field;
} ButtonAck;

typedef struct _EmmcFixPermission { 
    char dummy_field;
} EmmcFixPermission;

typedef struct _GetDeviceInfo { 
    char dummy_field;
} GetDeviceInfo;

typedef struct _GetFeatures { 
    char dummy_field;
} GetFeatures;

typedef struct _Initialize { 
    char dummy_field;
} Initialize;

typedef struct _ReadSEPublicCert { 
    char dummy_field;
} ReadSEPublicCert;

typedef struct _ReadSEPublicKey { 
    char dummy_field;
} ReadSEPublicKey;

typedef struct _WipeDevice { 
    char dummy_field;
} WipeDevice;

typedef struct _ButtonRequest { 
    bool has_code;
    ButtonRequestType code; 
} ButtonRequest;

typedef PB_BYTES_ARRAY_T(32) DeviceInfo_NFT_voucher_t;
typedef struct _DeviceInfo { 
    bool has_serial_no;
    char serial_no[32]; 
    bool has_spiFlash_info;
    char spiFlash_info[16]; 
    bool has_SE_info;
    char SE_info[16]; 
    bool has_NFT_voucher;
    DeviceInfo_NFT_voucher_t NFT_voucher; 
    bool has_cpu_info;
    char cpu_info[16]; 
    bool has_pre_firmware;
    char pre_firmware[16]; 
} DeviceInfo;

typedef struct _DeviceInfoSettings { 
    bool has_serial_no;
    char serial_no[32]; 
    bool has_cpu_info;
    char cpu_info[16]; 
    bool has_pre_firmware;
    char pre_firmware[16]; 
} DeviceInfoSettings;

typedef struct _EmmcDir { 
    char path[256]; 
    pb_callback_t child_dirs; 
    pb_callback_t child_files; 
} EmmcDir;

typedef struct _EmmcDirList { 
    char path[256]; 
} EmmcDirList;

typedef struct _EmmcDirMake { 
    char path[256]; 
} EmmcDirMake;

typedef struct _EmmcDirRemove { 
    char path[256]; 
} EmmcDirRemove;

typedef struct _EmmcFile { 
    char path[256]; 
    uint32_t offset; 
    uint32_t len; 
    pb_callback_t data; 
    bool has_data_hash;
    uint32_t data_hash; 
    bool has_processed_byte;
    uint32_t processed_byte; 
} EmmcFile;

typedef struct _EmmcFileDelete { 
    char path[256]; 
} EmmcFileDelete;

typedef struct _EmmcPath { 
    bool exist; 
    uint64_t size; 
    uint32_t year; 
    uint32_t month; 
    uint32_t day; 
    uint32_t hour; 
    uint32_t minute; 
    uint32_t second; 
    bool readonly; 
    bool hidden; 
    bool system; 
    bool archive; 
    bool directory; 
} EmmcPath;

typedef struct _EmmcPathInfo { 
    char path[256]; 
} EmmcPathInfo;

typedef struct _Failure { 
    bool has_code;
    FailureType code; 
    bool has_message;
    char message[256]; 
} Failure;

typedef PB_BYTES_ARRAY_T(20) Features_revision_t;
typedef PB_BYTES_ARRAY_T(32) Features_onekey_board_hash_t;
typedef PB_BYTES_ARRAY_T(32) Features_onekey_boot_hash_t;
typedef PB_BYTES_ARRAY_T(32) Features_onekey_se_hash_t;
typedef PB_BYTES_ARRAY_T(32) Features_onekey_firmware_hash_t;
typedef struct _Features { 
    bool has_vendor;
    char vendor[33]; 
    uint32_t major_version; 
    uint32_t minor_version; 
    uint32_t patch_version; 
    bool has_bootloader_mode;
    bool bootloader_mode; 
    bool has_device_id;
    char device_id[25]; 
    bool has_language;
    char language[17]; 
    bool has_label;
    char label[33]; 
    bool has_initialized;
    bool initialized; 
    bool has_revision;
    Features_revision_t revision; 
    bool has_firmware_present;
    bool firmware_present; 
    bool has_model;
    char model[17]; 
    bool has_fw_major;
    uint32_t fw_major; 
    bool has_fw_minor;
    uint32_t fw_minor; 
    bool has_fw_patch;
    uint32_t fw_patch; 
    bool has_fw_vendor;
    char fw_vendor[256]; 
    bool has_offset;
    uint32_t offset; 
    bool has_ble_name;
    char ble_name[32]; 
    bool has_ble_ver;
    char ble_ver[16]; 
    bool has_ble_enable;
    bool ble_enable; 
    bool has_se_enable;
    bool se_enable; 
    bool has_se_ver;
    char se_ver[16]; 
    bool has_backup_only;
    bool backup_only; 
    bool has_onekey_version;
    char onekey_version[32]; 
    bool has_bootloader_version;
    char bootloader_version[8]; 
    bool has_serial_no;
    char serial_no[32]; 
    bool has_initstates;
    uint32_t initstates; 
    bool has_boardloader_version;
    char boardloader_version[32]; 
    bool has_onekey_device_type;
    OneKeyDeviceType onekey_device_type; 
    bool has_onekey_se_type;
    OneKeySeType onekey_se_type; 
    bool has_onekey_board_version;
    char onekey_board_version[32]; 
    bool has_onekey_board_hash;
    Features_onekey_board_hash_t onekey_board_hash; 
    bool has_onekey_boot_version;
    char onekey_boot_version[16]; 
    bool has_onekey_boot_hash;
    Features_onekey_boot_hash_t onekey_boot_hash; 
    bool has_onekey_se_version;
    char onekey_se_version[16]; 
    bool has_onekey_se_hash;
    Features_onekey_se_hash_t onekey_se_hash; 
    bool has_onekey_se_build_id;
    char onekey_se_build_id[8]; 
    bool has_onekey_firmware_version;
    char onekey_firmware_version[16]; 
    bool has_onekey_firmware_hash;
    Features_onekey_firmware_hash_t onekey_firmware_hash; 
    bool has_onekey_firmware_build_id;
    char onekey_firmware_build_id[64]; 
    bool has_onekey_serial_no;
    char onekey_serial_no[32]; 
    bool has_onekey_boot_build_id;
    char onekey_boot_build_id[8]; 
} Features;

typedef struct _FirmwareErase { 
    bool has_length;
    uint32_t length; 
} FirmwareErase;

typedef struct _FirmwareErase_ex { 
    bool has_length;
    uint32_t length; 
} FirmwareErase_ex;

typedef struct _FirmwareRequest { 
    bool has_offset;
    uint32_t offset; 
    bool has_length;
    uint32_t length; 
} FirmwareRequest;

typedef struct _FirmwareUpdateEmmc { 
    char path[256]; 
    bool has_reboot_on_success;
    bool reboot_on_success; 
} FirmwareUpdateEmmc;

typedef PB_BYTES_ARRAY_T(32) FirmwareUpload_hash_t;
typedef struct _FirmwareUpload { 
    pb_callback_t payload; 
    bool has_hash;
    FirmwareUpload_hash_t hash; 
} FirmwareUpload;

typedef struct _Ping { 
    bool has_message;
    char message[256]; 
} Ping;

typedef struct _Reboot { 
    RebootType reboot_type; 
} Reboot;

typedef PB_BYTES_ARRAY_T(64) SEMessageSignature_signature_t;
typedef struct _SEMessageSignature { 
    SEMessageSignature_signature_t signature; 
} SEMessageSignature;

typedef PB_BYTES_ARRAY_T(416) SEPublicCert_public_cert_t;
typedef struct _SEPublicCert { 
    SEPublicCert_public_cert_t public_cert; 
} SEPublicCert;

typedef PB_BYTES_ARRAY_T(64) SEPublicKey_public_key_t;
typedef struct _SEPublicKey { 
    SEPublicKey_public_key_t public_key; 
} SEPublicKey;

typedef PB_BYTES_ARRAY_T(1024) SESignMessage_message_t;
typedef struct _SESignMessage { 
    SESignMessage_message_t message; 
} SESignMessage;

typedef struct _Success { 
    bool has_message;
    char message[256]; 
} Success;

typedef PB_BYTES_ARRAY_T(416) WriteSEPublicCert_public_cert_t;
typedef struct _WriteSEPublicCert { 
    WriteSEPublicCert_public_cert_t public_cert; 
} WriteSEPublicCert;

typedef struct _EmmcFileRead { 
    EmmcFile file; 
    bool has_ui_percentage;
    uint32_t ui_percentage; 
} EmmcFileRead;

typedef struct _EmmcFileWrite { 
    EmmcFile file; 
    bool overwrite; 
    bool append; 
    bool has_ui_percentage;
    uint32_t ui_percentage; 
} EmmcFileWrite;


/* Helper constants for enums */
#define _MessageType_MIN MessageType_MessageType_Initialize
#define _MessageType_MAX MessageType_MessageType_EmmcDirRemove
#define _MessageType_ARRAYSIZE ((MessageType)(MessageType_MessageType_EmmcDirRemove+1))

#define _RebootType_MIN RebootType_Normal
#define _RebootType_MAX RebootType_BootLoader
#define _RebootType_ARRAYSIZE ((RebootType)(RebootType_BootLoader+1))

#define _OneKeyDeviceType_MIN OneKeyDeviceType_CLASSIC
#define _OneKeyDeviceType_MAX OneKeyDeviceType_TOUCH_PRO
#define _OneKeyDeviceType_ARRAYSIZE ((OneKeyDeviceType)(OneKeyDeviceType_TOUCH_PRO+1))

#define _OneKeySeType_MIN OneKeySeType_THD89
#define _OneKeySeType_MAX OneKeySeType_SE608A
#define _OneKeySeType_ARRAYSIZE ((OneKeySeType)(OneKeySeType_SE608A+1))

#define _FailureType_MIN FailureType_Failure_UnexpectedMessage
#define _FailureType_MAX FailureType_Failure_ProcessError
#define _FailureType_ARRAYSIZE ((FailureType)(FailureType_Failure_ProcessError+1))

#define _ButtonRequestType_MIN ButtonRequestType_ButtonRequest_Other
#define _ButtonRequestType_MAX ButtonRequestType_ButtonRequest_Other
#define _ButtonRequestType_ARRAYSIZE ((ButtonRequestType)(ButtonRequestType_ButtonRequest_Other+1))


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define Initialize_init_default                  {0}
#define GetFeatures_init_default                 {0}
#define Features_init_default                    {false, "", 0, 0, 0, false, 0, false, "", false, "", false, "", false, 0, false, {0, {0}}, false, 0, false, "", false, 0, false, 0, false, 0, false, "", false, 0, false, "", false, "", false, 0, false, 0, false, "", false, 0, false, "", false, "", false, "", false, 0, false, "", false, _OneKeyDeviceType_MIN, false, _OneKeySeType_MIN, false, "", false, {0, {0}}, false, "", false, {0, {0}}, false, "", false, {0, {0}}, false, "", false, "", false, {0, {0}}, false, "", false, "", false, ""}
#define Ping_init_default                        {false, ""}
#define Success_init_default                     {false, ""}
#define Failure_init_default                     {false, _FailureType_MIN, false, ""}
#define WipeDevice_init_default                  {0}
#define ButtonRequest_init_default               {false, _ButtonRequestType_MIN}
#define ButtonAck_init_default                   {0}
#define FirmwareErase_init_default               {false, 0}
#define FirmwareRequest_init_default             {false, 0, false, 0}
#define FirmwareErase_ex_init_default            {false, 0}
#define FirmwareUpload_init_default              {{{NULL}, NULL}, false, {0, {0}}}
#define Reboot_init_default                      {_RebootType_MIN}
#define FirmwareUpdateEmmc_init_default          {"", false, 0}
#define EmmcFixPermission_init_default           {0}
#define EmmcPath_init_default                    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define EmmcPathInfo_init_default                {""}
#define EmmcFile_init_default                    {"", 0, 0, {{NULL}, NULL}, false, 0, false, 0}
#define EmmcFileRead_init_default                {EmmcFile_init_default, false, 0}
#define EmmcFileWrite_init_default               {EmmcFile_init_default, 0, 0, false, 0}
#define EmmcFileDelete_init_default              {""}
#define EmmcDir_init_default                     {"", {{NULL}, NULL}, {{NULL}, NULL}}
#define EmmcDirList_init_default                 {""}
#define EmmcDirMake_init_default                 {""}
#define EmmcDirRemove_init_default               {""}
#define DeviceInfoSettings_init_default          {false, "", false, "", false, ""}
#define GetDeviceInfo_init_default               {0}
#define DeviceInfo_init_default                  {false, "", false, "", false, "", false, {0, {0}}, false, "", false, ""}
#define ReadSEPublicKey_init_default             {0}
#define SEPublicKey_init_default                 {{0, {0}}}
#define WriteSEPublicCert_init_default           {{0, {0}}}
#define ReadSEPublicCert_init_default            {0}
#define SEPublicCert_init_default                {{0, {0}}}
#define SESignMessage_init_default               {{0, {0}}}
#define SEMessageSignature_init_default          {{0, {0}}}
#define Initialize_init_zero                     {0}
#define GetFeatures_init_zero                    {0}
#define Features_init_zero                       {false, "", 0, 0, 0, false, 0, false, "", false, "", false, "", false, 0, false, {0, {0}}, false, 0, false, "", false, 0, false, 0, false, 0, false, "", false, 0, false, "", false, "", false, 0, false, 0, false, "", false, 0, false, "", false, "", false, "", false, 0, false, "", false, _OneKeyDeviceType_MIN, false, _OneKeySeType_MIN, false, "", false, {0, {0}}, false, "", false, {0, {0}}, false, "", false, {0, {0}}, false, "", false, "", false, {0, {0}}, false, "", false, "", false, ""}
#define Ping_init_zero                           {false, ""}
#define Success_init_zero                        {false, ""}
#define Failure_init_zero                        {false, _FailureType_MIN, false, ""}
#define WipeDevice_init_zero                     {0}
#define ButtonRequest_init_zero                  {false, _ButtonRequestType_MIN}
#define ButtonAck_init_zero                      {0}
#define FirmwareErase_init_zero                  {false, 0}
#define FirmwareRequest_init_zero                {false, 0, false, 0}
#define FirmwareErase_ex_init_zero               {false, 0}
#define FirmwareUpload_init_zero                 {{{NULL}, NULL}, false, {0, {0}}}
#define Reboot_init_zero                         {_RebootType_MIN}
#define FirmwareUpdateEmmc_init_zero             {"", false, 0}
#define EmmcFixPermission_init_zero              {0}
#define EmmcPath_init_zero                       {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define EmmcPathInfo_init_zero                   {""}
#define EmmcFile_init_zero                       {"", 0, 0, {{NULL}, NULL}, false, 0, false, 0}
#define EmmcFileRead_init_zero                   {EmmcFile_init_zero, false, 0}
#define EmmcFileWrite_init_zero                  {EmmcFile_init_zero, 0, 0, false, 0}
#define EmmcFileDelete_init_zero                 {""}
#define EmmcDir_init_zero                        {"", {{NULL}, NULL}, {{NULL}, NULL}}
#define EmmcDirList_init_zero                    {""}
#define EmmcDirMake_init_zero                    {""}
#define EmmcDirRemove_init_zero                  {""}
#define DeviceInfoSettings_init_zero             {false, "", false, "", false, ""}
#define GetDeviceInfo_init_zero                  {0}
#define DeviceInfo_init_zero                     {false, "", false, "", false, "", false, {0, {0}}, false, "", false, ""}
#define ReadSEPublicKey_init_zero                {0}
#define SEPublicKey_init_zero                    {{0, {0}}}
#define WriteSEPublicCert_init_zero              {{0, {0}}}
#define ReadSEPublicCert_init_zero               {0}
#define SEPublicCert_init_zero                   {{0, {0}}}
#define SESignMessage_init_zero                  {{0, {0}}}
#define SEMessageSignature_init_zero             {{0, {0}}}

/* Field tags (for use in manual encoding/decoding) */
#define ButtonRequest_code_tag                   1
#define DeviceInfo_serial_no_tag                 1
#define DeviceInfo_spiFlash_info_tag             2
#define DeviceInfo_SE_info_tag                   3
#define DeviceInfo_NFT_voucher_tag               4
#define DeviceInfo_cpu_info_tag                  5
#define DeviceInfo_pre_firmware_tag              6
#define DeviceInfoSettings_serial_no_tag         1
#define DeviceInfoSettings_cpu_info_tag          2
#define DeviceInfoSettings_pre_firmware_tag      3
#define EmmcDir_path_tag                         1
#define EmmcDir_child_dirs_tag                   2
#define EmmcDir_child_files_tag                  3
#define EmmcDirList_path_tag                     1
#define EmmcDirMake_path_tag                     1
#define EmmcDirRemove_path_tag                   1
#define EmmcFile_path_tag                        1
#define EmmcFile_offset_tag                      2
#define EmmcFile_len_tag                         3
#define EmmcFile_data_tag                        4
#define EmmcFile_data_hash_tag                   5
#define EmmcFile_processed_byte_tag              6
#define EmmcFileDelete_path_tag                  1
#define EmmcPath_exist_tag                       1
#define EmmcPath_size_tag                        2
#define EmmcPath_year_tag                        3
#define EmmcPath_month_tag                       4
#define EmmcPath_day_tag                         5
#define EmmcPath_hour_tag                        6
#define EmmcPath_minute_tag                      7
#define EmmcPath_second_tag                      8
#define EmmcPath_readonly_tag                    9
#define EmmcPath_hidden_tag                      10
#define EmmcPath_system_tag                      11
#define EmmcPath_archive_tag                     12
#define EmmcPath_directory_tag                   13
#define EmmcPathInfo_path_tag                    1
#define Failure_code_tag                         1
#define Failure_message_tag                      2
#define Features_vendor_tag                      1
#define Features_major_version_tag               2
#define Features_minor_version_tag               3
#define Features_patch_version_tag               4
#define Features_bootloader_mode_tag             5
#define Features_device_id_tag                   6
#define Features_language_tag                    9
#define Features_label_tag                       10
#define Features_initialized_tag                 12
#define Features_revision_tag                    13
#define Features_firmware_present_tag            18
#define Features_model_tag                       21
#define Features_fw_major_tag                    22
#define Features_fw_minor_tag                    23
#define Features_fw_patch_tag                    24
#define Features_fw_vendor_tag                   25
#define Features_offset_tag                      500
#define Features_ble_name_tag                    501
#define Features_ble_ver_tag                     502
#define Features_ble_enable_tag                  503
#define Features_se_enable_tag                   504
#define Features_se_ver_tag                      506
#define Features_backup_only_tag                 507
#define Features_onekey_version_tag              508
#define Features_bootloader_version_tag          510
#define Features_serial_no_tag                   511
#define Features_initstates_tag                  513
#define Features_boardloader_version_tag         519
#define Features_onekey_device_type_tag          600
#define Features_onekey_se_type_tag              601
#define Features_onekey_board_version_tag        602
#define Features_onekey_board_hash_tag           603
#define Features_onekey_boot_version_tag         604
#define Features_onekey_boot_hash_tag            605
#define Features_onekey_se_version_tag           606
#define Features_onekey_se_hash_tag              607
#define Features_onekey_se_build_id_tag          608
#define Features_onekey_firmware_version_tag     609
#define Features_onekey_firmware_hash_tag        610
#define Features_onekey_firmware_build_id_tag    611
#define Features_onekey_serial_no_tag            612
#define Features_onekey_boot_build_id_tag        613
#define FirmwareErase_length_tag                 1
#define FirmwareErase_ex_length_tag              1
#define FirmwareRequest_offset_tag               1
#define FirmwareRequest_length_tag               2
#define FirmwareUpdateEmmc_path_tag              1
#define FirmwareUpdateEmmc_reboot_on_success_tag 2
#define FirmwareUpload_payload_tag               1
#define FirmwareUpload_hash_tag                  2
#define Ping_message_tag                         1
#define Reboot_reboot_type_tag                   1
#define SEMessageSignature_signature_tag         1
#define SEPublicCert_public_cert_tag             1
#define SEPublicKey_public_key_tag               1
#define SESignMessage_message_tag                1
#define Success_message_tag                      1
#define WriteSEPublicCert_public_cert_tag        1
#define EmmcFileRead_file_tag                    1
#define EmmcFileRead_ui_percentage_tag           2
#define EmmcFileWrite_file_tag                   1
#define EmmcFileWrite_overwrite_tag              2
#define EmmcFileWrite_append_tag                 3
#define EmmcFileWrite_ui_percentage_tag          4

/* Struct field encoding specification for nanopb */
#define Initialize_FIELDLIST(X, a) \

#define Initialize_CALLBACK NULL
#define Initialize_DEFAULT NULL

#define GetFeatures_FIELDLIST(X, a) \

#define GetFeatures_CALLBACK NULL
#define GetFeatures_DEFAULT NULL

#define Features_FIELDLIST(X, a) \
X(a, STATIC,   OPTIONAL, STRING,   vendor,            1) \
X(a, STATIC,   REQUIRED, UINT32,   major_version,     2) \
X(a, STATIC,   REQUIRED, UINT32,   minor_version,     3) \
X(a, STATIC,   REQUIRED, UINT32,   patch_version,     4) \
X(a, STATIC,   OPTIONAL, BOOL,     bootloader_mode,   5) \
X(a, STATIC,   OPTIONAL, STRING,   device_id,         6) \
X(a, STATIC,   OPTIONAL, STRING,   language,          9) \
X(a, STATIC,   OPTIONAL, STRING,   label,            10) \
X(a, STATIC,   OPTIONAL, BOOL,     initialized,      12) \
X(a, STATIC,   OPTIONAL, BYTES,    revision,         13) \
X(a, STATIC,   OPTIONAL, BOOL,     firmware_present,  18) \
X(a, STATIC,   OPTIONAL, STRING,   model,            21) \
X(a, STATIC,   OPTIONAL, UINT32,   fw_major,         22) \
X(a, STATIC,   OPTIONAL, UINT32,   fw_minor,         23) \
X(a, STATIC,   OPTIONAL, UINT32,   fw_patch,         24) \
X(a, STATIC,   OPTIONAL, STRING,   fw_vendor,        25) \
X(a, STATIC,   OPTIONAL, UINT32,   offset,          500) \
X(a, STATIC,   OPTIONAL, STRING,   ble_name,        501) \
X(a, STATIC,   OPTIONAL, STRING,   ble_ver,         502) \
X(a, STATIC,   OPTIONAL, BOOL,     ble_enable,      503) \
X(a, STATIC,   OPTIONAL, BOOL,     se_enable,       504) \
X(a, STATIC,   OPTIONAL, STRING,   se_ver,          506) \
X(a, STATIC,   OPTIONAL, BOOL,     backup_only,     507) \
X(a, STATIC,   OPTIONAL, STRING,   onekey_version,  508) \
X(a, STATIC,   OPTIONAL, STRING,   bootloader_version, 510) \
X(a, STATIC,   OPTIONAL, STRING,   serial_no,       511) \
X(a, STATIC,   OPTIONAL, UINT32,   initstates,      513) \
X(a, STATIC,   OPTIONAL, STRING,   boardloader_version, 519) \
X(a, STATIC,   OPTIONAL, UENUM,    onekey_device_type, 600) \
X(a, STATIC,   OPTIONAL, UENUM,    onekey_se_type,  601) \
X(a, STATIC,   OPTIONAL, STRING,   onekey_board_version, 602) \
X(a, STATIC,   OPTIONAL, BYTES,    onekey_board_hash, 603) \
X(a, STATIC,   OPTIONAL, STRING,   onekey_boot_version, 604) \
X(a, STATIC,   OPTIONAL, BYTES,    onekey_boot_hash, 605) \
X(a, STATIC,   OPTIONAL, STRING,   onekey_se_version, 606) \
X(a, STATIC,   OPTIONAL, BYTES,    onekey_se_hash,  607) \
X(a, STATIC,   OPTIONAL, STRING,   onekey_se_build_id, 608) \
X(a, STATIC,   OPTIONAL, STRING,   onekey_firmware_version, 609) \
X(a, STATIC,   OPTIONAL, BYTES,    onekey_firmware_hash, 610) \
X(a, STATIC,   OPTIONAL, STRING,   onekey_firmware_build_id, 611) \
X(a, STATIC,   OPTIONAL, STRING,   onekey_serial_no, 612) \
X(a, STATIC,   OPTIONAL, STRING,   onekey_boot_build_id, 613)
#define Features_CALLBACK NULL
#define Features_DEFAULT NULL

#define Ping_FIELDLIST(X, a) \
X(a, STATIC,   OPTIONAL, STRING,   message,           1)
#define Ping_CALLBACK NULL
#define Ping_DEFAULT (const pb_byte_t*)"\x0a\x00\x00"

#define Success_FIELDLIST(X, a) \
X(a, STATIC,   OPTIONAL, STRING,   message,           1)
#define Success_CALLBACK NULL
#define Success_DEFAULT (const pb_byte_t*)"\x0a\x00\x00"

#define Failure_FIELDLIST(X, a) \
X(a, STATIC,   OPTIONAL, UENUM,    code,              1) \
X(a, STATIC,   OPTIONAL, STRING,   message,           2)
#define Failure_CALLBACK NULL
#define Failure_DEFAULT (const pb_byte_t*)"\x08\x01\x00"

#define WipeDevice_FIELDLIST(X, a) \

#define WipeDevice_CALLBACK NULL
#define WipeDevice_DEFAULT NULL

#define ButtonRequest_FIELDLIST(X, a) \
X(a, STATIC,   OPTIONAL, UENUM,    code,              1)
#define ButtonRequest_CALLBACK NULL
#define ButtonRequest_DEFAULT (const pb_byte_t*)"\x08\x01\x00"

#define ButtonAck_FIELDLIST(X, a) \

#define ButtonAck_CALLBACK NULL
#define ButtonAck_DEFAULT NULL

#define FirmwareErase_FIELDLIST(X, a) \
X(a, STATIC,   OPTIONAL, UINT32,   length,            1)
#define FirmwareErase_CALLBACK NULL
#define FirmwareErase_DEFAULT NULL

#define FirmwareRequest_FIELDLIST(X, a) \
X(a, STATIC,   OPTIONAL, UINT32,   offset,            1) \
X(a, STATIC,   OPTIONAL, UINT32,   length,            2)
#define FirmwareRequest_CALLBACK NULL
#define FirmwareRequest_DEFAULT NULL

#define FirmwareErase_ex_FIELDLIST(X, a) \
X(a, STATIC,   OPTIONAL, UINT32,   length,            1)
#define FirmwareErase_ex_CALLBACK NULL
#define FirmwareErase_ex_DEFAULT NULL

#define FirmwareUpload_FIELDLIST(X, a) \
X(a, CALLBACK, REQUIRED, BYTES,    payload,           1) \
X(a, STATIC,   OPTIONAL, BYTES,    hash,              2)
#define FirmwareUpload_CALLBACK pb_default_field_callback
#define FirmwareUpload_DEFAULT NULL

#define Reboot_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, UENUM,    reboot_type,       1)
#define Reboot_CALLBACK NULL
#define Reboot_DEFAULT NULL

#define FirmwareUpdateEmmc_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, STRING,   path,              1) \
X(a, STATIC,   OPTIONAL, BOOL,     reboot_on_success,   2)
#define FirmwareUpdateEmmc_CALLBACK NULL
#define FirmwareUpdateEmmc_DEFAULT NULL

#define EmmcFixPermission_FIELDLIST(X, a) \

#define EmmcFixPermission_CALLBACK NULL
#define EmmcFixPermission_DEFAULT NULL

#define EmmcPath_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, BOOL,     exist,             1) \
X(a, STATIC,   REQUIRED, UINT64,   size,              2) \
X(a, STATIC,   REQUIRED, UINT32,   year,              3) \
X(a, STATIC,   REQUIRED, UINT32,   month,             4) \
X(a, STATIC,   REQUIRED, UINT32,   day,               5) \
X(a, STATIC,   REQUIRED, UINT32,   hour,              6) \
X(a, STATIC,   REQUIRED, UINT32,   minute,            7) \
X(a, STATIC,   REQUIRED, UINT32,   second,            8) \
X(a, STATIC,   REQUIRED, BOOL,     readonly,          9) \
X(a, STATIC,   REQUIRED, BOOL,     hidden,           10) \
X(a, STATIC,   REQUIRED, BOOL,     system,           11) \
X(a, STATIC,   REQUIRED, BOOL,     archive,          12) \
X(a, STATIC,   REQUIRED, BOOL,     directory,        13)
#define EmmcPath_CALLBACK NULL
#define EmmcPath_DEFAULT NULL

#define EmmcPathInfo_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, STRING,   path,              1)
#define EmmcPathInfo_CALLBACK NULL
#define EmmcPathInfo_DEFAULT NULL

#define EmmcFile_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, STRING,   path,              1) \
X(a, STATIC,   REQUIRED, UINT32,   offset,            2) \
X(a, STATIC,   REQUIRED, UINT32,   len,               3) \
X(a, CALLBACK, OPTIONAL, BYTES,    data,              4) \
X(a, STATIC,   OPTIONAL, UINT32,   data_hash,         5) \
X(a, STATIC,   OPTIONAL, UINT32,   processed_byte,    6)
#define EmmcFile_CALLBACK pb_default_field_callback
#define EmmcFile_DEFAULT NULL

#define EmmcFileRead_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, MESSAGE,  file,              1) \
X(a, STATIC,   OPTIONAL, UINT32,   ui_percentage,     2)
#define EmmcFileRead_CALLBACK NULL
#define EmmcFileRead_DEFAULT NULL
#define EmmcFileRead_file_MSGTYPE EmmcFile

#define EmmcFileWrite_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, MESSAGE,  file,              1) \
X(a, STATIC,   REQUIRED, BOOL,     overwrite,         2) \
X(a, STATIC,   REQUIRED, BOOL,     append,            3) \
X(a, STATIC,   OPTIONAL, UINT32,   ui_percentage,     4)
#define EmmcFileWrite_CALLBACK NULL
#define EmmcFileWrite_DEFAULT NULL
#define EmmcFileWrite_file_MSGTYPE EmmcFile

#define EmmcFileDelete_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, STRING,   path,              1)
#define EmmcFileDelete_CALLBACK NULL
#define EmmcFileDelete_DEFAULT NULL

#define EmmcDir_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, STRING,   path,              1) \
X(a, CALLBACK, OPTIONAL, STRING,   child_dirs,        2) \
X(a, CALLBACK, OPTIONAL, STRING,   child_files,       3)
#define EmmcDir_CALLBACK pb_default_field_callback
#define EmmcDir_DEFAULT NULL

#define EmmcDirList_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, STRING,   path,              1)
#define EmmcDirList_CALLBACK NULL
#define EmmcDirList_DEFAULT NULL

#define EmmcDirMake_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, STRING,   path,              1)
#define EmmcDirMake_CALLBACK NULL
#define EmmcDirMake_DEFAULT NULL

#define EmmcDirRemove_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, STRING,   path,              1)
#define EmmcDirRemove_CALLBACK NULL
#define EmmcDirRemove_DEFAULT NULL

#define DeviceInfoSettings_FIELDLIST(X, a) \
X(a, STATIC,   OPTIONAL, STRING,   serial_no,         1) \
X(a, STATIC,   OPTIONAL, STRING,   cpu_info,          2) \
X(a, STATIC,   OPTIONAL, STRING,   pre_firmware,      3)
#define DeviceInfoSettings_CALLBACK NULL
#define DeviceInfoSettings_DEFAULT NULL

#define GetDeviceInfo_FIELDLIST(X, a) \

#define GetDeviceInfo_CALLBACK NULL
#define GetDeviceInfo_DEFAULT NULL

#define DeviceInfo_FIELDLIST(X, a) \
X(a, STATIC,   OPTIONAL, STRING,   serial_no,         1) \
X(a, STATIC,   OPTIONAL, STRING,   spiFlash_info,     2) \
X(a, STATIC,   OPTIONAL, STRING,   SE_info,           3) \
X(a, STATIC,   OPTIONAL, BYTES,    NFT_voucher,       4) \
X(a, STATIC,   OPTIONAL, STRING,   cpu_info,          5) \
X(a, STATIC,   OPTIONAL, STRING,   pre_firmware,      6)
#define DeviceInfo_CALLBACK NULL
#define DeviceInfo_DEFAULT NULL

#define ReadSEPublicKey_FIELDLIST(X, a) \

#define ReadSEPublicKey_CALLBACK NULL
#define ReadSEPublicKey_DEFAULT NULL

#define SEPublicKey_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, BYTES,    public_key,        1)
#define SEPublicKey_CALLBACK NULL
#define SEPublicKey_DEFAULT NULL

#define WriteSEPublicCert_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, BYTES,    public_cert,       1)
#define WriteSEPublicCert_CALLBACK NULL
#define WriteSEPublicCert_DEFAULT NULL

#define ReadSEPublicCert_FIELDLIST(X, a) \

#define ReadSEPublicCert_CALLBACK NULL
#define ReadSEPublicCert_DEFAULT NULL

#define SEPublicCert_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, BYTES,    public_cert,       1)
#define SEPublicCert_CALLBACK NULL
#define SEPublicCert_DEFAULT NULL

#define SESignMessage_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, BYTES,    message,           1)
#define SESignMessage_CALLBACK NULL
#define SESignMessage_DEFAULT NULL

#define SEMessageSignature_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, BYTES,    signature,         1)
#define SEMessageSignature_CALLBACK NULL
#define SEMessageSignature_DEFAULT NULL

extern const pb_msgdesc_t Initialize_msg;
extern const pb_msgdesc_t GetFeatures_msg;
extern const pb_msgdesc_t Features_msg;
extern const pb_msgdesc_t Ping_msg;
extern const pb_msgdesc_t Success_msg;
extern const pb_msgdesc_t Failure_msg;
extern const pb_msgdesc_t WipeDevice_msg;
extern const pb_msgdesc_t ButtonRequest_msg;
extern const pb_msgdesc_t ButtonAck_msg;
extern const pb_msgdesc_t FirmwareErase_msg;
extern const pb_msgdesc_t FirmwareRequest_msg;
extern const pb_msgdesc_t FirmwareErase_ex_msg;
extern const pb_msgdesc_t FirmwareUpload_msg;
extern const pb_msgdesc_t Reboot_msg;
extern const pb_msgdesc_t FirmwareUpdateEmmc_msg;
extern const pb_msgdesc_t EmmcFixPermission_msg;
extern const pb_msgdesc_t EmmcPath_msg;
extern const pb_msgdesc_t EmmcPathInfo_msg;
extern const pb_msgdesc_t EmmcFile_msg;
extern const pb_msgdesc_t EmmcFileRead_msg;
extern const pb_msgdesc_t EmmcFileWrite_msg;
extern const pb_msgdesc_t EmmcFileDelete_msg;
extern const pb_msgdesc_t EmmcDir_msg;
extern const pb_msgdesc_t EmmcDirList_msg;
extern const pb_msgdesc_t EmmcDirMake_msg;
extern const pb_msgdesc_t EmmcDirRemove_msg;
extern const pb_msgdesc_t DeviceInfoSettings_msg;
extern const pb_msgdesc_t GetDeviceInfo_msg;
extern const pb_msgdesc_t DeviceInfo_msg;
extern const pb_msgdesc_t ReadSEPublicKey_msg;
extern const pb_msgdesc_t SEPublicKey_msg;
extern const pb_msgdesc_t WriteSEPublicCert_msg;
extern const pb_msgdesc_t ReadSEPublicCert_msg;
extern const pb_msgdesc_t SEPublicCert_msg;
extern const pb_msgdesc_t SESignMessage_msg;
extern const pb_msgdesc_t SEMessageSignature_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define Initialize_fields &Initialize_msg
#define GetFeatures_fields &GetFeatures_msg
#define Features_fields &Features_msg
#define Ping_fields &Ping_msg
#define Success_fields &Success_msg
#define Failure_fields &Failure_msg
#define WipeDevice_fields &WipeDevice_msg
#define ButtonRequest_fields &ButtonRequest_msg
#define ButtonAck_fields &ButtonAck_msg
#define FirmwareErase_fields &FirmwareErase_msg
#define FirmwareRequest_fields &FirmwareRequest_msg
#define FirmwareErase_ex_fields &FirmwareErase_ex_msg
#define FirmwareUpload_fields &FirmwareUpload_msg
#define Reboot_fields &Reboot_msg
#define FirmwareUpdateEmmc_fields &FirmwareUpdateEmmc_msg
#define EmmcFixPermission_fields &EmmcFixPermission_msg
#define EmmcPath_fields &EmmcPath_msg
#define EmmcPathInfo_fields &EmmcPathInfo_msg
#define EmmcFile_fields &EmmcFile_msg
#define EmmcFileRead_fields &EmmcFileRead_msg
#define EmmcFileWrite_fields &EmmcFileWrite_msg
#define EmmcFileDelete_fields &EmmcFileDelete_msg
#define EmmcDir_fields &EmmcDir_msg
#define EmmcDirList_fields &EmmcDirList_msg
#define EmmcDirMake_fields &EmmcDirMake_msg
#define EmmcDirRemove_fields &EmmcDirRemove_msg
#define DeviceInfoSettings_fields &DeviceInfoSettings_msg
#define GetDeviceInfo_fields &GetDeviceInfo_msg
#define DeviceInfo_fields &DeviceInfo_msg
#define ReadSEPublicKey_fields &ReadSEPublicKey_msg
#define SEPublicKey_fields &SEPublicKey_msg
#define WriteSEPublicCert_fields &WriteSEPublicCert_msg
#define ReadSEPublicCert_fields &ReadSEPublicCert_msg
#define SEPublicCert_fields &SEPublicCert_msg
#define SESignMessage_fields &SESignMessage_msg
#define SEMessageSignature_fields &SEMessageSignature_msg

/* Maximum encoded size of messages (where known) */
/* FirmwareUpload_size depends on runtime parameters */
/* EmmcFile_size depends on runtime parameters */
/* EmmcFileRead_size depends on runtime parameters */
/* EmmcFileWrite_size depends on runtime parameters */
/* EmmcDir_size depends on runtime parameters */
#define ButtonAck_size                           0
#define ButtonRequest_size                       2
#define DeviceInfoSettings_size                  67
#define DeviceInfo_size                          135
#define EmmcDirList_size                         258
#define EmmcDirMake_size                         258
#define EmmcDirRemove_size                       258
#define EmmcFileDelete_size                      258
#define EmmcFixPermission_size                   0
#define EmmcPathInfo_size                        258
#define EmmcPath_size                            59
#define Failure_size                             260
#define Features_size                            1017
#define FirmwareErase_ex_size                    6
#define FirmwareErase_size                       6
#define FirmwareRequest_size                     12
#define FirmwareUpdateEmmc_size                  260
#define GetDeviceInfo_size                       0
#define GetFeatures_size                         0
#define Initialize_size                          0
#define Ping_size                                258
#define ReadSEPublicCert_size                    0
#define ReadSEPublicKey_size                     0
#define Reboot_size                              2
#define SEMessageSignature_size                  66
#define SEPublicCert_size                        419
#define SEPublicKey_size                         66
#define SESignMessage_size                       1027
#define Success_size                             258
#define WipeDevice_size                          0
#define WriteSEPublicCert_size                   419

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
