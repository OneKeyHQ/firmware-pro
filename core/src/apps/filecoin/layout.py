from typing import TYPE_CHECKING

from trezor.strings import format_amount

from .transaction import Transaction

if TYPE_CHECKING:
    from trezor.wire import Context


async def require_confirm_tx(
    ctx: Context,
    tx: Transaction,
) -> None:
    from trezor.ui.layouts.lvgl import should_show_details, confirm_filecoin_payment
    from trezor.strings import strip_amount
    from trezor.lvglui.i18n import gettext as _, keys as i18n_keys

    amount = f"{format_amount(tx.value, 18)} FIL"
    striped_amount, striped = strip_amount(amount)
    title = _(i18n_keys.TITLE__SEND_MULTILINE).format(striped_amount)
    if await should_show_details(ctx, tx.to, title):
        fee_max = tx.gasfeecap * tx.gaslimit
        await confirm_filecoin_payment(
            ctx,
            title,
            tx.source,
            tx.to,
            amount,
            f"{tx.gaslimit}",
            f"{format_amount(tx.gasfeecap, 18)} FIL",
            f"{format_amount(tx.gaspremium, 18)} FIL",
            f"{format_amount(fee_max + tx.value, 18)} FIL",
            striped=striped,
        )
