from micropython import const
from typing import TYPE_CHECKING

from trezor.enums import ButtonRequestType
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.strings import format_amount
from trezor.ui.layouts import should_show_details

if TYPE_CHECKING:
    from typing import Awaitable
    from trezor.wire import Context


def require_confirm_fee(
    ctx: Context,
    from_address: str | None = None,
    to_address: str | None = None,
    value: int = 0,
    gas_price: int = 0,
    raw_data: bytes | None = None,
) -> Awaitable[None]:
    from trezor.ui.layouts.lvgl.altcoin import confirm_total_alephium

    return confirm_total_alephium(
        ctx,
        format_alephium_amount(value),
        None,
        from_address,
        to_address,
        format_alephium_amount(gas_price),
        raw_data=raw_data,
    )


def require_show_overview(
    ctx: Context,
    to_addr: str,
    value: int,
) -> Awaitable[bool]:
    from trezor.strings import strip_amount

    return should_show_details(
        ctx,
        title=_(i18n_keys.TITLE__SEND_MULTILINE).format(
            strip_amount(format_alephium_amount(value))[0]
        ),
        address=to_addr or _(i18n_keys.LIST_VALUE__NEW_CONTRACT),
        br_code=ButtonRequestType.SignTx,
    )


def format_alephium_amount(value: int) -> str:
    suffix = "ALPH"
    decimals = const(18)
    return f"{format_amount(value, decimals)} {suffix}"
