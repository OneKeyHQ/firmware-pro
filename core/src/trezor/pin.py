from typing import Any

_previous_progress: int | None = None
_previous_seconds: int | None = None
keepalive_callback: Any = None

_pre_scr: Any = None
_scr: Any = None
_timer: Any = None


def _timer_callback(timer: Any) -> None:

    global _scr, _timer

    if _scr is not None:
        try:
            _scr.delete()
            _scr = None
            if _timer is not None:
                _timer._del()
                _timer = None
        except Exception:
            _scr = None
            _timer = None


def show_pin_timeout(seconds: int, progress: int, message: str) -> bool:
    from trezor.lvglui.scrs import lv
    from trezor.lvglui.scrs.common import FullSizeWindow
    from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
    from trezor.lvglui.lv_colors import lv_colors

    global _previous_progress
    global _scr, _timer

    if callable(keepalive_callback):
        keepalive_callback()

    _scr_need_create = _scr is None
    if _scr_need_create:
        try:
            _scr.subtitle
        except Exception:
            _scr = None
            _scr_need_create = True

    if _scr_need_create:
        if message is not None:
            text = ""
            if message == "read fp data":
                text = _(i18n_keys.MSG__READING_FINGERPRINT_FROM_SECURITY_CHIP)
            else:
                text = message
        else:
            text = ""

        _scr = FullSizeWindow(title="", subtitle=text)
        _scr.subtitle.set_size(480, 200)
        _scr.subtitle.set_pos(0, 462)
        _scr.subtitle.set_style_text_align(lv.TEXT_ALIGN.CENTER, 0)

        arc_loader = lv.arc(_scr)
        arc_loader.set_bg_angles(0, 360)
        arc_loader.set_size(160, 160)
        arc_loader.set_rotation(270)
        arc_loader.set_angles(0, 0)
        arc_loader.remove_style(None, lv.PART.KNOB)
        arc_loader.set_style_arc_color(lv_colors.WHITE, lv.PART.INDICATOR)
        arc_loader.center()

        _scr.arc_loader = arc_loader

        _timer = lv.timer_create(_timer_callback, 10, None)
        _timer.set_repeat_count(1)

    _scr.arc_loader.set_end_angle(progress * 360 // 1000)
    lv.refr_now(None)

    if progress == 1000:
        if _scr is not None:
            try:
                _scr.delete()
                _scr = None
                if _timer is not None:
                    _timer._del()
                    _timer = None
            except Exception:
                _scr = None
                _timer = None

    return True
