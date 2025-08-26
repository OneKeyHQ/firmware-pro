from trezor import wire
from trezor.crypto.curve import ed25519
from trezor.crypto.hashlib import sha3_256
from trezor.lvglui.scrs import lv
from trezor.messages import AptosMessageSignature, AptosSignSIWAMessage

from apps.common import paths, seed
from apps.common.keychain import Keychain, auto_keychain

from . import ICON, PRIMARY_COLOR
from .helper import aptos_address_from_pubkey

DOMAIN_SEPARATOR = b"SIGN_IN_WITH_APTOS::"


@auto_keychain(__name__)
async def sign_siwa_message(
    ctx: wire.Context, msg: AptosSignSIWAMessage, keychain: Keychain
) -> AptosMessageSignature:

    await paths.validate_path(ctx, keychain, msg.address_n)

    node = keychain.derive(msg.address_n)
    pub_key_bytes = seed.remove_ed25519_prefix(node.public_key())
    address = aptos_address_from_pubkey(pub_key_bytes)

    from trezor.ui.layouts import confirm_signverify

    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
    await confirm_signverify(ctx, "SIWA", msg.siwa_payload, address, False)

    domain_separator_hash = sha3_256(DOMAIN_SEPARATOR).digest()
    siwa_message = domain_separator_hash + msg.siwa_payload.encode()

    signature = ed25519.sign(node.private_key(), siwa_message)
    return AptosMessageSignature(signature=signature, address=address)
