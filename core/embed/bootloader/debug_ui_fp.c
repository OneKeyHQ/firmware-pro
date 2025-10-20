#include "debug_ui_fp.h"
#include "debug_ui.h"
#include "debug_utils.h"

#include "common.h"

#include "systick.h"
#include "ble.h"
#include "display.h"
#include "mipi_lcd.h"
#include "emmc_fs.h"
#include "ff.h"

#include "fingerprint.h"
#include "fp_algo_interface.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/*
add far frr sequence button
each sequence will do following:
1. create a path_folder with current HAL tick
2. loop for 100 times and each loop
   - take a picture
   - save the picture as bin file in the path_folder
   - run the fingerprint match
   - save the (count/match result/process time/current calculated match FAR or FRR) in a csv file
   - update the status bar with the current count and match result
3. after the loop, save the csv file in the path_folder
 */

#pragma GCC optimize("O0")

#pragma GCC diagnostic ignored "-Wcpp"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

// config (480*800)
#define DISPLAY_CHAR_HIGHT 26
static DBGUI_CONFIG_t ui_cfg_480x800 = {
    .char_height = DISPLAY_CHAR_HIGHT,
    .live_bar_hight = 5,
    .btn_barrier = 5,
    .btn_spacer = 5,
    .status_bar_hight = 5 + DISPLAY_CHAR_HIGHT + 5 + DISPLAY_CHAR_HIGHT + 5,
    .button_radius = 8,
    .x_offset = 10,
    .line_offset = 45,
    .line_y_start = 55,
};
static DBGUI_CONFIG_t* cfg = &ui_cfg_480x800;

// ui objs
typedef enum
{
    DBGUI_ENUM_ITEM(INDEX, Invalid) = -1, // DBGUI_INDEX_Invalid

    DBGUI_ENUM_ITEM(INDEX, Control),              // DBGUI_INDEX_Control
    DBGUI_ENUM_ITEM(INDEX, ShowImage),            // DBGUI_INDEX_ShowImage
    DBGUI_ENUM_ITEM(INDEX, EnrollOverlapTestOnly), // DBGUI_INDEX_EnrollOverlapTestOnly
    DBGUI_ENUM_ITEM(INDEX, TestAllowMixSlots),    // DBGUI_INDEX_TestAllowMixSlots
    DBGUI_ENUM_ITEM(INDEX, Reboot),               // DBGUI_INDEX_Reboot
    DBGUI_ENUM_ITEM(INDEX, PowerOff),             // DBGUI_INDEX_PowerOff
    DBGUI_ENUM_ITEM(INDEX, GoToBoard),            // DBGUI_INDEX_GoToBoard

    DBGUI_ENUM_ITEM(INDEX, Operations),      // DBGUI_INDEX_Operations // title
    DBGUI_ENUM_ITEM(INDEX, ShowStatus),      // DBGUI_INDEX_ShowStatus
    DBGUI_ENUM_ITEM(INDEX, EnrollEasy),      // DBGUI_INDEX_EnrollEasy
    DBGUI_ENUM_ITEM(INDEX, EnrollHard),      // DBGUI_INDEX_EnrollHard
    DBGUI_ENUM_ITEM(INDEX, MatchSingleShot), // DBGUI_INDEX_MatchSingleShot
    DBGUI_ENUM_ITEM(INDEX, MatchMultiShot),  // DBGUI_INDEX_MatchMultiShot
    DBGUI_ENUM_ITEM(INDEX, DeleteAll),       // DBGUI_INDEX_DeleteAll
    DBGUI_ENUM_ITEM(INDEX, Reset),           // DBGUI_INDEX_Reset

    DBGUI_ENUM_ITEM(INDEX, Tests),               // DBGUI_INDEX_Tests
    DBGUI_ENUM_ITEM(INDEX, FalseAcceptanceRate), // DBGUI_INDEX_FalseAcceptanceRate
    DBGUI_ENUM_ITEM(INDEX, FalseRejectionRate),  // DBGUI_INDEX_FalseAcceptanceRate
    DBGUI_ENUM_ITEM(INDEX, Clear),               // DBGUI_INDEX_Clear

} DBGUI_OBJ_INDEXs_t;

static DBGUI_OBJ_t ui_objs[] = {
    DBGUI_OBJ_DEF_GroupTitle(Control),          //
    DBGUI_OBJ_DEF_Button(ShowImage),            //
    DBGUI_OBJ_DEF_Button(EnrollOverlapTestOnly), //
    DBGUI_OBJ_DEF_Button(TestAllowMixSlots),    //
    DBGUI_OBJ_DEF_Button(Reboot),               //
    DBGUI_OBJ_DEF_Button(PowerOff),             //
    DBGUI_OBJ_DEF_Button(GoToBoard),            //

    DBGUI_OBJ_DEF_GroupTitle(Operations), //
    DBGUI_OBJ_DEF_Button(ShowStatus),     //
    DBGUI_OBJ_DEF_Button(EnrollEasy),     //
    DBGUI_OBJ_DEF_Button(EnrollHard),     //
    DBGUI_OBJ_DEF_Button(Match),          //
    DBGUI_OBJ_DEF_Button(MatchMulti),     //
    DBGUI_OBJ_DEF_Button(DeleteAll),      //
    DBGUI_OBJ_DEF_Button(Reset),          //

    DBGUI_OBJ_DEF_GroupTitle(Tests),           //
    DBGUI_OBJ_DEF_Button(FalseAcceptanceRate), //
    DBGUI_OBJ_DEF_Button(FalseRejectionRate),  //
    DBGUI_OBJ_DEF_Button(Clear),               //
};

typedef struct
{
    bool show_image;
    bool enroll_overlap_test_only;
    bool test_allow_mix_slots;
    // status cache
    uint8_t emroll_warn_InsufficientFeature;
    uint8_t emroll_warn_NeedsMove;
} DBGUI_SETTINGS_t;

// vars
char lable_title[64] = "";
static DBGUI_SETTINGS_t settings;

// private internal methods
static bool save_counter(const char* path, uint32_t* count)
{
    uint32_t written = 0;
    bool result = emmc_fs_file_write(path, 0, (void*)count, sizeof(uint32_t), &written, true, false);
    if ( !result || (written != sizeof(uint32_t)) )
    {
        DBG_PRINTF("Failed to write counter to %s, written %u bytes\n", path, written);
        return false;
    }
    DBG_PRINTF("Counter saved to %s, value: %u\n", path, count);
    return true;
}
static bool load_counter(const char* path, uint32_t* count)
{
    uint32_t read = 0;
    bool result = emmc_fs_file_read(path, 0, (void*)count, sizeof(uint32_t), &read);
    if ( !result || (read != sizeof(uint32_t)) )
    {
        DBG_PRINTF("Failed to read counter from %s, read %u bytes\n", path, read);
        return false;
    }
    DBG_PRINTF("Counter loaded from %s, value: %u\n", path, count);
    return true;
}
static bool save_image(const char* path, const void* buf, size_t len)
{
    uint32_t written = 0;
    bool result = emmc_fs_file_write(path, 0, (void*)buf, len, &written, true, false);
    if ( !result || (written != len) )
    {
        DBG_PRINTF("Failed to write image to %s, written %u bytes\n", path, written);
        return false;
    }
    DBG_PRINTF("Image saved to %s, size: %u bytes\n", path, written);
    return true;
}
static bool append_csv(const char* path, const char* fmt, ...)
{
    // format text
    char line[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    // file handling
    FIL fp = {0};
    // open file
    if ( f_open(&fp, path, FA_OPEN_APPEND | FA_WRITE) != FR_OK )
    {
        DBG_PRINTF("Failed to open file %s for appending: %d\n", path, fp.err);
        return false;
    }
    // move to end of file
    if ( f_lseek(&fp, f_size(&fp)) != FR_OK )
    {
        DBG_PRINTF("Failed to seek to end of file %s: %d\n", path, fp.err);
        f_close(&fp);
        return false;
    }
    // write line
    if ( f_write(&fp, (void*)line, strlen(line), NULL) != FR_OK )
    {
        DBG_PRINTF("Failed to write to file %s: %d\n", path, fp.err);
        f_close(&fp);
        return false;
    }
    // close file
    if ( f_close(&fp) != FR_OK )
    {
        DBG_PRINTF("Failed to close file %s\n", path);
        return false;
    }

    DBG_PRINTF("Appended line to %s: %s\n", path, line);
    return true;
}

// internal functions
static void dbgui_fp_power_manage_minimal(void)
{
    // fetch ble
    ble_uart_poll();

    // power status update
    static bool charge_configured = false;
    static bool charge_enabled = false;
    if ( !ble_charging_state() )
    {
        ble_cmd_req(BLE_PWR, BLE_PWR_CHARGING);
        return;
    }
    if ( ble_get_charge_type() == CHARGE_TYPE_USB )
    {
        if ( !charge_enabled || !charge_configured )
        {
            charge_configured = true;
            charge_enabled = true;
            ble_cmd_req(BLE_PWR, BLE_PWR_CHARGE_ENABLE);
        }
    }
    else
    {
        if ( charge_enabled || !charge_configured )
        {
            charge_configured = true;
            charge_enabled = false;
            ble_cmd_req(BLE_PWR, BLE_PWR_CHARGE_DISABLE);
        }
    }

    // skip init dummmy value
    if ( battery_cap == 0xff )
        return;

    // power status print
    static char title_bak[64] = {'\0'};
    snprintf(
        lable_title, sizeof(lable_title), "Fingerprint Test [Batt %u%%] %s", battery_cap,
        (charge_enabled ? "Charging" : "Discharging")
    );
    if ( strncmp(title_bak, lable_title, 64) != 0 )
    {
        strncpy(title_bak, lable_title, 64);
        debug_ui_title_update(lable_title);
        debug_ui_draw();
    }
}

static void dbgui_fp_status_print()
{
    // button color update
    debug_ui_btn_color_set(DBGUI_INDEX_ShowStatus, COLOR_YELLOW);
    debug_ui_draw();

    int print_y = ui_objs[(sizeof(ui_objs) / sizeof(DBGUI_OBJ_t)) - 1].cord_y + cfg->line_offset;

    uint8_t fp_count = 0;
    bool fp_slots[FP_MAX_USER_COUNT] = {0};

    fingerprint_get_count(&fp_count);
    fingerprint_get_list((uint8_t*)fp_slots, sizeof(fp_slots));

    int wipe_width = DISPLAY_RESX - 80; // avoid overlap with fp image

    // erase height set to (DISPLAY_CHAR_HEIGHT + 2) due to some font char
    // heights are different, which may cause ghosting issue
    display_bar(0, print_y, wipe_width, DISPLAY_CHAR_HEIGHT + 2, COLOR_BLACK);
    print_y += (DISPLAY_CHAR_HEIGHT + 2);
    display_text_printf(cfg->x_offset, print_y, "Registered total: %u", fp_count);

    for ( uint16_t i = 0; i < FP_MAX_USER_COUNT; i++ )
    {
        display_bar(0, print_y, wipe_width, DISPLAY_CHAR_HEIGHT + 2, COLOR_BLACK);
        print_y += (DISPLAY_CHAR_HEIGHT + 2);
        display_text_printf(cfg->x_offset, print_y, "Slot %u: %s", i, (fp_slots[i] ? "Registered" : "Empty"));
    }

    // last enroll status
    display_print_color(COLOR_BLUE, COLOR_BLACK);

    display_bar(0, print_y, wipe_width, DISPLAY_CHAR_HEIGHT + 2, COLOR_BLACK);
    print_y += (DISPLAY_CHAR_HEIGHT + 2);
    display_text_printf(cfg->x_offset, print_y, "Enroll BadImage: %u", settings.emroll_warn_InsufficientFeature);

    display_bar(0, print_y, wipe_width, DISPLAY_CHAR_HEIGHT + 2, COLOR_BLACK);
    print_y += (DISPLAY_CHAR_HEIGHT + 2);
    display_text_printf(cfg->x_offset, print_y, "Enroll TryDiffArea: %u", settings.emroll_warn_NeedsMove);

    display_print_color(COLOR_WHITE, COLOR_BLACK);

    // button color update
    debug_ui_btn_color_set(DBGUI_INDEX_ShowStatus, COLOR_GRAY);
    debug_ui_draw();
}

#if 0
static void dbgui_fp_results_display(void * results) {
  int print_y = ui_objs[(sizeof(ui_objs) / sizeof(DBGUI_OBJ_t)) - 1].cord_y +
                cfg->line_offset;
  print_y += (DISPLAY_CHAR_HEIGHT + 2);
  int print_x = cfg->x_offset;
  int bar_w = 0;
  int bar_h = DISPLAY_CHAR_HEIGHT / 3;

  for (size_t i = 0; i < act_list_len; ++i) {
    bar_w = act_list[i].duration_us / 10;
    // limit max and min
    bar_w = MAX(bar_w, 2);
    bar_w = MIN(bar_w, 460);

    if ((print_x + cfg->x_offset + bar_w) > DISPLAY_RESX) {
      print_x = cfg->x_offset;
      print_y += (bar_h + 1);
    }
    switch (act_list[i].state) {
      case MOTOR_COAST:
        display_bar(print_x, print_y, bar_w, bar_h, COLOR_GRAY);
        print_x += bar_w;
        break;
      case MOTOR_FORWARD:
        display_bar(print_x, print_y, bar_w, bar_h, COLOR_GREEN);
        print_x += bar_w;
        break;
      case MOTOR_REVERSE:
        display_bar(print_x, print_y, bar_w, bar_h, COLOR_RED);
        print_x += bar_w;
        break;
      case MOTOR_BRAKE:
        display_bar(print_x, print_y, bar_w, bar_h, COLOR_YELLOW);
        print_x += bar_w;
        break;
    }
    print_x += 1;
  }
}
#endif

// control methods
static void dbgui_fp_motor_showImage()
{
    settings.show_image = !settings.show_image;
    debug_ui_btn_color_set(DBGUI_INDEX_ShowImage, (settings.show_image ? COLOR_GREEN : COLOR_RED));
    debug_ui_draw();
}
static void dbgui_fp_motor_EnrollOverlapTestOnly()
{
    settings.enroll_overlap_test_only = !settings.enroll_overlap_test_only;
    debug_ui_btn_color_set(
        DBGUI_INDEX_EnrollOverlapTestOnly, (settings.enroll_overlap_test_only ? COLOR_GREEN : COLOR_RED)
    );
    debug_ui_draw();
}
static void dbgui_fp_motor_TestAllowMixSlots()
{
    settings.test_allow_mix_slots = !settings.test_allow_mix_slots;
    debug_ui_btn_color_set(DBGUI_INDEX_TestAllowMixSlots, (settings.test_allow_mix_slots ? COLOR_GREEN : COLOR_RED));
    debug_ui_draw();
}

static void dbgui_fp_control_Reboot()
{
    reboot_to_boot();
}
static void dbgui_fp_control_GoToBoard()
{
    reboot_to_board();
}

// Operations methods
static bool dbgui_fp_operation_enroll(bool easy_hard, bool overlap_test_only)
{
    // task result
    bool result = false;
    bool loop = true;

    // button color update
    debug_ui_btn_color_set((easy_hard ? DBGUI_INDEX_EnrollEasy : DBGUI_INDEX_EnrollHard), COLOR_YELLOW);
    debug_ui_draw();

    debug_ui_status_bar_clear(true);
    debug_ui_status_bar_clear(false);
    debug_ui_status_bar_progress(COLOR_WHITE, 0);

    FP_RESULT fp_result_curr;
    FP_RESULT fp_result_last = 0x99;
    uint8_t repeat_detect_num = 5;
    uint8_t overlap_detect_num = easy_hard ? 10 : 20;
    uint8_t enroll_step = 0;
    uint8_t enroll_step_max = overlap_test_only ? overlap_detect_num : FP_MAX_ENROLL_STEP;

    settings.emroll_warn_NeedsMove = 0;
    settings.emroll_warn_InsufficientFeature = 0;

    while ( loop )
    {
        fp_result_curr = fingerprint_enroll(enroll_step, repeat_detect_num, overlap_detect_num);
        if ( fp_result_curr == fp_result_last )
            continue;
        else
            fp_result_last = fp_result_curr;

        switch ( fp_result_curr )
        {
        case FP_RESULT_NoFinger:

            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint wait press\n",
                fp_result_curr
            );

            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_GREEN, "Fingerprint wait press");

            continue;
        case FP_RESULT_NeedRelease:

            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint wait release\n",
                fp_result_curr
            );

            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_GREEN, "Fingerprint wait release");

            continue;
        case FP_RESULT_Duplicate:

            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint already enrolled\n",
                fp_result_curr
            );

            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_RED, "Fingerprint already enrolled");
            result = false;
            loop = false;
            break;
        case FP_RESULT_InsufficientFeature:

            settings.emroll_warn_InsufficientFeature++;

            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint bad image\n",
                fp_result_curr
            );

            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_YELLOW, "Fingerprint bad image");

            hal_delay(500);
            continue;
        case FP_RESULT_NeedsMove:

            settings.emroll_warn_NeedsMove++;

            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint try different area\n",
                fp_result_curr
            );

            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_YELLOW, "Fingerprint try different area");

            hal_delay(500);
            continue;
        case FP_RESULT_OK:

            enroll_step++;

            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Enroll step %u ...\n",
                fp_result_curr, enroll_step
            );

            debug_ui_status_bar_clear(false);
            debug_ui_status_bar_update(false, COLOR_WHITE, "Enroll step %u ...", enroll_step);
            debug_ui_status_bar_progress(rgb_loop_val(false), ((100 * enroll_step) / enroll_step_max));

            if ( enroll_step < enroll_step_max )
            {
                continue;
            }
            else
            {
                uint8_t id;
                fingerprint_save(&id);

                DBG_PRINTF(
                    "fp_result_curr = %u, "
                    "Fingerprint enrolled, slot %u\n",
                    fp_result_curr, id
                );

                for ( uint8_t flash = 0; flash < 4; flash++ )
                {
                    debug_ui_status_bar_clear(true);
                    hal_delay(300);
                    debug_ui_status_bar_clear(true);
                    debug_ui_status_bar_update(true, COLOR_GREEN, "Fingerprint enrolled, slot %u", id);
                    hal_delay(300);
                }
                hal_delay(800);
                debug_ui_status_bar_clear(true);
                debug_ui_status_bar_clear(false);
                debug_ui_status_bar_progress(COLOR_WHITE, 0);


                result = true;
                loop = false;
                break;
            }
        case FP_RESULT_SlotFull:

            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint slot full\n",
                fp_result_curr
            );

            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_BLUE, "Fingerprint slot full");

            result = false;
            loop = false;
            break;
        case FP_RESULT_ErrOther:
        default:

            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint enroll unknown error\n",
                fp_result_curr
            );

            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_BLUE, "Fingerprint enroll unknown error %u", fp_result_curr);

            result = false;
            loop = false;
            break;
        }
    }

    // button color update
    debug_ui_btn_color_set(
        (easy_hard ? DBGUI_INDEX_EnrollEasy : DBGUI_INDEX_EnrollHard), (result ? COLOR_GREEN : COLOR_RED)
    );
    debug_ui_draw();
    return result;
}

static bool dbgui_fp_operation_match_single_shot(bool slot_0_only)
{
    // task result
    bool result = false;
    bool loop = true;

    // button color update
    debug_ui_btn_color_set(DBGUI_INDEX_MatchSingleShot, COLOR_YELLOW);
    debug_ui_draw();

    FP_RESULT fp_result_curr;
    FP_RESULT fp_result_last = 0x99;
    uint8_t match_id = 0;
    while ( loop )
    {
        fp_result_curr = fingerprint_match(&match_id, slot_0_only);
        if ( fp_result_curr == fp_result_last )
            continue;
        else
            fp_result_last = fp_result_curr;

        switch ( fp_result_curr )
        {
        case FP_RESULT_NoFinger:
            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_GREEN, "Fingerprint wait press");
            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint wait press\n",
                fp_result_curr
            );

            continue;
        case FP_RESULT_NeedRelease: // ignore
            // debug_ui_status_bar_clear(true);
            // debug_ui_status_bar_update(true, COLOR_GREEN, "Fingerprint wait
            // release");
            continue;
        case FP_RESULT_NoMatch:
            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_RED, "Fingerprint match none");
            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint match none\n",
                fp_result_curr
            );
            continue;
        case FP_RESULT_SlotMissmatch:
            if ( slot_0_only )
            {
                debug_ui_status_bar_clear(true);
                debug_ui_status_bar_update(true, COLOR_YELLOW, "Fingerprint match other slots");
                DBG_PRINTF(
                    "fp_result_curr = %u, "
                    "Fingerprint match other slots\n",
                    fp_result_curr
                );
                result = false;
                loop = false;
                break;
            }
            else
                continue;
        case FP_RESULT_Match:
            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_GREEN, "Fingerprint match slot %u", match_id);
            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint match slot %u\n",
                fp_result_curr, match_id
            );
            result = true;
            loop = false;
            break;
        case FP_RESULT_ErrState:
            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_RED, "Fingerprint match state error");
            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint match state error\n",
                fp_result_curr
            );
            result = false;
            loop = false;
            break;
        case FP_RESULT_ErrOther:
        default:
            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_BLUE, "Fingerprint match unknown error %u", fp_result_curr);
            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint match unknown error\n",
                fp_result_curr
            );
            result = false;
            loop = false;
            break;
        }
    }

    // button color update
    debug_ui_btn_color_set(DBGUI_INDEX_MatchSingleShot, (result ? COLOR_GREEN : COLOR_RED));
    debug_ui_draw();
    return result;
}

static bool dbgui_fp_operation_match_multi_shot(uint8_t loop_val)
{
    // task result
    bool result = true;
    // button color update
    debug_ui_btn_color_set(DBGUI_INDEX_MatchMultiShot, COLOR_YELLOW);
    debug_ui_draw();

    debug_ui_status_bar_progress(COLOR_WHITE, 0);

    for ( uint8_t i = 0; i < loop_val; i++ )
    {
        if ( dbgui_fp_operation_match_single_shot(false) )
        {
            debug_ui_status_bar_clear(false);
            debug_ui_status_bar_update(false, COLOR_WHITE, "match step %u ...", i + 1);
        }
        else
        {
            debug_ui_status_bar_clear(false);
            debug_ui_status_bar_update(false, COLOR_WHITE, "match step %u ... ERROR", i + 1);
            result = false;
            break;
        }
        debug_ui_status_bar_progress(rgb_loop_val(false), ((100 * (i + 1)) / loop_val));
    }

    for ( uint8_t flash = 0; flash < 4; flash++ )
    {
        debug_ui_status_bar_clear(true);
        hal_delay(300);
        debug_ui_status_bar_clear(true);
        debug_ui_status_bar_update(true, COLOR_GREEN, "match %u times reached", loop_val);
        hal_delay(300);
    }
    hal_delay(800);
    debug_ui_status_bar_clear(true);
    debug_ui_status_bar_clear(false);
    debug_ui_status_bar_progress(COLOR_WHITE, 0);

    // button color update
    debug_ui_btn_color_set(DBGUI_INDEX_MatchMultiShot, (result ? COLOR_GREEN : COLOR_RED));
    debug_ui_draw();
    return result;
}

static bool dbgui_fp_operation_delete_all()
{
    // task result
    bool result = false;

    // button color update
    debug_ui_btn_color_set(DBGUI_INDEX_DeleteAll, COLOR_YELLOW);
    debug_ui_draw();

    FP_RESULT fp_result = fingerprint_delete_all();
    result = (fp_result == FP_RESULT_OK);

    debug_ui_status_bar_clear(true);
    debug_ui_status_bar_clear(false);
    debug_ui_status_bar_progress(COLOR_WHITE, 0);

    // button color update
    debug_ui_btn_color_set(DBGUI_INDEX_DeleteAll, (result ? COLOR_GREEN : COLOR_RED));
    debug_ui_draw();
    return result;
}

static bool dbgui_fp_operation_reset()
{
    // task result
    bool result = false;

    // button color update
    debug_ui_btn_color_set(DBGUI_INDEX_Reset, COLOR_YELLOW);
    debug_ui_draw();

    fingerprint_wipe_storage();

    debug_ui_status_bar_clear(true);
    debug_ui_status_bar_clear(false);
    debug_ui_status_bar_progress(COLOR_WHITE, 0);

    // button color update
    debug_ui_btn_color_set(DBGUI_INDEX_Reset, COLOR_GRAY);
    debug_ui_draw();
    return result;
}

// Test methods

// False Acceptance Rate / False Rejection Rate
static void dbgui_fp_test_exec(bool far_frr, uint32_t loop_val)
{
    // ui setup
    debug_ui_btn_color_set(far_frr ? DBGUI_INDEX_FalseAcceptanceRate : DBGUI_INDEX_FalseRejectionRate, COLOR_YELLOW);
    debug_ui_draw();
    debug_ui_status_bar_progress(COLOR_WHITE, 0);

    // vars
    char test_type[4];
    char path_counter[64];
    char path_folder[64];
    char path_csv[64];
    char path_img[64];
    uint32_t count = 0;
    uint32_t loop = 0;

    snprintf(test_type, sizeof(test_type), "%s", far_frr ? "FAR" : "FRR");
    snprintf(path_counter, sizeof(path_counter), "1:FP_TEST/%s_count.bin", test_type);

    // counter load or init
    if ( !load_counter(path_counter, &count) )
    {
        DBG_PRINTF("Failed to load counter from %s, initializing to 0\n", path_counter);
        count = 0;
        if ( !save_counter(path_counter, &count) )
        {
            DBG_PRINTF("Failed to save counter to %s\n", path_counter);
            return;
        }
    }

    // populate paths
    snprintf(path_folder, sizeof(path_folder), "1:FP_TEST/%s_%lu", test_type, count);
    snprintf(path_csv, sizeof(path_csv), "%s/results.csv", path_folder);
    // snprintf(path_img, sizeof(path_img), "%s/%lu.bin", path_folder, loop);

    // check if folder exists
    if ( emmc_fs_path_exist(path_folder) )
    {
        debug_ui_status_bar_clear(true);
        debug_ui_status_bar_update(true, COLOR_RED, "Folder overlap, please clear");
        DBG_PRINTF("Directory %s already exists, please clear test results\n", path_folder);
        return;
    }

    // make dir
    if ( !emmc_fs_dir_make(path_folder) )
    {
        DBG_PRINTF("Failed to create directory %s\n", path_folder);
        return;
    }

    // csv header
    append_csv(path_csv, "Index,MatchResult,Rate(%s),ProcessTime(ms),ImageFile,Image\n", test_type);

    // match loop
    FP_RESULT fp_result_curr;
    FP_RESULT fp_result_last = 0x99;
    uint8_t match_id = 0;
    uint32_t match_count = 0;
    while ( loop < loop_val )
    {
        // try match
        uint32_t t0 = HAL_GetTick();
        fp_result_curr = fingerprint_match(&match_id, !settings.test_allow_mix_slots);
        uint32_t t1 = HAL_GetTick();

        // filter duplicate result
        if ( fp_result_curr == fp_result_last )
            continue;
        else
            fp_result_last = fp_result_curr;

        // check result
        switch ( fp_result_curr )
        {
            // flow control status
        case FP_RESULT_NoFinger:
            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_GREEN, "Fingerprint wait press");
            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint wait press\n",
                fp_result_curr
            );
            continue;
        case FP_RESULT_NeedRelease: // ignore
            // debug_ui_status_bar_update(true, COLOR_GREEN, "Fingerprint wait
            // release");
            continue;

            // result status
        case FP_RESULT_NoMatch:
            loop++;
            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_RED, "Fingerprint match none");
            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint match none\n",
                fp_result_curr
            );

            snprintf(path_img, sizeof(path_img), "%s/%lu.bin", path_folder, loop);
            save_image(path_img, fp_buff_img_raw, IMG_BUFFER_SIZE);
            snprintf(path_img, sizeof(path_img), "%s/%lu_processed.bin", path_folder, loop);
            save_image(path_img, fp_buff_img, IMG_BUFFER_SIZE);
            append_csv(
                path_csv, "%lu,%s,%.02f%%,%lums,%lu.bin\n",
                loop,                                                             // Index
                "NoMatch",                                                        // MatchResult
                (far_frr                                                          // Rate
                     ? ((double)match_count * 100 / (double)loop)                 // FAR, should not match
                     : (double)100.0 - ((double)match_count * 100 / (double)loop) // FRR, should match
                ),                                                                //
                t1 - t0,                                                          // ProcessTime(ms)
                loop                                                              // ImageFile
            );
            break;
        case FP_RESULT_SlotMissmatch:

            if ( settings.test_allow_mix_slots )
                match_count++;
            loop++;
            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_YELLOW, "Fingerprint match other slots");
            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint match other slots\n",
                fp_result_curr
            );

            snprintf(path_img, sizeof(path_img), "%s/%lu.bin", path_folder, loop);
            save_image(path_img, fp_buff_img_raw, IMG_BUFFER_SIZE);
            snprintf(path_img, sizeof(path_img), "%s/%lu_processed.bin", path_folder, loop);
            save_image(path_img, fp_buff_img, IMG_BUFFER_SIZE);
            append_csv(
                path_csv, "%lu,%s,%.02f%%,%lums,%lu.bin\n",
                loop,                                                             // Index
                "SlotMissmatch",                                                  // MatchResult
                (far_frr                                                          // Rate
                     ? ((double)match_count * 100 / (double)loop)                 // FAR, should not match
                     : (double)100.0 - ((double)match_count * 100 / (double)loop) // FRR, should match
                ),                                                                //
                t1 - t0,                                                          // ProcessTime(ms)
                loop                                                              // ImageFile
            );
            break;
        case FP_RESULT_Match:

            match_count++;
            loop++;

            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_GREEN, "Fingerprint match slot %u", match_id);
            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint match slot %u\n",
                fp_result_curr, match_id
            );

            snprintf(path_img, sizeof(path_img), "%s/%lu.bin", path_folder, loop);
            save_image(path_img, fp_buff_img_raw, IMG_BUFFER_SIZE);
            snprintf(path_img, sizeof(path_img), "%s/%lu_processed.bin", path_folder, loop);
            save_image(path_img, fp_buff_img, IMG_BUFFER_SIZE);
            append_csv(
                path_csv, "%lu,%s,%.02f%%,%lums,%lu.bin\n",
                loop,                                                             // Index
                "Match",                                                          // MatchResult
                (far_frr                                                          // Rate
                     ? ((double)match_count * 100 / (double)loop)                 // FAR, should not match
                     : (double)100.0 - ((double)match_count * 100 / (double)loop) // FRR, should match
                ),                                                                //
                t1 - t0,                                                          // ProcessTime(ms)
                loop                                                              // ImageFile
            );
            break;
        case FP_RESULT_ErrState:
            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_RED, "Fingerprint match state error");
            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint match state error\n",
                fp_result_curr
            );
            break;
        case FP_RESULT_ErrOther:
        default:
            debug_ui_status_bar_clear(true);
            debug_ui_status_bar_update(true, COLOR_BLUE, "Fingerprint match unknown error %u", fp_result_curr);
            DBG_PRINTF(
                "fp_result_curr = %u, "
                "Fingerprint match unknown error\n",
                fp_result_curr
            );
            break;
        }

        // update progress bar
        debug_ui_status_bar_clear(false);
        debug_ui_status_bar_update(
            false, COLOR_WHITE, "[%s %lu/%lu]__[%.02f%%]__[%lums]", test_type, loop, loop_val,
            (far_frr                                                          //
                 ? ((double)match_count * 100 / (double)loop)                 // FAR, should not match
                 : (double)100.0 - ((double)match_count * 100 / (double)loop) // FRR, should match
            ),
            (t1 - t0)
        );
        debug_ui_status_bar_progress(rgb_loop_val(false), ((100 * (loop)) / loop_val));
    }

    // update far/frr count
    count++;
    if ( !save_counter(path_counter, &count) )
    {
        DBG_PRINTF("Failed to save counter to %s\n", path_counter);
        return;
    }

    // flash results
    for ( uint8_t flash = 0; flash < 4; flash++ )
    {
        debug_ui_status_bar_clear(true);
        hal_delay(300);
        debug_ui_status_bar_clear(true);
        debug_ui_status_bar_update(true, COLOR_WHITE, "Test done");
        hal_delay(300);
    }

    hal_delay(800);
    debug_ui_status_bar_clear(true);
    debug_ui_status_bar_clear(false);
    debug_ui_status_bar_progress(COLOR_WHITE, 0);

    // button color update
    debug_ui_btn_color_set(far_frr ? DBGUI_INDEX_FalseAcceptanceRate : DBGUI_INDEX_FalseRejectionRate, COLOR_GRAY);
    debug_ui_draw();
}
static void dbgui_fp_test_clear()
{
    debug_ui_btn_color_set(DBGUI_INDEX_Clear, COLOR_YELLOW);
    debug_ui_draw();

    debug_ui_status_bar_clear(true);
    debug_ui_status_bar_clear(false);
    debug_ui_status_bar_progress(COLOR_WHITE, 0);

    bool result = emmc_fs_dir_delete("1:FP_TEST") && emmc_fs_dir_make("1:FP_TEST");

    debug_ui_btn_color_set(DBGUI_INDEX_FalseAcceptanceRate, COLOR_GRAY);
    debug_ui_btn_color_set(DBGUI_INDEX_FalseRejectionRate, COLOR_GRAY);

    // button color update
    debug_ui_btn_color_set(DBGUI_INDEX_Clear, result ? COLOR_GREEN : COLOR_RED);
    debug_ui_draw();
}

// main
void debug_ui_fp()
{
    // MAIN
    display_clear();
    display_print_clear();

    // TITLE
    snprintf(lable_title, sizeof(lable_title), "Fingerprint Test");

    // default settings
    settings.show_image = false;
    settings.test_allow_mix_slots = false;
    settings.enroll_overlap_test_only = false;

    // ui init
    debug_ui_setup(lable_title, &ui_cfg_480x800, ui_objs, (sizeof(ui_objs) / sizeof(DBGUI_OBJ_t)));
    debug_ui_populate_cord(cfg->x_offset, cfg->line_y_start);
    // button color handling
    debug_ui_btn_color_reset();
    debug_ui_btn_color_set(DBGUI_INDEX_ShowImage, (settings.show_image ? COLOR_GREEN : COLOR_RED));
    debug_ui_btn_color_set(DBGUI_INDEX_TestAllowMixSlots, (settings.test_allow_mix_slots ? COLOR_GREEN : COLOR_RED));
    debug_ui_btn_color_set(
        DBGUI_INDEX_EnrollOverlapTestOnly, (settings.enroll_overlap_test_only ? COLOR_GREEN : COLOR_RED)
    );
    debug_ui_draw();

    // fs init
    emmc_fs_mount(true, true);
    if ( !emmc_fs_dir_make("1:FP_TEST") )
    {
        DBG_PRINTF("Failed to create directory 1:FP_TEST\n");
        return;
    }

    // fp init
    FP_RESULT fpresult;
    fpresult = fingerprint_init();
    UNUSED(fpresult); // ignored as fp library init return value is faulty
    // if ( fpresult != FP_RESULT_OK )
    // {
    //     display_printf("fingerprint_init failed!");
    //     while ( 1 )
    //         ;
    // }

    bool loop_initial = true;
    bool loop_keep = true;
    bool loop_continue = false;
    while ( loop_keep )
    {
        // process touch
        DBGUI_OBJ_INDEXs_t obj_idx = debug_ui_touch_to_btn();
        switch ( obj_idx )
        {

        // Control
        case DBGUI_INDEX_ShowImage:
            dbgui_fp_motor_showImage();
            break;
        case DBGUI_INDEX_EnrollOverlapTestOnly:
            dbgui_fp_motor_EnrollOverlapTestOnly();
            break;
        case DBGUI_INDEX_TestAllowMixSlots:
            dbgui_fp_motor_TestAllowMixSlots();
            break;
        case DBGUI_INDEX_Reboot:
            emmc_fs_unmount(true, true);
            dbgui_fp_control_Reboot();
            break;
        case DBGUI_INDEX_PowerOff:
            emmc_fs_unmount(true, true);
            ble_power_off();
            break;
        case DBGUI_INDEX_GoToBoard:
            emmc_fs_unmount(true, true);
            dbgui_fp_control_GoToBoard();
            break;

        // Operations
        case DBGUI_INDEX_ShowStatus:
            dbgui_fp_status_print();
            break;
        case DBGUI_INDEX_EnrollEasy:
            dbgui_fp_operation_enroll(true, settings.enroll_overlap_test_only);
            dbgui_fp_status_print();
            break;
        case DBGUI_INDEX_EnrollHard:
            dbgui_fp_operation_enroll(false, settings.enroll_overlap_test_only);
            dbgui_fp_status_print();
            break;
        case DBGUI_INDEX_MatchSingleShot:
            dbgui_fp_operation_match_single_shot(false);
            dbgui_fp_status_print();
            break;
        case DBGUI_INDEX_MatchMultiShot:
            dbgui_fp_operation_match_multi_shot(30);
            dbgui_fp_status_print();
            break;
        case DBGUI_INDEX_DeleteAll:
            dbgui_fp_operation_delete_all();
            dbgui_fp_status_print();
            break;
        case DBGUI_INDEX_Reset:
            dbgui_fp_operation_reset();
            dbgui_fp_status_print();
            break;

        // Tests
        case DBGUI_INDEX_FalseAcceptanceRate:
            dbgui_fp_test_exec(true, 100);
            break;
        case DBGUI_INDEX_FalseRejectionRate:
            dbgui_fp_test_exec(false, 100);
            break;
        case DBGUI_INDEX_Clear:
            dbgui_fp_test_clear();
            break;

        default:
            if ( loop_initial )
                loop_initial = false;
            else
                loop_continue = true;
            break;
        }

        if ( loop_continue )
        {
            // active indicator
            display_bar(0, 0, DISPLAY_RESX, cfg->live_bar_hight, rgb_loop_val(false));

            // print image
            if ( settings.show_image && !debug_ui_user_touch() )
            {
                // FP_RESULT result = fingerprint_get_image();
                // if ( result == FP_RESULT_OK )

                uint8_t result = CaptureGSL61xx((uint8_t*)FMC_SDRAM_BOOLOADER_BUFFER_ADDRESS);
                if ( result == FINGER_DOWN )
                {
                    display_fp(
                        DISPLAY_RESX - IMG_WIDTH - 30, DISPLAY_RESY - IMG_HEIGHT - 100, IMG_WIDTH, IMG_HEIGHT,
                        (uint8_t*)FMC_SDRAM_BOOLOADER_BUFFER_ADDRESS
                    );
                }
            }

            // reduced refresh freq
            if ( HAL_GetTick() % 0xff == 0 )
            {
                // power manage refresh
                dbgui_fp_power_manage_minimal();
            }

            // continue
            loop_continue = false;
            continue;
        }
    }

    // exit
    display_clear();
    display_print_clear();
}