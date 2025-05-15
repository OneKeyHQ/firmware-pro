#include "user_assert.h"
#include "string.h"
#include "stdio.h"
#include "common.h"


static const char* short_file_name(const char* path) {
  const char* file = strrchr(path, '/');
  if (!file) file = strrchr(path, '\\');
  file = file ? file + 1 : path;

  size_t len = strlen(file);
  return (len > 8) ? file + (len - 8) : file;
}

void show_assert(const char* msg, const char* file, int line) {
  char str1[64];
  sprintf(str1, "assert,file=%s,line=%u", short_file_name(file), line);
  error_shutdown(msg, str1, NULL, NULL);
}

