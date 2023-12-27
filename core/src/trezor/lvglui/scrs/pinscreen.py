from trezor import utils

from ..i18n import gettext as _, keys as i18n_keys
from . import font_GeistRegular26, font_GeistSemiBold48
from .common import FullSizeWindow, lv, lv_colors  # noqa: F401,F403
from .components.button import NormalButton
from .components.container import ContainerFlexCol
from .components.keyboard import NumberKeyboard
from .components.listitem import ListItemWithLeadingCheckbox
from .widgets.style import StyleWrapper


class PinTip(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__SETUP_CREATE_ENABLE_PIN_PROTECTION),
            _(i18n_keys.SUBTITLE__SETUP_CREATE_ENABLE_PIN_PROTECTION),
            anim_dir=0,
        )
        self.container = ContainerFlexCol(
            self.content_area,
            self.subtitle,
            pos=(0, 30),
            padding_row=10,
            clip_corner=False,
        )
        # self.container.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
        self.item1 = ListItemWithLeadingCheckbox(
            self.container,
            _(i18n_keys.CHECK__SETUP_SET_A_PIN__1),
            radius=40,
        )
        self.item2 = ListItemWithLeadingCheckbox(
            self.container,
            _(i18n_keys.CHECK__SETUP_SET_A_PIN__2),
            radius=40,
        )
        self.btn = NormalButton(self, _(i18n_keys.BUTTON__CONTINUE), False)
        self.container.add_event_cb(self.eventhandler, lv.EVENT.VALUE_CHANGED, None)
        self.cb_cnt = 0

    def eventhandler(self, event_obj: lv.event_t):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.btn:
                self.channel.publish(1)
                self.destroy()
        elif code == lv.EVENT.VALUE_CHANGED:
            if target == self.item1.checkbox:
                if target.get_state() & lv.STATE.CHECKED:
                    self.item1.enable_bg_color()
                    self.cb_cnt += 1
                else:
                    self.item1.enable_bg_color(False)
                    self.cb_cnt -= 1
            elif target == self.item2.checkbox:
                if target.get_state() & lv.STATE.CHECKED:
                    self.item2.enable_bg_color()
                    self.cb_cnt += 1
                else:
                    self.item2.enable_bg_color(False)
                    self.cb_cnt -= 1
            if self.cb_cnt == 2:
                self.btn.enable(
                    bg_color=lv_colors.ONEKEY_GREEN, text_color=lv_colors.BLACK
                )
            elif self.cb_cnt < 2:
                self.btn.disable()


class InputPin(FullSizeWindow):

    _instance = None

    @classmethod
    def get_window_if_visible(cls) -> "InputPin" | None:
        return (
            cls._instance
            if (cls._instance is not None and cls._instance.is_visible())
            else None
        )

    def __init__(self, **kwargs):
        super().__init__(
            title=kwargs.get("title") or _(i18n_keys.TITLE__ENTER_PIN),
            subtitle=kwargs.get("subtitle", ""),
            anim_dir=0,
        )
        self.__class__._instance = self
        self.allow_fingerprint = kwargs.get("allow_fingerprint", True)
        self.title.add_style(
            StyleWrapper()
            .text_font(font_GeistSemiBold48)
            .text_align_center()
            .text_letter_space(0),
            0,
        )
        self.title.align(lv.ALIGN.TOP_MID, 0, 24)
        self.subtitle.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular26)
            .text_color(lv_colors.ONEKEY_RED_1)
            .text_align_center(),
            0,
        )
        self.subtitle.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)
        self._show_fingerprint_prompt_if_necessary()
        self.clear_flag(lv.obj.FLAG.SCROLLABLE)
        self.keyboard = NumberKeyboard(self)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.READY, None)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.CANCEL, None)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.VALUE_CHANGED, None)

    def _show_fingerprint_prompt_if_necessary(self):
        from storage import device
        from trezorio import fingerprint

        if (
            self.allow_fingerprint
            and device.is_fingerprint_unlock_enabled()
            and utils.pin_verified_since_boot()
            and fingerprint.get_template_count() > 0
            and device.finger_failed_count() < 5
        ):
            self.fingerprint_prompt = lv.img(self.content_area)
            self.fingerprint_prompt.set_src("A:/res/fingerprint-prompt.png")
            self.fingerprint_prompt.set_pos(414, 30)

    def refresh_fingerprint_prompt(self):
        if hasattr(self, "fingerprint_prompt"):
            self.fingerprint_prompt.delete()

    def on_event(self, event_obj):
        code = event_obj.code
        if code == lv.EVENT.VALUE_CHANGED:
            utils.lcd_resume()
            if self.keyboard.ta.get_text() != "":
                self.subtitle.set_text("")
            return
        elif code == lv.EVENT.READY:
            input = self.keyboard.ta.get_text()
            if len(input) < 4:
                return
            self.channel.publish(input)
        elif code == lv.EVENT.CANCEL:
            self.channel.publish(0)

        self.clean()
        self.destroy()


class InputLitePin(FullSizeWindow):
    def __init__(self):
        super().__init__(
            title=_(i18n_keys.TITLE__ENTER_ONEKEY_LITE_PIN),
            subtitle=None,
            anim_dir=0,
        )
        self.title.add_style(
            StyleWrapper()
            .text_font(font_GeistSemiBold48)
            .text_align_center()
            .text_letter_space(0),
            0,
        )
        self.title.align(lv.ALIGN.TOP_MID, 0, 24)
        self.subtitle.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular26)
            .text_color(lv_colors.ONEKEY_RED_1)
            .text_align_center(),
            0,
        )
        self.subtitle.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)
        self.clear_flag(lv.obj.FLAG.SCROLLABLE)
        self.keyboard = NumberKeyboard(self, max_len=6, min_len=6)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.READY, None)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.CANCEL, None)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.VALUE_CHANGED, None)

    def on_event(self, event_obj):
        code = event_obj.code
        if code == lv.EVENT.VALUE_CHANGED:
            utils.lcd_resume()
            return
        elif code == lv.EVENT.READY:
            input = self.keyboard.ta.get_text()
            if len(input) < 6:
                return
            self.channel.publish(input)
        elif code == lv.EVENT.CANCEL:
            self.channel.publish(0)

        self.clean()
        self.destroy()


class SetupComplete(FullSizeWindow):
    def __init__(self, subtitle=""):
        super().__init__(
            title=_(i18n_keys.TITLE__WALLET_IS_READY),
            subtitle=subtitle,
            confirm_text=_(i18n_keys.BUTTON__CONTINUE),
            icon_path="A:/res/success.png",
            anim_dir=0,
        )

    def eventhandler(self, event_obj: lv.event_t):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.btn_yes:
                self.channel.publish(1)
                self.destroy()
                lv.scr_act().del_delayed(500)
                from apps.base import set_homescreen

                set_homescreen()
