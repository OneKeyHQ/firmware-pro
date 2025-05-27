#ifndef _FINGERPRINT_H_
#define _FINGERPRINT_H_

typedef enum _FP_RESULT {
  FP_OK = 0,
  FP_ERROR_OTHER = 1,
  FP_DUPLICATE = 2,
  FP_GET_IMAGE_FAIL = 3,
  FP_EXTRACT_FEATURE_FAIL = 4,
  FP_NO_FP = 5,
  FP_NOT_MATCH = 6,
} FP_RESULT;

#define fingerprint_enroll(...) 0
#define fingerprint_detect() 1
#define fpsensor_get_max_template_count() 0
#define fingerprint_register_template(...) 0
#define fingerprint_save(...) 0
#define fingerprint_get_group(...)
#define fingerprint_match(...) 0
#define fingerprint_delete(...) 0
#define fingerprint_delete_all() 0
#define fingerprint_get_count(...) 1
#define fingerprint_get_list(...) 0
#define fingerprint_enter_sleep() 0
#define fpsensor_template_cache_clear(...)
#define fpsensor_data_upgrade_prompted()
#define fpsensor_data_upgrade_is_prompted() 0
#define fingerprint_delete_group(...) 0

#endif  // _FINGERPRINT_H_
