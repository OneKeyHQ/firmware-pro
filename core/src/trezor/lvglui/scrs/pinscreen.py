from trezor import utils

from ..i18n import gettext as _, keys as i18n_keys
from . import font_GeistRegular26, font_GeistSemiBold48
from .common import FullSizeWindow, lv, lv_colors  # noqa: F401,F403
from .components.button import NormalButton
from .components.container import ContainerFlexCol
from .components.keyboard import IndexKeyboard, NumberKeyboard
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


class InputNum(FullSizeWindow):
    _instance = None

    @classmethod
    def get_window_if_visible(cls) -> "InputNum" | None:
        try:
            if cls._instance is not None and cls._instance.is_visible():
                return cls._instance
        except Exception:
            pass
        return None

    def __init__(self, **kwargs):
        super().__init__(
            title=kwargs.get("title") or _(i18n_keys.TITLE__ENTER_PIN),
            subtitle=kwargs.get("subtitle", ""),
            anim_dir=0,
        )
        self.__class__._instance = self

        self.title.add_style(
            StyleWrapper()
            .text_font(font_GeistSemiBold48)
            .text_align_center()
            .text_letter_space(0),
            0,
        )
        self.title.align(lv.ALIGN.TOP_MID, 0, 24)

        if self.subtitle.get_text() != "":
            self.subtitle.add_style(
                StyleWrapper().text_font(font_GeistRegular26)
                # .max_width(310)
                .max_width(368)
                .text_color(lv_colors.WHITE)
                .bg_color(lv_colors.ONEKEY_RED_2)
                .bg_opa(lv.OPA.COVER)
                .pad_hor(8)
                .pad_ver(16)
                .radius(40)
                .text_align_center(),
                0,
            )
            # self.subtitle.add_style(
            #     StyleWrapper()
            #     .text_font(font_GeistRegular26)
            #     .max_width(368)
            #     .text_color(lv_colors.ONEKEY_RED_1)
            #     .text_align_center(),
            #     0,
            # )

            title_height = self.title.get_height()
            subtitle_y = 40 if title_height > 60 else 70
            self.subtitle.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, subtitle_y)
            self.subtitle.move_foreground()

        self.clear_flag(lv.obj.FLAG.SCROLLABLE)
        self.keyboard = IndexKeyboard(
            self, min_len=1, max_len=11, is_pin=kwargs.get("is_pin", True)
        )
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.READY, None)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.CANCEL, None)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.VALUE_CHANGED, None)

        self.keyboard.ta.add_style(
            StyleWrapper().bg_opa(lv.OPA.TRANSP),
            0,
        )

    def on_event(self, event_obj):
        code = event_obj.code
        if code == lv.EVENT.VALUE_CHANGED:
            utils.lcd_resume()
            if self.keyboard.ta.get_text() != "":
                self.subtitle.set_text("")
                self.subtitle.remove_style_all()

            return
        elif code == lv.EVENT.READY:
            input = self.keyboard.ta.get_text()
            if input.startswith("#"):
                input = input[1:]
            if len(input) < 1:
                return
            self.channel.publish(input)
        elif code == lv.EVENT.CANCEL:
            self.channel.publish(0)

        self.clean()
        self.destroy(250)


class InputPin(FullSizeWindow):

    _instance = None

    @classmethod
    def get_window_if_visible(cls) -> "InputPin" | None:
        try:
            if cls._instance is not None and cls._instance.is_visible():
                return cls._instance
        except Exception:
            pass
        return None

    def __init__(self, **kwargs):
        subtitle = kwargs.get("subtitle", "")
        super().__init__(
            title=kwargs.get("title") or _(i18n_keys.TITLE__ENTER_PIN),
            subtitle=subtitle,
            anim_dir=0,
        )
        self.__class__._instance = self
        self.allow_fingerprint = kwargs.get("allow_fingerprint", True)
        self.standy_wall_only = kwargs.get("standy_wall_only", False)
        self.min_len = kwargs.get("min_len", 4)
        self.title.add_style(
            StyleWrapper()
            .text_font(font_GeistSemiBold48)
            .text_align_center()
            .text_letter_space(0),
            0,
        )
        self.title.align(lv.ALIGN.TOP_MID, 0, 24)

        standard_wallet_text = _(i18n_keys.CONTENT__PIN_FOR_STANDARD_WALLET)
        is_standard_wallet = subtitle == standard_wallet_text
        self.subtitle.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular26)
            .max_width(368)
            .text_color(lv_colors.LIGHT_GRAY)
            .bg_color(
                lv_colors.BLACK
                if is_standard_wallet
                else (lv_colors.ONEKEY_RED_2 if subtitle else lv_colors.BLACK)
            )
            .bg_opa(lv.OPA.COVER)
            .pad_hor(8)
            .pad_ver(16)
            .radius(40)
            .text_align_center(),
            0,
        )
        self.subtitle.set_text(subtitle)
        title_height = self.title.get_height()
        subtitle_y = (16 if is_standard_wallet else 24) if title_height > 60 else 70
        self.subtitle.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, subtitle_y)
        self.subtitle.set_text(subtitle)
        self._show_fingerprint_prompt_if_necessary()
        self.clear_flag(lv.obj.FLAG.SCROLLABLE)
        self.keyboard = NumberKeyboard(self, min_len=self.min_len)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.READY, None)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.CANCEL, None)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.VALUE_CHANGED, None)

        self.keyboard.ta.add_style(
            StyleWrapper().bg_opa(lv.OPA.TRANSP),
            0,
        )

    def change_subtitle(self, subtitle: str):
        from apps.common import passphrase

        # if standy_wall_only :
        if (
            subtitle == _(i18n_keys.CONTENT__PIN_FOR_STANDARD_WALLET)
            and passphrase.is_passphrase_pin_enabled()
        ):
            self.subtitle.set_style_bg_color(lv_colors.BLACK, 0)
            self.subtitle.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, 16)
        else:
            self.subtitle.set_style_bg_color(
                lv_colors.ONEKEY_RED_2 if subtitle else lv_colors.BLACK, 0
            )
            self.subtitle.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, 24)

        self.subtitle.set_text(subtitle)
        keyboard_text = self.keyboard.ta.get_text()
        if keyboard_text:
            if subtitle:
                self.keyboard.ta.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 10)
            else:
                self.keyboard.ta.align(lv.ALIGN.TOP_MID, 0, 188)

    def _show_fingerprint_prompt_if_necessary(self):
        from . import fingerprints

        if self.allow_fingerprint and fingerprints.is_available():
            self.fingerprint_prompt = lv.img(self.content_area)
            self.fingerprint_prompt.set_src("A:/res/fingerprint-prompt.png")
            self.fingerprint_prompt.set_pos(414, 30)
            self.anim = lv.anim_t()
            self.anim.init()
            self.anim.set_var(self.fingerprint_prompt)
            self.anim.set_values(414, 404)
            self.anim.set_time(100)
            self.anim.set_playback_delay(10)
            self.anim.set_playback_time(100)
            self.anim.set_repeat_delay(20)
            self.anim.set_repeat_count(2)
            self.anim.set_path_cb(lv.anim_t.path_ease_in_out)
            self.anim.set_custom_exec_cb(lambda _a, val: self.anim_set_x(val))

    def anim_set_x(self, val):
        try:
            self.fingerprint_prompt.set_x(val)
        except Exception:
            pass

    def refresh_fingerprint_prompt(self):
        if hasattr(self, "fingerprint_prompt"):
            try:
                self.fingerprint_prompt.delete()
                del self.fingerprint_prompt
                del self.anim
                self.change_subtitle("")
            except Exception:
                pass

    def show_fp_failed_prompt(self, level: int = 0):
        if level:
            if level == 1:
                subtitle = _(i18n_keys.MSG__FINGERPRINT_NOT_RECOGNIZED_TRY_AGAIN)
            elif level == 2:
                subtitle = _(
                    i18n_keys.MSG__YOUR_PIN_CODE_REQUIRED_TO_ENABLE_FINGERPRINT_UNLOCK
                )
            elif level == 3:
                subtitle = _(i18n_keys.MSG__PUT_FINGER_ON_THE_FINGERPRINT)
            elif level == 4:
                subtitle = _(i18n_keys.MSG__CLEAN_FINGERPRINT_SENSOR_AND_TRY_AGAIN)
            else:
                subtitle = ""
            self.change_subtitle(subtitle)
        if hasattr(self, "fingerprint_prompt"):
            lv.anim_t.start(self.anim)

    def on_event(self, event_obj):
        code = event_obj.code
        if code == lv.EVENT.VALUE_CHANGED:
            utils.lcd_resume()
            if self.keyboard.ta.get_text() != "":
                from apps.common import passphrase

                if self.standy_wall_only and passphrase.is_passphrase_pin_enabled():

                    self.change_subtitle(_(i18n_keys.CONTENT__PIN_FOR_STANDARD_WALLET))
                else:
                    self.change_subtitle("")
            return
        elif code == lv.EVENT.READY:
            input_text = self.keyboard.ta.get_text()
            if len(input_text) < self.min_len:
                return
            self.channel.publish(input_text)
        elif code == lv.EVENT.CANCEL:
            self.channel.publish(0)

        self.clean()
        self.destroy(250)


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


class InputLitePinConfirm(FullSizeWindow):
    def __init__(self, title):
        super().__init__(
            title=title,
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
        self.clear_flag(lv.obj.FLAG.SCROLLABLE)
        self.keyboard = NumberKeyboard(self, max_len=6, min_len=6)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.READY, None)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.CANCEL, None)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.VALUE_CHANGED, None)
        self.input_result = None

    def on_event(self, event_obj):
        code = event_obj.code
        if code == lv.EVENT.VALUE_CHANGED:
            utils.lcd_resume()
            return
        elif code == lv.EVENT.READY:
            input = self.keyboard.ta.get_text()
            if len(input) < 6:
                return
            self.input_result = input
            self.channel.publish(self.input_result)
        elif code == lv.EVENT.CANCEL:
            self.channel.publish(0)

        self.clean()
        self.destroy()


async def pin_mismatch(ctx) -> None:
    from trezor.ui.layouts import show_warning

    await show_warning(
        ctx=ctx,
        br_type="pin_not_match",
        header=_(i18n_keys.TITLE__NOT_MATCH),
        content=_(
            i18n_keys.CONTENT__THE_TWO_ONEKEY_LITE_USED_FOR_CONNECTION_ARE_NOT_THE_SAME
        ),
        icon="A:/res/danger.png",
        btn_yes_bg_color=lv_colors.ONEKEY_BLACK,
    )


async def request_lite_pin(ctx, prompt: str) -> str:
    pin_screen = InputLitePinConfirm(prompt)
    pin = await ctx.wait(pin_screen.request())
    return pin


async def request_lite_pin_confirm(ctx) -> str:
    while True:
        pin1 = await request_lite_pin(ctx, _(i18n_keys.TITLE__SET_ONEKEY_LITE_PIN))
        if pin1 == 0:
            return pin1
        pin2 = await request_lite_pin(ctx, _(i18n_keys.TITLE__CONFIRM_ONEKEY_LITE_PIN))
        if pin2 == 0:
            return pin2
        if pin1 == pin2:
            return pin1
        await pin_mismatch(ctx)


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


class InputPassphrasePinConfirm(FullSizeWindow):
    def __init__(self, title, original_input=None):
        super().__init__(
            title=title,
            subtitle=None,
            anim_dir=0,
        )
        self.title.add_style(
            StyleWrapper().text_font(font_GeistSemiBold48).text_align_center(),
            0,
        )
        self.title.align(lv.ALIGN.TOP_MID, 0, 24)
        self.clear_flag(lv.obj.FLAG.SCROLLABLE)
        self.keyboard = NumberKeyboard(self, max_len=50, min_len=6)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.READY, None)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.CANCEL, None)
        self.keyboard.add_event_cb(self.on_event, lv.EVENT.VALUE_CHANGED, None)
        self.input_result = None

    def on_event(self, event_obj):
        code = event_obj.code
        if code == lv.EVENT.VALUE_CHANGED:
            utils.lcd_resume()
            return
        elif code == lv.EVENT.READY:
            input = self.keyboard.ta.get_text()
            if len(input) < 6:
                return
            self.input_result = input
            self.channel.publish(self.input_result)
        elif code == lv.EVENT.CANCEL:
            self.channel.publish(0)

        self.clean()
        self.destroy()


async def request_passphrase_pin(ctx, prompt: str) -> str:
    pin_screen = InputPassphrasePinConfirm(prompt)
    pin = await ctx.wait(pin_screen.request())
    return pin


async def request_passphrase_pin_confirm(ctx) -> str:
    while True:
        pin1 = await request_passphrase_pin(
            ctx, _(i18n_keys.PASSPHRASE__SET_PASSPHRASE_PIN)
        )
        if pin1 == 0:
            return pin1

        pin2 = await request_passphrase_pin(ctx, _(i18n_keys.TITLE__ENTER_PIN_AGAIN))
        if pin2 == 0:
            return pin2
        if pin1 == pin2:
            return pin1
        await passphrase_pin_mismatch(ctx)


async def passphrase_pin_mismatch(ctx) -> None:
    from trezor.ui.layouts import show_warning

    await show_warning(
        ctx=ctx,
        br_type="pin_not_match",
        header=_(i18n_keys.TITLE__NOT_MATCH),
        content=_(i18n_keys.SUBTITLE__SETUP_SET_PIN_PIN_NOT_MATCH),
        icon="A:/res/danger.png",
        btn_yes_bg_color=lv_colors.ONEKEY_BLACK,
    )


class InputMainPin(FullSizeWindow):
    def __init__(self):
        super().__init__(
            title=_(i18n_keys.TITLE__ENTER_PIN),
            subtitle=_(i18n_keys.CONTENT__PIN_FOR_STANDARD_WALLET),
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
        self.clear_flag(lv.obj.FLAG.SCROLLABLE)
        self.keyboard = NumberKeyboard(self, max_len=50, min_len=4)
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
