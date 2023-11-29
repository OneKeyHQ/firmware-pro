from typing import TYPE_CHECKING

from trezor.crypto.curve import secp256k1
from trezor.crypto.hashlib import sha3_256
from trezor.lvglui.scrs import lv
from trezor.messages import ConfluxMessageSignature
from trezor.utils import HashWriter

from apps.common import paths
from apps.common.keychain import Keychain, auto_keychain
from apps.common.signverify import decode_message

from . import ICON, PRIMARY_COLOR
from .helpers import address_from_bytes

if TYPE_CHECKING:
    from trezor.messages import ConfluxSignMessageCIP23
    from trezor.wire import Context


def message_digest(domain_hash: bytes, message_hash: bytes) -> bytes:
    h = HashWriter(sha3_256(keccak=True))
    h.extend(b"\x19\x01")
    h.extend(domain_hash)
    h.extend(message_hash)
    return h.get_digest()


@auto_keychain(__name__)
async def sign_message_cip23(
    ctx: Context, msg: ConfluxSignMessageCIP23, keychain: Keychain
) -> ConfluxMessageSignature:
    await paths.validate_path(ctx, keychain, msg.address_n)

    node = keychain.derive(msg.address_n)
    assert msg.domain_hash is not None, "domain_hash is required"

    message_hash = msg.message_hash or b""
    address = address_from_bytes(node.ethereum_pubkeyhash())
    # cfx_address = address_from_hex(address, 1029)
    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
    ctx.name = "Conflux"

    from apps.ethereum.layout import confirm_typed_hash, confirm_typed_hash_final

    await confirm_typed_hash(
        ctx, decode_message(msg.domain_hash), decode_message(message_hash)
    )
    await confirm_typed_hash_final(ctx)

    digest = message_digest(msg.domain_hash, message_hash)
    signature = secp256k1.sign(
        node.private_key(),
        digest,
        False,
        secp256k1.CANONICAL_SIG_ETHEREUM,
    )

    return ConfluxMessageSignature(
        address=address,
        signature=signature[1:] + bytearray([signature[0] - 27]),
    )
