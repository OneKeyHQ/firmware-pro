from trezor import wire
from trezor.crypto import rlp
from trezor.crypto.curve import secp256k1
from trezor.crypto.hashlib import sha3_256
from trezor.lvglui.scrs import lv
from trezor.messages import ConfluxSignTx, ConfluxTxAck, ConfluxTxRequest
from trezor.ui.layouts import confirm_final
from trezor.utils import HashWriter

from apps.common import paths
from apps.common.keychain import Keychain, auto_keychain

from . import ICON, PRIMARY_COLOR, tokens
from .helpers import address_from_bytes, address_from_hex, bytes_from_address
from .layout import (
    require_confirm_fee,
    require_confirm_unknown_token,
    require_show_overview,
)


@auto_keychain(__name__)
async def sign_tx(
    ctx: wire.Context, msg: ConfluxSignTx, keychain: Keychain
) -> ConfluxTxRequest:

    data_total = msg.data_length if msg.data_length is not None else 0

    await paths.validate_path(ctx, keychain, msg.address_n)
    node = keychain.derive(msg.address_n)

    owner_address = address_from_bytes(node.ethereum_pubkeyhash(), None)
    chain_id = msg.chain_id

    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
    recipient = address_from_hex(msg.to, chain_id, True)
    owner_cfx_address = address_from_hex(owner_address, chain_id)
    token = None
    amount = int.from_bytes(msg.value, "big")
    if (
        len(msg.value) == 0
        and data_total == 68
        and len(msg.data_initial_chunk) == 68
        and msg.data_initial_chunk[:16]
        == b"\xa9\x05\x9c\xbb\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    ):
        amount = int.from_bytes(msg.data_initial_chunk[36:68], "big")
        token = tokens.token_by_address("CRC20", recipient)
        if token == tokens.UNKNOWN_TOKEN:
            await require_confirm_unknown_token(ctx, recipient)
        recipient = address_from_hex(
            address_from_bytes(msg.data_initial_chunk[16:36], None), chain_id
        )

    show_details = await require_show_overview(
        ctx,
        recipient,
        amount,
        token,
    )
    if show_details:
        has_raw_data = True if token is None and msg.data_length > 0 else False

        await require_confirm_fee(
            ctx,
            from_address=owner_cfx_address,
            to_address=recipient,
            value=amount,
            gas_price=int.from_bytes(msg.gas_price, "big"),
            gas_limit=int.from_bytes(msg.gas_limit, "big"),
            token=token,
            raw_data=msg.data_initial_chunk if has_raw_data else None,
        )

    data = bytearray()
    data += msg.data_initial_chunk
    data_left = data_total - len(msg.data_initial_chunk)

    total_length = get_total_length(
        value=msg.value,
        gas_price=msg.gas_price,
        gas_limit=msg.gas_limit,
        to=msg.to,
        storage_limit=msg.storage_limit,
        epoch_height=msg.epoch_height,
        nonce=msg.nonce,
        chain_id=chain_id,
        data_initial_chunk=msg.data_initial_chunk,
        data_total=data_total,
    )

    sha = HashWriter(sha3_256(keccak=True))
    rlp.write_header(sha, total_length, rlp.LIST_HEADER_BYTE)
    rlp.write(sha, msg.nonce)
    rlp.write(sha, msg.gas_price)
    rlp.write(sha, msg.gas_limit)
    rlp.write(sha, bytes_from_address(msg.to))
    rlp.write(sha, msg.value)
    rlp.write(sha, msg.storage_limit)
    rlp.write(sha, msg.epoch_height)
    rlp.write(sha, chain_id)

    if data_left == 0:
        rlp.write(sha, data)
    else:
        rlp.write_header(sha, data_total, rlp.STRING_HEADER_BYTE, data)
        sha.extend(data)

    while data_left > 0:
        resp = await send_request_chunk(ctx, data_left)
        data_chunk = resp.data_chunk if resp.data_chunk is not None else b""
        data_left -= len(data_chunk)
        sha.extend(data_chunk)
    digest = sha.get_digest()
    signature = secp256k1.sign(
        node.private_key(), digest, False, secp256k1.CANONICAL_SIG_ETHEREUM
    )
    await confirm_final(ctx, "CFX")
    req = ConfluxTxRequest()
    req.signature_v = signature[0] - 27
    req.signature_r = signature[1:33]
    req.signature_s = signature[33:]
    return req


def get_total_length(
    value: bytes,
    gas_price: bytes,
    gas_limit: bytes,
    to: str,
    storage_limit: bytes,
    epoch_height: bytes,
    nonce: bytes,
    chain_id: int,
    data_initial_chunk: bytes,
    data_total: int,
) -> int:
    length = 0

    fields: tuple[rlp.RLPItem, ...] = (
        nonce,
        gas_price,
        gas_limit,
        bytes_from_address(to),
        value,
        storage_limit,
        epoch_height,
        chain_id,
    )

    for field in fields:
        length += rlp.length(field)

    length += rlp.header_length(data_total, data_initial_chunk)
    length += data_total

    return length


async def send_request_chunk(ctx: wire.Context, data_left: int) -> ConfluxTxAck:
    req = ConfluxTxRequest()
    if data_left <= 1024:
        req.data_length = data_left
    else:
        req.data_length = 1024

    return await ctx.call(req, ConfluxTxAck)
