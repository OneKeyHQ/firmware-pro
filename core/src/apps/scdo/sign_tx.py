from binascii import hexlify

from trezor import utils, wire
from trezor.crypto import rlp
from trezor.crypto.curve import secp256k1
from trezor.crypto.hashlib import sha3_256
from trezor.lvglui.scrs import lv
from trezor.messages import ScdoSignedTx, ScdoSignTx, ScdoTxAck
from trezor.ui.layouts import confirm_final
from trezor.utils import HashWriter

from apps.common import paths
from apps.common.keychain import Keychain, auto_keychain

from . import ICON, PRIMARY_COLOR, tokens
from .helpers import address_from_public_key, bytes_from_address
from .layout import (
    require_confirm_fee,
    require_confirm_unknown_token,
    require_show_overview,
)


@auto_keychain(__name__)
async def sign_tx(
    ctx: wire.Context, msg: ScdoSignTx, keychain: Keychain
) -> ScdoSignedTx:

    data_total = msg.data_length if msg.data_length is not None else 0

    await paths.validate_path(ctx, keychain, msg.address_n)
    node = keychain.derive(msg.address_n)
    if utils.USE_THD89:
        from trezor.crypto import se_thd89

        public_key = se_thd89.uncompress_pubkey("secp256k1", node.public_key())
    else:
        seckey = node.private_key()
        public_key = secp256k1.publickey(seckey, False)

    owner_address = address_from_public_key(public_key[1:65])

    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
    recipient = msg.to

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
        token = tokens.token_by_address("SRC20", recipient)
        if token == tokens.UNKNOWN_TOKEN:
            await require_confirm_unknown_token(ctx, recipient)
        recipient = "1S" + hexlify(msg.data_initial_chunk[16:36]).decode()

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
            from_address=owner_address,
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
        msg=msg,
        from_addr=owner_address,
        data_total=data_total,
    )

    sha = HashWriter(sha3_256(keccak=True))
    rlp.write_header(sha, total_length, rlp.LIST_HEADER_BYTE)

    if msg.tx_type is not None:
        rlp.write(sha, msg.tx_type)

    rlp.write(sha, bytes_from_address(owner_address))
    rlp.write(sha, bytes_from_address(msg.to))
    rlp.write(sha, msg.value)
    rlp.write(sha, msg.nonce)
    rlp.write(sha, msg.gas_price)
    rlp.write(sha, msg.gas_limit)
    rlp.write(sha, msg.timestamp)

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
        node.private_key(),
        digest,
        False,
        secp256k1.CANONICAL_SIG_ETHEREUM,
    )

    await confirm_final(ctx, "SCDO")
    req = ScdoSignedTx()
    req.signature = signature[1:] + bytearray([signature[0] - 27])
    return req


def get_total_length(
    msg: ScdoSignTx,
    from_addr: str,
    data_total: int,
) -> int:

    length = 0

    fields: tuple[rlp.RLPItem, ...] = (
        msg.tx_type,
        bytes_from_address(from_addr),
        bytes_from_address(msg.to),
        msg.value,
        msg.nonce,
        msg.gas_price,
        msg.gas_limit,
        msg.timestamp,
    )

    for field in fields:
        length += rlp.length(field)

    length += rlp.header_length(data_total, msg.data_initial_chunk)
    length += data_total

    return length


async def send_request_chunk(ctx: wire.Context, data_left: int) -> ScdoTxAck:
    req = ScdoSignedTx()
    if data_left <= 1024:
        req.data_length = data_left
    else:
        req.data_length = 1024

    return await ctx.call(req, ScdoTxAck)
