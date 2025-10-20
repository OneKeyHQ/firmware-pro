#ifndef _fingerprint_H_
#define _fingerprint_H_

#include "util_macros.h"
#include "secbool.h"

#define FP_ENUM_ITEM(CLASS, TYPE) JOIN_EXPR(FP, CLASS, TYPE)
// regex -> ^(\s*)FP_ENUM_ITEM\((.*), (.*)\)(.*),.*
// replace -> $1FP_ENUM_ITEM($2, $3)$4, // FP_$2_$3

#define FP_EC_FP_RESULT(expr) ExecuteCheck_ADV(expr, FP_RESULT_OK, { return ret; })

typedef enum
{
    // general
    FP_ENUM_ITEM(RESULT, OK) = 0,   // FP_RESULT_OK
    FP_ENUM_ITEM(RESULT, ErrOther), // FP_RESULT_ErrOther
    FP_ENUM_ITEM(RESULT, ErrState), // FP_RESULT_ErrState

    // fp
    FP_ENUM_ITEM(RESULT, NoFinger),    // FP_RESULT_NoFinger
    FP_ENUM_ITEM(RESULT, NeedRelease), // FP_RESULT_NeedRelease

    // enroll
    FP_ENUM_ITEM(RESULT, NeedsMove),           // FP_RESULT_NeedsMove
    FP_ENUM_ITEM(RESULT, Duplicate),           // FP_RESULT_Duplicate
    FP_ENUM_ITEM(RESULT, InsufficientFeature), // FP_RESULT_InsufficientFeature
    FP_ENUM_ITEM(RESULT, SlotFull),            // FP_RESULT_SlotFull

    // verify
    FP_ENUM_ITEM(RESULT, Match) = sectrue,             // FP_RESULT_Match
    FP_ENUM_ITEM(RESULT, SlotMissmatch) = sectrue + 1, // FP_RESULT_SlotMissmatch
    FP_ENUM_ITEM(RESULT, NoMatch) = secfalse,          // FP_RESULT_NoMatch

} FP_RESULT;

bool fingerprint_module_status_get(void);

FP_RESULT fingerprint_init(void);

FP_RESULT fingerprint_get_version(char* version);

FP_RESULT fingerprint_get_image(void);
FP_RESULT fingerprint_match(uint8_t* match_id, bool single_target);
FP_RESULT fingerprint_enroll(uint8_t counter, uint8_t repeat_detect_num, uint8_t overlap_detect_threshold);

FP_RESULT fingerprint_save(uint8_t *id);
FP_RESULT fingerprint_delete(uint8_t id);
FP_RESULT fingerprint_delete_all(void);
void fingerprint_wipe_storage(void);

FP_RESULT fingerprint_get_count(uint8_t* count);
FP_RESULT fingerprint_get_list(uint8_t* list, uint8_t len);

FP_RESULT fingerprint_enter_sleep(void);

void fp_test_getImage(void);
void fp_test_enroll(void);
void fp_test_verify(void);

extern uint8_t* fp_buff_img;
extern uint8_t* fp_buff_img_raw;

// void fingerprint_test(void);
// void fp_test(void);

#endif // _fingerprint_H_
