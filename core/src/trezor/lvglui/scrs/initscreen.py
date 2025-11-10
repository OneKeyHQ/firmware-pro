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
        # QuickStart()
        workflow.spawn(
            setup_onboarding(language),
        )

    def on_crt_btn(self, _event_obj):
        from .template import CertificationInfo

        CertificationInfo()

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)


async def setup_onboarding(lang: str) -> None:

    utils.mark_initialization_processing()
    if lang is not None:
        i18n_refresh(lang)

    utils.play_dead()

    try:
        from apps.common.request_pin import request_pin_confirm
        from trezor import config, wire

        # request and set new PIN
        newpin = await request_pin_confirm(DUMMY_CONTEXT)
        if not config.change_pin("", newpin, None, None):
            raise wire.ProcessError("Failed to set PIN")
        from apps.common.request_pin import show_pin_set_success

        await show_pin_set_success(DUMMY_CONTEXT)
        if not __debug__:
            from trezor.lvglui.scrs import fingerprints

            await fingerprints.request_add_fingerprint()

        # create wallet or recovery wallet
        while True:
            screen = SetupDevice()
            result = await screen.request()
            if isinstance(result, tuple):  # create wallet with custom backup type
                backup_type, strength = result
                await reset_device(
                    DUMMY_CONTEXT,
                    ResetDevice(
                        strength=strength,
                        language=language,
                        backup_type=backup_type,
                    ),
                )
                break
            elif result is None:
                screen.destroy(10)
                continue
            elif result == 0:  # recovery wallet
                recovery_type = await SelectImportType().request()
                try:
                    if recovery_type == 0:
                        await recovery_device(
                            DUMMY_CONTEXT,
                            RecoveryDevice(
                                enforce_wordlist=True,
                                language=language,
                            ),
                            "phrase",
                        )
                    elif recovery_type == 1:
                        await recovery_device(
                            DUMMY_CONTEXT,
                            RecoveryDevice(
                                enforce_wordlist=True,
                                language=language,
                            ),
                            "lite",
                        )
                except BaseException:
                    continue
                else:
                    break
            elif result == 1:  # create wallet with default backup type
                await reset_device(
                    DUMMY_CONTEXT,
                    ResetDevice(
                        strength=128,
                        language=language,
                    ),
                )
                break
        from trezor.ui.layouts import show_onekey_app_guide

        await show_onekey_app_guide()
    except BaseException as e:
        raise e
    finally:
        utils.mark_initialization_done()
        from trezor import loop

        loop.clear()


class SetupDevice(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__SET_YOUR_WALLET),
            _(i18n_keys.TITLE__SET_YOUR_WALLET_DESC),
            confirm_text=_(i18n_keys.BUTTON__CREATE_NEW_WALLET),
            cancel_text=_(i18n_keys.BUTTON__IMPORT_WALLET),
            anim_dir=0,
        )
        self.add_nav_back_right(btn_bg_img="A:/res/nav-options-icon.png")
        self.btn_layout_ver()
        self.add_event_cb(self.on_nav_back, lv.EVENT.CLICKED, None)

    def destroy(self, delay_ms=50):
        try:
            self.del_delayed(delay_ms)
        except Exception:
            pass

    def on_nav_back(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.nav_back.nav_btn:
                from .reset_device import BackupTypeSelector

                BackupTypeSelector(self)


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
        optional_str = _(i18n_keys.TITLE__RECOVERY_PHRASE) + "\n" + "OneKey Lite"
        self.choices = RadioTrigger(self, optional_str)
        self.add_event_cb(self.on_ready, lv.EVENT.READY, None)
        # self.add_event_cb(self.on_back, lv.EVENT.CLICKED, None)
        # self.add_event_cb(self.on_nav_back, lv.EVENT.GESTURE, None)

    # def on_nav_back(self, event_obj):
    #     code = event_obj.code
    #     if code == lv.EVENT.GESTURE:
    #         _dir = lv.indev_get_act().get_gesture_dir()
    #         if _dir == lv.DIR.RIGHT:
    #             lv.event_send(self.nav_back.nav_btn, lv.EVENT.CLICKED, None)

    def on_ready(self, event_obj):
        code = event_obj.code
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
        self.show_dismiss_anim()
        selected_index = self.choices.get_selected_index()
        self.channel.publish(selected_index)
        # if selected_index == 0:
        #     workflow.spawn(
        #         recovery_device(
        #             DUMMY_CONTEXT,
        #             RecoveryDevice(
        #                 enforce_wordlist=True,
        #                 language=language,
        #                 pin_protection=True,
        #             ),
        #             "phrase",
        #         )
        #     )
        # elif selected_index == 1:
        #     workflow.spawn(
        #         recovery_device(
        #             DUMMY_CONTEXT,
        #             RecoveryDevice(
        #                 enforce_wordlist=True,
        #                 language=language,
        #                 pin_protection=True,
        #             ),
        #             "lite",
        #         )
        #     )

    # def on_back(self, event_obj):
    #     target = event_obj.get_target()
    #     if target == self.nav_back.nav_btn:
    #         self.channel.publish(0)
    #         self.show_dismiss_anim()
