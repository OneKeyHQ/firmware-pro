#ifndef __RTC_H_
#define __RTC_H_

#include <stdbool.h>

void enter_stop_mode(bool restart, uint32_t shutdown_seconds);
void rtc_init(void);
void rtc_disable(void);
void rtc_set_period(uint32_t period);
#endif
