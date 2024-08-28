from typing import TYPE_CHECKING

from trezor import utils
from trezor.crypto.curve import secp256k1
from trezor.crypto.hashlib import sha3_256
from trezor.lvglui.scrs import lv
from trezor.messages import ScdoSignedMessage, ScdoSignMessage
from trezor.ui.layouts import confirm_signverify
from trezor.utils import HashWriter

from apps.common import paths
from apps.common.helpers import validate_message
from apps.common.keychain import Keychain, auto_keychain
from apps.common.signverify import decode_message

from . import ICON, PRIMARY_COLOR
from .helpers import address_from_public_key

if TYPE_CHECKING:
    from trezor.wire import Context


def message_digest(message: bytes) -> bytes:
    h = HashWriter(sha3_256(keccak=True))
    signed_message_header = b"\x19Scdo Signed Message:\n"
    h.extend(signed_message_header)
    h.extend(str(len(message)).encode())
    h.extend(message)
    return h.get_digest()


@auto_keychain(__name__)
async def sign_message(
    ctx: Context, msg: ScdoSignMessage, keychain: Keychain
) -> ScdoSignedMessage:
    message = msg.message if msg.message is not None else b""
    validate_message(message)
    await paths.validate_path(ctx, keychain, msg.address_n)

    node = keychain.derive(msg.address_n)

    if utils.USE_THD89:
        from trezor.crypto import se_thd89

        public_key = se_thd89.uncompress_pubkey("secp256k1", node.public_key())
    else:
        seckey = node.private_key()
        public_key = secp256k1.publickey(seckey, False)

    scdo_address = address_from_public_key(public_key[1:65])

    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
    await confirm_signverify(
        ctx, "SCDO", decode_message(message), scdo_address, verify=False
    )

    digest = message_digest(message)
    signature = secp256k1.sign(
        node.private_key(),
        digest,
        False,
        secp256k1.CANONICAL_SIG_ETHEREUM,
    )

    return ScdoSignedMessage(
        address=scdo_address,
        signature=signature[1:] + bytearray([signature[0] - 27]),
    )
