#ifndef DEBUG_UI_H
#define DEBUG_UI_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "util_macros.h"

// MACRO

#define DBGUI_ENUM_ITEM(CLASS, TYPE) JOIN_EXPR(DBGUI, CLASS, TYPE)
// regex -> (DBGUI_ENUM_ITEM\((.*), (.*)\).*,).*
// replace -> $1 // DBGUI_$2_$3

// TYPES

typedef struct {
  int char_height;
  int live_bar_hight;
  int btn_barrier;
  int btn_spacer;
  int status_bar_hight;
  int button_radius;
  int x_offset;
  int line_offset;
  int line_y_start;
} DBGUI_CONFIG_t;

typedef enum {
  DBGUI_ENUM_ITEM(OBJ_TYPE, GroupTitle),  // DBGUI_OBJ_TYPE_GroupTitle
  DBGUI_ENUM_ITEM(OBJ_TYPE, Button),      // DBGUI_OBJ_TYPE_Button
} DBGUI_OBJ_TYPE_t;

typedef struct {
  DBGUI_OBJ_TYPE_t type;
  bool disabled;
  uint16_t color_fg;
  uint16_t color_bg;
  int cord_x;
  int cord_y;
  int text_width;
  char* text;
} DBGUI_OBJ_t;

#define DBGUI_OBJ_DEF_GroupTitle(name) \
  { .type = DBGUI_OBJ_TYPE_GroupTitle, .text = #name, .color_fg = COLOR_WHITE, }
#define DBGUI_OBJ_DEF_Button(name)                                   \
  {                                                                  \
    .type = DBGUI_OBJ_TYPE_Button, .disabled = false, .text = #name, \
    .color_fg = COLOR_WHITE,                                         \
  }

// ui helpers
uint16_t rgb_loop_val(bool reset);
bool debug_ui_user_touch();
void debug_ui_setup(
    char* ui_title, DBGUI_CONFIG_t* ui_config, DBGUI_OBJ_t* obj_list,
    uint16_t obj_count);  // obj_count = (sizeof(ui_objs) / sizeof(DBGUI_OBJ_t)

// ui title
void debug_ui_title_update(char* ui_title);

// ui status bar
void debug_ui_status_bar_update(bool main_slave, uint16_t color,
                                const char* fmt, ...);
void debug_ui_status_bar_clear(bool main_slave);
void debug_ui_status_bar_progress(uint16_t color, uint8_t percent);

// ui group and buttons
void debug_ui_btn_color_set(uint8_t obj_index, uint16_t color);
void debug_ui_btn_color_reset();
bool debug_ui_populate_cord(int offset_x, int offset_y);
void debug_ui_draw();
int debug_ui_touch_to_btn(void);

#endif  // DEBUG_UI_H