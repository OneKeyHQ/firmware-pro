from ..i18n import gettext as _, keys as i18n_keys
from ..lv_colors import lv_colors
from . import lv
from .common import FullSizeWindow
from .widgets.style import StyleWrapper


class RequestAddFingerprintScreen(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__FINGERPRINT),
            _(i18n_keys.TITLE__FINGERPRINT_DESC),
            _(i18n_keys.BUTTON__ADD_FINGERPRINT),
            _(i18n_keys.BUTTON__CLOSE),
            icon_path="A:/res/fingerprint.png",
        )
        self.btn_layout_ver()


class CollectFingerprintStart(FullSizeWindow):
    def __init__(self):
        super().__init__(
            title=_(i18n_keys.TITLE__GET_STARTED),
            subtitle=_(
                i18n_keys.CONTENT__PLACE_YOUR_FINGER_ON_THE_SENSOR_LOCATED_ON_THE_SIDE_OF_THE_PHONE
            ),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
        )
        self.img = lv.img(self.content_area)
        self.img.remove_style_all()
        self.img.set_src("A:/res/finger-start.png")
        self.img.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 86)

        self.arrow = lv.img(self.content_area)
        self.arrow.remove_style_all()
        self.arrow.set_src("A:/res/finger-start-arrow.png")
        self.arrow.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_RIGHT, 0, 220)

        self.anim = lv.anim_t()
        self.anim.init()
        self.anim.set_var(self.arrow)
        self.anim.set_values(372, 278)
        self.anim.set_time(400)
        self.anim.set_playback_delay(100)
        self.anim.set_playback_time(400)
        self.anim.set_repeat_delay(100)
        self.anim.set_repeat_count(0xFFFF)  # infinite
        self.anim.set_path_cb(lv.anim_t.path_ease_in_out)
        self.anim.set_custom_exec_cb(lambda _a, val: self.anim_set_x(val))
        lv.anim_t.start(self.anim)

    def anim_set_x(self, x):
        try:
            self.arrow.set_x(x)
        except Exception:
            pass


async def add_fingerprint() -> None:
    while True:
        CollectFingerprintStart()
        break


async def request_delete_fingerprint(fingerprint_name: str, on_remove) -> None:
    confirmed = await RequestRemoveFingerprint(fingerprint_name).request()
    if confirmed:
        confirmed = await ConfirmRemoveFingerprint().request()
        if confirmed:
            await on_remove()


class RequestRemoveFingerprint(FullSizeWindow):
    def __init__(self, fingerprint_name: str):
        super().__init__(
            fingerprint_name,
            None,
            confirm_text=_(i18n_keys.BUTTON__REMOVE),
            icon_path="A:/res/fingerprint.png",
        )
        self.add_nav_back()
        self.btn_yes.add_style(StyleWrapper().bg_color(lv_colors.ONEKEY_RED_1), 0)

        self.add_event_cb(self.on_nav_back, lv.EVENT.CLICKED, None)

    def on_nav_back(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.nav_back.nav_btn:
                self.destroy(50)

    def show_unload_anim(self):
        # if self.anim_dir == ANIM_DIRS.HOR:
        #     Anim(0, -480, self.set_pos, time=200, y_axis=False, delay=200, del_cb=self._delete).start()
        # else:
        #     self.show_dismiss_anim()
        self.destroy(100)


class ConfirmRemoveFingerprint(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__REMOVE_THIS_FINGERPRINT),
            _(i18n_keys.TITLE__REMOVE_THIS_FINGERPRINT_DESC),
            confirm_text=_(i18n_keys.BUTTON__REMOVE),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            icon_path="A:/res/fingerprint.png",
        )
        self.add_nav_back()
        self.btn_yes.add_style(StyleWrapper().bg_color(lv_colors.ONEKEY_RED_1), 0)
        self.add_event_cb(self.on_nav_back, lv.EVENT.CLICKED, None)

    def on_nav_back(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.nav_back.nav_btn:
                self.destroy(50)
