from typing import TYPE_CHECKING

from trezor import wire
from trezor.crypto import base58, hashlib
from trezor.lvglui.scrs import lv
from trezor.messages import AlephiumAddress
from trezor.ui.layouts import show_address

from apps.common import paths
from apps.common.keychain import auto_keychain

from . import ICON, PRIMARY_COLOR

if TYPE_CHECKING:
    from trezor.messages import AlephiumGetAddress


def bytesToBinUnsafe(byte_string):
    def pad_binary(b, width):
        return "0" * (width - len(b)) + b

    result = ""
    for byte in byte_string:
        bin_value = bin(byte)[2:]
        padded_bin = pad_binary(bin_value, 8)
        result += padded_bin
    return result


def generate_alephium_address(public_key: bytes) -> str:
    hash = hashlib.blake2b(data=public_key, outlen=32).digest()
    address_bytes = bytes([0x00]) + hash
    address = base58.encode(address_bytes)
    return address


@auto_keychain(__name__)
async def get_address(
    ctx: wire.Context, msg: AlephiumGetAddress, keychain
) -> AlephiumAddress:

    await paths.validate_path(ctx, keychain, msg.address_n)
    node = keychain.derive(msg.address_n)
    public_key = node.public_key()
    address = generate_alephium_address(public_key)

    if msg.show_display:
        path = paths.address_n_to_str(msg.address_n)
        ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
        await show_address(
            ctx,
            address=address,
            address_n=path,
            network="Alephium",
        )

    if msg.include_public_key:
        return AlephiumAddress(address=address, public_key=public_key)
    else:
        return AlephiumAddress(address=address)
