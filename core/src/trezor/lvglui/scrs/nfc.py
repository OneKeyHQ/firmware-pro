from ..i18n import gettext as _, keys as i18n_keys
from . import lv
from .common import FullSizeWindow


class SearchDeviceScreen(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__SEARCHING),
            _(i18n_keys.CONTENT__KEEP_LITE_DEVICE_TOGETHER_BACKUP_COMPLETE),
            _(i18n_keys.BUTTON__CANCEL),
        )
        self.img_bg = lv.img(self.content_area)
        self.img_bg.set_src("A:/res/nfc-bg.png")
        self.img_bg.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 125)

        self.img_searching = lv.img(self.content_area)
        self.img_searching.set_src("A:/res/nfc-icon-searching.png")
        self.img_searching.align_to(self.img_bg, lv.ALIGN.CENTER, 0, 0)

        self.anim = lv.anim_t()
        self.anim.init()
        self.anim.set_var(self.img_bg)
        self.anim.set_values(0, 360)
        self.anim.set_time(1000)
        self.anim.set_playback_delay(100)
        self.anim.set_playback_time(1000)
        self.anim.set_repeat_delay(100)
        self.anim.set_repeat_count(0xFFFF)  # infinite
        self.anim.set_path_cb(lv.anim_t.path_ease_in_out)
        self.anim.set_custom_exec_cb(lambda _a, val: self.set_angle(val))
        lv.anim_t.start(self.anim)

    def set_angle(self, angle):
        try:
            self.img_bg.set_angle(angle)
        except Exception:
            pass


class TransferDataScreen(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__TRANSFERRING),
            _(i18n_keys.TITLE__TRANSFERRING_DESC),
            _(i18n_keys.BUTTON__CANCEL),
        )
        self.img_bg = lv.img(self.content_area)
        self.img_bg.set_src("A:/res/nfc-bg.png")
        self.img_bg.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 125)

        self.img_searching = lv.img(self.content_area)
        self.img_searching.set_src("A:/res/nfc-icon-transfering.png")
        self.img_searching.align_to(self.img_bg, lv.ALIGN.CENTER, 0, 0)

        self.anim = lv.anim_t()
        self.anim.init()
        self.anim.set_var(self.img_bg)
        self.anim.set_values(0, 360)
        self.anim.set_time(1000)
        self.anim.set_playback_delay(100)
        self.anim.set_playback_time(1000)
        self.anim.set_repeat_delay(100)
        self.anim.set_repeat_count(0xFFFF)  # infinite
        self.anim.set_path_cb(lv.anim_t.path_ease_in_out)
        self.anim.set_custom_exec_cb(lambda _a, val: self.set_angle(val))
        lv.anim_t.start(self.anim)

    def set_angle(self, angle):
        try:
            self.img_bg.set_angle(angle)
        except Exception:
            pass
