from trezor import utils
from trezor.langs import langs_keys, langs_values
from trezor.lvglui.i18n import gettext as _, i18n_refresh, keys as i18n_keys

from .common import FullSizeWindow, Screen, lv  # noqa: F401,F403,F405
from .components.container import ContainerFlexCol
from .components.radio import RadioTrigger

word_cnt_strength_map = {
    12: 128,
    18: 192,
    24: 256,
}

language = "en"


class InitScreen(Screen):
    def __init__(self):
        if not hasattr(self, "_init"):
            self._init = True
            super().__init__(
                title=_(i18n_keys.TITLE__LANGUAGE), icon_path="A:/res/language.png"
            )
        else:
            return
        self.container = ContainerFlexCol(
            self.content_area, self.title, padding_row=2, pos=(0, 30)
        )
        self.choices = RadioTrigger(self.container, langs_values)
        self.container.add_event_cb(self.on_ready, lv.EVENT.READY, None)

    def on_ready(self, _event_obj):
        global language
        language = langs_keys[self.choices.get_selected_index()]
        i18n_refresh(language)
        QuickStart()

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)


class QuickStart(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__QUICK_START),
            _(i18n_keys.SUBTITLE__SETUP_QUICK_START),
            confirm_text=_(i18n_keys.BUTTON__CREATE_NEW_WALLET),
            cancel_text=_(i18n_keys.BUTTON__IMPORT_WALLET),
            anim_dir=0,
        )
        self.add_nav_back()
        self.btn_layout_ver()
        self.add_event_cb(self.on_nav_back, lv.EVENT.GESTURE, None)

    def on_nav_back(self, event_obj):
        code = event_obj.code
        if code == lv.EVENT.GESTURE:
            _dir = lv.indev_get_act().get_gesture_dir()
            if _dir == lv.DIR.RIGHT:
                lv.event_send(self.nav_back.nav_btn, lv.EVENT.CLICKED, None)

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.btn_yes:
                from trezor import workflow
                from trezor.wire import DUMMY_CONTEXT
                from apps.management.reset_device import reset_device
                from trezor.messages import ResetDevice

                # pyright: off
                workflow.spawn(
                    reset_device(
                        DUMMY_CONTEXT,
                        ResetDevice(
                            strength=128,
                            language=language,
                            pin_protection=True,
                        ),
                    )
                )
            elif target == self.btn_no:
                from apps.management.recovery_device import recovery_device
                from trezor.messages import RecoveryDevice
                from trezor import workflow
                from trezor.wire import DUMMY_CONTEXT

                workflow.spawn(
                    recovery_device(
                        DUMMY_CONTEXT,
                        RecoveryDevice(
                            enforce_wordlist=True,
                            language=language,
                            pin_protection=True,
                        ),
                    )
                )  # pyright: on
            elif target == self.nav_back.nav_btn:
                pass
            else:
                return
            self.destroy(100)
