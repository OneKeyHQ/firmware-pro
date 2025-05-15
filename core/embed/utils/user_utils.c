#include "user_utils.h"
#include "stdio.h"
#include "ctype.h"
#include "user_memory.h"

void PrintArray(const char *name, const void *data, uint16_t length)
{
    printf("%s,length=%d\n", name, length);
    for (uint32_t i = 0; i < length; i++) {
        if (i % 32 == 0 && i != 0) {
            printf("\n");
        }
        printf("%02X ", ((uint8_t *)data)[i]);
    }
    printf("\n");
}


void PrintU16Array(const char *name, const uint16_t *data, uint16_t length)
{
    printf("%s,length=%d\n", name, length);
    for (uint32_t i = 0; i < length; i++) {
        if (i % 16 == 0 && i != 0) {
            printf("\n");
        }
        printf("%04X ", data[i]);
    }
    printf("\n");
}


void PrintU32Array(const char *name, const uint32_t *data, uint16_t length)
{
    printf("%s,length=%d\n", name, length);
    for (uint32_t i = 0; i < length; i++) {
        if (i % 8 == 0 && i != 0) {
            printf("\n");
        }
        printf("%8lX ", data[i]);
    }
    printf("\n");
}


uint8_t *StrToHex(const char *str)
{
    // Check if the input string is NULL
    if (str == NULL) {
        return NULL;
    }
    // Check if the string length is even (each byte requires two hex characters)
    size_t len = strlen(str);
    if (len == 0 || len % 2 != 0) {
        return NULL;
    }
    // Allocate memory for the result
    size_t byteLen = len / 2;
    uint8_t *result = (uint8_t *)SRAM_MALLOC(byteLen);
    if (result == NULL) {
        return NULL; // Memory allocation failed
    }
    // Convert the string to a hexadecimal byte array
    for (size_t i = 0; i < byteLen; i++) {
        char highNibble = str[2 * i];
        char lowNibble = str[2 * i + 1];

        // Check if the characters are valid hexadecimal digits
        if (!isxdigit(highNibble) || !isxdigit(lowNibble)) {
            SRAM_FREE(result); // Free allocated memory
            return NULL;  // Invalid hexadecimal character
        }
        // Convert characters to their corresponding hexadecimal values
        uint8_t high = (highNibble <= '9') ? (highNibble - '0') : (tolower(highNibble) - 'a' + 10);
        uint8_t low = (lowNibble <= '9') ? (lowNibble - '0') : (tolower(lowNibble) - 'a' + 10);
        result[i] = (high << 4) | low;
    }

    return result;
}


/// @brief Simply Check the entropy of an array.
/// @param[in] array Array to be checked.
/// @param[in] len Array length.
/// @return True if the array entropy is fine.
bool CheckEntropy(const uint8_t *array, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        if (array[i] != 0 && array[i] != 0xFF) {
            return true;
        }
    }
    return false;
}

/// @brief Check the array if all data are 0xFF.
/// @param[in] array Array to be checked.
/// @param[in] len Array length.
/// @return True if the array data are all 0xFF.
bool CheckAllFF(const uint8_t *array, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        if (array[i] != 0xFF) {
            return false;
        }
    }
    return true;
}

/// @brief Check the array if all data are zero.
/// @param[in] array Array to be checked.
/// @param[in] len Array length.
/// @return True if the array data are all zero.
bool CheckAllZero(const uint8_t *array, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        if (array[i] != 0) {
            return false;
        }
    }
    return true;
}

void RemoveFormatChar(char *str)
{
    char *str_c = str;
    int i, j = 0;
    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] != ' ' && str[i] != '\r' && str[i] != '\n' && str[i] != '\t')
            str_c[j++] = str[i];
    }
    str_c[j] = '\0';
    str = str_c;
}

/// @brief Check the string if it is readable. (All characters are readable).
/// @param str String to be checked.
/// @return True if the string is readable.
bool ReadAble(const char *str)
{
    if (str == NULL || str[0] == '\0') {
        return false;
    }
    size_t len = strlen(str);
    for (uint32_t i = 0; i < len; i++) {
        if (str[i] < 0x20 || str[i] > 0x7E) {
            if (str[i] != '\r' && str[i] != '\n' && str[i] != '\t') {
                return false;
            }
        }
    }
    return true;
}

/// @brief Difference between two numbers is less than a threshold.
/// @param a
/// @param b
/// @param threshold
/// @return True if the difference is less than the threshold.
bool DifferenceLessThan(uint32_t a, uint32_t b, uint32_t threshold)
{
    if (a >= b) {
        return (a - b) < threshold;
    } else {
        return (b - a) < threshold;
    }
}


