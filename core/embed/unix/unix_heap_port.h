#ifndef __UNIX_HEAP_PORT_H__
#define __UNIX_HEAP_PORT_H__

#include <stdlib.h>

#define pvPortMalloc    malloc
#define pvPortReMalloc  realloc
#define vPortFree       free

#endif /* __UNIX_HEAP_PORT_H__ */