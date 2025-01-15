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

#include STM32_HAL_H

#include <string.h>

#include "bootui.h"
#include "br_check.h"
#include "device.h"
#include "display.h"
#include "icon_cancel.h"
#include "icon_confirm.h"
#include "icon_done.h"
#include "icon_fail.h"
#include "icon_info.h"
#include "icon_install.h"
#include "icon_logo.h"
#include "icon_safeplace.h"
#include "icon_welcome.h"
#include "icon_wipe.h"
#include "mini_printf.h"
#include "version.h"

#if defined TREZOR_MODEL_T
#include "touch.h"
#elif defined TREZOR_MODEL_R
#include "button.h"
#else
#error Unknown Trezor model
#endif

#if PRODUCTION_MODEL == 'H'
#include "ble.h"
#include "common.h"
#include "flash.h"
#include "icon_onekey.h"
#include "image.h"
#include "se_thd89.h"
#include "thd89_boot.h"
#include "usb.h"

#define BACKLIGHT_NORMAL 150

#define COLOR_BL_BG COLOR_BLACK                  // background
#define COLOR_BL_FG COLOR_WHITE                  // foreground
#define COLOR_BL_FAIL RGB16(0xFF, 0x00, 0x00)    // red
#define COLOR_BL_DANGER RGB16(0xFF, 0x11, 0x00)  // onekey red
#define COLOR_BL_DONE RGB16(0x00, 0xFF, 0x33)    // green
#define COLOR_BL_PROCESS COLOR_PROCESS
#define COLOR_BL_GRAY RGB16(0x99, 0x99, 0x99)          // gray
#define COLOR_BL_DARK RGB16(0x2D, 0x2D, 0x2D)          // gray
#define COLOR_BL_PANEL RGB16(0x1E, 0x1E, 0x1E)         //
#define COLOR_BL_TAGVALUE RGB16(0xB4, 0xB4, 0xB4)      //
#define COLOR_BL_SUBTITLE RGB16(0xD2, 0xD2, 0xD2)      //
#define COLOR_BL_FG_INFO_ICON RGB16(0x96, 0x96, 0x96)  // info icon foreground

#define COLOR_WELCOME_BG COLOR_WHITE  // welcome background
#define COLOR_WELCOME_FG COLOR_BLACK  // welcome foreground

#define STATUS_BAR_HEIGHT 44
#define LOGO_SIZE 128
#define LOGO_OFFSET_X ((DISPLAY_RESX - LOGO_SIZE) / 2)
#define LOGO_OFFSET_Y (STATUS_BAR_HEIGHT + 92)

#define INFO_ICON_SIZE 48
#define INFO_ICON_OFFSET_X (DISPLAY_RESX - 20 - INFO_ICON_SIZE)
#define INFO_ICON_OFFSET_Y (STATUS_BAR_HEIGHT + 16)
#define CLOCK_AREA_EXPAND 10
#define TITLE_OFFSET_Y (STATUS_BAR_HEIGHT + 274)
#define SUBTITLE_OFFSET_Y (STATUS_BAR_HEIGHT + 332)
#define BOARD_OFFSET_X 12
#define BUTTON_LEFT_OFFSET_X 12
#define BUTTON_OFFSET_Y 694
#define BUTTON_RIGHT_OFFSET_X 244
#define BUTTON_HALF_WIDTH 224
#define BUTTON_FULL_WIDTH 456
#define BUTTON_HEIGHT 98
#define BUTTON_RADIUS 49
// common shared functions

static void ui_confirm_cancel_buttons(const char* cancel_text,
                                      const char* confirm_text,
                                      uint16_t cancel_bg_color,
                                      uint16_t confirm_bg_color) {
  display_bar_radius_ex(BUTTON_LEFT_OFFSET_X, BUTTON_OFFSET_Y,
                        BUTTON_HALF_WIDTH, BUTTON_HEIGHT, cancel_bg_color,
                        COLOR_BL_BG, BUTTON_HEIGHT / 2);
  display_text_center(DISPLAY_RESX / 4, 755, cancel_text, -1, FONT_PJKS_BOLD_26,
                      COLOR_BL_FG, cancel_bg_color);
  display_bar_radius_ex(BUTTON_RIGHT_OFFSET_X, BUTTON_OFFSET_Y,
                        BUTTON_HALF_WIDTH, BUTTON_HEIGHT, confirm_bg_color,
                        COLOR_BL_BG, BUTTON_HEIGHT / 2);
  display_text_center(DISPLAY_RESX - DISPLAY_RESX / 4, 755, confirm_text, -1,
                      FONT_PJKS_BOLD_26, COLOR_BL_BG, confirm_bg_color);
}

const char* format_ver(const char* format, uint32_t version) {
  static char ver_str[64];
  mini_snprintf(ver_str, sizeof(ver_str), format, (int)(version & 0xFF),
                (int)((version >> 8) & 0xFF), (int)((version >> 16) & 0xFF)
                // ignore build field (int)((version >> 24) & 0xFF)
  );
  return ver_str;
}

// boot UI

static uint16_t boot_background;
static bool ble_name_show = false;
static int ui_bootloader_page_current = 0;
int get_ui_bootloader_page_current(void) { return ui_bootloader_page_current; }

void ui_logo_onekey(void) {
  display_image(LOGO_OFFSET_X, LOGO_OFFSET_Y, LOGO_SIZE, LOGO_SIZE,
                toi_icon_onekey + 12, sizeof(toi_icon_onekey) - 12);
}

void ui_logo_warning(void) {
  display_image(LOGO_OFFSET_X, LOGO_OFFSET_Y, LOGO_SIZE, LOGO_SIZE,
                toi_warning + 12, sizeof(toi_warning) - 12);
}

void ui_logo_done(void) {
  display_image(LOGO_OFFSET_X, LOGO_OFFSET_Y, LOGO_SIZE, LOGO_SIZE,
                toi_done + 12, sizeof(toi_done) - 12);
}

void ui_screen_boot(const vendor_header* const vhdr,
                    const image_header* const hdr) {
  display_clear();
  const int show_string = ((vhdr->vtrust & VTRUST_STRING) == 0);
  // if ((vhdr->vtrust & VTRUST_RED) == 0) {
  //   boot_background = RGB16(0xFF, 0x00, 0x00);  // red
  // } else {
  //   boot_background = COLOR_BLACK;
  // }

  boot_background = COLOR_BLACK;

  // const uint8_t *vimg = vhdr->vimg;
#if PRODUCTION_MODEL == 'H'
  const uint32_t fw_version = hdr->onekey_version;
#else
  const uint32_t fw_version = hdr->version;
#endif

  display_bar(0, 0, DISPLAY_RESX, DISPLAY_RESY, boot_background);

#if PRODUCTION_MODEL == 'H'
  // int image_top = show_string ? 128 : (DISPLAY_RESY - 120) / 2;
#else
  int image_top = show_string ? 30 : (DISPLAY_RESY - 120) / 2;
#endif

  // check whether vendor image
  // if (memcmp(vimg, "TOIf", 4) == 0) {
  //   uint16_t width = vimg[4] + (vimg[5] << 8);
  //   uint16_t height = vimg[6] + (vimg[7] << 8);
  //   uint32_t datalen = *(uint32_t *)(vimg + 8);
  //   display_image((DISPLAY_RESX - width) / 2, image_top, width, height,
  //                 vimg + 12, datalen);
  // }

  if (show_string) {
#if PRODUCTION_MODEL == 'H'
    display_text(8, 96, vhdr->vstr, vhdr->vstr_len, FONT_NORMAL, COLOR_BL_FG,
                 boot_background);
    const char* ver_str = format_ver("v%d.%d.%d", fw_version);
    display_text(8, 140, ver_str, -1, FONT_NORMAL, COLOR_BL_FG,
                 boot_background);
#else
    display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY - 5 - 50, vhdr->vstr,
                        vhdr->vstr_len, FONT_NORMAL, COLOR_BL_BG,
                        boot_background);
    const char* ver_str = format_ver("%d.%d.%d", fw_version);
    display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY - 5 - 25, ver_str, -1,
                        FONT_NORMAL, COLOR_BL_BG, boot_background);
#endif
  }
}

void ui_screen_boot_wait(int wait_seconds) {
  char wait_str[32];
  mini_snprintf(wait_str, sizeof(wait_str), "Starting in %d s", wait_seconds);
  display_bar(0, DISPLAY_RESY - 5 - 20, DISPLAY_RESX, 5 + 20, boot_background);
#if PRODUCTION_MODEL == 'H'
  ui_statusbar_update();
  display_bar(0, 600, DISPLAY_RESX, 100, boot_background);
  display_text_center(DISPLAY_RESX / 2, 655, wait_str, -1, FONT_NORMAL,
                      COLOR_BL_FG, boot_background);
#else
  display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY - 5, wait_str, -1,
                      FONT_NORMAL, COLOR_BL_BG, boot_background);
#endif
}

void ui_screen_boot_click(void) {
  display_bar(0, DISPLAY_RESY - 5 - 20, DISPLAY_RESX, 5 + 20, boot_background);
#if PRODUCTION_MODEL == 'H'
  display_bar(0, 784, DISPLAY_RESX, 40, boot_background);
  display_text(8, 784, "Tap to continue ...", -1, FONT_NORMAL, COLOR_BL_FG,
               boot_background);
#else
  display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY - 5,
                      "click to continue ...", -1, FONT_NORMAL, COLOR_BL_BG,
                      boot_background);
#endif
}

// info UI
static int display_vendor_string(const char* text, int textlen,
                                 uint16_t fgcolor) {
  int split = display_text_split(text, textlen, FONT_NORMAL, DISPLAY_RESX - 55);
  if (split >= textlen) {
    display_text(55, 95, text, textlen, FONT_NORMAL, fgcolor, COLOR_BL_BG);
    return 120;
  } else {
    display_text(55, 95, text, split, FONT_NORMAL, fgcolor, COLOR_BL_BG);
    if (text[split] == ' ') {
      split++;
    }
    display_text(55, 120, text + split, textlen - split, FONT_NORMAL, fgcolor,
                 COLOR_BL_BG);
    return 145;
  }
}

void ui_screen_firmware_info(const vendor_header* const vhdr,
                             const image_header* const hdr) {
  display_clear();
  const char* ver_str = format_ver("Bootloader %d.%d.%d", VERSION_UINT32);
  display_text(16, 32, ver_str, -1, FONT_NORMAL, COLOR_BL_FG, COLOR_BL_BG);
  display_bar(16, 44, DISPLAY_RESX - 14 * 2, 1, COLOR_BL_BG);
  display_icon(16, 54, 32, 32, toi_icon_info + 12, sizeof(toi_icon_info) - 12,
               COLOR_BL_GRAY, COLOR_BL_BG);
  if (vhdr && hdr) {
#if PRODUCTION_MODEL == 'H'
    ver_str = format_ver("Firmware %d.%d.%d by", (hdr->onekey_version));
#else
    ver_str = format_ver("Firmware %d.%d.%d by", (hdr->version));
#endif
    display_text(55, 70, ver_str, -1, FONT_NORMAL, COLOR_BL_GRAY, COLOR_BL_BG);
    display_vendor_string(vhdr->vstr, vhdr->vstr_len, COLOR_BL_GRAY);
  } else {
    display_text(55, 70, "No Firmware", -1, FONT_NORMAL, COLOR_BL_GRAY,
                 COLOR_BL_BG);
  }
  display_text_center(120, 220, "Go to onekey.so", -1, FONT_NORMAL, COLOR_BL_FG,
                      COLOR_BL_BG);
}

void ui_screen_firmware_fingerprint(const image_header* const hdr) {
  display_clear();
  display_text(16, 32, "Firmware fingerprint", -1, FONT_NORMAL, COLOR_BL_FG,
               COLOR_BL_BG);
  display_bar(16, 44, DISPLAY_RESX - 14 * 2, 1, COLOR_BL_BG);

  static const char* hexdigits = "0123456789abcdef";
  char fingerprint_str[64];
  for (int i = 0; i < 32; i++) {
    fingerprint_str[i * 2] = hexdigits[(hdr->fingerprint[i] >> 4) & 0xF];
    fingerprint_str[i * 2 + 1] = hexdigits[hdr->fingerprint[i] & 0xF];
  }
  for (int i = 0; i < 4; i++) {
    display_text_center(120, 70 + i * 25, fingerprint_str + i * 16, 16,
                        FONT_NORMAL, COLOR_BL_FG, COLOR_BL_BG);
  }

  display_bar_radius(9, 184, 222, 50, COLOR_BL_DONE, COLOR_BL_BG, 4);
  display_icon(9 + (222 - 19) / 2, 184 + (50 - 16) / 2, 20, 16,
               toi_icon_confirm + 12, sizeof(toi_icon_confirm) - 12,
               COLOR_BL_BG, COLOR_BL_DONE);
}

// install UI

void ui_screen_install_confirm_newvendor_or_downgrade_wipe(
    const vendor_header* const vhdr, const image_header* const hdr,
    secbool downgrade_wipe) {
  vendor_header current_vhdr;
  image_header current_hdr;
  // char str[128] = {0};
  if (sectrue == load_vendor_header((const uint8_t*)FIRMWARE_START, FW_KEY_M,
                                    FW_KEY_N, FW_KEYS, &current_vhdr)) {
    if (sectrue ==
        load_image_header((const uint8_t*)FIRMWARE_START + current_vhdr.hdrlen,
                          FIRMWARE_IMAGE_MAGIC, FIRMWARE_IMAGE_MAXSIZE,
                          current_vhdr.vsig_m, current_vhdr.vsig_n,
                          current_vhdr.vpub, &current_hdr)) {
    }
  }

  display_clear();
  ui_statusbar_update();
  ui_logo_warning();

  display_text_center(
      MAX_DISPLAY_RESX / 2, TITLE_OFFSET_Y,
      (sectrue == downgrade_wipe) ? "Firmware Downgrade" : "Vendor Change", -1,
      FONT_PJKS_BOLD_38, COLOR_BL_FG, COLOR_BL_BG);
  display_text_center(DISPLAY_RESX / 2, SUBTITLE_OFFSET_Y,
                      "Still installing this firmware?", -1, FONT_NORMAL,
                      COLOR_BL_SUBTITLE, COLOR_BL_BG);
  // strlcat(str, "Install firmware by ", sizeof(str));
  // strlcat(str, vhdr->vstr, sizeof(str));

  // int split = 0, offset = 0, loop = 0;
  // do {
  //   split = display_text_split(str + offset, -1, FONT_NORMAL,
  //   MAX_DISPLAY_RESX); display_text_center(MAX_DISPLAY_RESX / 2, 240 + loop *
  //   28, str + offset,
  //                       split, FONT_NORMAL, COLOR_BL_SUBTITLE, COLOR_BL_BG);
  //   loop++;
  //   offset += split;
  // } while (split);
  display_bar_radius_ex(BOARD_OFFSET_X, 295, BUTTON_FULL_WIDTH, BUTTON_HEIGHT,
                        COLOR_BL_PANEL, COLOR_BL_BG, BUTTON_RADIUS);
  const char* ver_str = format_ver("%d.%d.%d", current_hdr.onekey_version);
  display_text_right(MAX_DISPLAY_RESX / 2 - 25, 350, ver_str, -1, FONT_NORMAL,
                     COLOR_BL_SUBTITLE, COLOR_BL_DARK);
  ver_str = format_ver("%d.%d.%d", hdr->onekey_version);

  display_text(MAX_DISPLAY_RESX / 2 + 25, 350, ver_str, -1, FONT_NORMAL,
               COLOR_BL_SUBTITLE, COLOR_BL_DARK);

  display_image(227, 330, 25, 23, toi_icon_arrow_right + 12,
                sizeof(toi_icon_arrow_right) - 12);

  ui_confirm_cancel_buttons("Cancel", "Install", COLOR_BL_DARK, COLOR_BL_FAIL);
}

void ui_screen_confirm(char* title, char* note_l1, char* note_l2, char* note_l3,
                       char* note_l4) {
  if (title != NULL)
    display_text_center(DISPLAY_RESX / 2, TITLE_OFFSET_Y, title, -1,
                        FONT_PJKS_BOLD_38, COLOR_BL_FG, COLOR_BL_BG);

  if (note_l1 != NULL) {
    display_text_center(DISPLAY_RESX / 2, SUBTITLE_OFFSET_Y, note_l1, -1,
                        FONT_NORMAL, COLOR_BL_SUBTITLE, COLOR_BL_BG);
  }
  if (note_l2 != NULL) {
    display_text_center(DISPLAY_RESX / 2, SUBTITLE_OFFSET_Y + 30, note_l2, -1,
                        FONT_NORMAL, COLOR_BL_SUBTITLE, COLOR_BL_BG);
  }
  if (note_l3 != NULL) {
    display_text_center(DISPLAY_RESX / 2, SUBTITLE_OFFSET_Y + 60, note_l3, -1,
                        FONT_NORMAL, COLOR_BL_SUBTITLE, COLOR_BL_BG);
  }
  if (note_l4 != NULL) {
    display_text_center(DISPLAY_RESX / 2, SUBTITLE_OFFSET_Y + 90, note_l4, -1,
                        FONT_NORMAL, COLOR_BL_SUBTITLE, COLOR_BL_BG);
  }

  ui_confirm_cancel_buttons("Back", "OK", COLOR_BL_DARK, COLOR_BL_FAIL);
}

void ui_screen_progress_bar_prepare(char* title, char* notes) {
  ui_statusbar_update();
  ui_logo_onekey();
  ui_screen_progress_bar_update(title, notes, -1);
}

void ui_screen_progress_bar_update(char* title, char* notes, int progress) {
  if (title != NULL)
    display_text_center(DISPLAY_RESX / 2, TITLE_OFFSET_Y, title, -1,
                        FONT_PJKS_BOLD_38, COLOR_BL_FG, COLOR_BL_BG);

  if ((progress >= 0) && (progress <= 100)) {
    if (progress > 0) {
      display_progress(NULL, progress * 10);
    }
  } else {
    display_progress(notes ? notes : "Keep connected.", 0);
  }
}

void ui_screen_install_start(void) {
  ui_statusbar_update();
  ui_logo_onekey();
  display_text_center(DISPLAY_RESX / 2, TITLE_OFFSET_Y, "Installing", -1,
                      FONT_PJKS_BOLD_38, COLOR_BL_FG, COLOR_BL_BG);

  display_progress("Keep connected.", 0);
}

void ui_screen_install_progress_erase(int pos, int len) {
  ui_statusbar_update();
  ui_logo_onekey();
  display_text_center(DISPLAY_RESX / 2, TITLE_OFFSET_Y, "Installing", -1,
                      FONT_PJKS_BOLD_38, COLOR_BL_FG, COLOR_BL_BG);

  display_progress("Keep connected.", 250 * pos / len);
}

void ui_screen_install_progress_upload(int pos) { display_progress(NULL, pos); }

// wipe UI

void ui_screen_wipe_confirm(void) {
  display_clear();
  display_text(16, 32, "Wipe device", -1, FONT_NORMAL, COLOR_BL_FG,
               COLOR_BL_BG);
  display_bar(16, 44, DISPLAY_RESX - 14 * 2, 1, COLOR_BL_BG);
  display_icon(16, 54, 32, 32, toi_icon_info + 12, sizeof(toi_icon_info) - 12,
               COLOR_BL_FG, COLOR_BL_BG);
  display_text(55, 70, "Do you want to", -1, FONT_NORMAL, COLOR_BL_FG,
               COLOR_BL_BG);
  display_text(55, 95, "wipe the device?", -1, FONT_NORMAL, COLOR_BL_FG,
               COLOR_BL_BG);

  display_text_center(120, 170, "Seed will be erased!", -1, FONT_NORMAL,
                      COLOR_BL_FAIL, COLOR_BL_BG);
  ui_confirm_cancel_buttons("Cancel", "Wipe", COLOR_BL_DARK, COLOR_BL_FAIL);
}

void ui_screen_wipe(void) {
  display_clear();
  ui_statusbar_update();
  ui_logo_warning();
  display_text_center(DISPLAY_RESX / 2, TITLE_OFFSET_Y, "Wipe Device", -1,
                      FONT_PJKS_BOLD_38, COLOR_BL_FG, COLOR_BL_BG);

  display_progress("Wiping device...", 0);
}

void ui_screen_wipe_progress(int pos, int len) {
  display_progress(NULL, 1000 * pos / len);
}

void ui_screen_wipe_done(void) {
  display_clear();
  ui_statusbar_update();
  ui_logo_done();
  display_text_center(DISPLAY_RESX / 2, TITLE_OFFSET_Y, "Wipe Done", -1,
                      FONT_PJKS_BOLD_38, COLOR_BL_FG, COLOR_BL_BG);

  display_text_center(DISPLAY_RESX / 2, SUBTITLE_OFFSET_Y,
                      "Click the button below to restart.", -1, FONT_NORMAL,
                      COLOR_BL_SUBTITLE, COLOR_BL_BG);
  display_bar_radius_ex(BUTTON_LEFT_OFFSET_X, BUTTON_OFFSET_Y,
                        BUTTON_FULL_WIDTH, BUTTON_HEIGHT, COLOR_BL_DARK,
                        COLOR_BL_BG, BUTTON_HEIGHT / 2);
  display_text_center(DISPLAY_RESX / 2, 755, "Restart", -1, FONT_PJKS_BOLD_26,
                      COLOR_BL_FG, COLOR_BL_DARK);
}

// done UI

void ui_screen_success(char* title, char* notes) {
  ui_statusbar_update();
  ui_logo_done();

  if (title != NULL)
    display_text_center(DISPLAY_RESX / 2, TITLE_OFFSET_Y, title, -1,
                        FONT_PJKS_BOLD_38, COLOR_BL_FG, COLOR_BL_BG);

  display_bar(0, DISPLAY_RESY - 78 - 30, DISPLAY_RESX, 30, COLOR_BL_BG);
  display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY - 78, notes, -1,
                      FONT_NORMAL, COLOR_BL_FG, COLOR_BL_BG);
}

void ui_screen_done(int restart_seconds, secbool full_redraw) {
  const char* str;
  char count_str[24];
  if (restart_seconds >= 1) {
    mini_snprintf(count_str, sizeof(count_str), "Done! Restarting in %d s",
                  restart_seconds);
    str = count_str;
  } else {
    str = "Done! Tap to restart ...";
  }
  if (sectrue == full_redraw) {
    display_clear();
    ui_statusbar_update();
    ui_logo_done();
  }
  if (secfalse == full_redraw) {
#if PRODUCTION_MODEL == 'H'
    display_bar(0, DISPLAY_RESY - 24 - 18, 240, 23, COLOR_BL_BG);
#else
    display_bar(0, DISPLAY_RESY - 24 - 18, 240, 23, COLOR_BL_BG);
#endif
  }
#if PRODUCTION_MODEL == 'H'
  display_bar(0, DISPLAY_RESY - 78 - 30, DISPLAY_RESX, 30, COLOR_BL_BG);
  display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY - 78, str, -1, FONT_NORMAL,
                      COLOR_BL_FG, COLOR_BL_BG);
#else
  display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY - 24, str, -1, FONT_NORMAL,
                      COLOR_BL_FG, COLOR_BL_BG);
#endif
}

// error UI

void ui_screen_fail(void) {
#if PRODUCTION_MODEL == 'H'
  display_bar(0, DISPLAY_RESY / 2, DISPLAY_RESX, DISPLAY_RESY, COLOR_BL_BG);
  ui_statusbar_update();
  ui_logo_onekey();
#else
  display_clear();
#endif

#if PRODUCTION_MODEL == 'H'
  display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY - 78,
                      "Failed! Tap to restart and try again.", -1, FONT_NORMAL,
                      COLOR_BL_FAIL, COLOR_BL_BG);
#else
  display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY - 24,
                      "Failed! Please, reconnect.", -1, FONT_NORMAL,
                      COLOR_BL_FG, COLOR_BL_BG);
#endif
}

// general functions

void ui_fadein(void) { display_fade(0, BACKLIGHT_NORMAL, 200); }

void ui_fadeout(void) {
  display_fade(BACKLIGHT_NORMAL, 0, 200);
  display_clear();
}

int ui_user_input(int zones) {
  for (;;) {
    uint32_t evt = touch_click();
    uint16_t x = touch_unpack_x(evt);
    uint16_t y = touch_unpack_y(evt);
    // clicked on Cancel button
    if ((zones & INPUT_CANCEL) && x >= 9 && x < 9 + 108 && y > 184 &&
        y < 184 + 50) {
      return INPUT_CANCEL;
    }
    // clicked on Confirm button
    if ((zones & INPUT_CONFIRM) && x >= 123 && x < 123 + 108 && y > 184 &&
        y < 184 + 50) {
      return INPUT_CONFIRM;
    }
    // clicked on Long Confirm button
    if ((zones & INPUT_LONG_CONFIRM) && x >= 9 && x < 9 + 222 && y > 184 &&
        y < 184 + 50) {
      return INPUT_LONG_CONFIRM;
    }
    // clicked on Info icon
    if ((zones & INPUT_INFO) && x >= 16 && x < 16 + 32 && y > 54 &&
        y < 54 + 32) {
      return INPUT_INFO;
    }
  }
}

int ui_input_poll(int zones, bool poll) {
  do {
    uint32_t evt = touch_click();
    if (evt) {
      hal_delay(50);
      uint16_t x = touch_unpack_x(evt);
      uint16_t y = touch_unpack_y(evt);
      // clicked on Cancel button
      if ((zones & INPUT_CANCEL) && x >= BUTTON_LEFT_OFFSET_X &&
          x < BUTTON_LEFT_OFFSET_X + BUTTON_HALF_WIDTH && y > BUTTON_OFFSET_Y &&
          y < BUTTON_OFFSET_Y + BUTTON_HEIGHT) {
        return INPUT_CANCEL;
      }
      // clicked on Confirm button
      if ((zones & INPUT_CONFIRM) && x >= BUTTON_RIGHT_OFFSET_X &&
          x < BUTTON_RIGHT_OFFSET_X + BUTTON_HALF_WIDTH &&
          y > BUTTON_OFFSET_Y && y < BUTTON_OFFSET_Y + BUTTON_HEIGHT) {
        return INPUT_CONFIRM;
      }
      // clicked on Info icon
      if ((zones & INPUT_INFO) && x >= INFO_ICON_OFFSET_X - CLOCK_AREA_EXPAND &&
          x < INFO_ICON_OFFSET_X + INFO_ICON_SIZE + CLOCK_AREA_EXPAND &&
          y > INFO_ICON_OFFSET_Y - CLOCK_AREA_EXPAND &&
          y < INFO_ICON_OFFSET_Y + INFO_ICON_SIZE + CLOCK_AREA_EXPAND) {
        return INPUT_INFO;
      }
      // clicked on next button
      if ((zones & INPUT_NEXT) && x >= BUTTON_LEFT_OFFSET_X &&
          x < BUTTON_LEFT_OFFSET_X + BUTTON_FULL_WIDTH && y > BUTTON_OFFSET_Y &&
          y < BUTTON_OFFSET_Y + BUTTON_HEIGHT) {
        return (zones & INPUT_NEXT);
      }
      // clicked on previous button
      if ((zones & INPUT_PREVIOUS) && x >= BUTTON_LEFT_OFFSET_X &&
          x < BUTTON_LEFT_OFFSET_X + BUTTON_HALF_WIDTH && y > BUTTON_OFFSET_Y &&
          y < BUTTON_OFFSET_Y + BUTTON_HEIGHT) {
        return (zones & INPUT_PREVIOUS);
      }
      // clicked on restart button
      if ((zones & INPUT_RESTART) && x >= BUTTON_RIGHT_OFFSET_X &&
          x < BUTTON_RIGHT_OFFSET_X + BUTTON_HALF_WIDTH &&
          y > BUTTON_OFFSET_Y && y < BUTTON_OFFSET_Y + BUTTON_HEIGHT) {
        return (zones & INPUT_RESTART);
      }
      // bootloader version
      if ((zones & INPUT_BOOT_VERSION_TEXT) && x >= 0 && x <= 480 && y > 520 &&
          y < 580) {
        return (zones & INPUT_BOOT_VERSION_TEXT);
      }
      // build id
      if ((zones & INPUT_BUILD_ID_TEXT) && x >= 0 && x <= 480 && y > 580 &&
          y < 640) {
        return (zones & INPUT_BUILD_ID_TEXT);
      }
    }
  } while (poll);
  return 0;
}

void ui_statusbar_update(void) {
  char battery_str[8] = {0};
  uint32_t len = 0;
  uint32_t offset_x = 8;
  uint32_t offset_y = 6;
  uint16_t battery_color = COLOR_WHITE;

  ble_get_dev_info();
  display_bar(0, 0, DISPLAY_RESX, 44, boot_background);

  if (dev_pwr_sta == 1) {
    offset_x += 24;
    display_icon(DISPLAY_RESX - offset_x, offset_y, 24, 32,
                 toi_icon_charging + 12, sizeof(toi_icon_charging) - 12,
                 COLOR_BL_FG, boot_background);
    battery_color = RGB16(0x00, 0xCC, 0x36);
  }

  if (battery_cap <= 100) {
    offset_x += 34;
    uint8_t bat_width =
        (battery_cap * 25 / 100 > 0) ? (battery_cap * 25 / 100) : 1;
    display_image(DISPLAY_RESX - offset_x, offset_y, 34, 32,
                  toi_icon_battery + 12, sizeof(toi_icon_battery) - 12);

    if (battery_cap < 20 && dev_pwr_sta != 1) {
      display_bar(DISPLAY_RESX - offset_x + 3, 10 + offset_y, bat_width, 12,
                  RGB16(0xDF, 0x32, 0x0C));
    } else {
      display_bar(DISPLAY_RESX - offset_x + 3, 10 + offset_y, bat_width, 12,
                  battery_color);
    }
  } else {
    display_bar(DISPLAY_RESX - 32, offset_y, 32, 32, boot_background);
  }
  if (battery_cap != 0xFF && dev_pwr_sta == 1) {
    offset_x += 4;
    mini_snprintf(battery_str, sizeof(battery_str), "%d%%", battery_cap);
    len = display_text_width(battery_str, -1, FONT_PJKS_REGULAR_20);
    offset_x += len;
    display_text(DISPLAY_RESX - offset_x, 24 + offset_y, battery_str, -1,
                 FONT_PJKS_REGULAR_20, COLOR_BL_SUBTITLE, boot_background);
  }
  if (ble_connect_state()) {
    offset_x += 32;
    display_icon(DISPLAY_RESX - offset_x, offset_y, 32, 32,
                 toi_icon_bluetooth_connected + 12,
                 sizeof(toi_icon_bluetooth_connected) - 12, COLOR_BL_FG,
                 boot_background);
  } else if (ble_switch_state()) {
    offset_x += 32;
    if (!ble_get_switch()) {
      display_icon(DISPLAY_RESX - offset_x, offset_y, 32, 32,
                   toi_icon_bluetooth_closed + 12,
                   sizeof(toi_icon_bluetooth_closed) - 12, COLOR_BL_FG,
                   boot_background);
    } else {
      display_icon(DISPLAY_RESX - offset_x, offset_y, 32, 32,
                   toi_icon_bluetooth + 12, sizeof(toi_icon_bluetooth) - 12,
                   COLOR_BL_FG, boot_background);
    }
  }

  if (is_usb_connected()) {
    offset_x += 26;
    display_icon(DISPLAY_RESX - offset_x, offset_y, 24, 32, toi_icon_usb + 12,
                 sizeof(toi_icon_usb) - 12, COLOR_BL_FG, boot_background);
  }
}

void ui_wipe_confirm(const image_header* const hdr) {
  ui_statusbar_update();
  ui_logo_warning();
  // if (hdr && (hdr->onekey_version != 0)) {
  //   const char *ver_str = format_ver("v%d.%d.%d", (hdr->onekey_version));
  //   display_text_center(DISPLAY_RESX / 2, 246, ver_str, -1, FONT_NORMAL,
  //                       COLOR_BL_GRAY, COLOR_BL_BG);
  // }
  display_text_center(DISPLAY_RESX / 2, TITLE_OFFSET_Y, "Wipe Device", -1,
                      FONT_PJKS_BOLD_38, COLOR_BL_FG, COLOR_BL_BG);
  display_text_center(DISPLAY_RESX / 2, SUBTITLE_OFFSET_Y,
                      "Do you want to wipe the device?", -1, FONT_NORMAL,
                      COLOR_BL_SUBTITLE, COLOR_BL_BG);
  display_text_center(DISPLAY_RESX / 2, SUBTITLE_OFFSET_Y + 30,
                      "Recovery phrase will be erased", -1, FONT_NORMAL,
                      COLOR_BL_SUBTITLE, COLOR_BL_BG);
  ui_confirm_cancel_buttons("Cancel", "Wipe", COLOR_BL_DARK, COLOR_BL_FAIL);
}

void ui_install_confirm(image_header* current_hdr,
                        const image_header* const new_hdr) {
  if ((current_hdr == NULL) || (new_hdr == NULL)) return;
  ui_statusbar_update();
  ui_logo_onekey();
  display_text_center(DISPLAY_RESX / 2, TITLE_OFFSET_Y, "System Update", -1,
                      FONT_PJKS_BOLD_38, COLOR_BL_FG, COLOR_BL_BG);
  display_text_center(DISPLAY_RESX / 2, SUBTITLE_OFFSET_Y,
                      "Install firmware by OneKey?", -1, FONT_NORMAL,
                      COLOR_BL_SUBTITLE, COLOR_BL_BG);

  display_bar_radius_ex(BOARD_OFFSET_X, SUBTITLE_OFFSET_Y + 35,
                        BUTTON_FULL_WIDTH, BUTTON_HEIGHT, COLOR_BL_PANEL,
                        COLOR_BL_BG, BUTTON_RADIUS);
  const char* ver_str = format_ver("%d.%d.%d", current_hdr->onekey_version);
  display_text_right(DISPLAY_RESX / 2 - 25, SUBTITLE_OFFSET_Y + 90, ver_str, -1,
                     FONT_NORMAL, COLOR_WHITE, COLOR_BL_PANEL);
  ver_str = format_ver("%d.%d.%d", new_hdr->onekey_version);
  display_text(DISPLAY_RESX / 2 + 25, SUBTITLE_OFFSET_Y + 90, ver_str, -1,
               FONT_NORMAL, COLOR_WHITE, COLOR_BL_PANEL);

  display_image(227, SUBTITLE_OFFSET_Y + 70, 25, 23, toi_icon_arrow_right + 12,
                sizeof(toi_icon_arrow_right) - 12);

  ui_confirm_cancel_buttons("Cancel", "Install", COLOR_BL_DARK, COLOR_BL_DONE);
}

void ui_install_ble_confirm(void) {
  ui_statusbar_update();
  ui_logo_onekey();
  display_text_center(DISPLAY_RESX / 2, TITLE_OFFSET_Y, "Bluetooth Update", -1,
                      FONT_PJKS_BOLD_38, COLOR_BL_FG, COLOR_BL_BG);
  display_text_center(DISPLAY_RESX / 2, SUBTITLE_OFFSET_Y,
                      "A new bluetooth firmware is", -1, FONT_NORMAL,
                      COLOR_BL_SUBTITLE, COLOR_BL_BG);
  display_text_center(DISPLAY_RESX / 2, SUBTITLE_OFFSET_Y + 30,
                      "avaliable! The current version is", -1, FONT_NORMAL,
                      COLOR_BL_SUBTITLE, COLOR_BL_BG);
  display_text_center(DISPLAY_RESX / 2, SUBTITLE_OFFSET_Y + 60, ble_get_ver(),
                      -1, FONT_NORMAL, COLOR_BL_SUBTITLE, COLOR_BL_BG);
  ui_confirm_cancel_buttons("Cancel", "Install", COLOR_BL_DARK, COLOR_BL_DONE);
}

void ui_install_thd89_confirm(const char* old_ver, const char* boot_ver) {
  char str[128] = {0};
  ui_statusbar_update();
  ui_logo_onekey();
  display_text_center(DISPLAY_RESX / 2, TITLE_OFFSET_Y, "SE Update", -1,
                      FONT_PJKS_BOLD_38, COLOR_BL_FG, COLOR_BL_BG);
  display_text_center(DISPLAY_RESX / 2, SUBTITLE_OFFSET_Y,
                      "A new SE firmware is avaliable! The", -1, FONT_NORMAL,
                      COLOR_BL_SUBTITLE, COLOR_BL_BG);
  strcat(str, "current version is ");
  strcat(str, old_ver);
  display_text_center(DISPLAY_RESX / 2, SUBTITLE_OFFSET_Y + 30, str, -1,
                      FONT_NORMAL, COLOR_BL_SUBTITLE, COLOR_BL_BG);
  memset(str, 0, sizeof(str));
  strcat(str, "boot version is ");
  strcat(str, boot_ver);
  display_text_center(DISPLAY_RESX / 2, SUBTITLE_OFFSET_Y + 60, str, -1,
                      FONT_NORMAL, COLOR_BL_SUBTITLE, COLOR_BL_BG);

  ui_confirm_cancel_buttons("Cancel", "Install", COLOR_BL_DARK, COLOR_BL_DONE);
}

void ui_bootloader_first(const image_header* const hdr) {
  ui_bootloader_page_current = 0;
  uint8_t se_state;
  char se_info[64] = {0};

  ui_statusbar_update();
  // info icon 48 * 48 - the entry point of the bootloader details
  display_icon(INFO_ICON_OFFSET_X, INFO_ICON_OFFSET_Y, INFO_ICON_SIZE,
               INFO_ICON_SIZE, toi_ok_icon_info + 12,
               sizeof(toi_ok_icon_info) - 12, COLOR_BL_FG_INFO_ICON,
               COLOR_BL_BG);
  ui_logo_onekey();
  display_text_center(DISPLAY_RESX / 2, TITLE_OFFSET_Y, "Update Mode", -1,
                      FONT_PJKS_BOLD_38, COLOR_BL_FG, COLOR_BL_BG);

  if (ble_name_state()) {
    char* ble_name;
    ble_name = ble_get_name();
    display_text_center(DISPLAY_RESX / 2, SUBTITLE_OFFSET_Y, ble_name, -1,
                        FONT_NORMAL, COLOR_BL_SUBTITLE, COLOR_BL_BG);
    ble_name_show = true;
  }

  display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY - 92, "SafeOS", -1,
                      FONT_PJKS_BOLD_38, COLOR_BL_FG, COLOR_BL_BG);
  if (hdr) {
    const char* ver_str = format_ver("%d.%d.%d", (hdr->onekey_version));
    display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY - 50, ver_str, -1,
                        FONT_NORMAL, COLOR_BL_SUBTITLE, COLOR_BL_BG);
  }
  se_state = se_get_state();
  if (se_state != 0) {
    strcat(se_info, "SE ");
    if (se_state & THD89_1ST_IN_BOOT) {
      strcat(se_info, "1st ");
    }
    if (se_state & THD89_2ND_IN_BOOT) {
      strcat(se_info, "2nd ");
    }
    if (se_state & THD89_3RD_IN_BOOT) {
      strcat(se_info, "3rd ");
    }
    if (se_state & THD89_4TH_IN_BOOT) {
      strcat(se_info, "4th ");
    }
    strcat(se_info, "in boot");
    display_text_center(DISPLAY_RESX / 2, 300, se_info, -1, FONT_NORMAL,
                        COLOR_BL_SUBTITLE, COLOR_BL_BG);
    display_text_center(DISPLAY_RESX / 2, 330, "please install se firmware", -1,
                        FONT_NORMAL, COLOR_BL_SUBTITLE, COLOR_BL_BG);
  }
}

void ui_bootloader_view_details(const image_header* const hdr) {
  ui_bootloader_page_current = 1;

  int offset_x = 32, offset_y = 95, offset_seg = 44, offset_line = 30;
  const char* ver_str = NULL;

  ui_statusbar_update();
  display_bar_radius_ex(BOARD_OFFSET_X, 50, BUTTON_FULL_WIDTH, 627,
                        COLOR_BL_PANEL, COLOR_BL_BG, BUTTON_RADIUS);
  display_text(offset_x, offset_y, "Model", -1, FONT_PJKS_BOLD_26,
               COLOR_BL_TAGVALUE, COLOR_BL_PANEL);
  offset_y += offset_line;
  display_text(offset_x, offset_y, "OneKey Pro", -1, FONT_NORMAL, COLOR_BL_FG,
               COLOR_BL_PANEL);
  offset_y += offset_seg;

  display_text(offset_x, offset_y, "Firmware Version", -1, FONT_PJKS_BOLD_26,
               COLOR_BL_TAGVALUE, COLOR_BL_PANEL);
  offset_y += offset_line;
  if (hdr && hdr->onekey_version != 0) {
    ver_str = format_ver("%d.%d.%d", (hdr->onekey_version));
    display_text(offset_x, offset_y, ver_str, -1, FONT_NORMAL, COLOR_BL_FG,
                 COLOR_BL_PANEL);
  } else {
    display_text(offset_x, offset_y, "No Firmware", -1, FONT_NORMAL,
                 COLOR_BL_FG, COLOR_BL_PANEL);
  }
  offset_y += offset_seg;

  display_text(offset_x, offset_y, "Bluetooth Version", -1, FONT_PJKS_BOLD_26,
               COLOR_BL_TAGVALUE, COLOR_BL_PANEL);
  offset_y += offset_line;
  if (ble_ver_state()) {
    ver_str = ble_get_ver();
    display_text(offset_x, offset_y, ver_str, -1, FONT_NORMAL, COLOR_BL_FG,
                 COLOR_BL_PANEL);
  } else {
    display_text(offset_x, offset_y, "Pending", -1, FONT_NORMAL, COLOR_BL_FG,
                 COLOR_BL_PANEL);
  }
  offset_y += offset_seg;

  display_text(offset_x, offset_y, "Serial Number", -1, FONT_PJKS_BOLD_26,
               COLOR_BL_TAGVALUE, COLOR_BL_PANEL);
  offset_y += offset_line;
  char* dev_serial;
  if (device_get_serial(&dev_serial)) {
    display_text(offset_x, offset_y, dev_serial, -1, FONT_NORMAL, COLOR_BL_FG,
                 COLOR_BL_PANEL);
  } else {
    display_text(offset_x, offset_y, "NULL", -1, FONT_NORMAL, COLOR_BL_FG,
                 COLOR_BL_PANEL);
  }

  offset_y += offset_seg;
  display_text(offset_x, offset_y, "SE", -1, FONT_PJKS_BOLD_26,
               COLOR_BL_TAGVALUE, COLOR_BL_PANEL);
  offset_y += offset_line;
  const char* se_version = se01_get_version();
  char se_version_str[32] = {0};
  strcat(se_version_str, "THD89-");
  strcat(se_version_str, se_version);
  display_text(offset_x, offset_y, se_version_str, -1, FONT_NORMAL, COLOR_BL_FG,
               COLOR_BL_PANEL);

  offset_y += offset_seg;
  display_text(offset_x, offset_y, "Boardloader Version", -1, FONT_PJKS_BOLD_26,
               COLOR_BL_TAGVALUE, COLOR_BL_PANEL);
  offset_y += offset_line;
  display_text(offset_x, offset_y, get_boardloader_version(), -1, FONT_NORMAL,
               COLOR_BL_FG, COLOR_BL_PANEL);

  offset_y += offset_seg;
  display_text(offset_x, offset_y, "Bootloader Version", -1, FONT_PJKS_BOLD_26,
               COLOR_BL_TAGVALUE, COLOR_BL_PANEL);
  offset_y += offset_line;
  ver_str = format_ver("%d.%d.%d", VERSION_UINT32);
  display_text(offset_x, offset_y, ver_str, -1, FONT_NORMAL, COLOR_BL_FG,
               COLOR_BL_PANEL);

  offset_y += offset_seg;
  display_text(offset_x, offset_y, "BuildID", -1, FONT_PJKS_BOLD_26,
               COLOR_BL_TAGVALUE, COLOR_BL_PANEL);
  offset_y += offset_line;
  display_text(offset_x, offset_y, BUILD_ID + strlen(BUILD_ID) - 7, -1,
               FONT_NORMAL, COLOR_BL_FG, COLOR_BL_PANEL);

  ui_confirm_cancel_buttons("Back", "Restart", COLOR_BL_DARK, COLOR_BL_DANGER);
}

void ui_bootloader_restart_confirm(void) {
  ui_bootloader_page_current = 4;

  ui_statusbar_update();
  int title_offset_y = 90;
  int title_offset_x = 12;
  display_text(title_offset_x, title_offset_y, "Restart  Device?", -1,
               FONT_PJKS_BOLD_38, COLOR_BL_FG, COLOR_BL_BG);

  display_text(title_offset_x, title_offset_y + 46,
               "Restarting  will  exit  the  device  from", -1, FONT_NORMAL,
               COLOR_BL_SUBTITLE, COLOR_BL_BG);
  display_text(title_offset_x, title_offset_y + 76,
               "update  mode  and  interrupt  the", -1, FONT_NORMAL,
               COLOR_BL_SUBTITLE, COLOR_BL_BG);
  display_text(title_offset_x, title_offset_y + 106, "upgrade  process.", -1,
               FONT_NORMAL, COLOR_BL_SUBTITLE, COLOR_BL_BG);

  ui_confirm_cancel_buttons("Cancel", "Restart", COLOR_BL_DARK,
                            COLOR_BL_DANGER);
}

void ui_bootloader_factory(void) {
  ui_logo_onekey();
  display_text_center(DISPLAY_RESX / 2, 277, "Factory Mode", -1,
                      FONT_PJKS_BOLD_38, COLOR_BL_FG, COLOR_BL_BG);
}

void ui_bootloader_device_test(void) {
  ui_bootloader_page_current = 2;
  display_text_center(DISPLAY_RESX / 2, 277, "Test Mode", -1, FONT_PJKS_BOLD_38,
                      COLOR_BL_FG, COLOR_BL_BG);

  ui_confirm_cancel_buttons("Back", "Enter", COLOR_BL_DARK, COLOR_BL_FAIL);
}

void ui_bootloader_generate_trng_data() {
  ui_bootloader_page_current = 3;
  ui_logo_onekey();
  ui_screen_confirm("TRNG Generate", "Will generate 2 * 10MB Random Data",
                    "The process may take up to 30 mins",
                    "Enter Boardloader to obtain results at ",
                    "\"TRNG_Test_Data\" Folder");
}

void ui_bootloader_page_switch(const image_header* const hdr) {
  int response;

  static uint32_t click = 0, click_pre = 0, click_now = 0;

  if (ui_bootloader_page_current == 0) {
    response = ui_input_poll(INPUT_INFO, false);
    if (INPUT_INFO == response) {
      display_clear();
      ui_bootloader_view_details(hdr);
    }
    if (!ble_name_show && ble_name_state()) {
      ui_bootloader_first(hdr);
    }
  } else if (ui_bootloader_page_current == 1) {
    click_now = HAL_GetTick();
    if ((click_now - click_pre) > (1000 / 2)) {
      click = 0;
    }
    response = ui_input_poll(INPUT_PREVIOUS | INPUT_RESTART |
                                 INPUT_BOOT_VERSION_TEXT | INPUT_BUILD_ID_TEXT,
                             false);
    if (INPUT_PREVIOUS == response) {
      display_clear();
      ui_bootloader_first(hdr);
    } else if (INPUT_RESTART == response) {
      display_clear();
      ui_bootloader_restart_confirm();
    } else if (INPUT_BOOT_VERSION_TEXT == response) {
      click++;
      click_pre = click_now;
      if (click == 5) {
        click = 0;
        display_clear();
        ui_bootloader_device_test();
        click_pre = click_now;
      }
    } else if (INPUT_BUILD_ID_TEXT == response) {
      click++;
      click_pre = click_now;
      if (click == 5) {
        click = 0;
        display_clear();
        ui_bootloader_generate_trng_data();
        click_pre = click_now;
      }
    }
  } else if (ui_bootloader_page_current == 2) {
    response = ui_input_poll(INPUT_PREVIOUS | INPUT_RESTART, false);
    if (INPUT_PREVIOUS == response) {
      display_clear();
      ui_bootloader_first(hdr);
    } else if (INPUT_RESTART == response) {
      device_burnin_test_clear_flag();
    }
    click_now = HAL_GetTick();
    if (click_now - click_pre > (1000 * 3)) {
      display_clear();
      ui_bootloader_first(hdr);
    }
  } else if (ui_bootloader_page_current == 3) {
    response = ui_input_poll(INPUT_PREVIOUS | INPUT_RESTART, false);
    if (INPUT_PREVIOUS == response) {
      display_clear();
      ui_bootloader_first(hdr);
    } else if (INPUT_RESTART == response) {
      device_generate_trng_data();
    }
    click_now = HAL_GetTick();
    if (click_now - click_pre > (1000 * 10)) {
      display_clear();
      ui_bootloader_first(hdr);
    }
  } else if (ui_bootloader_page_current == 4) {
    response = ui_input_poll(INPUT_PREVIOUS | INPUT_RESTART, false);
    if (INPUT_PREVIOUS == response) {
      display_clear();
      ui_bootloader_view_details(hdr);
    } else if (INPUT_RESTART == response) {
      HAL_NVIC_SystemReset();
    }
  }
}
