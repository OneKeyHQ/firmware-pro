#ifndef FPALGORITHM_INTERFACE_H
#define FPALGORITHM_INTERFACE_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "GSL61xx.h"

#define FP_MAX_ENROLL_STEP 30
#define FP_MAX_USER_COUNT  3 // must less than 10

#define TENPERSIZE         128 * 1024 // Template memory size
#define MEM_SIZE           40 * 1024  // 算法内存大小 algorithm memory size

/* FPS operation fail code */
typedef enum
{
    FPS_SUCCESS = 0x00,                   // 0   Success
    FPS_LESSEFATUREPOINT = 0x01,          // 1   feature points too less
    FPS_MULTIFEATUREPOINT = 0x02,         // 2   feature points too multiple
    FPS_POSITIONOVERLAPPED = 0x03,        // 3   current finger position and previous overlapped
    FPS_POSITIONMISMATCH = 0x04,          // 4   position mismatch
    FPS_AREASMALL = 0x05,                 // 5   finger area is too small on the fps
    FPS_IMAGEBLURRING = 0x06,             // 6   finger image is blurring
    FPS_TEMPLETNULL = 0x07,               // 7   finger templet is null
    FPS_TEMPLETMISMATCH = 0x08,           // 8   finger templet match fail
    FPS_PARAMETERERROR = 0x09,            // 9   Memory run error
    FPS_NOFINGER = 0x0A,                  // 10  no finger on fps while enrolling
    FPS_IDEXIST = 0x0B,                   // 11  finger id is existed
    FPS_IDOUTRANGE = 0x0C,                // 12  finger id is out of set range
    FPS_FINGEREXIST = 0x0D,               // 13  current finger has enrolled
    FPS_IMAGEABNORMAL = 0x0E,             // 14  finger image abnormal
    FPS_IDNONEXISTENT = 0x0F,             // 15  finger id is non-existent
    FPS_IDABNORMAL = 0x10,                // 16  finger id abnormal
    FPS_ENROLLINVALID = 0x11,             // 17  the enroll operation is completed but templete is invalid
    FPS_SPIERROR = 0x12,                  // 18  sensor communication error
    FPS_TEMPLETSAVEFAILURE = 0x13,        // 19  save templet failure
    FPS_UPDATEFAILURE = 0x14,             // 20  templet update failure
    FPS_WATER_FLAG = 0x15,                // 21
    FPS_TEMPLATE_ALREADY_EXIST = 0x27,    // 39  templet EXIST
    FPS_TEMPLATE_NULL = 0x28,             // 40  templet NUll
    FPS_LOCK_REGISTER_FAIL = 0x63,        // 99
    FPS_LOCK_REGISTER_OK = 0x64,          // 100
    FPS_LOCK_REGISTER_MOVE_SLOW = 0x65,   // 101
    FPS_LOCK_REGISTER_MOVE_LARGE = 0x66,  // 102
    FPS_LOCK_REGISTER_LOW_QUALITY = 0x67, // 103
    FPS_LOCK_REGISTER_PRESS_SMALL = 0x68, // 104
    FPS_MAX_ENROLL = 0x69,                // 105 max enroll finger number
} FPS_FailCode;                           // 错误码

/* ID read return struct */
typedef struct
{
    uint8_t ExistIdNum;
    uint8_t ExistId[10]; // is fixed, must less or equal than 10
} FPS_IdRead_return_struct;

typedef struct
{
    uint32_t template_offset;   // header 4KB + 128KB per template
    uint32_t noise_base_offset; // fixed 20KB
    uint8_t MAX_ENROLL_NUM;     //
    uint8_t MAX_USER_COUNT;     //
} FPS_Init_Data;

/* Get algorithm version */
void Get_version(uint32_t* alg_version);

/* Get finger image data
 * img: pointer to image data
 * return: 0 finger up, 1 finger down
 */
bool Finger_ImageGet(uint8_t* img);

/* Algorithm init
 * p_init_data: pointer to FPS_Init_Data
 * FP_MEM: pointer to memory buffer
 * return: 0 success
 */
uint8_t SLAlg_Init(void* p_init_data, uint8_t* FP_MEM);

/* Read finger exist id
 * return: FPS_IdRead_return_struct
 * note: minimal 15 enroll needed for a template to be considered valid, otherwise ignored
 */
FPS_IdRead_return_struct SL_IdRead(void);

/* Finger enrollment: includes image capture, extract features and so on
 * gBmp: image data
 * enrollNum: enroll progress from zero to (MAX_ENROLL_NUM -1)
 * repeatNum: same finger enroll detection control threshold (try match in FIRST x enroll)
 * overlapNum: finger position overlap detection control threshold (detects TOTAL x times)
 * return:
 * FPS_SUCCESS: enrollment success,
 * FPS_TEMPLATE_ALREADY_EXIST: finger exist,
 * FPS_LESSEFATUREPOINT: less features
 */
FPS_FailCode SL_Enroll(uint8_t* gBmp, uint8_t enrollNum, uint8_t repeatNum, uint8_t overlapNum);

/* Save template after enrollment */
void SL_SaveTemp(uint8_t* s_id);

/* Extract features when authentication */
FPS_FailCode SL_GetFeature(uint8_t* gBmp);

/* Image authentication
 * reco_score: match score
 * reco_learn: learning score
 * matchID: matched ID
 * return:
 * FPS_SUCCESS: Authentication success,
 * FPS_AREASMALL: Authentication fail,
 * FPS_TEMPLATE_NULL: template NULL
 */
FPS_FailCode SL_Match(uint8_t* reco_score, uint8_t* reco_learn, uint8_t* matchID);

/* Authentication success, if updateFlag = 1, then update template */
void SL_UpDate(uint8_t ID);

/* Delete Id: delete template index */
void SL_DellChar(uint8_t id);

/* Delete all templates */
void SL_DeleteAll();

/* Calculate checksum */
void SL_Check_template(uint8_t* fp_template, int32_t temp_size, uint64_t* temp_sum);

#endif
