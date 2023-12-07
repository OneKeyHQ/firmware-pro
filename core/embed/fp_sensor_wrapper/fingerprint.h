#ifndef _fingerprint_H_
#define _fingerprint_H_

void fingerprint_get_version(char* version);
void fingerprint_init(void);
int fingerprint_detect(void);
int fingerprint_enroll(uint8_t counter);
int fingerprint_save(uint8_t id);
int fingerprint_match(uint8_t* match_id);
int fingerprint_delete(uint8_t id);
int fingerprint_delete_all(void);
int fingerprint_get_count(uint8_t* count);
int fingerprint_get_list(uint8_t *list,uint8_t len);
void fingerprint_test(void);
void fp_test(void);

#endif // _fingerprint_H_
