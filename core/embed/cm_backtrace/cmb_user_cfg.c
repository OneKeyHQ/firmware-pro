#include "cmb_user_cfg.h"
#include "stdarg.h"
#include "string.h"
#include "stdio.h"

#include "emmc_fs.h"

void cmb_user_println(const char *format, ...)
{
    va_list args;
    char str[256] = {0};

    va_start(args, format);
    vsprintf(str, format, args);
    strcat(str, "\n");
    emmc_fs_file_write("0:err_info.txt", 0, (void *)str, strlen(str), NULL, false, true);

    va_end(args);
}
