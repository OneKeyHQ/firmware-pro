#ifndef _fingerprint_H_
#define _fingerprint_H_

typedef enum _FP_RESULT
{
    FP_OK = 0,
    FP_ERROR_OTHER = 1,
    FP_DUPLICATE = 2,
    FP_GET_IMAGE_FAIL = 3,
    FP_EXTRACT_FEATURE_FAIL = 4,
    FP_NO_FP = 5,
    FP_NOT_MATCH = 6,
} FP_RESULT;

void fingerprint_get_version(char* version);
void fingerprint_init(void);
int fingerprint_detect(void);
FP_RESULT fingerprint_enroll(uint8_t counter);
int fingerprint_save(uint8_t id);
FP_RESULT fingerprint_match(uint8_t* match_id);
int fingerprint_delete(uint8_t id);
int fingerprint_delete_all(void);
int fingerprint_get_count(uint8_t* count);
int fingerprint_get_list(uint8_t* list, uint8_t len);
int fingerprint_enter_sleep(void);
void fingerprint_test(void);
void fp_test(void);

#endif // _fingerprint_H_
