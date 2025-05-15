#ifndef _USER_MEMORY_H
#define _USER_MEMORY_H

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"

void *SramMallocTrack(size_t size, const char *file, int line, const char *func);
void SramFreeTrack(void *p, const char *file, int line, const char *func);
void *SramReallocTrack(void *p, size_t size, const char *file, int line, const char *func);
void *SramMalloc(size_t size);
void SramFree(void *p);

void *SdramMallocTrack(size_t size, const char *file, int line, const char *func);
void SdramFreeTrack(void *p, const char *file, int line, const char *func);
void *SdramReallocTrack(void *p, size_t size, const char *file, int line, const char *func);
void *SdramMalloc(size_t size);
void SdramFree(void *p);


void PrintHeapInfo(void);

#define SRAM_MALLOC(size)           SramMallocTrack(size, __FILE__, __LINE__, __func__)
#define SRAM_FREE(p)                SramFreeTrack(p, __FILE__, __LINE__, __func__)
#define SRAM_REALLOC(p, size)       SramReallocTrack(p, size, __FILE__, __LINE__, __func__)

#define SDRAM_MALLOC(size)          SdramMallocTrack(size, __FILE__, __LINE__, __func__)
#define SDRAM_FREE(p)               SdramFreeTrack(p, __FILE__, __LINE__, __func__)
#define SDRAM_REALLOC(p, size)      SdramReallocTrack(p, size, __FILE__, __LINE__, __func__)

#endif
