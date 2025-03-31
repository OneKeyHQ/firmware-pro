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

#define _GNU_SOURCE

#include "qr-code-generator/qrcodegen.h"

#include "uzlib.h"

#include "common.h"
#include "display.h"
#include "fonts/font_bitmap.h"

#if defined TREZOR_MODEL_T

// TT new rust UI
#if TREZOR_UI2

#ifdef TREZOR_FONT_NORMAL_ENABLE
#include "fonts/font_tthoves_regular_18.h"
#define FONT_NORMAL_DATA Font_TTHoves_Regular_18
#define FONT_NORMAL_HEIGHT 18
#endif
#ifdef TREZOR_FONT_MEDIUM_ENABLE
#include "fonts/font_tthoves_medium_20.h"
#define FONT_MEDIUM_DATA Font_TTHoves_Medium_20
#define FONT_MEDIUM_HEIGHT 20
#endif
#ifdef TREZOR_FONT_BOLD_ENABLE
#include "fonts/font_tthoves_bold_16.h"
#define FONT_BOLD_DATA Font_TTHoves_Bold_16
#define FONT_BOLD_HEIGHT 16
#endif
#ifdef TREZOR_FONT_MONO_ENABLE
#include "fonts/font_robotomono_regular_20.h"
#define FONT_MONO_DATA Font_RobotoMono_Regular_20
#define FONT_MONO_HEIGHT 20
#endif

// TT old python UI
#else

#ifdef TREZOR_FONT_NORMAL_ENABLE
#include "fonts/font_plusjakartasans_regular_26.h"
#define FONT_NORMAL_DATA Font_PlusJakartaSans_Regular_26
#define FONT_NORMAL_HEIGHT 26
#endif
#ifdef TREZOR_FONT_BOLD_ENABLE
#include "fonts/font_roboto_bold_20.h"
#define FONT_BOLD_DATA Font_Roboto_Bold_20
#define FONT_BOLD_HEIGHT 20
#endif
#ifdef TREZOR_FONT_MONO_ENABLE
#include "fonts/font_robotomono_regular_20.h"
#define FONT_MONO_DATA Font_RobotoMono_Regular_20
#define FONT_MONO_HEIGHT 20
#endif
#ifdef TREZOR_FONT_BOLD36_ENABLE
#include "fonts/font_roboto_bold_36.h"
#define FONT_BOLD36_DATA Font_Roboto_Bold_36
#define FONT_BOLD36_HEIGHT 36
#endif
#ifdef TREZOR_FONT_ROBOT_REGULAR_ENABLE
#include "fonts/font_roboto_regular_24.h"
#define FONT_ROBOT_REGULAR_24_DATA Font_Roboto_Regular_24
#define FONT_ROBOT_REGULAR_24_HEIGHT 24
#endif
#ifdef TREZOR_FONT_PJKS_BOLD_ENABLE
#include "fonts/font_plusjakartasans_bold_38.h"
#define FONT_PJKS_BOLD38_DATA Font_PlusJakartaSans_Bold_38
#define FONT_PJKS_BOLD38_HEIGHT 38
#endif
#ifdef TREZOR_FONT_PJKS_REGULAR_20_ENABLE
#include "fonts/font_plusjakartasans_regular_20.h"
#define FONT_PJKS_REGULAR20_DATA Font_PlusJakartaSans_Regular_20
#define FONT_PJKS_REGULAR20_HEIGHT 20
#endif
#ifdef TREZOR_FONT_PJKS_BOLD_26_ENABLE
#include "fonts/font_plusjakartasans_bold_26.h"
#define FONT_PJKS_BOLD26_DATA Font_PlusJakartaSans_Bold_26
#define FONT_PJKS_BOLD26_HEIGHT 26
#endif
#endif

#elif TREZOR_MODEL == 1

#ifdef TREZOR_FONT_NORMAL_ENABLE
#include "fonts/font_pixeloperator_regular_8.h"
#define FONT_NORMAL_DATA Font_PixelOperator_Regular_8
#define FONT_NORMAL_HEIGHT 8
#endif
#ifdef TREZOR_FONT_MEDIUM_ENABLE
#include "fonts/font_pixeloperator_regular_8.h"
#define FONT_MEDIUM_DATA Font_PixelOperator_Regular_8
#define FONT_MEDIUM_HEIGHT 8
#endif
#ifdef TREZOR_FONT_BOLD_ENABLE
#include "fonts/font_pixeloperator_bold_8.h"
#define FONT_BOLD_DATA Font_PixelOperator_Bold_8
#define FONT_BOLD_HEIGHT 8
#endif
#ifdef TREZOR_FONT_MONO_ENABLE
#include "fonts/font_pixeloperatormono_regular_8.h"
#define FONT_MONO_DATA Font_PixelOperatorMono_Regular_8
#define FONT_MONO_HEIGHT 8
#endif

#else
#error Unknown Trezor model
#endif

#include <stdarg.h>
#include <string.h>

#include "memzero.h"

static struct {
  int x, y;
} DISPLAY_OFFSET;

#include "mipi_lcd.h"

// common display functions

static inline uint16_t interpolate_color(uint16_t color0, uint16_t color1,
                                         uint8_t step) {
  uint8_t cr = 0, cg = 0, cb = 0;
  cr = (((color0 & 0xF800) >> 11) * step +
        ((color1 & 0xF800) >> 11) * (15 - step)) /
       15;
  cg = (((color0 & 0x07E0) >> 5) * step +
        ((color1 & 0x07E0) >> 5) * (15 - step)) /
       15;
  cb = ((color0 & 0x001F) * step + (color1 & 0x001F) * (15 - step)) / 15;
  return (cr << 11) | (cg << 5) | cb;
}

static inline void set_color_table(uint16_t colortable[16], uint16_t fgcolor,
                                   uint16_t bgcolor) {
  for (int i = 0; i < 16; i++) {
    colortable[i] = interpolate_color(fgcolor, bgcolor, i);
  }
}

static inline void clamp_coords(int x, int y, int w, int h, int *x0, int *y0,
                                int *x1, int *y1) {
  *x0 = MAX(x, 0);
  *y0 = MAX(y, 0);
  *x1 = MIN(x + w - 1, DISPLAY_RESX - 1);
  *y1 = MIN(y + h - 1, DISPLAY_RESY - 1);
}

void display_clear(void) {
  fb_fill_rect(0, 0, MAX_DISPLAY_RESX, MAX_DISPLAY_RESY, 0x0000);
}

void display_bar(int x, int y, int w, int h, uint16_t c) {
  fb_fill_rect(x, y, w, h, c);
}

static void display_rounded_corner(int x, int y, int r, int delta_x,
                                   int delta_y, uint16_t c, uint16_t b) {
  int r2 = r * r;
  int r2_1 = (r + 1) * (r + 1);
  int r2_2 = (r + 2) * (r + 2);
  for (int i = 0; i < r + 2; ++i) {
    for (int j = 0; j < r + 2; ++j) {
      int d = i * i + j * j;
      if (d <= r2) {
        // fb_write_pixel(x + delta_x * i, y + delta_y * j, c);
      } else if (d <= r2_1) {
        uint16_t color = interpolate_color(c, b, 9);
        fb_write_pixel(x + delta_x * i, y + delta_y * j, color);
      } else if (d <= r2_2) {
        uint16_t color = interpolate_color(c, b, 3);
        fb_write_pixel(x + delta_x * i, y + delta_y * j, color);
      } else {
        fb_write_pixel(x + delta_x * i, y + delta_y * j, b);
      }
    }
  }
}

void display_bar_radius_ex(int x, int y, int w, int h, uint16_t c, uint16_t b,
                           int r) {
  display_bar(x, y, w, h, c);

  uint16_t color = interpolate_color(c, b, 9);

  fb_draw_hline(x + r, y - 1, w - 2 * r, color);
  fb_draw_hline(x + r, y + h, w - 2 * r, color);

  display_rounded_corner(x + r, y + r, r, -1, -1, c, b);
  display_rounded_corner(x + w - r, y + r, r, 1, -1, c, b);
  display_rounded_corner(x + r, y + h - r, r, -1, 1, c, b);
  display_rounded_corner(x + w - r, y + h - r, r, 1, 1, c, b);
}

#define CORNER_RADIUS 16

static const uint8_t cornertable[CORNER_RADIUS * CORNER_RADIUS] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  5,  9,  12, 14, 15, 0,  0,  0,
    0,  0,  0,  0,  0,  3,  9,  15, 15, 15, 15, 15, 15, 0,  0,  0,  0,  0,  0,
    0,  8,  15, 15, 15, 15, 15, 15, 15, 15, 0,  0,  0,  0,  0,  3,  12, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 0,  0,  0,  0,  3,  14, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 0,  0,  0,  3,  14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 0,  0,  0,  12, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 0,  0,
    8,  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 0,  3,  15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 0,  9,  15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 1,  15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 5,  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 9,  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 12,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15,
};

void display_bar_radius(int x, int y, int w, int h, uint16_t c, uint16_t b,
                        uint8_t r) {
  if (r != 2 && r != 4 && r != 8 && r != 16) {
    return;
  } else {
    r = 16 / r;
  }
  uint16_t colortable[16] = {0};
  set_color_table(colortable, c, b);
  x += DISPLAY_OFFSET.x;
  y += DISPLAY_OFFSET.y;
  int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  clamp_coords(x, y, w, h, &x0, &y0, &x1, &y1);
  for (int j = y0; j <= y1; j++) {
    for (int i = x0; i <= x1; i++) {
      int rx = i - x;
      int ry = j - y;
      if (rx < CORNER_RADIUS / r && ry < CORNER_RADIUS / r) {
        uint8_t c = cornertable[rx * r + ry * r * CORNER_RADIUS];
        fb_write_pixel(i, j, colortable[c]);
      } else if (rx < CORNER_RADIUS / r && ry >= h - CORNER_RADIUS / r) {
        uint8_t c = cornertable[rx * r + (h - 1 - ry) * r * CORNER_RADIUS];
        fb_write_pixel(i, j, colortable[c]);
      } else if (rx >= w - CORNER_RADIUS / r && ry < CORNER_RADIUS / r) {
        uint8_t c = cornertable[(w - 1 - rx) * r + ry * r * CORNER_RADIUS];
        fb_write_pixel(i, j, colortable[c]);
      } else if (rx >= w - CORNER_RADIUS / r && ry >= h - CORNER_RADIUS / r) {
        uint8_t c =
            cornertable[(w - 1 - rx) * r + (h - 1 - ry) * r * CORNER_RADIUS];
        fb_write_pixel(i, j, colortable[c]);
      } else {
        fb_write_pixel(i, j, c);
      }
    }
  }
}

#define UZLIB_WINDOW_SIZE (1 << 10)

static void uzlib_prepare(struct uzlib_uncomp *decomp, uint8_t *window,
                          const void *src, uint32_t srcsize, void *dest,
                          uint32_t destsize) {
  memzero(decomp, sizeof(struct uzlib_uncomp));
  if (window) {
    memzero(window, UZLIB_WINDOW_SIZE);
  }
  memzero(dest, destsize);
  decomp->source = (const uint8_t *)src;
  decomp->source_limit = decomp->source + srcsize;
  decomp->dest = (uint8_t *)dest;
  decomp->dest_limit = decomp->dest + destsize;
  uzlib_uncompress_init(decomp, window, window ? UZLIB_WINDOW_SIZE : 0);
}

void display_image(int x, int y, int w, int h, const void *data,
                   uint32_t datalen) {
#if defined TREZOR_MODEL_T
  x += DISPLAY_OFFSET.x;
  y += DISPLAY_OFFSET.y;
  int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  int x_pos = 0, y_pos = 0;
  clamp_coords(x, y, w, h, &x0, &y0, &x1, &y1);
  x_pos = x0;
  y_pos = y0;
  display_set_window(x0, y0, x1, y1);
  x0 -= x;
  x1 -= x;
  y0 -= y;
  y1 -= y;

  struct uzlib_uncomp decomp = {0};
  uint8_t decomp_window[UZLIB_WINDOW_SIZE] = {0};
  uint8_t decomp_out[2] = {0};
  uzlib_prepare(&decomp, decomp_window, data, datalen, decomp_out,
                sizeof(decomp_out));

  for (uint32_t pos = 0; pos < w * h; pos++) {
    int st = uzlib_uncompress(&decomp);
    if (st == TINF_DONE) break;  // all OK
    if (st < 0) break;           // error
    const int px = pos % w;
    const int py = pos / w;
    if (px >= x0 && px <= x1 && py >= y0 && py <= y1) {
      fb_write_pixel(x_pos + px, y_pos + py,
                     (decomp_out[0] << 8) | decomp_out[1]);
    }
    decomp.dest = (uint8_t *)&decomp_out;
  }
#endif
}

#define AVATAR_BORDER_SIZE 4
#define AVATAR_BORDER_LOW                        \
  (AVATAR_IMAGE_SIZE / 2 - AVATAR_BORDER_SIZE) * \
      (AVATAR_IMAGE_SIZE / 2 - AVATAR_BORDER_SIZE)
#define AVATAR_BORDER_HIGH (AVATAR_IMAGE_SIZE / 2) * (AVATAR_IMAGE_SIZE / 2)
#define AVATAR_ANTIALIAS 1

void display_avatar(int x, int y, const void *data, uint32_t datalen,
                    uint16_t fgcolor, uint16_t bgcolor) {
#if defined TREZOR_MODEL_T
  x += DISPLAY_OFFSET.x;
  y += DISPLAY_OFFSET.y;
  int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  int x_pos = 0, y_pos = 0;
  clamp_coords(x, y, AVATAR_IMAGE_SIZE, AVATAR_IMAGE_SIZE, &x0, &y0, &x1, &y1);
  x_pos = x0;
  y_pos = y0;
  display_set_window(x0, y0, x1, y1);
  x0 -= x;
  x1 -= x;
  y0 -= y;
  y1 -= y;

  struct uzlib_uncomp decomp = {0};
  uint8_t decomp_window[UZLIB_WINDOW_SIZE] = {0};
  uint8_t decomp_out[2] = {0};
  uzlib_prepare(&decomp, decomp_window, data, datalen, decomp_out,
                sizeof(decomp_out));

  for (uint32_t pos = 0; pos < AVATAR_IMAGE_SIZE * AVATAR_IMAGE_SIZE; pos++) {
    int st = uzlib_uncompress(&decomp);
    if (st == TINF_DONE) break;  // all OK
    if (st < 0) break;           // error
    const int px = pos % AVATAR_IMAGE_SIZE;
    const int py = pos / AVATAR_IMAGE_SIZE;
    if (px >= x0 && px <= x1 && py >= y0 && py <= y1) {
      int d = (px - AVATAR_IMAGE_SIZE / 2) * (px - AVATAR_IMAGE_SIZE / 2) +
              (py - AVATAR_IMAGE_SIZE / 2) * (py - AVATAR_IMAGE_SIZE / 2);
      if (d < AVATAR_BORDER_LOW) {
        // inside border area
        fb_write_pixel(x_pos + px, y_pos + py,
                       (decomp_out[0] << 8) | decomp_out[1]);
      } else if (d > AVATAR_BORDER_HIGH) {
        // outside border area
        fb_write_pixel(x_pos + px, y_pos + py, bgcolor);
      } else {
        // border area
#if AVATAR_ANTIALIAS
        d = 31 * (d - AVATAR_BORDER_LOW) /
            (AVATAR_BORDER_HIGH - AVATAR_BORDER_LOW);
        uint16_t c = 0;
        if (d >= 16) {
          c = interpolate_color(bgcolor, fgcolor, d - 16);
        } else {
          c = interpolate_color(fgcolor, (decomp_out[0] << 8) | decomp_out[1],
                                d);
        }
        fb_write_pixel(x_pos + px, y_pos + py, c);
#else
        fb_write_pixel(x_pos + px, y_pos + py, fgcolor);
#endif
      }
    }
    decomp.dest = (uint8_t *)&decomp_out;
  }
#endif
}

void display_icon(int x, int y, int w, int h, const void *data,
                  uint32_t datalen, uint16_t fgcolor, uint16_t bgcolor) {
  x += DISPLAY_OFFSET.x;
  y += DISPLAY_OFFSET.y;
  x &= ~1;  // cannot draw at odd coordinate
  int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  clamp_coords(x, y, w, h, &x0, &y0, &x1, &y1);
  int x_pos = 0, y_pos = 0;
  display_set_window(x0, y0, x1, y1);
  x_pos = x0;
  y_pos = y0;
  x0 -= x;
  x1 -= x;
  y0 -= y;
  y1 -= y;

  uint16_t colortable[16] = {0};
  set_color_table(colortable, fgcolor, bgcolor);

  struct uzlib_uncomp decomp = {0};
  uint8_t decomp_window[UZLIB_WINDOW_SIZE] = {0};
  uint8_t decomp_out = 0;
  uzlib_prepare(&decomp, decomp_window, data, datalen, &decomp_out,
                sizeof(decomp_out));

  for (uint32_t pos = 0; pos < w * h / 2; pos++) {
    int st = uzlib_uncompress(&decomp);
    if (st == TINF_DONE) break;  // all OK
    if (st < 0) break;           // error
    const int px = 2 * pos % w;
    const int py = 2 * pos / w;
    if (px >= x0 && px <= x1 && py >= y0 && py <= y1) {
      fb_write_pixel(x_pos + px, y_pos + py, colortable[decomp_out >> 4]);
      fb_write_pixel(x_pos + px + 1, y_pos + py, colortable[decomp_out & 0x0F]);
    }
    decomp.dest = (uint8_t *)&decomp_out;
  }
}

// see docs/misc/toif.md for definition of the TOIF format
bool display_toif_info(const uint8_t *data, uint32_t len, uint16_t *out_w,
                       uint16_t *out_h, bool *out_grayscale) {
  if (len < 12 || memcmp(data, "TOI", 3) != 0) {
    return false;
  }
  bool grayscale = false;
  if (data[3] == 'f') {
    grayscale = false;
  } else if (data[3] == 'g') {
    grayscale = true;
  } else {
    return false;
  }

  uint16_t w = *(uint16_t *)(data + 4);
  uint16_t h = *(uint16_t *)(data + 6);

  uint32_t datalen = *(uint32_t *)(data + 8);
  if (datalen != len - 12) {
    return false;
  }

  if (out_w != NULL && out_h != NULL && out_grayscale != NULL) {
    *out_w = w;
    *out_h = h;
    *out_grayscale = grayscale;
  }
  return true;
}

#if PRODUCTION_MODEL == 'H'

void display_loader(uint16_t progress, bool indeterminate, int yoffset,
                    uint16_t fgcolor, uint16_t bgcolor, const uint8_t *icon,
                    uint32_t iconlen, uint16_t iconfgcolor) {
  display_progress(NULL, progress);
}

#if defined TREZOR_MODEL_T
#include "loader_T.h"
#endif

void display_loader_ex(uint16_t progress, bool indeterminate, int yoffset,
                       uint16_t fgcolor, uint16_t bgcolor, const uint8_t *icon,
                       uint32_t iconlen, uint16_t iconfgcolor) {
#if defined TREZOR_MODEL_T
  uint16_t colortable[16] = {0}, iconcolortable[16] = {0};
  set_color_table(colortable, fgcolor, bgcolor);
  if (icon) {
    set_color_table(iconcolortable, iconfgcolor, bgcolor);
  }
  if ((DISPLAY_RESY / 2 - img_loader_size + yoffset < 0) ||
      (DISPLAY_RESY / 2 + img_loader_size - 1 + yoffset >= DISPLAY_RESY)) {
    return;
  }
  display_set_window(DISPLAY_RESX / 2 - img_loader_size,
                     DISPLAY_RESY / 2 - img_loader_size + yoffset,
                     DISPLAY_RESX / 2 + img_loader_size - 1,
                     DISPLAY_RESY / 2 + img_loader_size - 1 + yoffset);
  int x_pos = DISPLAY_RESX / 2 - img_loader_size;
  int y_pos = DISPLAY_RESY / 2 - img_loader_size + yoffset;
  if (icon && memcmp(icon, "TOIg", 4) == 0 &&
      LOADER_ICON_SIZE == *(uint16_t *)(icon + 4) &&
      LOADER_ICON_SIZE == *(uint16_t *)(icon + 6) &&
      iconlen == 12 + *(uint32_t *)(icon + 8)) {
    uint8_t icondata[(LOADER_ICON_SIZE * LOADER_ICON_SIZE) / 2] = {0};
    memzero(&icondata, sizeof(icondata));
    struct uzlib_uncomp decomp = {0};
    uzlib_prepare(&decomp, NULL, icon + 12, iconlen - 12, icondata,
                  sizeof(icondata));
    uzlib_uncompress(&decomp);
    icon = icondata;
  } else {
    icon = NULL;
  }
  for (int y = 0; y < img_loader_size * 2; y++) {
    for (int x = 0; x < img_loader_size * 2; x++) {
      int mx = x, my = y;
      uint16_t a = 0;
      if ((mx >= img_loader_size) && (my >= img_loader_size)) {
        mx = img_loader_size * 2 - 1 - x;
        my = img_loader_size * 2 - 1 - y;
        a = 499 - (img_loader[my][mx] >> 8);
      } else if (mx >= img_loader_size) {
        mx = img_loader_size * 2 - 1 - x;
        a = img_loader[my][mx] >> 8;
      } else if (my >= img_loader_size) {
        my = img_loader_size * 2 - 1 - y;
        a = 500 + (img_loader[my][mx] >> 8);
      } else {
        a = 999 - (img_loader[my][mx] >> 8);
      }
// inside of circle - draw glyph
#define LOADER_ICON_CORNER_CUT 2
#define LOADER_INDETERMINATE_WIDTH 100
      if (icon &&
          mx + my > (((LOADER_ICON_SIZE / 2) + LOADER_ICON_CORNER_CUT) * 2) &&
          mx >= img_loader_size - (LOADER_ICON_SIZE / 2) &&
          my >= img_loader_size - (LOADER_ICON_SIZE / 2)) {
        int i =
            (x - (img_loader_size - (LOADER_ICON_SIZE / 2))) +
            (y - (img_loader_size - (LOADER_ICON_SIZE / 2))) * LOADER_ICON_SIZE;
        uint8_t c = 0;
        if (i % 2) {
          c = icon[i / 2] & 0x0F;
        } else {
          c = (icon[i / 2] & 0xF0) >> 4;
        }
        fb_write_pixel(x_pos + x, y_pos + y, iconcolortable[c]);
      } else {
        uint8_t c = 0;
        if (indeterminate) {
          uint16_t diff =
              (progress > a) ? (progress - a) : (1000 + progress - a);
          if (diff < LOADER_INDETERMINATE_WIDTH ||
              diff > 1000 - LOADER_INDETERMINATE_WIDTH) {
            c = (img_loader[my][mx] & 0x00F0) >> 4;
          } else {
            c = img_loader[my][mx] & 0x000F;
          }
        } else {
          if (progress > a) {
            c = (img_loader[my][mx] & 0x00F0) >> 4;
          } else {
            c = img_loader[my][mx] & 0x000F;
          }
        }
        fb_write_pixel(x_pos + x, y_pos + y, colortable[c]);
      }
    }
  }

#endif
}
#endif

#ifndef TREZOR_PRINT_DISABLE

static char display_print_buf[DISPLAY_PRINT_ROWS][DISPLAY_PRINT_COLS];
static uint16_t display_print_fgcolor = COLOR_WHITE,
                display_print_bgcolor = COLOR_BLACK;

// set colors for display_print function
void display_print_color(uint16_t fgcolor, uint16_t bgcolor) {
  display_print_fgcolor = fgcolor;
  display_print_bgcolor = bgcolor;
}

static uint8_t row = 0, col = 0;

void display_print_clear(void) {
  memset(display_print_buf, 0x00, sizeof(display_print_buf));
  row = col = 0;
}

// display text using bitmap font
void display_print(const char *text, int textlen) {
  static bool redraw_needed = false;

  if (row == 0) {
    redraw_needed = true;
  }
  if (redraw_needed) {
    display_bar(0, 0, DISPLAY_RESX, DISPLAY_RESY, display_print_bgcolor);
    redraw_needed = false;
  }

  // determine text length if not provided
  if (textlen < 0) {
    textlen = strlen(text);
  }

  // bool is_start = true;
  // int spilt = 0, offset = 0, line = 0;
  static int width = 0;
  int w = 0;

  // print characters to internal buffer (display_print_buf)
  for (int i = 0; i < textlen; i++) {
    switch (text[i]) {
      case '\r':
        // erase height set to (DISPLAY_CHAR_HEIGHT * 2) due to some font char
        // heights are different, which may cause ghosting issue
        display_bar(0, DISPLAY_CHAR_HEIGHT * (row), DISPLAY_RESX,
                    DISPLAY_CHAR_HEIGHT * 2, display_print_bgcolor);
        col = 0;
        width = 0;
        break;
      case '\n':
        row++;
        col = 0;
        width = 0;
        break;
      default:
        w = display_text_width(&text[i], 1, FONT_NORMAL);
        width += w;

        if (width >= DISPLAY_RESX - 8) {
          width = w;
          col = 0;
          row++;
        }
        if (row >= DISPLAY_PRINT_ROWS) {
          display_bar(0, 0, DISPLAY_RESX, DISPLAY_RESY, display_print_bgcolor);
          for (int j = 0; j < DISPLAY_PRINT_ROWS - 1; j++) {
            memcpy(display_print_buf[j], display_print_buf[j + 1],
                   DISPLAY_PRINT_COLS);
          }
          memzero(display_print_buf[DISPLAY_PRINT_ROWS - 1],
                  DISPLAY_PRINT_COLS);
          row = DISPLAY_PRINT_ROWS - 1;
        }
        display_print_buf[row][col] = text[i];
        col++;
        break;
    }
  }

  if (row >= DISPLAY_PRINT_ROWS) {
    display_bar(0, 0, DISPLAY_RESX, DISPLAY_RESY, display_print_bgcolor);
    for (int j = 0; j < DISPLAY_PRINT_ROWS - 1; j++) {
      memcpy(display_print_buf[j], display_print_buf[j + 1],
             DISPLAY_PRINT_COLS);
    }
    memzero(display_print_buf[DISPLAY_PRINT_ROWS - 1], DISPLAY_PRINT_COLS);
    row = DISPLAY_PRINT_ROWS - 1;
  }

  for (int y = 0; y < DISPLAY_PRINT_ROWS; y++) {
    if (display_print_buf[y][0] != 0) {
      display_text(8, (y + 1) * DISPLAY_CHAR_HEIGHT, &display_print_buf[y][0],
                   -1, FONT_NORMAL, display_print_fgcolor,
                   display_print_bgcolor);
    }
  }
}

#ifdef TREZOR_EMULATOR
#define mini_vsnprintf vsnprintf
#include <stdio.h>
#else
#include "mini_printf.h"
#endif

// variadic display_print
void display_printf(const char *fmt, ...) {
  if (!strchr(fmt, '%')) {
    display_print(fmt, strlen(fmt));
  } else {
    va_list va;
    va_start(va, fmt);
    char buf[256] = {0};
    int len = mini_vsnprintf(buf, sizeof(buf), fmt, va);
    display_print(buf, len);
    va_end(va);
  }
}

#endif  // TREZOR_PRINT_DISABLE

static uint8_t convert_char(const uint8_t c) {
  static char last_was_utf8 = 0;

  // non-printable ASCII character
  if (c < ' ') {
    last_was_utf8 = 0;
    return 0x7F;
  }

  // regular ASCII character
  if (c < 0x80) {
    last_was_utf8 = 0;
    return c;
  }

  // UTF-8 handling: https://en.wikipedia.org/wiki/UTF-8#Encoding

  // bytes 11xxxxxx are first bytes of UTF-8 characters
  if (c >= 0xC0) {
    last_was_utf8 = 1;
    return 0x7F;
  }

  if (last_was_utf8) {
    // bytes 10xxxxxx can be successive UTF-8 characters ...
    return 0;  // skip glyph
  } else {
    // ... or they are just non-printable ASCII characters
    return 0x7F;
  }

  return 0;
}

static const uint8_t *get_glyph(int font, uint8_t c) {
  c = convert_char(c);
  if (!c) return 0;

  // printable ASCII character
  if (c >= ' ' && c < 0x7F) {
    switch (font) {
#ifdef TREZOR_FONT_NORMAL_ENABLE
      case FONT_NORMAL:
        return FONT_NORMAL_DATA[c - ' '];
#endif
#ifdef TREZOR_FONT_BOLD_ENABLE
      case FONT_BOLD:
        return FONT_BOLD_DATA[c - ' '];
#endif
#ifdef TREZOR_FONT_MONO_ENABLE
      case FONT_MONO:
        return FONT_MONO_DATA[c - ' '];
#endif
#ifdef TREZOR_FONT_BOLD36_ENABLE
      case FONT_BOLD36:
        return FONT_BOLD36_DATA[c - ' '];
#endif
#ifdef TREZOR_FONT_ROBOT_REGULAR_ENABLE
      case FONT_ROBOT_REGULAR_24:
        return FONT_ROBOT_REGULAR_24_DATA[c - ' '];
#endif
#ifdef TREZOR_FONT_PJKS_BOLD_ENABLE
      case FONT_PJKS_BOLD_38:
        return FONT_PJKS_BOLD38_DATA[c - ' '];
#endif
#ifdef TREZOR_FONT_PJKS_REGULAR_20_ENABLE
      case FONT_PJKS_REGULAR_20:
        return FONT_PJKS_REGULAR20_DATA[c - ' '];
#endif
#ifdef TREZOR_FONT_PJKS_BOLD_26_ENABLE
      case FONT_PJKS_BOLD_26:
        return FONT_PJKS_BOLD26_DATA[c - ' '];
#endif
    }
    return 0;
  }

// non-printable character
#define PASTER(s) s##_glyph_nonprintable
#define NONPRINTABLE_GLYPH(s) PASTER(s)

  switch (font) {
#ifdef TREZOR_FONT_NORMAL_ENABLE
    case FONT_NORMAL:
      return NONPRINTABLE_GLYPH(FONT_NORMAL_DATA);
#endif
#ifdef TREZOR_FONT_BOLD_ENABLE
    case FONT_BOLD:
      return NONPRINTABLE_GLYPH(FONT_BOLD_DATA);
#endif
#ifdef TREZOR_FONT_MONO_ENABLE
    case FONT_MONO:
      return NONPRINTABLE_GLYPH(FONT_MONO_DATA);
#endif
#ifdef TREZOR_FONT_BOLD36_ENABLE
    case FONT_BOLD36:
      return NONPRINTABLE_GLYPH(FONT_BOLD36_DATA);
#endif
  }
  return 0;
}

static void display_text_render(int x, int y, const char *text, int textlen,
                                int font, uint16_t fgcolor, uint16_t bgcolor) {
  // determine text length if not provided
  if (textlen < 0) {
    textlen = strlen(text);
  }

  uint16_t colortable[16] = {0};
  set_color_table(colortable, fgcolor, bgcolor);

  // render glyphs
  for (int i = 0; i < textlen; i++) {
    const uint8_t *g = get_glyph(font, (uint8_t)text[i]);
    if (!g) continue;
    const uint8_t w = g[0];      // width
    const uint8_t h = g[1];      // height
    const uint8_t adv = g[2];    // advance
    const uint8_t bearX = g[3];  // bearingX
    const uint8_t bearY = g[4];  // bearingY
    if (w && h) {
      const int sx = x + bearX;
      const int sy = y - bearY;
      int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
      clamp_coords(sx, sy, w, h, &x0, &y0, &x1, &y1);
      display_set_window(x0, y0, x1, y1);
      for (int j = y0; j <= y1; j++) {
        for (int i = x0; i <= x1; i++) {
          const int rx = i - sx;
          const int ry = j - sy;
          const int a = rx + ry * w;
#if TREZOR_FONT_BPP == 1
          const uint8_t c = ((g[5 + a / 8] >> (7 - (a % 8) * 1)) & 0x01) * 15;
#elif TREZOR_FONT_BPP == 2
          const uint8_t c = ((g[5 + a / 4] >> (6 - (a % 4) * 2)) & 0x03) * 5;
#elif TREZOR_FONT_BPP == 4
          const uint8_t c = (g[5 + a / 2] >> (4 - (a % 2) * 4)) & 0x0F;
#elif TREZOR_FONT_BPP == 8
          const uint8_t c = g[5 + a / 1] >> 4;
#else
#error Unsupported TREZOR_FONT_BPP value
#endif
          fb_write_pixel(i, j, colortable[c]);
        }
      }
    }
    x += adv - 1;
  }
}

void display_text(int x, int y, const char *text, int textlen, int font,
                  uint16_t fgcolor, uint16_t bgcolor) {
  x += DISPLAY_OFFSET.x;
  y += DISPLAY_OFFSET.y;
  display_text_render(x, y, text, textlen, font, fgcolor, bgcolor);
}

void display_text_center(int x, int y, const char *text, int textlen, int font,
                         uint16_t fgcolor, uint16_t bgcolor) {
  x += DISPLAY_OFFSET.x;
  y += DISPLAY_OFFSET.y;
  int w = display_text_width(text, textlen, font);
  display_text_render(x - w / 2, y, text, textlen, font, fgcolor, bgcolor);
}

void display_text_right(int x, int y, const char *text, int textlen, int font,
                        uint16_t fgcolor, uint16_t bgcolor) {
  x += DISPLAY_OFFSET.x;
  y += DISPLAY_OFFSET.y;
  int w = display_text_width(text, textlen, font);
  display_text_render(x - w, y, text, textlen, font, fgcolor, bgcolor);
}

// compute the width of the text (in pixels)
int display_text_width(const char *text, int textlen, int font) {
  int width = 0;
  // determine text length if not provided
  if (textlen < 0) {
    textlen = strlen(text);
  }
  for (int i = 0; i < textlen; i++) {
    const uint8_t *g = get_glyph(font, (uint8_t)text[i]);
    if (!g) continue;
    const uint8_t adv = g[2];  // advance
    width += adv - 1;
    /*
    if (i != textlen - 1) {
        const uint8_t adv = g[2]; // advance
        width += adv;
    } else { // last character
        const uint8_t w = g[0]; // width
        const uint8_t bearX = g[3]; // bearingX
        width += (bearX + w);
    }
    */
  }
  return width;
}

// Returns how many characters of the string can be used before exceeding
// the requested width. Tries to avoid breaking words if possible.
int display_text_split(const char *text, int textlen, int font,
                       int requested_width) {
  int width = 0;
  int lastspace = 0;
  // determine text length if not provided
  if (textlen < 0) {
    textlen = strlen(text);
  }
  for (int i = 0; i < textlen; i++) {
    if (text[i] == ' ') {
      lastspace = i;
    }
    const uint8_t *g = get_glyph(font, (uint8_t)text[i]);
    if (!g) continue;
    const uint8_t adv = g[2];  // advance
    width += adv - 1;
    if (width > requested_width) {
      if (lastspace > 0) {
        return lastspace;
      } else {
        return i;
      }
    }
  }
  return textlen;
}

void display_text_printf(int x, int y, const char *fmt, ...) {
  if (!strchr(fmt, '%')) {
    display_text(x, y, fmt, strlen(fmt), FONT_NORMAL, display_print_fgcolor,
                 display_print_bgcolor);
  } else {
    va_list va;
    va_start(va, fmt);
    char buf[256] = {0};
    int len = mini_vsnprintf(buf, sizeof(buf), fmt, va);
    display_text(x, y, buf, len, FONT_NORMAL, display_print_fgcolor,
                 display_print_bgcolor);
    va_end(va);
  }
}

#define QR_MAX_VERSION 9

void display_qrcode(int x, int y, const char *data, uint32_t datalen,
                    uint8_t scale) {
  if (scale < 1 || scale > 10) return;

  uint8_t codedata[qrcodegen_BUFFER_LEN_FOR_VERSION(QR_MAX_VERSION)] = {0};
  uint8_t tempdata[qrcodegen_BUFFER_LEN_FOR_VERSION(QR_MAX_VERSION)] = {0};

  int side = 0;
  if (qrcodegen_encodeText(data, tempdata, codedata, qrcodegen_Ecc_MEDIUM,
                           qrcodegen_VERSION_MIN, QR_MAX_VERSION,
                           qrcodegen_Mask_AUTO, true)) {
    side = qrcodegen_getSize(codedata);
  }

  x += DISPLAY_OFFSET.x - (side + 2) * scale / 2;
  y += DISPLAY_OFFSET.y - (side + 2) * scale / 2;
  int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  clamp_coords(x, y, (side + 2) * scale, (side + 2) * scale, &x0, &y0, &x1,
               &y1);
  display_set_window(x0, y0, x1, y1);
  for (int j = y0; j <= y1; j++) {
    for (int i = x0; i <= x1; i++) {
      int rx = (i - x) / scale - 1;
      int ry = (j - y) / scale - 1;
      // 1px border
      if (rx < 0 || ry < 0 || rx >= side || ry >= side) {
        fb_write_pixel(i, j, 0xFFFF);
        continue;
      }
      if (qrcodegen_getModule(codedata, rx, ry)) {
        fb_write_pixel(i, j, 0x0000);
      } else {
        fb_write_pixel(i, j, 0xFFFF);
      }
    }
  }
}

void display_offset(int set_xy[2], int *get_x, int *get_y) {
  if (set_xy) {
    DISPLAY_OFFSET.x = set_xy[0];
    DISPLAY_OFFSET.y = set_xy[1];
  }
  *get_x = DISPLAY_OFFSET.x;
  *get_y = DISPLAY_OFFSET.y;
}

void display_fade(int start, int end, int delay) {
  for (int i = 0; i < 100; i++) {
    display_backlight(start + i * (end - start) / 100);
    hal_delay(delay / 100);
  }
  display_backlight(end);
}

#define UTF8_IS_CONT(ch) (((ch)&0xC0) == 0x80)

void display_utf8_substr(const char *buf_start, size_t buf_len, int char_off,
                         int char_len, const char **out_start, int *out_len) {
  size_t i = 0;

  for (; i < buf_len; i++) {
    if (char_off == 0) {
      break;
    }
    if (!UTF8_IS_CONT(buf_start[i])) {
      char_off--;
    }
  }
  size_t i_start = i;

  for (; i < buf_len; i++) {
    if (char_len == 0) {
      break;
    }
    if (!UTF8_IS_CONT(buf_start[i])) {
      char_len--;
    }
  }

  for (; i < buf_len; i++) {
    if (!UTF8_IS_CONT(buf_start[i])) {
      break;
    }
  }

  *out_start = buf_start + i_start;
  *out_len = i - i_start;
}

void display_progress_percent(int x, int y, int permil) {
  volatile char percent_asc[8] = {0};
  int i = 0;
  if (permil < 10) {
    percent_asc[i++] = permil + 0x30;
  } else if (permil < 100) {
    percent_asc[i++] = permil / 10 + 0x30;
    percent_asc[i++] = permil % 10 + 0x30;
  } else {
    permil = 100;
    percent_asc[i++] = permil / 100 + 0x30;
    percent_asc[i++] = permil % 100 / 10 + 0x30;
    percent_asc[i++] = permil % 10 + 0x30;
  }
  percent_asc[i] = '%';
#ifdef TREZOR_FONT_PJKS_REGULAR_20_ENABLE
  display_bar(0, y - 23, MAX_DISPLAY_RESX, 25, COLOR_BLACK);
  display_text_center(x, y, (const char *)percent_asc, -1, FONT_PJKS_REGULAR_20,
                      COLOR_WHITE, COLOR_BLACK);
#else
  display_bar(0, y - 28, MAX_DISPLAY_RESX, 28, COLOR_BLACK);
  display_text_center(x, y, (const char *)percent_asc, -1, FONT_NORMAL,
                      COLOR_WHITE, COLOR_BLACK);
#endif
}

void _display_progress(uint16_t y, const char *desc, int permil) {
  if (desc) {
    display_text_center(MAX_DISPLAY_RESX / 2, y + 50, desc, -1, FONT_NORMAL,
                        COLOR_WHITE, COLOR_BLACK);
  }
  display_progress_percent(MAX_DISPLAY_RESX / 2, y - 20, permil);
  if (permil == 0) {
    display_bar_radius(12, y, 456, 12, COLOR_PROCESS, COLOR_BLACK, 4);
  }

  uint16_t width = permil * 456 / 100;

  display_bar_radius(12, y, width, 12, COLOR_WHITE, COLOR_PROCESS, 4);
}

void display_progress(const char *desc, int permil) {
  _display_progress(720, desc, permil);
}
