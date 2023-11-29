from ..i18n import gettext as _, keys as i18n_keys
from . import font_GeistSemiBold64, lv, lv_colors
from .common import FullSizeWindow
from .widgets.style import StyleWrapper


class PairCodeDisplay(FullSizeWindow):
    def __init__(self, pair_code: str):
        super().__init__(
            _(i18n_keys.TITLE__BLUETOOTH_PAIR),
            _(i18n_keys.SUBTITLE__BLUETOOTH_PAIR),
            _(i18n_keys.BUTTON__CLOSE),
            icon_path="A:/res/icon-bluetooth.png",
        )
        self.panel = lv.obj(self.content_area)
        self.panel.set_size(456, lv.SIZE.CONTENT)
        self.panel.add_style(
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_DARK_BLUE)
            .radius(40)
            .pad_ver(48)
            .pad_hor(24)
            .border_width(0)
            .text_font(font_GeistSemiBold64)
            .text_color(lv_colors.WHITE),
            0,
        )
        self.panel.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 40)
        self.pair_code = lv.label(self.panel)
        self.pair_code.set_long_mode(lv.label.LONG.WRAP)
        self.pair_code.set_style_text_letter_space(-2, lv.PART.MAIN | lv.STATE.DEFAULT)
        self.pair_code.set_text(pair_code)
        self.pair_code.align(lv.ALIGN.CENTER, 0, 0)
        self.btn_yes.enable()
        self.destroyed = False

    def destroy(self, delay_ms=100):
        super().destroy(delay_ms)
        self.destroyed = True
