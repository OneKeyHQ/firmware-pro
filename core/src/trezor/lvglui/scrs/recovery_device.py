from ..i18n import gettext as _, keys as i18n_keys
from . import font_GeistSemiBold26
from .common import FullSizeWindow, lv, lv_colors
from .components.keyboard import BIP39Keyboard
from .components.radio import RadioTrigger
from .widgets.style import StyleWrapper


class WordEnter(FullSizeWindow):
    def __init__(self, title):
        super().__init__(title, None, anim_dir=0)
        self.add_nav_back()
        self.title.add_style(
            StyleWrapper()
            .text_font(font_GeistSemiBold26)
            .text_color(lv_colors.WHITE_2)
            .text_align_left()
            .text_letter_space(-1),
            0,
        )
        self.keyboard = BIP39Keyboard(self)
        self.keyboard.add_event_cb(self.on_ready, lv.EVENT.READY, None)
        self.add_event_cb(self.on_back, lv.EVENT.CLICKED, None)
        self.submitted = False

    #     self.add_event_cb(self.on_nav_back, lv.EVENT.GESTURE, None)

    # def on_nav_back(self, event_obj):
    #     code = event_obj.code
    #     if code == lv.EVENT.GESTURE:
    #         _dir = lv.indev_get_act().get_gesture_dir()
    #         if _dir == lv.DIR.RIGHT:
    #             lv.event_send(self.nav_back.nav_btn, lv.EVENT.CLICKED, None)

    def on_ready(self, _event_obj):
        if self.submitted:
            return
        input = self.keyboard.ta.get_text()
        if input == "":
            return
        self.submitted = True
        self.channel.publish(input)
        self.destroy(1000)

    def clear_input(self):
        self.keyboard.ta.set_text("")

    def show_tips(self):
        self.keyboard.tip_submitted()

    def on_back(self, event_obj):
        target = event_obj.get_target()
        if target == self.nav_back.nav_btn:
            self.channel.publish(0)
            self.show_dismiss_anim()


class SelectWordCounter(FullSizeWindow):
    def __init__(self, title: str):
        super().__init__(
            title, _(i18n_keys.SUBTITLE__DEVICE_RECOVER_READY_TO_RESTORE), anim_dir=0
        )
        self.add_nav_back()
        optional_str = (
            _(i18n_keys.OPTION__STR_WRODS).format(12)
            + "\n"
            + _(i18n_keys.OPTION__STR_WRODS).format(18)
            + "\n"
            + _(i18n_keys.OPTION__STR_WRODS).format(24)
        )
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

    def on_ready(self, _event_obj):
        self.show_dismiss_anim()
        self.channel.publish(int(self.choices.get_selected_str().split()[0]))

    def on_back(self, event_obj):
        target = event_obj.get_target()
        if target == self.nav_back.nav_btn:
            self.channel.publish(0)
            self.show_dismiss_anim()
