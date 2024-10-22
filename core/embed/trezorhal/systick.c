// clang-format off

/*
 * This file is part of the Trezor project, https://trezor.io/
 *
 * Copyright (c) SatoshiLabs
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "irq.h"
#include "systick.h"

#ifdef RDI
  #include "random_delays.h"
#endif

#include "systemview.h"

extern __IO uint32_t uwTick;

systick_dispatch_t systick_dispatch_table[SYSTICK_DISPATCH_NUM_SLOTS];

void SysTick_Handler(void) {
  SEGGER_SYSVIEW_RecordEnterISR();
  // this is a millisecond tick counter that wraps after approximately
  // 49.71 days = (0xffffffff / (24 * 60 * 60 * 1000))
  uint32_t uw_tick = uwTick + 1;
  uwTick = uw_tick;
#ifdef RDI
    rdi_handler(uw_tick);
#endif
  systick_dispatch_t f = systick_dispatch_table[uw_tick & (SYSTICK_DISPATCH_NUM_SLOTS - 1)];
  if (f != NULL) {
    f(uw_tick);
  }
  SEGGER_SYSVIEW_RecordExitISR();
}


// clang-format off
#define DEMCR                     (*(volatile uint32_t*) (0xE000EDFCuL))   // Debug Exception and Monitor Control Register
#define TRACEENA_BIT              (1uL << 24)                                   // Trace enable bit
#define DWT_CTRL                  (*(volatile uint32_t*) (0xE0001000uL))   // DWT Control Register
#define CYCCNTENA_BIT             (1uL << 0)
#define DWT_CYCCNT                (*(volatile uint32_t*) (0xE0001004uL))
// clang-format on

void dwt_init(void) {
  if ((DEMCR & TRACEENA_BIT) == 0) {
    DEMCR |= TRACEENA_BIT;
  }

  if ((DWT_CTRL & CYCCNTENA_BIT) == 0) {  // Cycle counter not enabled?
    DWT_CTRL |= CYCCNTENA_BIT;            // Enable Cycle counter
  }
  DWT_CYCCNT = 0;
}

// 400MHZ  min 2.5ns,max 10.73s
void dwt_delay_ns(uint32_t delay_ns) {
  DWT_CYCCNT = 0;
  uint32_t start = DWT_CYCCNT;
  uint32_t count = (delay_ns * (SystemCoreClock / 100000000)) / 10;
  while ((DWT_CYCCNT - start) < count)
    ;
}

void dwt_delay_us(uint32_t delay_us) {
  DWT_CYCCNT = 0;
  uint32_t start = DWT_CYCCNT;
  uint32_t count = delay_us * (SystemCoreClock / 1000000);
  while ((DWT_CYCCNT - start) < count)
    ;
}

static void dwt_delay_1ms(void) {
  DWT_CYCCNT = 0;
  uint32_t start = DWT_CYCCNT;
  uint32_t count = SystemCoreClock / 1000;
  while ((DWT_CYCCNT - start) < count)
    ;
}

void dwt_delay_ms(uint32_t delay_ms) {
  for (uint32_t i = 0; i < delay_ms; i++) {
    dwt_delay_1ms();
  }
}

static uint32_t accumulated_time_ms = 0;
static uint32_t last_tick = 0;

void dwt_reset(void) {
  DWT_CYCCNT = 0;
  accumulated_time_ms = 0;
  last_tick = 0;
}

uint32_t dwt_get_cycle(void) { return DWT_CYCCNT; }

uint32_t dwt_get_ms(void) { return accumulated_time_ms; }

bool dwt_is_timeout(uint32_t timeout) {
  uint32_t current = DWT_CYCCNT;
  uint32_t elapsed = (current >= last_tick)
                         ? (current - last_tick)
                         : (0xFFFFFFFF - last_tick + current + 1);
  uint32_t elapsed_time_ms = elapsed / (SystemCoreClock / 1000);
  if (elapsed_time_ms > 0) {
    accumulated_time_ms += elapsed_time_ms;
    last_tick = current;
  }
  return accumulated_time_ms >= timeout;
}
