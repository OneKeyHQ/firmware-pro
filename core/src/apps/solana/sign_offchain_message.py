from typing import TYPE_CHECKING

from storage import device
from trezor import wire
from trezor.crypto import base58
from trezor.crypto.curve import ed25519
from trezor.enums import SolanaOffChainMessageFormat, SolanaOffChainMessageVersion
from trezor.lvglui.scrs import lv
from trezor.messages import SolanaMessageSignature
from trezor.ui.layouts.lvgl import confirm_sol_message
from trezor.utils import BufferWriter

from apps.common import paths, seed, writers
from apps.common.keychain import auto_keychain
from apps.common.signverify import decode_message

from . import ICON, PRIMARY_COLOR
from .publickey import PublicKey

if TYPE_CHECKING:
    from trezor.messages import SolanaSignOffChainMessage
    from apps.common.keychain import Keychain

# The signing domain
_SIGN_DOMAIN = b"\xffsolana offchain"
# The header version, currently only version 0 is introduced in proposal[https://github.com/solana-labs/solana/blob/master/docs/src/proposals/off-chain-message-signing.md]
_ALLOWED_HEADER_VERSIONS = (SolanaOffChainMessageVersion.MESSAGE_VERSION_0,)
# The allowed message formats
_ALLOWED_MESSAGE_FORMATS = (
    SolanaOffChainMessageFormat.V0_RESTRICTED_ASCII,
    SolanaOffChainMessageFormat.V0_LIMITED_UTF8,
)
# The number of signers
_SIGNER_COUNT = 1
# The application domain length
_APPLICATION_DOMAIN_LENGTH = 32
# The public key length
_PUBLIC_KEY_LENGTH = 32
# Signing domain + header version + application domain + message format + signer count + signer public key + message length
_PREAMBLE_LENGTH = (
    16 + 1 + _APPLICATION_DOMAIN_LENGTH + 1 + 1 + _SIGNER_COUNT * _PUBLIC_KEY_LENGTH + 2
)
# The preamble length for ledger (signing domain + header version + message format + message length)
_PREAMBLE_LENGTH_LEDGER = 16 + 1 + 1 + 2
# 1232 is the maximum length of the message with the preamble
_MAX_MESSAGE_LENGTH_WITH_PREAMBLE = 1232
# The maximum length of the message
_MAX_MESSAGE_LENGTH = _MAX_MESSAGE_LENGTH_WITH_PREAMBLE - _PREAMBLE_LENGTH
# The maximum length of the message for ledger
_MAX_MESSAGE_LENGTH_LEDGER = _MAX_MESSAGE_LENGTH_WITH_PREAMBLE - _PREAMBLE_LENGTH_LEDGER


@auto_keychain(__name__)
async def sign_offchain_message(
    ctx: wire.Context, msg: SolanaSignOffChainMessage, keychain: Keychain
) -> SolanaMessageSignature:
    # sanitize message
    sanitize_message(msg)
    # path validation
    await paths.validate_path(ctx, keychain, msg.address_n)

    node = keychain.derive(msg.address_n)

    signer_pub_key_bytes = seed.remove_ed25519_prefix(node.public_key())
    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
    # the application domain displayed to the user is in base58 encoding
    if msg.application_domain:
        app_domain_fd = base58.encode(msg.application_domain)
    else:
        app_domain_fd = None
    message = decode_message(msg.message)
    address = str(PublicKey(signer_pub_key_bytes))

    if device.is_turbomode_enabled():
        from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
        from trezor.ui.layouts.lvgl import confirm_turbo

        await confirm_turbo(ctx, _(i18n_keys.MSG__SIGN_MESSAGE), "Solana")
    else:
        # display the message to confirm
        await confirm_sol_message(ctx, address, app_domain_fd, message)
    # prepare the message
    message_to_sign = prepare_message(msg, signer_pub_key_bytes)
    signature = ed25519.sign(node.private_key(), message_to_sign)
    return SolanaMessageSignature(signature=signature, public_key=signer_pub_key_bytes)


def prepare_message(
    msg: SolanaSignOffChainMessage, signer_pub_key_bytes: bytes
) -> bytes:
    """Prepare the message to be signed."""
    message = msg.message
    message_length = len(message)
    buffer = bytearray(
        message_length
        + (_PREAMBLE_LENGTH if msg.application_domain else _PREAMBLE_LENGTH_LEDGER)
    )
    bw = BufferWriter(buffer)
    writers.write_bytes_fixed(bw, _SIGN_DOMAIN, 16)
    writers.write_uint8(bw, msg.message_version)
    if msg.application_domain:
        writers.write_bytes_fixed(
            bw, msg.application_domain, _APPLICATION_DOMAIN_LENGTH
        )
    writers.write_uint8(bw, msg.message_format)
    if msg.application_domain:
        writers.write_uint8(bw, _SIGNER_COUNT)
        writers.write_bytes_fixed(bw, signer_pub_key_bytes, _PUBLIC_KEY_LENGTH)
    writers.write_uint16_le(bw, message_length)
    writers.write_bytes_unchecked(bw, msg.message)
    return bytes(bw.buffer)


def decode_offchain_message(
    message: bytes, *, standard: bool = False
) -> tuple[int, int, bytes, bytes | None]:
    message_length = len(message)
    min_length = _PREAMBLE_LENGTH if standard else _PREAMBLE_LENGTH_LEDGER
    max_length = _MAX_MESSAGE_LENGTH_WITH_PREAMBLE
    if message_length > max_length or message_length < min_length:
        raise Exception(
            f"Message length must be between {min_length} and {max_length} bytes, got {message_length}"
        )

    from trezor.utils import BufferReader

    br = BufferReader(message)
    sign_domain = br.read_memoryview(16)
    if sign_domain != _SIGN_DOMAIN:
        raise Exception(f"Invalid sign domain: {sign_domain}")
    message_version = int.from_bytes(br.read_memoryview(1), "little")
    if message_version not in _ALLOWED_HEADER_VERSIONS:
        raise Exception(f"Message version must be 0, got {message_version}")
    if standard:
        application_domain = br.read(_APPLICATION_DOMAIN_LENGTH)
    else:
        application_domain = None
    message_format = int.from_bytes(br.read_memoryview(1), "little")
    if message_format not in _ALLOWED_MESSAGE_FORMATS:
        raise Exception(f"Message format must be 0 or 1, got {message_format}")
    if standard:
        signer_count = int.from_bytes(br.read_memoryview(1), "little")
        if signer_count != _SIGNER_COUNT:
            raise Exception(f"Signer count must be 1, got {signer_count}")
        _signer_pub_key = br.read(_PUBLIC_KEY_LENGTH)
        if __debug__:
            print(f"signer_pub_key: {_signer_pub_key}")
    message_length = int.from_bytes(br.read_memoryview(2), "little")
    if message_length != br.remaining_count():
        raise Exception(f"Invalid message length: {message_length}")
    message = br.read(message_length)
    return (message_version, message_format, message, application_domain)


def sanitize_message(msg: SolanaSignOffChainMessage):
    """Sanitize the message."""
    message = msg.message
    msg_length = len(message)
    if (
        msg.application_domain
        and len(msg.application_domain) != _APPLICATION_DOMAIN_LENGTH
    ):
        raise wire.DataError(
            f"Application domain must be 32 bytes, got {len(msg.application_domain)}"
        )
    max_length = (
        _MAX_MESSAGE_LENGTH if msg.application_domain else _MAX_MESSAGE_LENGTH_LEDGER
    )
    if msg_length > max_length:
        raise wire.DataError(
            f"Message length must less than {max_length} bytes, got {msg_length}"
        )
    if msg.message_version not in _ALLOWED_HEADER_VERSIONS:
        raise wire.DataError(f"Message version must be 0, got {msg.message_version}")
    if msg.message_format not in _ALLOWED_MESSAGE_FORMATS:
        raise wire.DataError(f"Message format must be 0 or 1, got {msg.message_format}")
    elif msg.message_format == SolanaOffChainMessageFormat.V0_RESTRICTED_ASCII:
        if any(b < 0x20 or b > 0x7E for b in message):
            raise wire.DataError(
                "Message format 0 must contain only printable characters"
            )
    elif msg.message_format == SolanaOffChainMessageFormat.V0_LIMITED_UTF8:
        try:
            message.decode("utf-8")
        except UnicodeDecodeError:
            raise wire.DataError("Message format 1 must be a valid UTF-8 string")
