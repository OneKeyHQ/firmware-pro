#ifndef ___OROMAIN_H_INCLUDED___
#define ___OROMAIN_H_INCLUDED___

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define IMG_WIDTH     36  // image size - width
#define IMG_HEIGHT    160 // image size - height

#define TENPERSIZE    128 * 1024 // Template memory size
#define MEM_SIZE      40 * 1024  // �㷨�ڴ��С algorithm memory size

#define MAX_NUMENROLL 16 // �㷨�ڴ��С algorithm memory size

/* FPS operation fail code */
typedef enum
{
    FPS_SUCCESS = 0,        // Success
    FPS_LESSEFATUREPOINT,    // feature points too less
    FPS_MULTIFEATUREPOINT,  // feature points too multiple
    FPS_POSITIONOVERLAPPED, // current finger position and previous overlapped
    FPS_POSITIONMISMATCH,   // position mismatch
    FPS_AREASMALL,          // finger area is too samll on the fps
    FPS_IMAGEBLURRING,      // finger image is blurring
    FPS_TEMPLETNULL,        // finger templet is null
    FPS_TEMPLETMISMATCH,    // finger templet match fail
    FPS_PARAMETERERROR,     // Memory run error
    FPS_NOFINGER,           // no finger on fps while enrolling
    FPS_IDEXIST,            // finger id is existed
    FPS_IDOUTRANGE,         // finger id is out of set range
    FPS_FINGEREXIST,        // current finger has enrolled
    FPS_IMAGEABNORMAL,      // finger image abnormal
    FPS_IDNONEXISTENT,      // finger id is non-existent
    FPS_IDABNORMAL,         // finger id abnormal
    FPS_ENROLLINVALID,      // the enroll operation is completed but templete is invalid
    FPS_SPIERROR,           // sensor communication error
    FPS_TEMPLETSAVEFAILURE, // save templet failure
    FPS_UPDATEFAILURE,      // templet update failure
    FPS_WATER_FLAG,
    FPS_TEMPLATE_ALREADY_EXIST = 0x27, // templet EXIST
    FPS_TEMPLATE_NULL = 0x28,          // templet NUll
    FPS_LOCK_REGISTERFAIL = 99,
    FPS_LOCK_REGISTER_MOVE_SLOW = 101,
    FPS_LOCK_REGISTER_MOVE_LARGE = 102,
    FPS_LOCK_REGISTER_LOW_QUALITY = 103,
    FPS_FPS_LOCK_REGISTER_PRESSSMALL = 104,
    FPS_MAX_ENROLL = 105, // max enroll finger number

} FPS_FailCode; // ������

/* ID read return struct */
typedef struct
{
    uint8_t ExistIdNum;
    uint8_t ExistId[10];
} FPS_IdRead_return_struct;

typedef struct
{
    uint32_t flash_start_addr; // 0x08038000
    uint32_t base_start_addr;  //
    uint8_t MAX_ENROLL_NUM;    //
    uint8_t MAX_USER_COUNT;    //
} FPS_Init_Data;               // flash init

extern uint8_t gBmp[];

void Get_version(uint32_t* alg_version);

/*!
\brief      init
\param[in]  TemplateAddr
        \arg			Flash Address that save template
\param[out] none
\retval    0 success 1err
*/

uint8_t SLAlg_Init(void* p_init_data, uint8_t* FP_MEM_SIZE);

/*!
\brief      get finger image data
\param[out]  *ImageData
        \arg      	point of image data ( size :138 * 112 )

\retval
        \arg
        0��finger up
        1��finger down
*/
uint8_t Finger_ImageGet(uint8_t* img);

/*!
\brief      read finger exist id
\param[in]  none
        \arg

\param[out] none
        \arg

\retval     FPS_IdRead_return_struct
        ExistIdNum��template index
        ExistId:
        bits =1��enrolled
        bits =0��Not enrolled
*/
FPS_IdRead_return_struct SL_IdRead(void);

/*!
\brief      Finger enrollment: includes image capture, extract features and so on
\param[in]
gBmp��image data
enrollNum:
    enroll progress from zero to (MAX_ENROLL_NUM -1).
Repeat_Num��
                Same finger enroll detection control threshold (default input 3).
\retval
        FPS_SUCCESS: enrollment success
        FPS_TEMPLATE_ALREADY_EXIST: finger exist
        FPS_LESSEFATUREPOINT: less features
*/
uint8_t SL_Enroll(uint8_t* gBmp, uint8_t enrollNum, uint8_t Repeat_Num);

/*!
\brief      Finger enrollment: includes image capture, extract features and so on
*/
void SL_SaveTemp();

/*!
\brief      Extract features when authentication
*/
uint8_t SL_GetFeature(uint8_t* gBmp);

/*!
\brief     Image authentication
\param[in]
MatchId��Index ID
UpdateFlag��
        1��Rolling update
        0��Not update
reco_score��match score
\retval
        FPS_SUCCESS��     Authentication success
        FPS_AREASMALL Authentication fail
        FPS_TEMPLATE_NULL: temolate NULL
*/
uint8_t SL_Match(uint8_t* reco_score, uint8_t* reco_learn, uint8_t* matchID);

/*!
\brief     Authentication success, if updateFlag = 1, then update template
*/
void SL_UpDate(uint8_t ID);

/*!
\brief     Delete Id��delete template index
*/
void SL_DellChar(uint8_t id);

/*!
\brief    Delete all templates
*/
void SL_DeleteAll();

#endif
