from trezor.utils import lcd_resume

from ..i18n import gettext as _, keys as i18n_keys
from . import font_GeistMono28, font_GeistSemiBold26
from .common import FullSizeWindow, lv, lv_colors
from .components.button import NormalButton
from .components.container import ContainerGrid
from .components.keyboard import MnemonicKeyboard
from .components.radio import RadioTrigger
from .widgets.style import StyleWrapper


class WordEnter(FullSizeWindow):
    def __init__(self, title: str, is_slip39: bool = False):
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
        self.keyboard = MnemonicKeyboard(self, is_slip39)
        self.keyboard.add_event_cb(self.on_ready, lv.EVENT.READY, None)
        self.add_event_cb(self.on_nav_back, lv.EVENT.CLICKED, None)
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

    def on_nav_back(self, event_obj):
        target = event_obj.get_target()
        if target == self.nav_back.nav_btn:
            self.channel.publish(0)
            self.destroy(400)


class SelectWordCounter(FullSizeWindow):
    def __init__(self, title: str, optional_str: str):
        super().__init__(
            title, _(i18n_keys.SUBTITLE__DEVICE_RECOVER_READY_TO_RESTORE), anim_dir=0
        )
        self.add_nav_back_right()
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


class InvalidMnemonic(FullSizeWindow):
    def __init__(self, mnemonics: list[str]):
        word_count = len(mnemonics)
        super().__init__(
            _(i18n_keys.INVALID_PHRASES__TITLE),
            _(i18n_keys.INVALID_PHRASES__DESC),
            icon_path="A:/res/danger.png",
            anim_dir=0,
        )
        self.content_area.set_style_max_height(756, 0)
        row_dsc = [66] * (int((word_count + 1) // 2))
        row_dsc.append(lv.GRID_TEMPLATE.LAST)
        # 3 columns
        col_dsc = [
            225,
            225,
            lv.GRID_TEMPLATE.LAST,
        ]
        self.container = ContainerGrid(
            self.content_area,
            row_dsc=row_dsc,
            col_dsc=col_dsc,
            align_base=self.subtitle,
            pos=(-12, 40),
            pad_gap=10,
        )
        self.container.set_grid_align(lv.GRID_ALIGN.SPACE_BETWEEN, lv.GRID_ALIGN.CENTER)
        word_style = (
            StyleWrapper()
            .pad_hor(8)
            .pad_ver(16)
            .radius(40)
            .bg_color(lv_colors.ONEKEY_GRAY_3)
            .bg_opa(lv.OPA.COVER)
            .text_align_left()
        )
        self.container.add_style(
            StyleWrapper().text_font(font_GeistMono28).text_color(lv_colors.WHITE),
            0,
        )
        self.clear_flag(lv.obj.FLAG.SCROLLABLE)
        self.content_area.set_scroll_dir(lv.DIR.VER)
        self.content_area.clear_flag(lv.obj.FLAG.SCROLL_ELASTIC)
        self.content_area.clear_flag(lv.obj.FLAG.SCROLL_MOMENTUM)
        half = (word_count + 1) // 2
        self.words = []
        for i in range(word_count):
            col = 0 if i < half else 1
            row = i % half
            word = lv.obj(self.container)
            word.remove_style_all()
            word.add_style(word_style, 0)
            word_label = lv.label(word)
            word_label.set_align(lv.ALIGN.LEFT_MID)
            word_label.set_text(f"{i+1:>2}. {mnemonics[i]}")
            word.set_grid_cell(
                lv.GRID_ALIGN.STRETCH, col, 1, lv.GRID_ALIGN.STRETCH, row, 1
            )
            word.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            self.words.append(word)
        self.btn_no = NormalButton(self.content_area, _(i18n_keys.GLOBAL__START_OVER))
        # self.btn_no.enable_no_bg_mode()
        self.btn_no.align_to(self.container, lv.ALIGN.OUT_BOTTOM_MID, 0, 10)

        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def on_click(self, event_obj):
        target = event_obj.get_target()
        for i, word in enumerate(self.words):
            if word == target:
                self.show_dismiss_anim()
                self.channel.publish(i)
                break

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if lcd_resume():
                return
            if target == self.btn_no:
                self.show_dismiss_anim()
                self.channel.publish(None)
