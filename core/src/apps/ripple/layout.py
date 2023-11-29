from typing import TYPE_CHECKING

from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.strings import format_amount
from trezor.ui.layouts.lvgl import should_show_details

from . import helpers

if TYPE_CHECKING:
    from typing import Awaitable
    from trezor.wire import Context


def require_confirm_fee(
    ctx: Context,
    from_address: str | None = None,
    to_address: str | None = None,
    fee: int = 0,
    value: int = 0,
    tag: int = None,
) -> Awaitable[None]:
    from trezor.ui.layouts.lvgl import confirm_ripple_payment
    from trezor.strings import strip_amount

    amount_str = f"{format_amount(value, helpers.DECIMALS)} XRP"
    striped_amount, striped = strip_amount(amount_str)
    return confirm_ripple_payment(
        ctx,
        _(i18n_keys.TITLE__SEND_MULTILINE).format(striped_amount),
        from_address,
        to_address,
        amount_str,
        format_amount(fee, helpers.DECIMALS) + " XRP",
        format_amount(value + fee, helpers.DECIMALS) + " XRP",
        str(tag) if tag is not None else None,
        striped=striped,
    )


async def require_should_show_more(ctx: Context, to: str, value: int) -> bool:
    # return confirm_total_ripple(ctx, to, format_amount(value, helpers.DECIMALS))
    from trezor.strings import strip_amount

    amount_str = f"{format_amount(value, helpers.DECIMALS)} XRP"
    striped_amount, _striped = strip_amount(amount_str)
    return await should_show_details(
        ctx, to, _(i18n_keys.TITLE__SEND_MULTILINE).format(striped_amount)
    )
