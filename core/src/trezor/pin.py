from typing import Any

_previous_progress: int | None = None
_previous_seconds: int | None = None
keepalive_callback: Any = None

_pre_scr: Any = None
_scr: Any = None


def show_pin_timeout(seconds: int, progress: int, message: str) -> bool:
    from trezor import ui
    from trezor.lvglui.scrs import lv
    from trezor.lvglui.scrs.common import FullSizeWindow
    from trezor.lvglui.i18n import gettext as _, keys as i18n_keys

    global _previous_progress
    global _previous_seconds
    global _scr

    if callable(keepalive_callback):
        keepalive_callback()

    if message is not None:
        text = ""
        if message == "read fp data":
            text = _(i18n_keys.MSG__READING_FINGERPRINT_FROM_SECURITY_CHIP)
        else:
            text = message

        if _scr is None:
            _scr = FullSizeWindow(title="", subtitle=text)
            _scr.subtitle.set_size(480, 200)
            _scr.subtitle.set_pos(0, 462)
            _scr.subtitle.set_style_text_align(lv.TEXT_ALIGN.CENTER, 0)
            if progress != 0:
                lv.refr_now(None)

    if progress == 0:
        if progress != _previous_progress:
            # avoid overdraw in case of repeated progress calls
            ui.display.clear()
            ui.display.loader(progress, False, 0, ui.FG, ui.BG)
            _previous_seconds = None
        # ui.display.text_center(ui.WIDTH // 2, 37, message, ui.BOLD, ui.FG, ui.BG)
        if _scr is not None:
            _scr.invalidate()
            lv.refr_now(None)
    # if not utils.DISABLE_ANIMATION:
    else:
        ui.display.loader(progress, False, 0, ui.FG, ui.BG)

    if seconds != _previous_seconds:
        if seconds == 0:
            # remaining = ""
            ui.display.clear()
        # elif seconds == 1:
        #     remaining = "1 second left"
        # else:
        #     remaining = f"{seconds} seconds left"
        ui.display.bar(0, ui.HEIGHT - 42, ui.WIDTH, 25, ui.BG)
        # ui.display.text_center(
        #     ui.WIDTH // 2, ui.HEIGHT - 22, remaining, ui.BOLD, ui.FG, ui.BG
        # )
        _previous_seconds = seconds

    if progress == 1000:
        ui.display.clear()
        if _scr is not None:
            _scr.delete()
            _scr = None

    ui.refresh()
    _previous_progress = progress
    return False
