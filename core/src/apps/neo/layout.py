from typing import TYPE_CHECKING

from trezor.enums import ButtonRequestType
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.strings import strip_amount
from trezor.ui.layouts import should_show_details

if TYPE_CHECKING:
    from typing import Awaitable

    from trezor.wire import Context


def require_show_overview(
    ctx: Context,
    to: str,
    amount: str,
) -> Awaitable[bool]:

    return should_show_details(
        ctx,
        title=_(i18n_keys.TITLE__SEND_MULTILINE).format(strip_amount(amount)[0]),
        address=to,
        br_code=ButtonRequestType.SignTx,
    )


async def confirm_neo_transfer(
    ctx: Context,
    from_address: str,
    to_address: str,
    amount: str,
    fee: str,
    network_magic: int | None = None,
) -> None:
    from trezor.ui.layouts import confirm_neo_token_transfer

    await confirm_neo_token_transfer(
        ctx,
        from_address,
        to_address,
        fee,
        amount,
        network_magic,
    )


async def require_confirm_neo_vote(
    ctx: Context,
    from_address: str,
    vote_to: str,
    is_remove_vote: bool,
    network_magic: int | None = None,
) -> None:
    from trezor.ui.layouts import confirm_neo_vote

    await confirm_neo_vote(
        ctx,
        from_address,
        vote_to,
        is_remove_vote,
        network_magic,
    )
