from trezor.lvglui.i18n import gettext as _, keys as i18n_keys

from .. import (
    font_GeistRegular20,
    font_GeistRegular26,
    font_GeistSemiBold30,
    lv,
    lv_colors,
)
from ..widgets.style import StyleWrapper
from .transition import BtnClickTransition, DefaultTransition


class NormalButton(lv.btn):
    def __init__(self, parent, text=_(i18n_keys.BUTTON__NEXT), enable=True) -> None:
        super().__init__(parent)
        self.remove_style_all()
        self.set_size(456, 98)
        self.align_to(parent, lv.ALIGN.BOTTOM_MID, 0, -8)
        self.add_style(
            StyleWrapper()
            .radius(98)
            .bg_opa(lv.OPA.COVER)
            .text_opa(lv.OPA.COVER)
            .text_letter_space(-1)
            .text_font(font_GeistSemiBold30),
            0,
        )
        if enable:
            self.enable()
        else:
            self.disable()
        self.add_style(
            StyleWrapper()
            .bg_opa(lv.OPA._60)
            .transform_height(-2)
            .transform_width(-2)
            .transition(BtnClickTransition()),
            lv.PART.MAIN | lv.STATE.PRESSED,
        )
        # the next btn label
        self.label = lv.label(self)
        self.label.set_long_mode(lv.label.LONG.WRAP)
        self.label.set_text(text)
        self.label.set_align(lv.ALIGN.CENTER)
        self.add_flag(lv.obj.FLAG.EVENT_BUBBLE)

        self.click_mask = lv.obj(self)
        self.click_mask.add_style(
            StyleWrapper()
            .align(lv.ALIGN.CENTER)
            .height(50)
            .width(140)
            .bg_opa(lv.OPA.TRANSP)
            .border_width(0),
            0,
        )
        self.click_mask.clear_flag(lv.obj.FLAG.CLICKABLE)
        self.click_mask.add_flag(lv.obj.FLAG.EVENT_BUBBLE)

    def disable(
        self, bg_color=lv_colors.ONEKEY_BLACK_1, text_color=lv_colors.ONEKEY_GRAY
    ) -> None:
        self.add_style(StyleWrapper().bg_color(bg_color).text_color(text_color), 0)
        self.clear_flag(lv.btn.FLAG.CLICKABLE)

    def enable(
        self, bg_color=lv_colors.ONEKEY_GRAY_3, text_color=lv_colors.WHITE
    ) -> None:
        self.add_style(StyleWrapper().bg_color(bg_color).text_color(text_color), 0)
        self.add_flag(lv.btn.FLAG.CLICKABLE)

    def enable_no_bg_mode(self):
        self.add_style(StyleWrapper().bg_color(lv_colors.BLACK), 0)
        self.add_style(
            StyleWrapper().bg_color(lv_colors.ONEKEY_BLACK).bg_opa(),
            lv.PART.MAIN | lv.STATE.PRESSED,
        )
        self.clear_flag(lv.obj.FLAG.CLICKABLE)
        self.click_mask.add_flag(lv.obj.FLAG.CLICKABLE)


class ListItemBtn(lv.btn):
    def __init__(
        self,
        parent,
        text: str,
        right_text="",
        left_img_src: str = "",
        has_next: bool = False,
        has_bgcolor=True,
        use_transition=True,
        min_height=94,
        pad_ver=28,
    ) -> None:
        super().__init__(parent)
        self.remove_style_all()
        self.unique_bg = has_bgcolor
        self.set_size(456, lv.SIZE.CONTENT)
        self.add_style(
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_BLACK_3 if has_bgcolor else lv_colors.BLACK)
            .bg_opa(lv.OPA.COVER)
            .min_height(min_height)
            .text_font(font_GeistSemiBold30)
            .text_color(lv_colors.WHITE)
            .text_letter_space(-1)
            .pad_hor(24)
            .pad_ver(pad_ver),
            0,
        )
        if use_transition:
            self.add_style(
                StyleWrapper().bg_color(lv_colors.ONEKEY_BLACK_2).transform_height(-2)
                # .transform_width(-4)
                .transition(DefaultTransition()),
                lv.PART.MAIN | lv.STATE.PRESSED,
            )
        if left_img_src:
            self.img_left = lv.img(self)
            self.img_left.set_src(left_img_src)
            self.img_left.set_align(lv.ALIGN.LEFT_MID)
            self.img_left.add_flag(lv.obj.FLAG.CLICKABLE)
        if has_next:
            self.img_right = lv.img(self)
            self.img_right.set_src("A:/res/arrow-right.png")
            self.img_right.set_align(lv.ALIGN.RIGHT_MID)
        self.label_left = lv.label(self)
        self.label_left.set_width(360)
        self.label_left.set_long_mode(lv.label.LONG.WRAP)
        self.label_left.set_text(text)

        if left_img_src:
            self.label_left.align_to(self.img_left, lv.ALIGN.OUT_RIGHT_MID, 16, 0)
        else:
            self.label_left.set_align(lv.ALIGN.LEFT_MID)
        if right_text:
            self.label_right = lv.label(self)
            self.label_right.set_long_mode(lv.label.LONG.WRAP)
            self.label_right.set_width(225)
            self.label_right.set_text(right_text)
            self.label_right.add_style(
                StyleWrapper()
                .text_font(font_GeistRegular26)
                .text_color(lv_colors.LIGHT_GRAY)
                .text_letter_space(-1)
                .text_align_right(),
                0,
            )
            if has_next:
                self.label_right.align_to(self.img_right, lv.ALIGN.OUT_LEFT_MID, -10, 0)
            else:
                self.label_right.align(lv.ALIGN.RIGHT_MID, 0, 0)
        self.add_flag(lv.obj.FLAG.EVENT_BUBBLE)

    def add_check_img(self) -> None:
        self.img_right = lv.img(self)
        self.img_right.set_src("A:/res/checked-solid.png")
        self.img_right.set_align(lv.ALIGN.RIGHT_MID)
        self.img_right.add_flag(lv.obj.FLAG.HIDDEN)

    def set_checked(self) -> None:
        if self.img_right.has_flag(lv.obj.FLAG.HIDDEN):
            self.img_right.clear_flag(lv.obj.FLAG.HIDDEN)
            # self.label_left.set_style_text_color(lv_colors.WHITE, 0)
            if not self.unique_bg:
                self.add_style(StyleWrapper().bg_color(lv_colors.ONEKEY_GRAY_2), 0)

    def set_uncheck(self) -> None:
        if not self.img_right.has_flag(lv.obj.FLAG.HIDDEN):
            self.img_right.add_flag(lv.obj.FLAG.HIDDEN)
            # self.label_left.set_style_text_color(lv_colors.WHITE_2, 0)
            if not self.unique_bg:
                self.add_style(StyleWrapper().bg_color(lv_colors.BLACK), 0)

    def is_unchecked(self) -> bool:
        return self.img_right.has_flag(lv.obj.FLAG.HIDDEN)

    def text_layout_vertical(self, pad_top: int = 23, pad_ver: int = 23) -> None:
        assert hasattr(self, "img_left"), "No left image"
        self.add_style(
            StyleWrapper().pad_ver(pad_ver).pad_top(pad_top).min_height(104), 0
        )
        self.label_left.align_to(self.img_left, lv.ALIGN.OUT_RIGHT_TOP, 16, -6)
        self.label_right.set_width(344)
        self.label_left.add_style(StyleWrapper().pad_all(0), 0)
        self.label_right.add_style(
            StyleWrapper().text_font(font_GeistRegular20).text_align_left().pad_all(0),
            0,
        )
        self.label_right.align_to(self.label_left, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 4)

    def disable(self) -> None:
        self.add_style(
            StyleWrapper().bg_color(lv_colors.ONEKEY_GRAY_3),
            0,
        )
        self.label_left.set_style_text_color(lv_colors.WHITE_2, 0)
        if hasattr(self, "label_right"):
            self.label_right.set_style_text_color(lv_colors.ONEKEY_GRAY_1, 0)
        self.clear_flag(lv.obj.FLAG.CLICKABLE)


class ListItemBtnWithSwitch(lv.btn):
    def __init__(self, parent, text: str) -> None:
        super().__init__(parent)
        self.remove_style_all()

        self.set_size(456, 94)
        self.add_style(
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_GRAY_3)
            .bg_opa(lv.OPA.COVER)
            .radius(0)
            .pad_hor(24)
            .pad_ver(23)
            .text_font(font_GeistSemiBold30)
            .text_letter_space(-1)
            .text_color(lv_colors.WHITE),
            0,
        )
        self.add_style(
            StyleWrapper()
            # .bg_color(lv_colors.ONEKEY_BLACK_2)
            .transition(DefaultTransition()),
            lv.PART.MAIN | lv.STATE.PRESSED,
        )

        label_left = lv.label(self)
        label_left.set_width(330)
        label_left.set_text(text)
        label_left.set_long_mode(lv.label.LONG.WRAP)
        label_left.set_align(lv.ALIGN.LEFT_MID)
        self.switch = lv.switch(self)
        self.switch.set_size(80, 48)
        self.switch.set_align(lv.ALIGN.RIGHT_MID)
        self.switch.add_flag(lv.obj.FLAG.EVENT_BUBBLE)

        self.switch.add_style(
            StyleWrapper().bg_color(lv_colors.ONEKEY_GRAY).radius(24), 0
        )
        self.switch.add_style(
            StyleWrapper().bg_color(lv_colors.ONEKEY_GREEN).radius(24),
            lv.PART.INDICATOR | lv.STATE.CHECKED,
        )
        self.switch.add_style(
            StyleWrapper().bg_color(lv_colors.WHITE).pad_all(-8),
            lv.PART.KNOB | lv.STATE.DEFAULT,
        )
        self.switch.add_state(lv.STATE.CHECKED)
        self.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
        self.add_event_cb(self.eventhandler, lv.EVENT.CLICKED, None)

    def eventhandler(self, event) -> None:
        code = event.code
        target = event.get_target()
        if code == lv.EVENT.CLICKED and target != self.switch:
            if self.switch.get_state() == lv.STATE.CHECKED:
                self.clear_state()
            else:
                self.add_state()
            lv.event_send(self.switch, lv.EVENT.VALUE_CHANGED, None)

    def clear_state(self) -> None:
        self.switch.clear_state(lv.STATE.CHECKED)

    def add_state(self) -> None:
        self.switch.add_state(lv.STATE.CHECKED)
