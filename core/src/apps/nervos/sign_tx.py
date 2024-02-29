from binascii import hexlify
from typing import TYPE_CHECKING
from trezor import wire
from trezor.crypto import hashlib
from trezor.crypto.curve import secp256k1
from trezor.crypto.hashlib import sha256
from trezor.messages import NervosSignedTx, NervosSignTx
from trezor.ui.layouts import confirm_final
from apps.nervos.get_address import generate_ckb_short_address
from apps.common import paths
from apps.common.keychain import auto_keychain
from ubinascii import unhexlify


if TYPE_CHECKING:
    from apps.common.keychain import Keychain



@auto_keychain(__name__)
async def sign_tx(
    ctx: wire.Context,
    msg: NervosSignTx,
    keychain: Keychain,
) -> NervosSignedTx:
    if msg.network not in ["ckb", "ckt"]:
        raise ValueError(f"Invalid network: {msg.network}")
    await paths.validate_path(ctx, keychain, msg.address_n)
    node = keychain.derive(msg.address_n)
    address = generate_ckb_short_address(node, network=msg.network)
    private_key = node.private_key()
    await confirm_final(ctx, "NERVOS")
    unmodified_signature = secp256k1.sign(private_key, msg.raw_message,False)
    adjusted_byte = (unmodified_signature[0] - 27) % 256
    signature = unmodified_signature[1:] + bytes([adjusted_byte])
    return NervosSignedTx(signature=signature,address=address)

