#include "debug_utils.h"

void __debug_print(bool wait_click, const char* file, int line, const char* func, const char* fmt, ...)
{
    display_orientation(0);
    display_backlight(255);
    display_clear();
    display_refresh();
    display_print_color(RGB16(0x69, 0x69, 0x69), COLOR_BLACK);

    display_printf("\n");
    display_printf("=== Debug Info ===\n");

    display_printf("file: %s:%d\n", file, line);
    display_printf("func: %s\n", func);

    display_printf("message:\n");
    va_list va;
    va_start(va, fmt);
    char buf[256] = {0};
    int len = vsnprintf(buf, sizeof(buf), fmt, va);
    display_print(buf, len);
    va_end(va);
    display_printf("\n");

    display_text(8, 784, "Tap to continue ...", -1, FONT_NORMAL, COLOR_WHITE, COLOR_BLACK);
    while ( wait_click && !touch_click() ) {}
}

bool buffer_to_hex_string(const void* buff, size_t buff_len, char* str, size_t str_len, size_t* processed)
{
    const size_t byte_str_len = 2; // "xx"

    if ( (buff_len * byte_str_len) + 1 > str_len )
        return false;

    char* string_p = str;

    for ( size_t i = 0; i < buff_len; i++ )
    {
        snprintf(string_p, byte_str_len + 1, "%02X", *((uint8_t*)(buff) + i));
        string_p += byte_str_len;
        *processed = i + 1;
    }

    return true;
}

// void __print_buffer(void* buff, size_t buff_len, )
// {
//     size_t byte_str_len = buff_len * 3 + 1;
//     char str[byte_str_len];
//     size_t processed = 0;
//     memzero(str, byte_str_len);
//     if ( buffer_to_hex_string(buff, buff_len, str, byte_str_len, &processed) )
//         dbgprintf("buffer=%s\nprocessed=%lu\nbyte_str_len=%lu", str, processed, byte_str_len);
//     else
//         dbgprintf("failed, processed=%lu\n", processed);
// }

void dead_white(void)
{
    display_backlight(255);
    display_bar(0, 0, 480, 800, COLOR_WHITE);
    while ( 1 )
        ;
}

#ifdef RTT
//   #pragma GCC diagnostic ignored "-Wunused-function"
  #include <stdarg.h>
  #include <stdio.h>
  #include <ctype.h>
  #include "SEGGER_RTT.h"

int _DBG_PRINTF(const char* from, char* fmt, ...)
{
    int processed = 0;

    if ( from )
        processed += SEGGER_RTT_printf(0, "%s: ", from);

    va_list args;
    va_start(args, fmt);
    processed += SEGGER_RTT_vprintf(0, fmt, &args);
    va_end(args);

    return processed;
}
// Note:
// "##__VA_ARGS__" is a GCC hack to allow zero arg case, this is not standard

  #define HEXDUMP_BYTES_IN_LINE 32
  #define CHAR_CODE_MAX         0x7E
void _DBG_BUF_DUMP(const char* from, uint8_t* p_data, uint32_t data_len)
{
    char str_bytes[HEXDUMP_BYTES_IN_LINE * (2 + 1) + 1] = {0};
    char str_ascii[HEXDUMP_BYTES_IN_LINE * (1) + 1] = {0};

    uint32_t byte_processed = 0;
    uint32_t batch_index = 0;

    // print all data
    while ( byte_processed < data_len )
    {
        sprintf(str_bytes + (batch_index * (2 + 1)), " %02x", p_data[byte_processed]);
        char c = (char)p_data[byte_processed];
        sprintf(str_ascii + (batch_index * (1)), "%c", ((c <= CHAR_CODE_MAX) && isprint((int)c)) ? c : '.');
        byte_processed++;
        batch_index++;

        if ( batch_index >= HEXDUMP_BYTES_IN_LINE )
        {
            _DBG_PRINTF(from, "=> %s | %s\n", str_bytes, str_ascii);
            batch_index = 0;
        }
    }
    // print last part
    if ( batch_index != 0 )
    {
        for ( uint8_t i = batch_index; i < HEXDUMP_BYTES_IN_LINE; i++ )
        {
            sprintf(str_bytes + (i * (2 + 1)), " ..");
            sprintf(str_ascii + (i * (1)), " ");
        }
        _DBG_PRINTF(from, "-> %s | %s\n", str_bytes, str_ascii);
    }
}

#endif