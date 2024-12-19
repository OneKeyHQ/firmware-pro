from typing import TYPE_CHECKING

from trezor.lvglui.scrs import lv
from trezor.messages import BenfenAddress
from trezor.ui.layouts import show_address

from apps.common import paths, seed
from apps.common.keychain import auto_keychain

from . import ICON, PRIMARY_COLOR
from .helper import benfen_address_from_pubkey, try_convert_to_bfc_address

if TYPE_CHECKING:
    from trezor.messages import BenfenGetAddress
    from trezor.wire import Context
    from apps.common.keychain import Keychain


@auto_keychain(__name__)
async def get_address(
    ctx: Context, msg: BenfenGetAddress, keychain: Keychain
) -> BenfenAddress:
    await paths.validate_path(ctx, keychain, msg.address_n)
    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
    node = keychain.derive(msg.address_n)
    pub_key_bytes = seed.remove_ed25519_prefix(node.public_key())
    address = benfen_address_from_pubkey(pub_key_bytes)

    bfc_address = try_convert_to_bfc_address(address)
    if bfc_address:
        address = bfc_address

    if msg.show_display:
        path = paths.address_n_to_str(msg.address_n)
        await show_address(
            ctx,
            address=address,
            address_n=path,
            network="BENFEN",
        )
    return BenfenAddress(address=address)
