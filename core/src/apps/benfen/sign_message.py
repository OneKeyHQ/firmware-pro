from trezor import wire
from trezor.crypto.curve import ed25519
from trezor.crypto.hashlib import blake2b
from trezor.lvglui.scrs import lv
from trezor.messages import BenfenMessageSignature, BenfenSignMessage

from apps.common import paths, seed
from apps.common.keychain import Keychain, auto_keychain
from apps.common.signverify import decode_message

from . import ICON, PRIMARY_COLOR
from .helper import (
    PERSONALMESSAGE_INTENT_BYTES,
    benfen_address_from_pubkey,
    try_convert_to_bfc_address,
    uleb_encode,
)


@auto_keychain(__name__)
async def sign_message(
    ctx: wire.Context, msg: BenfenSignMessage, keychain: Keychain
) -> BenfenMessageSignature:

    await paths.validate_path(ctx, keychain, msg.address_n)

    node = keychain.derive(msg.address_n)
    pub_key_bytes = seed.remove_ed25519_prefix(node.public_key())
    address = benfen_address_from_pubkey(pub_key_bytes)
    bfc_address = try_convert_to_bfc_address(address)
    if bfc_address is None:
        raise wire.DataError("bfc_address is none")

    len_bytes = uleb_encode(len(msg.message))
    intentMessage = PERSONALMESSAGE_INTENT_BYTES + len_bytes + msg.message

    from trezor.ui.layouts import confirm_signverify

    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
    await confirm_signverify(
        ctx, "Benfen", decode_message(msg.message), bfc_address, False
    )

    signature = ed25519.sign(
        node.private_key(), blake2b(data=intentMessage, outlen=32).digest()
    )
    return BenfenMessageSignature(signature=signature, address=bfc_address)
