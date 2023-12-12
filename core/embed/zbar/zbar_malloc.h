#ifndef _ZBAR_MALLOC_H_
#define _ZBAR_MALLOC_H_

#include <stddef.h>

#include "secure_heap.h"

void *zbar_malloc(size_t size);
void *zbar_realloc(void *ptr, size_t size);
void zbar_free(void *ptr);
void *zbar_calloc(size_t nmemb, size_t size);

#undef malloc
#undef free
#undef realloc
#undef calloc
#define malloc zbar_malloc
#define free zbar_free
#define realloc zbar_realloc
#define calloc zbar_calloc

#endif
