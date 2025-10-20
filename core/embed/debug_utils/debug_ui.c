#include "debug_ui.h"

#include <stdio.h>
#include <string.h>

#include "common.h"
// #include "device.h"

#include "display.h"
// #include "mini_printf.h"
#include "mipi_lcd.h"
#include "systick.h"
#include "touch.h"

// vars
static char title[64];
static DBGUI_CONFIG_t* cfg = NULL;
static DBGUI_OBJ_t* ui_objs = NULL;
static uint16_t ui_objs_count = 0;

// ui helpers
uint16_t rgb_loop_val(bool reset)
{
    static uint16_t r = 31, g = 0, b = 0;
    static uint8_t stage = 0;

    if ( reset )
    {
        r = 31, g = 0, b = 0, stage = 0;
    }

    switch ( stage )
    {
    case 0:
        // r full, [g inc], b zero
        g++;
        if ( g >= 63 )
            stage = 1;
        break;
    case 1:
        // [r dec], g full, b zero
        r--;
        if ( r <= 0 )
            stage = 2;
        break;
    case 2:
        // r zero, g full, [b inc]
        b++;
        if ( b >= 31 )
            stage = 3;
        break;
    case 3:
        // r zero, [g dec], b full
        g--;
        if ( g <= 0 )
            stage = 4;
        break;
    case 4:
        // [r inc], g zero, b full
        r++;
        if ( r >= 31 )
            stage = 5;
        break;
    case 5:
        // r full, g zero, [b dec]
        b--;
        if ( b <= 0 )
            stage = 0;
        break;

    default:
        break;
    }

    return RGB16(r * 255 / 31, g * 255 / 63, b * 255 / 31);
}

bool debug_ui_user_touch()
{
    return touch_is_detected();
}

void debug_ui_setup(char* ui_title, DBGUI_CONFIG_t* ui_config, DBGUI_OBJ_t* obj_list, uint16_t obj_count)
{
    strcpy(title, ui_title);
    cfg = ui_config;
    ui_objs = obj_list;
    ui_objs_count = obj_count;
}

// ui title
void debug_ui_title_update(char* ui_title)
{
    strcpy(title, ui_title);
}

// ui status bar
void debug_ui_status_bar_update(bool main_slave, uint16_t color, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    char buf[256] = {0};
    int len = vsnprintf(buf, sizeof(buf), fmt, va);
    display_text(
        cfg->x_offset, DISPLAY_RESY - 5 - (5 + cfg->char_height) * (main_slave ? 2 : 1) + cfg->char_height, buf, len,
        FONT_NORMAL, color, COLOR_BLACK
    );
    va_end(va);
}
void debug_ui_status_bar_clear(bool main_slave)
{
    display_bar(
        0, DISPLAY_RESY - 5 - (5 + cfg->char_height) * (main_slave ? 2 : 1), DISPLAY_RESX, (5 + cfg->char_height),
        COLOR_BLACK
    );
    // debug
    // if ( main_slave )
    // {
    //     display_bar(
    //         0, DISPLAY_RESY - 5 - (5 + cfg->char_height) * 2, DISPLAY_RESX,
    //         (5 + cfg->char_height), COLOR_GREEN
    //     );
    // }
    // else
    // {
    //     display_bar(0, DISPLAY_RESY - 5 - (5 + cfg->char_height),
    //     DISPLAY_RESX, (5 + cfg->char_height), COLOR_BLUE);
    // }
}
void debug_ui_status_bar_progress(uint16_t color, uint8_t percent)
{
    if ( percent == 0 )
    {
        display_bar(0, DISPLAY_RESY - 5, DISPLAY_RESX, 5, COLOR_BLACK);
    }
    else
    {
        int target_w = DISPLAY_RESX * percent / 100;
        display_bar(0, DISPLAY_RESY - 5, target_w, 5, color);
    }
}

// ui group and buttons
void debug_ui_btn_color_set(uint8_t obj_index, uint16_t color)
{
    ui_objs[obj_index].color_bg = color;
}
void debug_ui_btn_color_reset()
{
    for ( uint8_t i = 0; i < ui_objs_count; i++ )
    {
        if ( ui_objs[i].type == DBGUI_OBJ_TYPE_Button )
            ui_objs[i].color_bg = COLOR_GRAY;
    }
}
bool debug_ui_populate_cord(int offset_x, int offset_y)
{
    int current_x = offset_x;
    int current_y = offset_y;

    for ( uint8_t i = 0; i < ui_objs_count; i++ )
    {
        switch ( ui_objs[i].type )
        {
        case DBGUI_OBJ_TYPE_GroupTitle:
            {
                if ( current_x != offset_x )
                {
                    // if cord not a new line, move to next line
                    current_x = offset_x;
                    current_y += cfg->line_offset;
                }
                ui_objs[i].cord_x = current_x;
                ui_objs[i].cord_y = current_y + cfg->char_height;

                // move cord to next new line
                current_x = offset_x;
                current_y += cfg->line_offset;
            }
            break;
        case DBGUI_OBJ_TYPE_Button:
            {
                ui_objs[i].text_width = display_text_width(ui_objs[i].text, -1, FONT_NORMAL);
                if ( (current_x + cfg->btn_spacer + (cfg->btn_barrier + ui_objs[i].text_width + cfg->btn_barrier)) >
                     DISPLAY_RESX )
                {
                    // if x space not enough for the button, move to next line
                    current_x = offset_x;
                    current_y += cfg->line_offset;
                }

                ui_objs[i].cord_x = current_x;
                ui_objs[i].cord_y = current_y;

                current_x += cfg->btn_spacer + (cfg->btn_barrier + ui_objs[i].text_width + cfg->btn_barrier);
            }
            break;

        default:
            return false;
        }
    }

    return true;
}
void debug_ui_draw()
{
    // draw title
#define DISPLAY_CHAR_HIGHT 26
    display_bar(0, cfg->live_bar_hight + 10, DISPLAY_RESX, DISPLAY_CHAR_HIGHT, COLOR_BLACK);
    display_text_center(
        DISPLAY_RESX / 2, cfg->live_bar_hight + DISPLAY_CHAR_HIGHT + 5, title, -1, FONT_NORMAL, COLOR_WHITE,
        COLOR_BLACK
    );

    // draw objs
    for ( uint8_t i = 0; i < ui_objs_count; i++ )
    {
        switch ( ui_objs[i].type )
        {
        case DBGUI_OBJ_TYPE_GroupTitle:
            {
                display_text(
                    ui_objs[i].cord_x, ui_objs[i].cord_y, ui_objs[i].text, -1, FONT_NORMAL, ui_objs[i].color_fg,
                    ui_objs[i].color_bg
                );
            }
            break;
        case DBGUI_OBJ_TYPE_Button:
            {
                // disabled handling
                if ( ui_objs[i].disabled )
                    ui_objs[i].color_fg = COLOR_DARK;
                // button bg
                display_bar_radius(
                    ui_objs[i].cord_x, ui_objs[i].cord_y, cfg->btn_barrier + ui_objs[i].text_width + cfg->btn_barrier,
                    cfg->btn_barrier + cfg->char_height + cfg->btn_barrier, ui_objs[i].color_bg, COLOR_BLACK,
                    cfg->button_radius
                );
                // for debug touch area
                // display_bar(
                //     ui_objs[i].cord_x, ui_objs[i].cord_y, cfg->btn_barrier +
                //     ui_objs[i].text_width + cfg->btn_barrier, cfg->btn_barrier +
                //     cfg->char_height + cfg->btn_barrier, COLOR_BLUE
                // );
                // button text
                display_text_center(
                    ui_objs[i].cord_x + (cfg->btn_barrier + ui_objs[i].text_width + cfg->btn_barrier) / 2,
                    ui_objs[i].cord_y + (cfg->btn_barrier + cfg->char_height + cfg->char_height) / 2, ui_objs[i].text,
                    -2, FONT_NORMAL, COLOR_BLACK, ui_objs[i].color_bg
                );
            }
            break;

        default:
            return;
        }
    }
}
int debug_ui_touch_to_btn(void)
{
    uint32_t evt = touch_click();

    if ( !evt )
        return -1;

    // get touch xy
    uint16_t touch_x = touch_unpack_x(evt);
    uint16_t touch_y = touch_unpack_y(evt);

    // check all buttons
    for ( uint8_t i = 0; i < ui_objs_count; i++ )
    {
        switch ( ui_objs[i].type )
        {
        case DBGUI_OBJ_TYPE_GroupTitle:
            continue;
        case DBGUI_OBJ_TYPE_Button:
            {
                // disabled handling
                if ( ui_objs[i].disabled )
                    continue;
                // for debug touch area
                // display_bar(
                //     ui_objs[i].cord_x, ui_objs[i].cord_y, cfg->btn_barrier +
                //     ui_objs[i].text_width + cfg->btn_barrier, cfg->btn_barrier +
                //     cfg->char_height + cfg->btn_barrier, COLOR_BLUE
                // );
                if ( (touch_y > ui_objs[i].cord_y &&
                      touch_y < (ui_objs[i].cord_y + cfg->btn_barrier + cfg->char_height + cfg->btn_barrier)) &&
                     (touch_x > ui_objs[i].cord_x &&
                      touch_x < ui_objs[i].cord_x + cfg->btn_barrier + ui_objs[i].text_width + cfg->btn_barrier) )
                {
                    return i;
                }
            }
            break;

        default:
            continue;
        }
    }

    // no valid button touch found
    return -1;
}