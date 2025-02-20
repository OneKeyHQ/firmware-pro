from typing import TYPE_CHECKING

from trezor.lvglui.scrs.common import lv
from trezor.messages import NeoAddress
from trezor.ui.layouts import show_address

from apps.common import paths
from apps.common.keychain import auto_keychain

from . import ICON, PRIMARY_COLOR
from .helpers import neo_address_from_pubkey

if TYPE_CHECKING:
    from trezor.messages import NeoGetAddress
    from trezor.wire import Context

    from apps.common.keychain import Keychain


@auto_keychain(__name__)
async def get_address(
    ctx: Context, msg: NeoGetAddress, keychain: Keychain
) -> NeoAddress:
    await paths.validate_path(ctx, keychain, msg.address_n)

    node = keychain.derive(msg.address_n)
    pub_key_bytes = node.public_key()
    address = neo_address_from_pubkey(pub_key_bytes)
    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
    if msg.show_display:
        path = paths.address_n_to_str(msg.address_n)
        await show_address(
            ctx,
            address=address,
            address_n=path,
            network="Neo",
        )

    return NeoAddress(address=address, public_key=pub_key_bytes)
