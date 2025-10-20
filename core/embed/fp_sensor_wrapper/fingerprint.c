#include <string.h>

#include "common.h"
#ifndef EMULATOR
  #include "fp_algo_interface.h"
  #include "fp_algo_interface_dark.h"
#endif
#include "fingerprint.h"
#include "irq.h"
#include "debug_utils.h"
#include "display.h"
#include "mipi_lcd.h"

#pragma GCC diagnostic ignored "-Wcpp"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

// disable log
#undef DBG_PRINTF
#define DBG_PRINTF(...)

// alg config
FPS_Init_Data algo_init_data = {
    .noise_base_offset = 0,
    .template_offset = 20 * 1024,
    .MAX_ENROLL_NUM = FP_MAX_ENROLL_STEP,
    .MAX_USER_COUNT = FP_MAX_USER_COUNT,
};

uint8_t* fp_buff_img;
uint8_t* fp_buff_img_raw;
uint8_t* fp_buff_algo;

static bool fingerprint_module_status = false;

bool fingerprint_module_status_get(void)
{
    return fingerprint_module_status;
}

#ifdef EMULATOR
FP_RESULT fingerprint_get_version(char* version)
{
    strcpy(version, "1.0.0");
    return FP_RESULT_OK;
}
FP_RESULT fingerprint_detect(void)
{
    return FP_RESULT_OK;
}
FP_RESULT fingerprint_enroll(uint8_t counter, uint8_t repeat_detect_num, uint8_t overlap_detect_threshold)
{
    (void)counter;
    (void)repeat_detect_num;
    (void)overlap_detect_threshold;
    return FP_RESULT_OK;
}
FP_RESULT fingerprint_save(uint8_t* id)
{
    (void)id;
    return FP_RESULT_OK;
}
FP_RESULT fingerprint_match(uint8_t* match_id)
{
    (void)match_id;
    return FP_RESULT_OK;
}
FP_RESULT fingerprint_delete(uint8_t id)
{
    (void)id;
    return FP_RESULT_OK;
}
FP_RESULT fingerprint_delete_all(void)
{
    return FP_RESULT_OK;
}
FP_RESULT fingerprint_get_count(uint8_t* count)
{
    (void)count;
    return FP_RESULT_OK;
}
FP_RESULT fingerprint_get_list(uint8_t* list, uint8_t len)
{
    (void)list;
    (void)len;
    return FP_RESULT_OK;
}
#else

FP_RESULT fingerprint_init(void)
{
    // image buffer
    fp_buff_img = (uint8_t*)FMC_SDRAM_BOOLOADER_BUFFER_ADDRESS; // image
    fp_buff_img_raw = fp_buff_img + IMG_BUFFER_SIZE;            // image (copy, use for debug display)
    fp_buff_algo = fp_buff_img_raw + IMG_BUFFER_SIZE;           // 0xD0201900

    // sensor
    GSL61xx_init();
    fingerprint_module_status = true;

    // config
    FPS_FailCode ret = SLAlg_Init(&algo_init_data, fp_buff_algo);

    if ( ret != FPS_SUCCESS )
    {
        return FP_RESULT_ErrOther;
    }
    fingerprint_module_status = true;
    return FP_RESULT_OK;
}

FP_RESULT fingerprint_get_version(char* version)
{
    uint32_t version_int = 0;
    Get_version(&version_int);
    sprintf(version, "0x%08lx", version_int);
    return FP_RESULT_OK;
}

FP_RESULT fingerprint_get_image_dark(void)
{
    if ( !fingerprint_module_status )
        return FP_RESULT_ErrState;

    uint8_t result_curr = 0;
    static uint8_t result_last = 0;
    static uint32_t need_release_count = 0;

    uint32_t irq = disable_irq();
    volatile FP_RESULT ret = FP_RESULT_ErrOther;

    result_curr = CaptureGSL61xx(fp_buff_img);

    if ( result_curr == FINGER_DOWN && result_last == FINGER_UP )
    {
        // backup original picture
        memcpy(fp_buff_img_raw, fp_buff_img, IMG_BUFFER_SIZE);
        // debug display
        display_fp(
            DISPLAY_RESX - IMG_WIDTH - 30, DISPLAY_RESY - IMG_HEIGHT - 100, IMG_WIDTH, IMG_HEIGHT, fp_buff_img_raw
        );

        ReduceBaseStep2(fp_buff_img, 100);
        ReduceBaseStep1(fp_buff_img, 36, 160);
        ret = FP_RESULT_OK;
        result_last = result_curr;
    }
    else if ( result_curr == FINGER_DOWN && result_last == FINGER_DOWN )
    {
        // with maximum release count
        if ( need_release_count < 150 )
        {
            need_release_count++;
            ret = FP_RESULT_NeedRelease;
        }
        else
        {
            need_release_count = 0;
            ret = FP_RESULT_NoFinger;
        }

        // without maximum release count
        // ret = FP_RESULT_NeedRelease;
    }
    else if ( result_curr == FINGER_UP )
    {
        ret = FP_RESULT_NoFinger;
        result_last = result_curr;
    }
    else
    {
        ret = FP_RESULT_ErrOther;
    }
    enable_irq(irq);

    return ret;
}

FP_RESULT fingerprint_get_image(void)
{
    // use self implemented Finger_ImageGet
    // mainly to have a img buffer copy
    return fingerprint_get_image_dark();

    // use library Finger_ImageGet
    if ( !fingerprint_module_status )
        return FP_RESULT_ErrState;

    uint8_t result_curr = 0;
    static uint8_t result_last = 0;

    uint32_t irq = disable_irq();
    volatile FP_RESULT ret = FP_RESULT_ErrOther;

    result_curr = !Finger_ImageGet(fp_buff_img);

    if ( result_curr == FINGER_DOWN && result_last == FINGER_UP )
    {
        // debug display
        display_fp(DISPLAY_RESX - IMG_WIDTH - 30, DISPLAY_RESY - IMG_HEIGHT - 100, IMG_WIDTH, IMG_HEIGHT, fp_buff_img);
        ret = FP_RESULT_OK;
        result_last = result_curr;
    }
    else if ( result_curr == FINGER_DOWN && result_last == FINGER_DOWN )
    {
        ret = FP_RESULT_NeedRelease;
    }
    else if ( result_curr == FINGER_UP )
    {
        ret = FP_RESULT_NoFinger;
        result_last = result_curr;
    }
    else
    {
        ret = FP_RESULT_ErrOther;
        result_last = 0;
    }
    enable_irq(irq);

    return ret;
}

// note this function only exec one enroll step
FP_RESULT fingerprint_enroll(uint8_t counter, uint8_t repeat_detect_num, uint8_t overlap_detect_threshold)
{
    // get image
    FP_EC_FP_RESULT(fingerprint_get_image());

    // enroll
    // 3rd arg = 0 to disable registred finger detection
    FPS_FailCode ret = SL_Enroll(fp_buff_img, counter, repeat_detect_num, overlap_detect_threshold);
    DBG_PRINTF("SL_Enroll ret -> 0x%02x (%u)\n", ret, ret);
    switch ( ret )
    {
    case FPS_LOCK_REGISTER_MOVE_SLOW:
    case FPS_LOCK_REGISTER_MOVE_LARGE:
    case FPS_LOCK_REGISTER_LOW_QUALITY:
        return FP_RESULT_NeedsMove;

    case FPS_TEMPLATE_ALREADY_EXIST:
        return FP_RESULT_Duplicate;

    case FPS_LESSEFATUREPOINT:
        return FP_RESULT_InsufficientFeature;

    case FPS_MAX_ENROLL:
        return FP_RESULT_SlotFull;

    case FPS_LOCK_REGISTER_OK:
    case FPS_SUCCESS:
        break;

    default:
        return FP_RESULT_ErrOther;
    }

    return FP_RESULT_OK;
}

FP_RESULT fingerprint_match(uint8_t* match_id, bool single_target)
{
    // get image
    FP_EC_FP_RESULT(fingerprint_get_image());

    FPS_FailCode ret;

    // feature
    ret = SL_GetFeature(fp_buff_img);
    DBG_PRINTF("SL_GetFeature ret -> 0x%02x (%u)\n", ret, ret);

    // match checks
    uint8_t reco_score = 0;
    uint8_t reco_learn = 0;
    uint8_t match_id_slg = 0;
    ret = SL_Match(&reco_score, &reco_learn, &match_id_slg);
    DBG_PRINTF("SL_Match ret -> 0x%02x (%u)\n", ret, ret);
    switch ( ret )
    {
    default:
        return FP_RESULT_ErrOther;
    case FPS_TEMPLETMISMATCH:
    case FPS_IMAGEBLURRING:
    case FPS_AREASMALL:
        return FP_RESULT_NoMatch;
    case FPS_TEMPLATE_NULL:
        return FP_RESULT_ErrState;
    case FPS_SUCCESS:
        if ( reco_score <= 0 )
            return FP_RESULT_NoMatch;
        break;
    }

    // learn if needed
    if ( reco_learn )
    {
        DBG_PRINTF("SL_UpDate triggered\n");
        SL_UpDate(match_id_slg);
    }

    // check fingger id
    if ( single_target && (match_id_slg != *match_id) )
    {
        return FP_RESULT_SlotMissmatch;
    }
    else
    {
        *match_id = match_id_slg;
        return FP_RESULT_Match;
    }
}

FP_RESULT fingerprint_save(uint8_t* id)
{
    DBG_PRINTF("SL_SaveTemp triggered\n");
    SL_SaveTemp(id);
    return FP_RESULT_OK;
}

FP_RESULT fingerprint_delete(uint8_t id)
{
    SL_DellChar(id);
    return FP_RESULT_OK;
}

FP_RESULT fingerprint_delete_all(void)
{
    SL_DeleteAll();
    return FP_RESULT_OK;
}

void fingerprint_wipe_storage(void)
{
    DATA_FLASH_Wipe();
}

FP_RESULT fingerprint_get_count(uint8_t* count)
{
    FPS_IdRead_return_struct fps_ids = SL_IdRead();
    *count = fps_ids.ExistIdNum;
    return FP_RESULT_OK;
}

FP_RESULT fingerprint_get_list(uint8_t* list, uint8_t len)
{
    FPS_IdRead_return_struct fps_ids = SL_IdRead();

    // clear
    memset(list, 0, len);

    // convert to bit group
    for ( uint8_t i = 0; (i < FP_MAX_USER_COUNT && i < len * 8); i++ )
    {
        list[i] = fps_ids.ExistId[i];
    }

    return FP_RESULT_OK;
}

void fp_test_getImage()
{
    FP_RESULT fp_result = FP_RESULT_ErrOther;

    display_fp(DISPLAY_RESX - IMG_WIDTH, DISPLAY_RESY - IMG_HEIGHT, IMG_WIDTH, IMG_HEIGHT, fp_buff_img);
    fp_result = CaptureGSL61xx(fp_buff_img);
    // UNUSED(fp_result);
    DBG_PRINTF("fp_result -> %u\n", fp_result);
}

#endif