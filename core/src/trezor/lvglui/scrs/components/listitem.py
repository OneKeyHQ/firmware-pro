from .. import (
    font_GeistMono28,
    font_GeistRegular20,
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
        self.label.set_recolor(True)
        self.label.set_text(content)
        self.label.add_style(
            StyleWrapper()
            .text_color(lv_colors.WHITE)
            .text_line_space(6)
            .text_letter_space(-2),
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
        url: str | None = None,
    ) -> None:
        super().__init__(parent, title, content, bg_color, radius, font)
        if url:
            self.url = lv.label(self)
            self.url.set_size(lv.pct(100), lv.SIZE.CONTENT)
            self.url.set_text(url)
            self.url.add_style(
                StyleWrapper()
                .text_color(lv_colors.ONEKEY_GREEN_2)
                .text_font(font_GeistRegular20)
                .text_line_space(6)
                .text_letter_space(-1),
                0,
            )
            self.url.align_to(self.label, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 6)


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
        self.label.set_size(360, lv.SIZE.CONTENT)
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


class DisplayItemWithTextPairs(lv.obj):
    def __init__(
        self,
        parent,
        title,
        content_pairs,
        bg_color=lv_colors.ONEKEY_GRAY_3,
        radius=0,
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

        y_offset = self.label_top.get_height() + 40
        for left_text, right_text in content_pairs:
            label_left = lv.label(self)
            label_left.set_text(left_text)
            label_left.align(lv.ALIGN.TOP_LEFT, 0, y_offset)
            label_right = lv.label(self)
            label_right.set_text(right_text)
            label_right.align(lv.ALIGN.TOP_RIGHT, 0, y_offset)
            y_offset += max(label_left.get_height(), label_right.get_height()) + 40


class DisplayItemWithFont_TextPairs(DisplayItemWithTextPairs):
    def __init__(
        self,
        parent,
        title,
        content_pairs,
        bg_color=lv_colors.ONEKEY_GRAY_3,
        radius: int = 0,
        font=font_GeistRegular30,
    ):
        super().__init__(parent, title, content_pairs, bg_color, radius, font)


class ShortInfoItem(lv.obj):
    def __init__(
        self,
        parent,
        img_src,
        title_text,
        subtitle_text,
        bg_color=lv_colors.ONEKEY_BLACK_4,
        border_color=lv_colors.WHITE_3,
        title_color=lv_colors.WHITE,
        subtitle_color=lv_colors.ONEKEY_GRAY_4,
        icon_boarder_color=lv_colors.WHITE,
    ):
        super().__init__(parent)
        self.remove_style_all()

        self.set_size(400, 70)
        self.add_style(
            StyleWrapper()
            .bg_color(bg_color)
            .bg_opa(lv.OPA._10)
            .radius(40)
            .border_width(1)
            .border_color(border_color)
            .border_opa(lv.OPA._10),
            0,
        )
        self.clear_flag(lv.obj.FLAG.SCROLLABLE)

        if img_src and img_src != "A:/res/evm-none.png":
            self.bottom_circle = lv.obj(self)
            self.bottom_circle.set_size(70, 70)
            self.bottom_circle.align(lv.ALIGN.LEFT_MID, -1, 0)
            self.bottom_circle.add_style(
                StyleWrapper()
                .bg_color(icon_boarder_color)
                .bg_opa(lv.OPA._50)
                .radius(35)
                .border_width(0),
                0,
            )
            self.bottom_circle.clear_flag(lv.obj.FLAG.SCROLLABLE)

            self.middle_circle = lv.obj(self)
            self.middle_circle.set_size(60, 60)
            self.middle_circle.align_to(self.bottom_circle, lv.ALIGN.CENTER, 0, 0)
            self.middle_circle.add_style(
                StyleWrapper()
                .bg_color(lv_colors.WHITE)
                .bg_opa(lv.OPA.COVER)
                .radius(30)
                .border_width(0),
                0,
            )
            self.middle_circle.clear_flag(lv.obj.FLAG.SCROLLABLE)

            self.img = lv.img(self)
            self.img.set_src(img_src)
            self.img.set_zoom(133)
            self.img.align_to(self.middle_circle, lv.ALIGN.CENTER, 0, 0)
            self.img.set_style_radius(25, 0)
            self.img.move_foreground()
        else:
            self.img = lv.img(self)
            self.img.set_src("A:/res/turbo-send.png")
            self.img.set_style_radius(35, 0)
            self.img.move_foreground()
            self.img.align(lv.ALIGN.LEFT_MID, -1, 0)

        self.title = lv.label(self)
        self.title.set_text(title_text)
        self.title.set_long_mode(lv.label.LONG.DOT)
        self.title.set_width(320)
        self.title.add_style(
            StyleWrapper()
            .text_color(title_color)
            .text_font(font_GeistSemiBold26)
            .text_letter_space(0),
            0,
        )
        if hasattr(self, "bottom_circle"):
            self.title.align_to(self.bottom_circle, lv.ALIGN.OUT_RIGHT_MID, 14, -12)
        else:
            self.title.align_to(self.img, lv.ALIGN.OUT_RIGHT_MID, 14, -12)

        self.subtitle = lv.label(self)
        self.subtitle.set_text(subtitle_text)
        self.subtitle.set_long_mode(lv.label.LONG.DOT)
        self.subtitle.set_width(320)
        self.subtitle.add_style(
            StyleWrapper()
            .text_color(subtitle_color)
            .text_font(font_GeistRegular20)
            .text_letter_space(0),
            0,
        )
        self.subtitle.align_to(self.title, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 4)

        self.add_flag(lv.obj.FLAG.CLICKABLE)
