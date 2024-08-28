from typing import TYPE_CHECKING

from trezor.crypto import hashlib
from trezor.crypto.curve import secp256k1
from trezor.lvglui.scrs import lv
from trezor.messages import AlephiumMessageSignature
from trezor.ui.layouts import confirm_signverify

from apps.alephium.get_address import generate_alephium_address
from apps.common import paths
from apps.common.helpers import validate_message_with_custom_limit
from apps.common.keychain import Keychain, auto_keychain

from . import ICON, PRIMARY_COLOR

if TYPE_CHECKING:
    from trezor.messages import AlephiumSignMessage
    from trezor.wire import Context


@auto_keychain(__name__)
async def sign_message(
    ctx: Context, msg: AlephiumSignMessage, keychain: Keychain
) -> AlephiumMessageSignature:

    message = msg.message or b""
    alephium_max_message_length = 1024 * 30
    validate_message_with_custom_limit(message, alephium_max_message_length)

    await paths.validate_path(ctx, keychain, msg.address_n)
    node = keychain.derive(msg.address_n)
    public_key = node.public_key()
    address = generate_alephium_address(public_key)

    if msg.message_type != b"alephium":
        raise ValueError("Unsupported Message Type")

    prefix = b"Alephium Signed Message: "
    prefixed_message = prefix + message
    hash_bytes = hashlib.blake2b(data=prefixed_message, outlen=32).digest()

    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
    await confirm_signverify(
        ctx, "Alephium", message.decode("utf-8"), address, verify=False
    )

    signature = secp256k1.sign(node.private_key(), hash_bytes, False)[1:]

    return AlephiumMessageSignature(address=address, signature=signature)
