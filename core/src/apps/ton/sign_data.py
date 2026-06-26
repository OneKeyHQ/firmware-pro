from typing import TYPE_CHECKING

from trezor import wire
from trezor.crypto import hashlib
from trezor.crypto.curve import ed25519
from trezor.enums import TonSignDataType, TonWalletVersion, TonWorkChain
from trezor.lvglui.scrs import lv
from trezor.messages import TonSignData, TonSignedData
from trezor.ui.layouts import confirm_ton_sign_request

from apps.common import paths, seed
from apps.common.keychain import Keychain, auto_keychain

from . import ICON, PRIMARY_COLOR
from .tonsdk import utils as ton_utils
from .tonsdk.boc._builder import begin_cell
from .tonsdk.boc._cell import Cell
from .tonsdk.contract.wallet import Wallets, WalletVersionEnum
from .tonsdk.utils._address import Address as TonAddress

if TYPE_CHECKING:
    from trezor.wire import Context


@auto_keychain(__name__)
async def sign_data(
    ctx: Context, msg: TonSignData, keychain: Keychain
) -> TonSignedData:
    await paths.validate_path(ctx, keychain, msg.address_n)

    node = keychain.derive(msg.address_n)
    public_key = seed.remove_ed25519_prefix(node.public_key())
    workchain = -1 if msg.workchain == TonWorkChain.MASTERCHAIN else 0

    if msg.wallet_version == TonWalletVersion.V4R2:
        wallet_version = WalletVersionEnum.v4r2
    else:
        raise wire.DataError("Invalid wallet version.")

    wallet = Wallets.ALL[wallet_version](
        public_key=public_key,
        wallet_id=msg.wallet_id,
        wc=workchain,
    )
    address = wallet.address.to_string(
        is_user_friendly=True,
        is_url_safe=True,
        is_bounceable=msg.is_bounceable,
        is_test_only=msg.is_testnet_only,
    )
    _validate_message(msg, address)

    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON

    if msg.type in (TonSignDataType.TEXT, TonSignDataType.BINARY):
        if msg.type == TonSignDataType.TEXT:
            message = msg.payload.decode()
        else:
            from binascii import hexlify

            message = hexlify(msg.payload).decode()
        await confirm_ton_sign_request(
            ctx,
            message,
            address,
            msg.appdomain,
            clear_sign=msg.type == TonSignDataType.TEXT,
        )
        payload_kind = b"txt" if msg.type == TonSignDataType.TEXT else b"bin"
        digest = _build_bytes_digest(
            payload_kind,
            msg.payload,
            msg.appdomain,
            msg.timestamp,
            wallet.address,
        )
    elif msg.type == TonSignDataType.CELL:
        try:
            payload_cell = Cell.one_from_boc(msg.payload)
        except Exception as exc:
            raise wire.DataError("Invalid TON CELL payload.") from exc
        schema = msg.schema
        if schema is None:
            raise wire.DataError("Schema is required for CELL payloads.")
        from binascii import b2a_base64

        await confirm_ton_sign_request(
            ctx,
            b2a_base64(msg.payload).decode(),
            address,
            msg.appdomain,
            clear_sign=False,
        )
        digest = _build_cell_digest(
            payload_cell,
            schema,
            msg.appdomain,
            msg.timestamp,
            wallet.address,
        )
    else:
        raise wire.DataError("Invalid TON sign data type.")
    signature = ed25519.sign(node.private_key(), digest)
    return TonSignedData(signature=signature, digest=digest if __debug__ else None)


def _build_bytes_digest(
    payload_kind: bytes,
    payload: bytes,
    appdomain: str,
    timestamp: int,
    address: TonAddress,
) -> bytes:
    address_buffer = address.wc.to_bytes(4, "big") + address.get_hash_part()
    domain_bytes = appdomain.encode("utf-8")
    message = (
        b"\xff\xff"
        + b"ton-connect/sign-data/"
        + address_buffer
        + len(domain_bytes).to_bytes(4, "big")
        + domain_bytes
        + timestamp.to_bytes(8, "big")
        + payload_kind
        + len(payload).to_bytes(4, "big")
        + payload
    )
    return hashlib.sha256(message).digest()


def _encode_appdomain_cell(appdomain: str) -> str:
    labels = [label for label in appdomain.lower().split(".") if label]
    tep81_domain = "\0".join(reversed(labels)) + "\0"
    return tep81_domain


def _build_cell_digest(
    payload_cell: Cell,
    schema: str,
    appdomain: str,
    timestamp: int,
    address: TonAddress,
) -> bytes:
    return (
        begin_cell()
        .store_uint(0x75569022, 32)
        .store_uint(ton_utils.crc32(schema.encode("utf-8")), 32)
        .store_uint(timestamp, 64)
        .store_address(address)
        .store_string_ref_tail(_encode_appdomain_cell(appdomain))
        .store_ref(payload_cell)
        .end_cell()
        .bytes_hash()
    )


def _validate_message(msg: TonSignData, address: str) -> None:
    if msg.from_address is not None and msg.from_address != address:
        raise wire.DataError("Invalid signer address provided.")

    if msg.type in (TonSignDataType.TEXT, TonSignDataType.BINARY):
        if msg.schema is not None:
            raise wire.DataError("Schema is only allowed for CELL payloads.")

        if msg.type == TonSignDataType.TEXT:
            try:
                msg.payload.decode("utf-8")
            except UnicodeError:
                raise wire.DataError("Invalid UTF-8 text payload.") from None
        return

    if msg.type == TonSignDataType.CELL:
        if msg.schema is None:
            raise wire.DataError("Schema is required for CELL payloads.")
        if len(msg.appdomain) > 126:
            raise wire.DataError("Domain too long.")
        return

    raise wire.DataError("Invalid TON sign data type.")
