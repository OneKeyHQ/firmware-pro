from trezor import wire
from trezor.crypto.curve import ed25519
from trezor.crypto.hashlib import sha3_256
from trezor.lvglui.scrs import lv
from trezor.messages import StarcoinVerifyMessage, Success
from trezor.ui.layouts import confirm_signverify, show_success

from apps.common.keychain import Keychain, auto_keychain
from apps.common.signverify import decode_message

from . import ICON, PRIMARY_COLOR
from .helper import get_address_from_public_key, serialize_u32_as_uleb128


@auto_keychain(__name__)
async def verify_message(
    ctx: wire.Context, msg: StarcoinVerifyMessage, keychain: Keychain
) -> Success:

    prefix = sha3_256(b"STARCOIN::SigningMessage", keccak=False).digest()
    msg_data = msg.message if msg.message is not None else b""
    msg_len = len(msg_data)
    data = prefix + serialize_u32_as_uleb128(msg_len) + msg_data

    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
    if (
        ed25519.verify(
            msg.public_key if msg.public_key is not None else b"",
            msg.signature if msg.signature is not None else b"",
            data,
        )
        is False
    ):
        raise wire.DataError("Invalid signature")

    await confirm_signverify(
        ctx,
        "STC",
        decode_message(msg_data),
        address=get_address_from_public_key(msg.public_key),
        verify=True,
    )

    from trezor.lvglui.i18n import gettext as _, keys as i18n_keys

    await show_success(
        ctx,
        "verify_message",
        header=_(i18n_keys.TITLE__VERIFIED),
        content=_(i18n_keys.SUBTITLE__THE_SIGNATURE_IS_VALID),
        button=_(i18n_keys.BUTTON__DONE),
    )
    return Success(message="Message verified")
