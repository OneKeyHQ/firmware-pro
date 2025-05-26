#ifndef TREZORHAL_TIMER_
#define TREZORHAL_TIMER_

#include "stdint.h"

void timer_init(void);
void TestTimerInit(void);
void TestTimerStart(void);
uint32_t TestTimerGetValue(void);

#endif
