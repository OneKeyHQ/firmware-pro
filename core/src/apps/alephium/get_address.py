from typing import TYPE_CHECKING

from trezor import wire
from trezor.crypto import hashlib
from trezor.lvglui.scrs import lv
from trezor.messages import AlephiumAddress
from trezor.ui.layouts import show_address

from apps.common import paths
from apps.common.keychain import auto_keychain

from . import ICON, PRIMARY_COLOR

if TYPE_CHECKING:
    from trezor.messages import AlephiumGetAddress

CODE_INDEX_SECP256K1_SINGLE = 0x00
FORMAT_TYPE_SHORT = 0x01

ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"


def bytesToBinUnsafe(byte_string):
    def pad_binary(b, width):
        return "0" * (width - len(b)) + b

    result = ""
    for byte in byte_string:
        bin_value = bin(byte)[2:]
        padded_bin = pad_binary(bin_value, 8)
        result += padded_bin
    return result


def b58encode(b):
    n = int.from_bytes(b, "big")
    res = []
    while n:
        n, r = divmod(n, 58)
        res.append(ALPHABET[r])
    res = "".join(reversed(res))
    pad = 0
    for c in b:
        if c == 0:
            pad += 1
        else:
            break
    return ALPHABET[0] * pad + res


def generate_alephium_address(public_key: bytes) -> str:
    hash = hashlib.blake2b(data=public_key, outlen=32).digest()
    address_bytes = bytes([0x00]) + hash
    address = b58encode(address_bytes)
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
        )

    return AlephiumAddress(address=address)
