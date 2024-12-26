from trezor.enums import ButtonRequestType
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.ui.layouts import should_show_details
from trezor.wire import Context

from .helper import format_benfen_amount


async def require_show_overview(
    ctx: Context,
    to_addr: str,
    value: int,
    currency_symbol: str = "BFC",
) -> bool:
    from trezor.strings import strip_amount

    return await should_show_details(
        ctx,
        title=_(i18n_keys.TITLE__SEND_MULTILINE).format(
            strip_amount(format_benfen_amount(value, currency_symbol))[0]
        ),
        address=to_addr,
        br_code=ButtonRequestType.SignTx,
    )


async def require_confirm_fee(
    ctx: Context,
    from_address: str,
    to_address: str,
    value: int,
    gas_price: int,
    gas_budget: int,
    currency_symbol: str = "BFC",
) -> None:
    from trezor.ui.layouts.lvgl.altcoin import confirm_total_ethereum

    total_amount = (
        format_benfen_amount(value + gas_price, currency_symbol)
        if currency_symbol == "BFC"
        else None
    )
    fee_currency = currency_symbol if currency_symbol == "BFC" else "BFC"

    await confirm_total_ethereum(
        ctx=ctx,
        amount=format_benfen_amount(value, currency_symbol),
        gas_price=None,
        fee_max=format_benfen_amount(gas_price, fee_currency),
        from_address=from_address,
        to_address=to_address,
        total_amount=total_amount,
    )
