#include "emmc_ui_progress.h"

static void emmc_ui_progress_state_reset(emmc_ui_progress_state* state)
{
    state->last_percentage = 0;
    state->has_last_percentage = false;
}

emmc_ui_progress_action emmc_ui_progress_next_action(
    emmc_ui_progress_state* state, bool has_percentage, int percentage, bool progress_visible
)
{
    if ( !has_percentage )
    {
        emmc_ui_progress_state_reset(state);
        return progress_visible ? EMMC_UI_PROGRESS_ACTION_CLEAR : EMMC_UI_PROGRESS_ACTION_NONE;
    }

    if ( (percentage < 0) || (percentage > 100) )
    {
        return EMMC_UI_PROGRESS_ACTION_INVALID;
    }

    if ( progress_visible && state->has_last_percentage && state->last_percentage == (uint32_t)percentage )
    {
        return EMMC_UI_PROGRESS_ACTION_NONE;
    }

    state->last_percentage = (uint32_t)percentage;
    state->has_last_percentage = true;

    if ( percentage == 100 )
    {
        emmc_ui_progress_state_reset(state);
        return EMMC_UI_PROGRESS_ACTION_COMPLETE;
    }

    return EMMC_UI_PROGRESS_ACTION_UPDATE;
}
