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

TOTAL_NUMBER_OF_GROUPS = 4


def generate_alephium_address(public_key: bytes) -> str:
    hash = hashlib.blake2b(data=public_key, outlen=32).digest()
    address_bytes = bytes([0x00]) + hash
    address = base58.encode(address_bytes)
    return address


def check_group(target_group: int) -> None:
    if target_group is not None and (
        target_group < 0 or target_group >= TOTAL_NUMBER_OF_GROUPS
    ):
        raise wire.ProcessError("Invalid target group")


def get_pub_key_group(pub_key: bytes, group_num: int) -> int:
    pub_key_hash = hashlib.blake2b(data=pub_key, outlen=32).digest()
    script_hint = djb_hash(pub_key_hash) | 1
    group_index = xor_bytes(script_hint)
    return group_index % group_num


def djb_hash(data: bytes) -> int:
    h = 5381
    for b in data:
        h = ((h << 5) + h) + b
    return h & 0xFFFFFFFF


def xor_bytes(value: int) -> int:
    return (
        (value >> 24) ^ ((value >> 16) & 0xFF) ^ ((value >> 8) & 0xFF) ^ (value & 0xFF)
    )


def derive_pub_key_for_group(keychain, address_n: list[int], target_group: int):
    while True:
        node = keychain.derive(address_n)
        public_key = node.public_key()
        if get_pub_key_group(public_key, TOTAL_NUMBER_OF_GROUPS) == target_group:
            return public_key, address_n[-1]
        address_n[-1] += 1
        if address_n[-1] > 0x80000000:
            raise wire.ProcessError("Could not find a public key for the target group")


@auto_keychain(__name__)
async def get_address(
    ctx: wire.Context, msg: AlephiumGetAddress, keychain
) -> AlephiumAddress:

    await paths.validate_path(ctx, keychain, msg.address_n)
    node = keychain.derive(msg.address_n)

    if msg.target_group is not None:
        check_group(msg.target_group)

    if msg.target_group is None:
        public_key = node.public_key()
        derived_path = list(msg.address_n)
    else:
        public_key, derived_index = derive_pub_key_for_group(
            keychain, list(msg.address_n), msg.target_group
        )
        derived_path = list(msg.address_n[:-1] + [derived_index])

    address = generate_alephium_address(public_key)

    if msg.show_display:
        path = paths.address_n_to_str(derived_path)
        ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
        await show_address(
            ctx,
            address=address,
            address_n=path,
            network="Alephium",
        )

    return AlephiumAddress(
        address=address,
        public_key=public_key if msg.include_public_key else None,
        derived_path=derived_path,
    )
