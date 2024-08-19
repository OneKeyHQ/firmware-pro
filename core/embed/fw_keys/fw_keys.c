#include "fw_keys.h"

#if PRODUCTION
const uint8_t FW_KEY_M = 4;
const uint8_t FW_KEY_N = 7;
const uint8_t * const FW_KEYS[] = {
    (const uint8_t
    *)"\x15\x4b\x8a\xb2\x61\xcc\x88\x79\x48\x3f\x68\x9a\x2d\x41\x24\x3a\xe7\xdb\xc4\x02\x16\x72\xbb\xd2\x5c\x33\x8a\xe8\x4d\x93\x11\x54",
    (const uint8_t
    *)"\xa9\xe6\x5e\x07\xfe\x6d\x39\xa8\xa8\x4e\x11\xa9\x96\xa0\x28\x3f\x88\x1e\x17\x5c\xba\x60\x2e\xb5\xac\x44\x2f\xb7\x5b\x39\xe8\xe0",
    (const uint8_t
    *)"\x6c\x88\x05\xab\xb2\xdf\x9d\x36\x79\xf1\xd2\x8a\x40\xcd\x99\x03\x99\xb9\x9f\xc3\xee\x4e\x06\x57\xd8\x1d\x38\x1e\xa1\x48\x8a\x12",
    (const uint8_t
    *)"\x3e\xd7\x97\x79\x06\x4d\x56\x57\x1b\x29\xbc\xaa\x73\x4c\xbb\x6d\xb6\x1d\x2e\x62\x65\x66\x62\x8e\xcf\x4c\x89\xe1\xdb\x45\xea\xec",
    (const uint8_t
    *)"\x54\xa4\x06\x33\xbf\xd9\xe6\x0b\x8a\x39\x12\x65\xb2\xe0\x06\x37\x4a\xbe\x63\x1d\x1e\x11\x07\x33\x2b\xca\x56\xbf\x9f\x8c\x5c\x99",
    (const uint8_t
    *)"\x4b\x71\x13\x4f\x18\xe0\x07\x87\xc5\x83\xd4\x07\x42\xcc\x18\x8e\x17\xfc\x85\xad\xe4\xcb\x47\x2d\xae\x5e\xf8\xe0\x69\xf0\xfe\xc5",
    (const uint8_t
    *)"\x2e\xcf\x80\xc8\x2b\x44\x98\x48\xc0\x00\x33\x50\x92\x13\x95\x51\xbf\xe4\x7b\x3c\x73\x17\xb4\x99\x50\xf6\x5e\x1d\x82\x43\x20\x24",
};
#else
const uint8_t FW_KEY_M = 2;
const uint8_t FW_KEY_N = 3;
const uint8_t * const FW_KEYS[] = {
  (const uint8_t *)"\x57\x11\x4f\x0a\xa6\x69\xd2\xf8\x37\xe0\x40\xab\x9b\xb5\x1c\x00\x99\x12\x09\xf8\x4b\xfd\x7b\xf0\xf8\x93\x67\x62\x46\xfb\xa2\x4a",
  (const uint8_t *)"\xdc\xae\x8e\x37\xdf\x5c\x24\x60\x27\xc0\x3a\xa9\x51\xbd\x6e\xc6\xca\xa7\xad\x32\xc1\x66\xb1\xf5\x48\xa4\xef\xcd\x88\xca\x3c\xa5",
  (const uint8_t *)"\x77\x29\x12\xab\x61\xd1\xdc\x4f\x91\x33\x32\x5e\x57\xe1\x46\xab\x9f\xac\x17\xa4\x57\x2c\x6f\xcd\xf3\x55\xf8\x00\x36\x10\x00\x04",
};
#endif