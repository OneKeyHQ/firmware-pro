from storage import device
from trezor import ui, utils
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys

from . import font_GeistRegular26
from .common import Screen, lv, lv_colors
from .widgets.style import StyleWrapper


class LockScreen(Screen):
    def __init__(self, device_name, ble_name="", dev_state=None):
        lockscreen = device.get_homescreen()
        if not hasattr(self, "_init"):
            self._init = True
            super().__init__(title=device_name, subtitle=ble_name)
            self.title.add_style(StyleWrapper().text_align_center(), 0)
            self.subtitle.add_style(
                StyleWrapper().text_align_center().text_color(lv_colors.WHITE), 0
            )
        else:
            if ble_name:
                self.subtitle.set_text(ble_name)
            self.add_style(
                StyleWrapper()
                .bg_img_src(lockscreen)
                .bg_img_opa(int(lv.OPA.COVER * 0.72)),
                0,
            )
            return
        self.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
        self.title.align_to(self.content_area, lv.ALIGN.TOP_MID, 0, 76)
        self.subtitle.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, 16)
        self.add_style(
            StyleWrapper().bg_img_src(lockscreen).bg_img_opa(int(lv.OPA.COVER * 0.72)),
            0,
        )
        self.tap_tip = lv.label(self.content_area)
        self.tap_tip.set_long_mode(lv.label.LONG.WRAP)
        self.tap_tip.set_text(_(i18n_keys.MSG__USE_FINGERPRINT_OR_SWIPE_UP_TO_UNLOCK))
        self.tap_tip.align(lv.ALIGN.BOTTOM_MID, 0, -24)
        self.tap_tip.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular26)
            .text_letter_space(-1)
            .max_width(456)
            .text_align_center(),
            0,
        )
        self.lock_state = lv.img(self.content_area)
        self.lock_state.set_src("A:/res/lock.png")
        self.lock_state.align_to(self.tap_tip, lv.ALIGN.OUT_TOP_MID, 0, -16)
        self.add_event_cb(self.on_slide_up, lv.EVENT.GESTURE, None)

    def eventhandler(self, event_obj: lv.event_t):
        code = event_obj.code
        if code == lv.EVENT.CLICKED:
            if self.channel.takers:
                self.channel.publish("clicked")
            else:
                if not ui.display.backlight() and not device.is_tap_awake_enabled():
                    return
                if utils.turn_on_lcd_if_possible():
                    return
                from trezor import workflow
                from apps.base import unlock_device

                workflow.spawn(unlock_device())

    def on_slide_up(self, event_obj: lv.event_t):
        code = event_obj.code
        if code == lv.EVENT.GESTURE:
            _dir = lv.indev_get_act().get_gesture_dir()
            if _dir == lv.DIR.TOP:
                if not ui.display.backlight():
                    return
                from trezor import workflow
                from apps.base import unlock_device

                workflow.spawn(unlock_device())

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)
