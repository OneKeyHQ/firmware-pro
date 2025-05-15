#ifndef _USER_ASSERT_H
#define _USER_ASSERT_H

#include "stdint.h"
#include "stdbool.h"

#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT(expr) ((expr) ? (void)0 : show_assert("general", __FILE__, __LINE__))

#ifdef assert
#undef assert
#endif
#define assert(expr) ((expr) ? (void)0 : show_assert("general", __FILE__, __LINE__))


#define SHOW_ASSERT(msg) (show_assert((msg), __FILE__, __LINE__))

void show_assert(const char* msg, const char* file, int line);


#endif
