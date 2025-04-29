from .. import font_GeistRegular26, lv, lv_colors
from ..widgets.style import StyleWrapper


class Overlay(lv.obj):
    def __init__(self, parent):
        super().__init__(parent)
        self.set_size(lv.pct(100), lv.pct(100))
        self.add_style(
            StyleWrapper().bg_color(lv_colors.BLACK).bg_opa(lv.OPA._80).border_width(0),
            0,
        )


class OverlayWithProcessBar(Overlay):
    def __init__(self, parent, max_value: int, value: int = 0):
        super().__init__(parent)
        self.max_value = max_value
        self.process_bar = lv.bar(self)
        self.process_bar.set_size(408, 12)
        self.process_bar.add_style(
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_GRAY_3)
            .bg_opa(lv.OPA.COVER)
            .radius(40),
            0,
        )
        self.process_bar.add_style(
            StyleWrapper().bg_color(lv_colors.WHITE),
            lv.PART.INDICATOR | lv.STATE.DEFAULT,
        )
        self.process_bar.align(lv.ALIGN.CENTER, 0, 0)
        self.process_bar.set_range(0, max_value)
        self.process_bar.set_value(value, lv.ANIM.OFF)
        self.precess_value = lv.label(self)
        self.precess_value.set_text(f"{value}/{max_value}")
        self.precess_value.add_style(
            StyleWrapper().text_font(font_GeistRegular26).text_color(lv_colors.WHITE),
            0,
        )
        self.precess_value.align_to(self.process_bar, lv.ALIGN.OUT_TOP_MID, 0, -16)

    def set_value(self, value: int):
        self.process_bar.set_value(value, lv.ANIM.ON)
        self.precess_value.set_text(f"{value}/{self.max_value}")


# class ScanSuccessOverlay(Overlay):
#     def __init__(self, parent, text: str):
#         super().__init__(parent)
#         self.set_size(lv.pct(100), 684)
#         self.add_style(
#             StyleWrapper().bg_color(lv_colors.BLACK).bg_opa(lv.OPA._90).border_width(0),
#             0,
#         )
#         self.fake_bg = lv.img(self)
#         self.fake_bg.set_src("A:/res/scan-success-overlay.png")
#         self.fake_bg.set_size(480, 480)
#         self.fake_bg.align(lv.ALIGN.TOP_MID, 0, 49)
#         self.panel = lv.obj(self)
#         self.panel.add_style(
#             StyleWrapper().bg_opa(lv.OPA.TRANSP).border_width(0),
#             0,
#         )
#         self.panel.set_size(440, lv.SIZE.CONTENT)
#         self.panel.align(lv.ALIGN.TOP_MID, 0, 223)
#         self.scan_loading = lv.img(self.panel)
#         self.scan_loading.set_src("A:/res/scan-success-loading.png")
#         self.scan_loading.set_size(64, 64)
#         self.scan_loading.align(lv.ALIGN.TOP_MID, 0, 0)
#         self.tips = lv.label(self.panel)
#         self.tips.set_text(text)
#         self.tips.add_style(
#             StyleWrapper()
#             .text_font(font_GeistRegular30)
#             .text_color(lv_colors.LIGHT_GRAY),
#             0,
#         )
#         self.tips.align_to(self.scan_loading, lv.ALIGN.OUT_BOTTOM_MID, 0, 16)
#         self.align(lv.ALIGN.BOTTOM_MID, 0, 0)
