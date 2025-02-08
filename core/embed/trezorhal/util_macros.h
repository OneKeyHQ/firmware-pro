#ifndef _UTIL_MACROS_H_
#define _UTIL_MACROS_H_

#define KB(x) ((x)*1024U)
#define MB(x) ((x)*1024U * 1024U)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define FORCE_IGNORE_RETURN(x) \
  { __typeof__(x) __attribute__((unused)) d = (x); }

#define FUN_NO_OPTMIZE __attribute__((optimize("O0")))

#define UNUSED_OBJ(X) ((void)(X))

#define JOIN_EXPR(a, b, c) a##_##b##_##c
// regex -> (JOIN_EXPR\((.*), (.*), (.*)\).*,).*
// replace -> $1 // $2_$3_$4

// from exisiting enum use following (change "xx")
// #define xx_ENUM_ITEM(CLASS, TYPE) JOIN_EXPR(xx, CLASS, TYPE)
// regex -> ^(\s*)(\S*)(.*),
// replace -> $1xx_ENUM_ITEM(CLASS, $2)$3,

#define ENUM_NAME_ARRAY_ITEM(x) [x] = #x

#define ExecuteCheck_ADV(expr, expected_result, on_false) \
  {                                                       \
    typeof(expected_result) ret = (expr);                 \
    if (ret != (expected_result)) {                       \
      on_false                                            \
    }                                                     \
  }

#include <stdbool.h>
#include <stdint.h>
#define EXEC_RETRY(MAX_RETRY, EXPECTED_RET, ON_INIT, ON_LOOP, ON_SUCCESS, \
                   ON_FALSE)                                              \
  {                                                                       \
    typeof(EXPECTED_RET) loop_result;                                     \
    typeof(EXPECTED_RET) loop_exec() { (ON_LOOP); }                       \
    (ON_INIT);                                                            \
    for (uint32_t retry = 0; retry < MAX_RETRY; retry++) {                \
      loop_result = loop_exec();                                          \
      if (loop_result == EXPECTED_RET) break;                             \
    }                                                                     \
    if (loop_result == EXPECTED_RET)                                      \
      (ON_SUCCESS);                                                       \
    else                                                                  \
      (ON_FALSE);                                                         \
  }

#endif  // _UTIL_MACROS_H_