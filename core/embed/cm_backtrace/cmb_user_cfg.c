#include "cmb_user_cfg.h"
#include "emmc_fs.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

#define CMB_ERR_INFO_FILE "0:fault_info.txt"

void delete_err_info_file(void) { emmc_fs_file_delete(CMB_ERR_INFO_FILE); }

void cmb_user_println(const char *format, ...) {
  va_list args;
  char str[512] = {0};
  size_t remaining = sizeof(str) - 1;
  bool ret;
  EMMC_PATH_INFO file_info = {0};
  uint32_t offset = 0;

  va_start(args, format);
  int written = vsnprintf(str, remaining, format, args);
  if (written > 0 && written < remaining) {
    strcat(str, "\n");
  }
  ret = emmc_fs_path_info(CMB_ERR_INFO_FILE, &file_info);
  if (ret) {
    if (file_info.path_exist) {
      offset = file_info.size;
    }
    emmc_fs_file_write(CMB_ERR_INFO_FILE, offset, (void *)str, strlen(str),
                       NULL, false, true);
  }
  va_end(args);
}
