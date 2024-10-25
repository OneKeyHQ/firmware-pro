from .. import font_GeistRegular26, font_GeistSemiBold26, lv, lv_colors
from ..widgets.style import StyleWrapper


class LEVEL:
    DEFAULT = 0
    HIGHLIGHT = 1
    WARNING = 2
    DANGER = 3


class Banner(lv.obj):
    def __init__(self, parent, level: int, text: str, title: str = None) -> None:
        super().__init__(parent)
        self.remove_style_all()
        bg_color, text_color, icon_path = get_style(level)
        self.add_style(
            StyleWrapper()
            .width(456)
            .height(lv.SIZE.CONTENT)
            .text_font(font_GeistRegular26)
            .text_color(text_color)
            .text_letter_space(-1)
            .bg_color(bg_color)
            .bg_opa()
            .radius(40)
            .border_width(0)
            .pad_hor(24)
            .pad_ver(16),
            0,
        )
        self.align(lv.ALIGN.BOTTOM_MID, 0, -8)
        self.lead_icon = lv.img(self)
        self.lead_icon.set_src(icon_path)
        self.lead_icon.set_align(lv.ALIGN.TOP_LEFT)
        if title:
            self.banner_title = lv.label(self)
            self.banner_title.set_size(368, lv.SIZE.CONTENT)
            self.banner_title.set_long_mode(lv.label.LONG.WRAP)
            self.banner_title.add_style(
                StyleWrapper().text_font(font_GeistSemiBold26),
                0,
            )
            self.banner_title.align_to(self.lead_icon, lv.ALIGN.OUT_RIGHT_TOP, 8, 3)
            self.banner_title.set_text(title)
        self.banner_desc = lv.label(self)
        self.banner_desc.set_size(368, lv.SIZE.CONTENT)
        self.banner_desc.set_long_mode(lv.label.LONG.WRAP)
        if title:
            self.banner_desc.align_to(self.banner_title, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 8)
        else:
            self.banner_desc.align_to(self.lead_icon, lv.ALIGN.OUT_RIGHT_TOP, 8, 3)
        self.banner_desc.set_text(text)


def get_style(level: int):
    if level == LEVEL.HIGHLIGHT:
        return (
            lv.color_hex(0x00206B),
            lv.color_hex(0x4178FF),
            "A:/res/banner-icon-blue.png",
        )
    elif level == LEVEL.WARNING:
        return (
            lv.color_hex(0x6B5C00),
            lv.color_hex(0xFFD500),
            "A:/res/banner-icon-yellow.png",
        )
    elif level == LEVEL.DANGER:
        return (
            lv.color_hex(0x640E00),
            lv.color_hex(0xFF1100),
            "A:/res/banner-icon-red.png",
        )
    else:
        return (
            lv_colors.ONEKEY_BLACK_3,
            lv_colors.LIGHT_GRAY,
            "A:/res/banner-icon-gray.png",
        )
