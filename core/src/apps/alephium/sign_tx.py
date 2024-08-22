from typing import TYPE_CHECKING

from trezor import wire
from trezor.crypto import hashlib
from trezor.crypto.curve import secp256k1
from trezor.lvglui.scrs import lv
from trezor.messages import (
    AlephiumBytecodeAck,
    AlephiumBytecodeRequest,
    AlephiumSignedTx,
    AlephiumSignTx,
    AlephiumTxAck,
    AlephiumTxRequest,
)
from trezor.ui.layouts import confirm_final

from apps.alephium.get_address import generate_alephium_address
from apps.common import paths
from apps.common.keychain import auto_keychain

from . import ICON, PRIMARY_COLOR
from .decode import decode_tx
from .layout import require_confirm_fee, require_show_overview

if TYPE_CHECKING:
    from apps.common.keychain import Keychain


@auto_keychain(__name__)
async def sign_tx(
    ctx: wire.Context,
    msg: AlephiumSignTx,
    keychain: Keychain,
) -> AlephiumSignedTx:

    await paths.validate_path(ctx, keychain, msg.address_n)
    node = keychain.derive(msg.address_n)
    public_key = node.public_key()
    address = generate_alephium_address(public_key)
    hasher = hashlib.blake2b(data=msg.data_initial_chunk, outlen=32)
    data = msg.data_initial_chunk
    if msg.data_length is not None and msg.data_length > 0:
        data_total = msg.data_length
        data_left = data_total - len(msg.data_initial_chunk)
        while data_left > 0:
            resp = await send_request_chunk(ctx, data_left)
            data_left -= len(resp.data_chunk)
            hasher.update(resp.data_chunk)
            data += resp.data_chunk

    raw_data = b""
    if data[2] == 0x00:
        pass
    elif data[2] == 0x01:
        resp_bytecode = await send_request_bytecode(ctx)
        bytecode = resp_bytecode.bytecode_data
        bytecode_len = len(bytecode)
        if data[3 : 3 + bytecode_len] == bytecode:
            data = data[:3] + data[3 + bytecode_len :]
            raw_data = bytecode
        else:
            raw_data = b""
            raise ValueError("Illegal contract data")

    else:
        raise ValueError("Illegal transaction data")

    decode_result = decode_tx(bytes(data))

    if decode_result["outputs"]:
        recv_address = None
        amount_alph = 0
        for output in decode_result["outputs"]:
            output_address = output["address"]
            if output_address != address:
                recv_address = output_address
                amount_alph = int(output["amount"])
                break

        if not recv_address:
            recv_address = decode_result["outputs"][0]["address"]
            amount_alph = int(decode_result["outputs"][0]["amount"])
    else:
        amount_alph = 0
        recv_address = ""

    gas_amount = decode_result["gasAmount"]
    gas_price_wei = int(decode_result["gasPrice"])
    gas_fee_alph = gas_amount * gas_price_wei

    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON

    show_details = await require_show_overview(
        ctx,
        str(recv_address),
        amount_alph,
    )
    if show_details:
        await require_confirm_fee(
            ctx,
            from_address=str(address),
            to_address=str(recv_address),
            value=amount_alph,
            gas_price=gas_fee_alph,
            raw_data=raw_data,
        )

    hash_bytes = hasher.digest()

    await confirm_final(ctx, "ALEPHIUM")
    signature = secp256k1.sign(node.private_key(), hash_bytes, False)[1:]

    return AlephiumSignedTx(signature=signature, address=address)


async def send_request_chunk(ctx: wire.Context, data_left: int) -> AlephiumTxAck:
    req = AlephiumTxRequest()
    if data_left <= 1024:
        req.data_length = data_left
    else:
        req.data_length = 1024
    return await ctx.call(req, AlephiumTxAck)


async def send_request_bytecode(ctx: wire.Context) -> AlephiumBytecodeAck:
    req = AlephiumBytecodeRequest()
    return await ctx.call(req, AlephiumBytecodeAck)
