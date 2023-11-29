from typing import TYPE_CHECKING

from trezor import ui
from trezor.enums import ButtonRequestType
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.strings import format_amount
from trezor.ui.layouts import confirm_address, should_show_details

from . import helpers, tokens

if TYPE_CHECKING:
    from typing import Awaitable

    from trezor.wire import Context


# def require_confirm_data(ctx: Context, data: bytes, data_total: int) -> Awaitable[None]:
#     from trezor.ui.layouts import confirm_data

#     return confirm_data(
#         ctx,
#         "confirm_data",
#         title=_(i18n_keys.TITLE__VIEW_DATA),
#         description=_(i18n_keys.SUBTITLE__STR_BYTES).format(data_total),
#         data=data,
#         br_code=ButtonRequestType.SignTx,
#     )


# def require_confirm_tx(
#     ctx: Context,
#     to: str,
#     value: int,
#     token: tokens.TokenInfo | None = None,
# ) -> Awaitable[None]:
#     if len(to) == 0:
#         to_str = _(i18n_keys.LIST_VALUE__NEW_CONTRACT)
#     else:
#         to_str = to
#     if token is None:
#         amount = format_conflux_amount(value, None)
#     else:
#         amount = format_conflux_amount(value, token)

#     return confirm_output(
#         ctx,
#         address=to_str,
#         amount=amount,
#         font_amount=ui.BOLD,
#         color_to=ui.GREY,
#         br_code=ButtonRequestType.SignTx,
#     )


def require_confirm_fee(
    ctx: Context,
    from_address: str | None = None,
    to_address: str | None = None,
    value: int = 0,
    gas_price: int = 0,
    gas_limit: int = 0,
    token: tokens.TokenInfo | None = None,
    raw_data: bytes | None = None,
) -> Awaitable[None]:
    from trezor.ui.layouts.lvgl.altcoin import confirm_total_ethereum

    fee_limit = gas_price * gas_limit

    return confirm_total_ethereum(
        ctx,
        format_conflux_amount(value, token),
        None,
        format_conflux_amount(fee_limit, None),
        from_address,
        to_address,
        format_conflux_amount(value + fee_limit, None) if token is None else None,
        raw_data=raw_data,
    )


def require_show_overview(
    ctx: Context,
    to_addr: str,
    value: int,
    token: tokens.TokenInfo | None = None,
) -> Awaitable[bool]:
    from trezor.strings import strip_amount

    return should_show_details(
        ctx,
        title=_(i18n_keys.TITLE__SEND_MULTILINE).format(
            strip_amount(format_conflux_amount(value, token))[0]
        ),
        address=to_addr or _(i18n_keys.LIST_VALUE__NEW_CONTRACT),
        br_code=ButtonRequestType.SignTx,
    )


def format_conflux_amount(value: int, token: tokens.TokenInfo | None) -> str:
    if token:
        suffix = token.symbol
        decimals = token.decimals
    else:
        suffix = "CFX"
        decimals = helpers.DECIMALS

    # Don't want to display wei values for tokens with small decimal numbers
    # if decimals > 9 and value < 10 ** (decimals - 9):
    #     suffix = "Drip " + suffix
    #     decimals = 0

    return f"{format_amount(value, decimals)} {suffix}"


def require_confirm_unknown_token(ctx: Context, address: str) -> Awaitable[None]:
    return confirm_address(
        ctx,
        _(i18n_keys.TITLE__UNKNOWN_TOKEN),
        address,
        description=_(i18n_keys.LIST_KEY__CONTRACT__COLON),
        br_type="unknown_token",
        icon="A:/res/shriek.png",
        icon_color=ui.ORANGE,
        br_code=ButtonRequestType.SignTx,
    )
