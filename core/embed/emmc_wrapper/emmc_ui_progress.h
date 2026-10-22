#ifndef __EMMC_UI_PROGRESS_H__
#define __EMMC_UI_PROGRESS_H__

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    EMMC_UI_PROGRESS_ACTION_NONE = 0,
    EMMC_UI_PROGRESS_ACTION_UPDATE,
    EMMC_UI_PROGRESS_ACTION_COMPLETE,
    EMMC_UI_PROGRESS_ACTION_CLEAR,
    EMMC_UI_PROGRESS_ACTION_INVALID,
} emmc_ui_progress_action;

typedef struct
{
    uint32_t last_percentage;
    bool has_last_percentage;
} emmc_ui_progress_state;

#define EMMC_UI_PROGRESS_STATE_INIT \
  {                                 \
    0, false                        \
  }

emmc_ui_progress_action emmc_ui_progress_next_action(
    emmc_ui_progress_state* state, bool has_percentage, int percentage, bool progress_visible
);

#endif
