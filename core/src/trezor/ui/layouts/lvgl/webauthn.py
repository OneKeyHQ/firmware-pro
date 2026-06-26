# from typing import TYPE_CHECKING

from trezor import wire
from trezor.enums import ButtonRequestType

from ...components.common.webauthn import ConfirmInfo
from .common import interact

# if TYPE_CHECKING:
#     from ...components.tt.confirm import Pageable


async def confirm_webauthn(
    ctx: wire.GenericContext | None,
    info: ConfirmInfo,
) -> bool:

    from trezor.lvglui.scrs.webauthn import ConfirmWebauthn

    title = info.get_header()
    icon_path = info.app_icon
    app_name = info.app_name()
    account_name = info.account_name()
    if account_name is not None:
        if app_name == account_name:
            account_name = None
    confirm = ConfirmWebauthn(title, icon_path, app_name, account_name)
    # Remember the shown window so an external CTAPHID_CANCEL can dismiss it; the
    # lvgl overlay is not removed automatically when the workflow coroutine is killed.
    info.screen = confirm
    try:
        if ctx is None:
            return bool(await confirm.request())
        else:
            return bool(
                await interact(
                    ctx, confirm, "confirm_webauthn", ButtonRequestType.Other
                )
            )
    finally:
        info.screen = None


async def select_webauthn_account(
    ctx: wire.GenericContext | None,
    info: ConfirmInfo,
) -> int | None:

    from trezor.lvglui.scrs.webauthn import SelectWebauthnAccount

    select = SelectWebauthnAccount(
        info.get_header(), info.app_icon, info.app_name(), info.account_names()
    )
    info.screen = select
    try:
        if ctx is None:
            result = await select.request()
        else:
            result = await interact(
                ctx, select, "select_webauthn_account", ButtonRequestType.Other
            )
        if isinstance(result, int) and result >= 0:
            return result
        return None
    finally:
        info.screen = None


async def confirm_webauthn_reset() -> bool:
    from trezor.lvglui.scrs.common import FullSizeWindow
    from trezor.lvglui.i18n import gettext as _, keys as i18n_keys

    screen = FullSizeWindow(
        _(i18n_keys.TITLE__FIDO2_RESET),
        _(i18n_keys.SUBTITLE__DO_YOU_REALLY_WANT_TO_ERASE_ALL_CREDENTIALS),
        _(i18n_keys.BUTTON__CONFIRM),
        _(i18n_keys.BUTTON__CANCEL),
        icon_path="A:/res/warning.png",
    )
    return bool(await screen.request())
