from micropython import const
from typing import TYPE_CHECKING

from trezor.enums import ButtonRequestType
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.strings import format_amount
from trezor.ui.layouts import should_show_details

if TYPE_CHECKING:
    from typing import Awaitable
    from trezor.wire import Context
    from typing import Optional


def require_confirm_fee(
    ctx: Context,
    from_address: Optional[str] = None,
    to_address: Optional[str] = None,
    amount: Optional[int] = None,
    gas_amount: Optional[int] = None,
    token_id: Optional[str] = None,
    token_amount: Optional[str] = None,
    raw_data: Optional[bytes] = None,
) -> Awaitable[None]:
    from trezor.ui.layouts.lvgl.altcoin import confirm_total_alephium

    formatted_value = format_alephium_amount(amount) if amount is not None else None
    formatted_gas_amount = (
        format_alephium_amount(gas_amount) if gas_amount is not None else None
    )

    return confirm_total_alephium(
        ctx,
        formatted_value,
        formatted_gas_amount,
        from_address,
        to_address,
        token_id=token_id,
        raw_data=raw_data,
        token_amount=token_amount,
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
