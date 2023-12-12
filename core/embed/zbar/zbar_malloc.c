
#include "zbar_malloc.h"
#include "py/runtime.h"
#include "py/objtype.h"

static int counter = 0;


void *zbar_malloc(size_t size) {
    void *ptr = pvPortMalloc(size);
    if (ptr == NULL) {
        mp_raise_msg(&mp_type_MemoryError, "out of heap memory");
    }
    return ptr;
}

void zbar_free(void *ptr) {    
  vPortFree(ptr);
}

void *zbar_realloc(void *ptr, size_t size) {

    void *ptr2 = pvPortReMalloc(ptr, size);
    if (ptr2 == NULL) {
        mp_raise_msg(&mp_type_MemoryError, "out of heap memory");
    }
    return ptr2;
}

void *zbar_calloc(size_t nmemb, size_t size) {

    void *ptr = pvPortCalloc(nmemb , size);
    if (ptr == NULL) {
        mp_raise_msg(&mp_type_MemoryError, "out of heap memory");
    }
    return ptr;
}
