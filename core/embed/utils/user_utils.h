#ifndef _USER_UTILS_H
#define _USER_UTILS_H

#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#define CLEAR_ARRAY(array)                      memset(array, 0, sizeof(array))
#define CLEAR_OBJECT(obj)                       memset(&obj, 0, sizeof(obj))
#define VALUE_CHECK(value, expect)              {if (value != expect) {printf("input err!\r\n"); return; }}

#define PRINT_LINE()                            printf("line=%d\n", __LINE__)

#ifndef UNUSED
#define UNUSED(x)   (void)x
#endif

#define GET_ALIGN(size, base)                   (((size) + ((base) - 1)) & ~((base) - 1))

#define MAX_VALUE_2( a, b )    ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )

#define LIMIT_MAX(value, max)                   do { if ((value) > (max)) { (value) = (max); } } while(0)
#define LIMIT_ZONE(value, max, min)     \
    do {                                \
        if ((value) > (max))            \
            (value) = (max);            \
        else if ((value) < (min))       \
            (value) = (min);            \
    } while(0)

#define CHECK_ERRCODE_BREAK(content, ret)       {if (ret != 0) {printf("%s err,0x%X,line=%d\r\n", content, ret, __LINE__); break; }}

void PrintArray(const char *name, const void *data, uint16_t length);
void PrintU16Array(const char *name, const uint16_t *data, uint16_t length);
void PrintU32Array(const char *name, const uint32_t *data, uint16_t length);
uint8_t *StrToHex(const char *str);
bool CheckEntropy(const uint8_t *array, uint32_t len);
bool CheckAllFF(const uint8_t *array, uint32_t len);
bool CheckAllZero(const uint8_t *array, uint32_t len);
void RemoveFormatChar(char *str);
bool ReadAble(const char *str);
bool DifferenceLessThan(uint32_t a, uint32_t b, uint32_t threshold);

#endif

