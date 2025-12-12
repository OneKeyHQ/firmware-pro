from ..i18n import gettext as _, keys as i18n_keys
from . import font_GeistSemiBold64, lv, lv_colors
from .common import FullSizeWindow
from .widgets.style import StyleWrapper


def _compare_version(version1: str, version2: str) -> int:
    """Compare two version strings.
    Returns:
        -1 if version1 < version2
        0 if version1 == version2
        1 if version1 > version2
    """
    if not version1 or not version2:
        return -1

    try:
        v1_parts = [int(x) for x in version1.split(".")]
        v2_parts = [int(x) for x in version2.split(".")]
    except (ValueError, AttributeError):
        return -1

    # Pad with zeros to make same length
    max_len = max(len(v1_parts), len(v2_parts))
    v1_parts.extend([0] * (max_len - len(v1_parts)))
    v2_parts.extend([0] * (max_len - len(v2_parts)))

    for v1, v2 in zip(v1_parts, v2_parts):
        if v1 < v2:
            return -1
        elif v1 > v2:
            return 1
    return 0


class PairCodeDisplay(FullSizeWindow):
    def __init__(self, pair_code: str):
        from trezor import uart

        ble_version = uart.get_ble_version()
        use_new_ui = ble_version and _compare_version(ble_version, "2.3.5") >= 0

        if use_new_ui:
            subtitle = _(i18n_keys.TITLE__BLUETOOTH_PAIR_DESC)
            confirm_text = _(i18n_keys.BUTTON__YES_THEY_MATCH)
            cancel_text = _(i18n_keys.BUTTON__NO_IT_DOES_NOT)
        else:
            subtitle = _(i18n_keys.SUBTITLE__BLUETOOTH_PAIR)
            confirm_text = ""
            cancel_text = _(i18n_keys.BUTTON__CLOSE)

        super().__init__(
            _(i18n_keys.TITLE__BLUETOOTH_PAIR),
            subtitle,
            confirm_text=confirm_text,
            cancel_text=cancel_text,
            icon_path="A:/res/icon-bluetooth.png",
            anim_dir=0,
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
        self.btn_layout_ver()
        self.destroyed = False

    def show_unload_anim(self):
        self.show_dismiss_anim()

    def destroy(self, delay_ms=100):

        super().destroy(delay_ms)
        self.destroyed = True
        from trezor.lvglui.scrs.homescreen import ScanScreen

        ScanScreen.notify_close()


class PairForbiddenScreen(FullSizeWindow):
    def __init__(self) -> None:
        super().__init__(
            _(i18n_keys.ONBOARDING_BLUETOOTH_PAIRING_BEFORE_SETUP_PIN_TITLE),
            _(i18n_keys.ONBOARDING_BLUETOOTH_PAIRING_BEFORE_SETUP_PIN_DESC),
            _(i18n_keys.BUTTON__CLOSE),
        )
        self.btn_yes.enable()


class PairFailedScreen(FullSizeWindow):
    def __init__(self) -> None:
        self.destroyed = False
        super().__init__(
            _(i18n_keys.TITLE__PAIR_FAILED),
            _(i18n_keys.TITLE__PAIR_FAILED_DESC),
            cancel_text=_(i18n_keys.BUTTON__CLOSE),
            icon_path="A:/res/danger.png",
        )
        self.btn_no.enable()

    def destroy(self, delay_ms=100):
        self.destroyed = True
        from trezor import uart, workflow

        if uart.PAIR_ERROR_SCREEN == self:
            uart.PAIR_ERROR_SCREEN = None
            if uart.PENDING_PAIR_CODE is not None:
                workflow.spawn(uart._show_pending_pair_code())
        super().destroy(delay_ms)


class PairSuccessScreen(FullSizeWindow):
    def __init__(self) -> None:
        self.destroyed = False
        super().__init__(
            _(i18n_keys.TITLE__DEVICE_PAIRED),
            _(i18n_keys.TITLE__ALREADY_PAIRED_DESC),
            icon_path="A:/res/success.png",
        )

    def destroy(self, delay_ms=100):
        self.destroyed = True
        from trezor import uart

        if uart.PAIR_SUCCESS_SCREEN == self:
            uart.PAIR_SUCCESS_SCREEN = None
        super().destroy(delay_ms)
