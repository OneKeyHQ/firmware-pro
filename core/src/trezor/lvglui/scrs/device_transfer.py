from trezor import utils
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys

from . import lv, lv_colors
from .common import FullSizeWindow


class TransferIntroduction(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__WALLET_TRANSFER),
            _(i18n_keys.SUBTITLE__WALLET_TRANSFER_INTRO),
            confirm_text=_(i18n_keys.BUTTON__CONTINUE),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            icon_path="A:/res/wallet-transfer-send.png",
            anim_dir=0,
        )


class TransferProgress(FullSizeWindow):
    def __init__(self):
        self.cancelled = False
        self.holding = False
        super().__init__(
            _(i18n_keys.TITLE__READY_TO_TRANSFER),
            _(i18n_keys.SUBTITLE__WALLET_TRANSFER_HOLD_BACK_TO_BACK),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            anim_dir=0,
        )
        self.title.align(lv.ALIGN.TOP_LEFT, 12, 32)
        self.subtitle.align_to(self.title, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 16)
        self.content_area.set_height(646)
        self.content_area.clear_flag(lv.obj.FLAG.SCROLLABLE)
        self.symbol = lv.img(self.content_area)
        self.symbol.set_src("A:/res/wallet-transfer-ready.png")
        self.symbol.align(lv.ALIGN.TOP_MID, 0, 344)

    def show_ready(self):
        self.move_foreground()
        lv.refr_now(None)

    def show_holding(self):
        if self.holding:
            return
        self.holding = True
        self.title.set_text(_(i18n_keys.TITLE__KEEP_HOLDING))
        self.subtitle.set_text(_(i18n_keys.SUBTITLE__WALLET_TRANSFER_TRANSFERRING))
        self.symbol.set_src("A:/res/wallet-transfer-holding.png")

    def eventhandler(self, event_obj):
        if event_obj.code == lv.EVENT.CLICKED and event_obj.get_target() == self.btn_no:
            if utils.lcd_resume():
                return
            # Keep the widgets alive until the transport has been closed.
            self.cancelled = True
            self.btn_no.disable()


class TransferResult(FullSizeWindow):
    def __init__(self, outcome: str):
        success = outcome == "success"
        if success:
            title = _(i18n_keys.TITLE__WALLET_TRANSFERRED)
            content = _(i18n_keys.SUBTITLE__WALLET_TRANSFER_USE_ON_BOTH_DEVICES)
        elif outcome == "timeout":
            title = _(i18n_keys.TITLE__TRANSFER_TIMEOUT)
            content = _(i18n_keys.SUBTITLE__WALLET_TRANSFER_TIMEOUT)
        elif outcome == "interrupted":
            title = _(i18n_keys.TITLE__TRANSFER_INTERRUPTED)
            content = _(i18n_keys.SUBTITLE__WALLET_TRANSFER_INTERRUPTED)
        else:
            title = _(i18n_keys.TITLE__TRANSFER_FAILED)
            content = _(i18n_keys.SUBTITLE__WALLET_TRANSFER_FAILED)
        super().__init__(
            title,
            content,
            confirm_text=_(
                i18n_keys.BUTTON__I_GOT_IT if success else i18n_keys.BUTTON__TRY_AGAIN
            ),
            cancel_text="" if success else _(i18n_keys.BUTTON__CANCEL),
            icon_path="A:/res/success.png" if success else "A:/res/danger.png",
            primary_color=lv_colors.ONEKEY_GREEN if success else lv_colors.WHITE,
            anim_dir=0,
        )
