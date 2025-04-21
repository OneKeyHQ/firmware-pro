from trezor import utils, workflow
from trezor.langs import langs_keys, langs_values
from trezor.lvglui.i18n import gettext as _, i18n_refresh, keys as i18n_keys
from trezor.messages import RecoveryDevice, ResetDevice
from trezor.wire import DUMMY_CONTEXT

from apps.management.recovery_device import recovery_device
from apps.management.reset_device import reset_device

from . import font_GeistRegular20, lv_colors
from .common import FullSizeWindow, Screen, lv  # noqa: F401,F403,F405
from .components.button import NormalButton
from .components.container import ContainerFlexCol
from .components.radio import RadioTrigger
from .components.transition import DefaultTransition
from .widgets.style import StyleWrapper

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

        pressed_style = (
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_BLACK_2)
            .transform_height(-2)
            .transition(DefaultTransition())
        )
        self.crt_btn = NormalButton(
            self.content_area,
            _(i18n_keys.CONTENT__CERTIFICATIONS),
            pressed_style=pressed_style,
        )
        self.crt_btn.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular20)
            .text_color(lv_colors.ONEKEY_WHITE_4),
            0,
        )
        self.crt_btn.enable_no_bg_mode(skip_pressed_style=True)
        self.crt_btn.align_to(self.container, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)
        self.crt_btn.add_event_cb(self.on_crt_btn, lv.EVENT.CLICKED, None)
        self.container.add_event_cb(self.on_ready, lv.EVENT.READY, None)

    def on_ready(self, _event_obj):
        global language
        language = langs_keys[self.choices.get_selected_index()]
        i18n_refresh(language)
        QuickStart()

    def on_crt_btn(self, _event_obj):
        CertificationInfo()

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)


class CertificationInfo(FullSizeWindow):
    class CertificationItem(lv.obj):
        def __init__(
            self,
            parent,
            left_text: str,
            right_text: str | None,
            icon_path: str | None = None,
            right_text_color=lv_colors.ONEKEY_GRAY_4,
        ):
            super().__init__(parent)
            self.remove_style_all()
            self.set_size(456, lv.SIZE.CONTENT)
            self.add_style(
                StyleWrapper()
                .bg_color(lv_colors.ONEKEY_BLACK_3)
                .bg_opa()
                .radius(0)
                .border_width(0)
                .pad_hor(24)
                .pad_ver(20)
                .text_font(font_GeistRegular20)
                .text_letter_space(-1)
                .text_align_left(),
                0,
            )
            self.label_left = lv.label(self)
            # self.label_left.set_recolor(True)
            self.label_left.set_size(lv.SIZE.CONTENT, lv.SIZE.CONTENT)
            self.label_left.set_long_mode(lv.label.LONG.WRAP)
            self.label_left.set_text(left_text)
            self.label_left.add_style(
                StyleWrapper().text_color(lv_colors.WHITE).align(lv.ALIGN.TOP_LEFT), 0
            )
            if right_text:
                self.label_right = lv.label(self)
                self.label_right.set_size(lv.SIZE.CONTENT, lv.SIZE.CONTENT)
                # self.label_right.set_recolor(True)
                self.label_right.set_text(right_text)
                self.label_right.add_style(
                    StyleWrapper().text_color(right_text_color), 0
                )
                self.label_right.align(lv.ALIGN.TOP_LEFT, 152, 0)
            if icon_path:
                self.img = lv.img(self)
                self.img.remove_style_all()
                self.img.set_src(icon_path)
                if right_text:
                    self.img.align_to(self.label_right, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 20)
                else:
                    self.img.align(lv.ALIGN.TOP_LEFT, 152, 0)

    def __init__(self):
        super().__init__(
            _(i18n_keys.CONTENT__CERTIFICATIONS),
            None,
        )
        self.add_nav_back()
        self.container = ContainerFlexCol(
            self.content_area, self.title, padding_row=0, pos=(0, 30)
        )
        self.container.add_style(
            StyleWrapper().bg_color(lv_colors.ONEKEY_BLACK_3).bg_opa(), 0
        )
        self.certs_info = [
            ("Model", "OneKey Pro", None, lv_colors.WHITE),
            (
                "United States",
                "FCC ID: 2BB8VP1",
                "A:/res/cert-fcc.png",
                lv_colors.ONEKEY_GRAY_4,
            ),
            ("Europe", None, "A:/res/cert-ce.png", lv_colors.ONEKEY_GRAY_4),
            (
                "Japan",
                "MIC: 211-240720",
                "A:/res/cert-mic.png",
                lv_colors.ONEKEY_GRAY_4,
            ),
            (
                "Brazil",
                "ANATEL ID: 02335-25-16343",
                "A:/res/cert-anatel.png",
                lv_colors.ONEKEY_GRAY_4,
            ),
        ]
        for i, (country, cert_id, icon_path, color) in enumerate(self.certs_info):
            self.CertificationItem(self.container, country, cert_id, icon_path, color)
            if i != len(self.certs_info) - 1:
                self.line = lv.line(self.container)
                self.line.set_size(408, 1)
                self.line.add_style(
                    StyleWrapper().bg_color(lv_colors.ONEKEY_GRAY_2).bg_opa(), 0
                )
        self.add_event_cb(self.on_nav_back, lv.EVENT.CLICKED, None)
        self.add_event_cb(self.on_nav_back, lv.EVENT.GESTURE, None)

    def on_nav_back(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.nav_back.nav_btn:
                self.destroy(50)
        elif code == lv.EVENT.GESTURE:
            _dir = lv.indev_get_act().get_gesture_dir()
            if _dir == lv.DIR.RIGHT:
                lv.event_send(self.nav_back.nav_btn, lv.EVENT.CLICKED, None)


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

                # pyright: off
                workflow.spawn(
                    reset_device(
                        DUMMY_CONTEXT,
                        ResetDevice(
                            strength=128,
                            language=language,
                            pin_protection=True,
                        ),
                    ),
                )
            elif target == self.btn_no:

                SelectImportType()

            elif target == self.nav_back.nav_btn:
                pass
            else:
                return
            self.destroy(100)


class SelectImportType(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__IMPORT_WALLET),
            _(i18n_keys.CONTENT__SELECT_THE_WAY_YOU_WANT_TO_IMPORT),
            anim_dir=0,
        )
        self.add_nav_back()
        optional_str = _(i18n_keys.TITLE__RECOVERY_PHRASE) + "\n" + "OneKey Lite"
        self.choices = RadioTrigger(self, optional_str)
        self.add_event_cb(self.on_ready, lv.EVENT.READY, None)
        self.add_event_cb(self.on_back, lv.EVENT.CLICKED, None)
        self.add_event_cb(self.on_nav_back, lv.EVENT.GESTURE, None)

    def on_nav_back(self, event_obj):
        code = event_obj.code
        if code == lv.EVENT.GESTURE:
            _dir = lv.indev_get_act().get_gesture_dir()
            if _dir == lv.DIR.RIGHT:
                lv.event_send(self.nav_back.nav_btn, lv.EVENT.CLICKED, None)

    def on_ready(self, event_obj):
        code = event_obj.code
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
        self.show_dismiss_anim()
        selected_index = self.choices.get_selected_index()
        if selected_index == 0:
            workflow.spawn(
                recovery_device(
                    DUMMY_CONTEXT,
                    RecoveryDevice(
                        enforce_wordlist=True,
                        language=language,
                        pin_protection=True,
                    ),
                    "phrase",
                )
            )
        elif selected_index == 1:
            workflow.spawn(
                recovery_device(
                    DUMMY_CONTEXT,
                    RecoveryDevice(
                        enforce_wordlist=True,
                        language=language,
                        pin_protection=True,
                    ),
                    "lite",
                )
            )

    def on_back(self, event_obj):
        target = event_obj.get_target()
        if target == self.nav_back.nav_btn:
            self.channel.publish(0)
            self.show_dismiss_anim()
