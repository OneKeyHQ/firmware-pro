from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from trezor.lvglui.scrs.common import FullSizeWindow

DEFAULT_ICON = "A:/res/icon_webauthn.png"


class ConfirmInfo:
    def __init__(self) -> None:
        self.app_icon: str | None = None
        # The on-screen confirmation window, if currently shown. Kept so that an
        # external CTAPHID_CANCEL can dismiss the retained lvgl overlay.
        self.screen: "FullSizeWindow | None" = None

    def get_header(self) -> str:
        raise NotImplementedError

    def app_name(self) -> str:
        raise NotImplementedError

    def account_name(self) -> str | None:
        return None

    def account_names(self) -> list[str]:
        return []

    def load_icon(self, rp_id_hash: bytes) -> None:
        from apps.webauthn import knownapps

        fido_app = knownapps.by_rp_id_hash(rp_id_hash)
        if fido_app is not None and fido_app.icon is not None:
            self.app_icon = fido_app.icon
        else:
            self.app_icon = DEFAULT_ICON
