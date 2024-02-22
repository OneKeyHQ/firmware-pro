from .. import (
    font_GeistMono28,
    font_GeistRegular26,
    font_GeistRegular30,
    font_GeistSemiBold26,
    lv,
    lv_colors,
)
from ..widgets.style import StyleWrapper


class ListItemWithLeadingCheckbox(lv.obj):
    def __init__(self, parent, text, radius: int = 0):
        super().__init__(parent)
        self.remove_style_all()
        self.set_size(456, lv.SIZE.CONTENT)
        self.add_style(
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_BLACK_4)
            .bg_opa(lv.OPA.COVER)
            .min_height(94)
            .radius(radius)
            .border_width(1)
            .border_color(lv_colors.ONEKEY_GRAY_2)
            .pad_hor(24)
            .pad_ver(20)
            .text_color(lv_colors.WHITE_1)
            .text_font(font_GeistRegular30)
            .text_letter_space(-1),
            0,
        )
        self.checkbox = lv.checkbox(self)
        self.checkbox.set_align(lv.ALIGN.TOP_LEFT)
        self.checkbox.set_text("")
        self.checkbox.add_style(
            StyleWrapper()
            .pad_all(0)
            .text_align(lv.TEXT_ALIGN.LEFT)
            .text_color(lv_colors.WHITE_1)
            .text_line_space(4),
            0,
        )
        self.checkbox.add_style(
            StyleWrapper()
            .radius(8)
            .pad_all(0)
            .bg_color(lv_colors.ONEKEY_BLACK_4)
            .border_color(lv_colors.ONEKEY_GRAY)
            .border_width(2)
            .border_opa(),
            lv.PART.INDICATOR | lv.STATE.DEFAULT,
        )
        self.checkbox.add_style(
            StyleWrapper()
            .radius(8)
            .bg_color(lv_colors.ONEKEY_GREEN)
            .text_color(lv_colors.BLACK)
            .text_font(font_GeistMono28)
            .text_align(lv.TEXT_ALIGN.CENTER)
            .border_width(0)
            .bg_opa(),
            lv.PART.INDICATOR | lv.STATE.CHECKED,
        )
        self.checkbox.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
        self.label = lv.label(self)
        self.label.remove_style_all()
        self.label.set_long_mode(lv.label.LONG.WRAP)
        self.label.set_size(360, lv.SIZE.CONTENT)
        self.label.align_to(self.checkbox, lv.ALIGN.OUT_RIGHT_TOP, 8, -4)
        self.label.set_text(text)
        self.add_flag(lv.obj.FLAG.EVENT_BUBBLE | lv.obj.FLAG.CLICKABLE)
        self.add_event_cb(self.eventhandler, lv.EVENT.CLICKED, None)

    def eventhandler(self, event):
        code = event.code
        target = event.get_target()
        # if target == self.checkbox ignore instead. because value_change event is also triggered which needless to deal with
        if code == lv.EVENT.CLICKED and target != self.checkbox:
            if self.checkbox.get_state() & lv.STATE.CHECKED:
                self.checkbox.clear_state(lv.STATE.CHECKED)
            else:
                self.checkbox.add_state(lv.STATE.CHECKED)
            lv.event_send(self.checkbox, lv.EVENT.VALUE_CHANGED, None)

    def get_checkbox(self):
        return self.checkbox

    def get_label(self):
        return self.label

    def enable_bg_color(self, enable: bool = True):
        if enable:
            self.add_style(
                StyleWrapper()
                .text_color(lv_colors.WHITE)
                .bg_color(lv_colors.ONEKEY_BLACK_3),
                0,
            )
        else:
            self.add_style(
                StyleWrapper()
                .text_color(lv_colors.WHITE_1)
                .bg_color(lv_colors.ONEKEY_BLACK_4),
                0,
            )


class DisplayItem(lv.obj):
    def __init__(
        self,
        parent,
        title,
        content,
        bg_color=lv_colors.ONEKEY_GRAY_3,
        radius: int = 0,
        font=font_GeistRegular26,
    ):
        super().__init__(parent)
        self.remove_style_all()
        self.set_size(456, lv.SIZE.CONTENT)
        self.add_style(
            StyleWrapper()
            .bg_color(bg_color)
            .bg_opa(lv.OPA.COVER)
            .min_height(82)
            .border_width(0)
            .pad_hor(24)
            .pad_ver(12)
            .radius(radius)
            .text_font(font)
            .text_align_left(),
            0,
        )
        if title:
            self.label_top = lv.label(self)
            self.label_top.set_recolor(True)
            self.label_top.set_size(lv.pct(100), lv.SIZE.CONTENT)
            self.label_top.set_long_mode(lv.label.LONG.WRAP)
            self.label_top.set_text(title)
            self.label_top.set_align(lv.ALIGN.TOP_LEFT)
            self.label_top.add_style(
                StyleWrapper()
                .text_color(lv_colors.ONEKEY_GRAY_4)
                .text_letter_space(-1),
                0,
            )

        self.label = lv.label(self)
        self.label.set_size(lv.pct(100), lv.SIZE.CONTENT)
        self.label.set_text(content)
        self.label.add_style(
            StyleWrapper()
            .text_color(lv_colors.WHITE)
            .text_line_space(6)
            .text_letter_space(-1),
            0,
        )
        if title:
            self.label.align_to(self.label_top, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 4)
        else:
            self.label.set_align(lv.ALIGN.TOP_LEFT)


class DisplayItemWithFont_30(DisplayItem):
    def __init__(
        self,
        parent,
        title,
        content,
        bg_color=lv_colors.ONEKEY_GRAY_3,
        radius: int = 0,
        font=font_GeistRegular30,
    ):
        super().__init__(parent, title, content, bg_color, radius, font)


class CardHeader(lv.obj):
    def __init__(self, parent, title, icon):
        super().__init__(parent)
        self.remove_style_all()
        self.set_size(456, 63)
        self.add_style(
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_GRAY_3)
            .bg_opa()
            .border_width(0)
            .pad_ver(16)
            .pad_bottom(0)
            .pad_hor(24)
            .radius(0)
            .text_font(font_GeistSemiBold26)
            .text_color(lv_colors.WHITE)
            .text_align_left(),
            0,
        )
        self.icon = lv.img(self)
        self.icon.set_src(icon)
        self.icon.set_size(32, 32)
        self.icon.align(lv.ALIGN.TOP_LEFT, 0, 0)
        self.label = lv.label(self)
        self.label.set_text(title)
        self.label.set_size(300, lv.SIZE.CONTENT)
        self.label.set_long_mode(lv.label.LONG.WRAP)
        self.label.align_to(self.icon, lv.ALIGN.OUT_RIGHT_MID, 8, 0)
        self.line = lv.line(self)
        self.line.set_size(408, 1)
        self.line.add_style(
            StyleWrapper().bg_color(lv_colors.ONEKEY_GRAY_2).bg_opa(), 0
        )
        self.line.align_to(self.icon, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 14)


class CardItem(CardHeader):
    def __init__(self, parent, title, content, icon):
        super().__init__(parent, title, icon)
        self.add_style(StyleWrapper().radius(40).pad_bottom(24), 0)
        self.set_size(456, lv.SIZE.CONTENT)
        self.content = lv.obj(self)
        self.content.set_size(408, lv.SIZE.CONTENT)
        self.content.add_style(
            StyleWrapper()
            .pad_all(12)
            .bg_color(lv_colors.ONEKEY_BLACK_3)
            .bg_opa()
            .radius(24)
            .text_color(lv_colors.LIGHT_GRAY)
            .text_font(font_GeistMono28)
            .border_width(0)
            .max_height(364)
            .text_align_left(),
            0,
        )
        self.content_label = lv.label(self.content)
        self.content_label.set_size(384, lv.SIZE.CONTENT)
        self.content_label.set_long_mode(lv.label.LONG.WRAP)
        self.content_label.set_text(content)
        self.content_label.add_style(
            StyleWrapper().text_letter_space(-2).max_height(320), 0
        )
        self.content_label.set_align(lv.ALIGN.CENTER)
        self.content.align_to(self.line, lv.ALIGN.OUT_BOTTOM_MID, 0, 24)


class DisplayItemNoBgc(DisplayItem):
    def __init__(self, parent, title, content):
        super().__init__(parent, title, content, bg_color=lv_colors.BLACK)
        self.add_style(
            StyleWrapper().min_height(0).pad_hor(0),
            0,
        )


class ImgGridItem(lv.img):
    """Home Screen setting display"""

    def __init__(
        self,
        parent,
        col_num,
        row_num,
        file_name: str,
        path_dir: str,
        img_path_other: str = "A:/res/checked-solid.png",
        is_internal: bool = False,
    ):
        super().__init__(parent)
        self.set_grid_cell(
            lv.GRID_ALIGN.CENTER, col_num, 1, lv.GRID_ALIGN.CENTER, row_num, 1
        )
        self.is_internal = is_internal
        self.file_name = file_name
        self.zoom_path = path_dir + file_name
        self.set_src(self.zoom_path)
        self.set_style_radius(40, 0)
        # self.set_style_clip_corner(True, 0)
        self.img_path = self.zoom_path.replace("zoom-", "")
        self.check = lv.img(self)
        self.check.set_src(img_path_other)
        self.check.center()
        self.set_checked(False)
        self.add_flag(lv.obj.FLAG.CLICKABLE)
        self.add_flag(lv.obj.FLAG.EVENT_BUBBLE)

    def set_checked(self, checked: bool):
        if checked:
            self.check.clear_flag(lv.obj.FLAG.HIDDEN)
        else:
            self.check.add_flag(lv.obj.FLAG.HIDDEN)
