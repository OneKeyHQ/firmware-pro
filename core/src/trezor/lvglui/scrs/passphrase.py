from ..i18n import gettext as _, keys as i18n_keys
from . import font_GeistSemiBold26, lv_colors
from .common import FullSizeWindow, lv
from .components.keyboard import PassphraseKeyboard
from .widgets.style import StyleWrapper


class PassphraseRequest(FullSizeWindow):
    def __init__(self, max_len):
        super().__init__(_(i18n_keys.CONTENT__ENTER_PASSPHRASE_COLON), None, anim_dir=0)
        self.add_nav_back()
        self.title.add_style(
            StyleWrapper()
            .text_font(font_GeistSemiBold26)
            .text_color(lv_colors.WHITE_2)
            .text_align_left()
            .text_letter_space(-1)
            .text_line_space(0),
            0,
        )
        self.keyboard = PassphraseKeyboard(self, max_len)
        self.keyboard.add_event_cb(self.on_ready, lv.EVENT.READY, None)

        self.nav_back.add_event_cb(self.on_cancel, lv.EVENT.CLICKED, None)
        # self.add_event_cb(self.on_nav_back, lv.EVENT.GESTURE, None)

    # def on_nav_back(self, event_obj):
    #     code = event_obj.code
    #     if code == lv.EVENT.GESTURE:
    #         _dir = lv.indev_get_act().get_gesture_dir()
    #         if _dir == lv.DIR.RIGHT:
    #             lv.event_send(self.nav_back.nav_btn, lv.EVENT.CLICKED, None)

    def on_ready(self, event_obj):
        input = self.keyboard.ta.get_text()
        self.channel.publish(input)
        self.keyboard.ta.set_text("")
        self.destroy(100)

    def on_cancel(self, event_obj):
        target = event_obj.get_target()
        if target == self.nav_back.nav_btn:
            self.channel.publish(None)
            self.destroy(10)
