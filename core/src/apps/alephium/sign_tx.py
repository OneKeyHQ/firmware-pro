from typing import TYPE_CHECKING, Any, Dict, List

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
from .layout import require_confirm_fee

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

    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON

    decode_result = decode_tx(bytes(data))

    transfers: List[Dict[str, Any]] = []
    for output in decode_result["outputs"]:
        output_address = output["address"]
        output_amount = int(output["amount"])

        if output_address != address:
            transfers.append(
                {"type": "ALPH", "amount": output_amount, "address": output_address}
            )

        if "tokens" in output and output["tokens"]:
            for token in output["tokens"]:
                transfers.append(
                    {
                        "type": "TOKEN",
                        "token_id": token["id"],
                        "amount": int(token["amount"]),
                        "address": output_address,
                    }
                )

    gas_amount = decode_result["gasAmount"]
    gas_price_wei = int(decode_result["gasPrice"])
    gas_fee_alph = gas_amount * gas_price_wei

    for transfer in transfers:
        if transfer["type"] == "ALPH":
            await require_confirm_fee(
                ctx,
                from_address=str(address),
                to_address=str(transfer["address"]),
                amount=transfer["amount"],
            )
        else:
            await require_confirm_fee(
                ctx,
                from_address=str(address),
                to_address=str(transfer["address"]),
                token_id=transfer["token_id"],
                token_amount=transfer["amount"],
            )

    if raw_data:
        await require_confirm_fee(
            ctx,
            raw_data=raw_data,
        )

    if gas_fee_alph:
        await require_confirm_fee(
            ctx,
            gas_amount=gas_fee_alph,
        )

    await confirm_final(ctx, "Alephium")
    hash_bytes = hasher.digest()
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
