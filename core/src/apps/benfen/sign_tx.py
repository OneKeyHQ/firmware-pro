import binascii
from typing import Tuple

from trezor import wire
from trezor.crypto.curve import ed25519
from trezor.crypto.hashlib import blake2b
from trezor.lvglui.scrs import lv
from trezor.messages import BenfenSignedTx, BenfenSignTx, BenfenTxAck, BenfenTxRequest
from trezor.ui.layouts import confirm_blind_sign_common, confirm_final

from apps.common import paths, seed
from apps.common.keychain import Keychain, auto_keychain

from . import ICON, PRIMARY_COLOR
from .helper import INTENT_BYTES, benfen_address_from_pubkey, try_convert_to_bfc_address
from .layout import require_confirm_fee, require_show_overview
from .tx_parser import TransactionParser


async def process_transaction(
    ctx,
    address: str,
    tx_bytes: bytes,
    coin_type: bytes,
) -> bytes:
    parser = TransactionParser()

    intent = tx_bytes[:3]
    if INTENT_BYTES != intent:
        raise wire.DataError("Invalid raw tx")

    if coin_type:
        try:
            if not all(c < 128 for c in coin_type):
                await confirm_blind_sign_common(ctx, address, tx_bytes)
                return blake2b(data=tx_bytes, outlen=32).digest()
            currency_symbol = coin_type.decode("ascii")
            if currency_symbol and "::" in currency_symbol:
                currency_symbol = currency_symbol.split("::")[-1]
            ALLOWED_TOKENS = {
                "BJPY",
                "BUSD",
                "LONG",
                "BF_USDC",
                "BF_USDT",
                "BFC",
                "BAUD",
                "BCAD",
                "BEUR",
                "BIDR",
                "BINR",
                "BKRW",
                "BMXN",
            }
            if currency_symbol not in ALLOWED_TOKENS:
                currency_symbol = "UNKNOWN"
        except UnicodeDecodeError:
            await confirm_blind_sign_common(ctx, address, tx_bytes)
            return blake2b(data=tx_bytes, outlen=32).digest()
    else:
        await confirm_blind_sign_common(ctx, address, tx_bytes)
        return blake2b(data=tx_bytes, outlen=32).digest()

    parsed_tx = parser.parse_tx(tx_bytes)
    if parsed_tx is None:
        await confirm_blind_sign_common(ctx, address, tx_bytes)
        return blake2b(data=tx_bytes, outlen=32).digest()

    is_valid = validate_transaction(parsed_tx)

    if is_valid:
        (
            amount_raw,
            recipient_bfc,
            sender_bfc,
            max_gas_fee,
        ) = parse_transaction(parsed_tx)
        show_details = await require_show_overview(
            ctx,
            recipient_bfc,
            amount_raw,
            currency_symbol,
        )
        if show_details:
            await require_confirm_fee(
                ctx,
                from_address=sender_bfc,
                to_address=recipient_bfc,
                value=amount_raw,
                gas_price=max_gas_fee,
                gas_budget=max_gas_fee,
                currency_symbol=currency_symbol,
            )
    else:
        await confirm_blind_sign_common(ctx, address, tx_bytes)
    return blake2b(data=tx_bytes, outlen=32).digest()


@auto_keychain(__name__)
async def sign_tx(
    ctx: wire.Context, msg: BenfenSignTx, keychain: Keychain
) -> BenfenSignedTx:

    await paths.validate_path(ctx, keychain, msg.address_n)

    node = keychain.derive(msg.address_n)
    pub_key_bytes = seed.remove_ed25519_prefix(node.public_key())
    hex_address = benfen_address_from_pubkey(pub_key_bytes)
    address = try_convert_to_bfc_address(hex_address)
    if address is None:
        raise wire.DataError("Invalid address format")
    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
    coin_type = msg.coin_type if msg.coin_type is not None else b""
    if msg.data_length and msg.data_length > 0:
        data = await process_data_chunks(ctx, msg)
        hash = await process_transaction(ctx, address, data, coin_type)
    else:
        hash = await process_transaction(ctx, address, msg.raw_tx, coin_type)

    await confirm_final(ctx, "BENFEN")

    signature = ed25519.sign(node.private_key(), hash)
    return BenfenSignedTx(public_key=pub_key_bytes, signature=signature)


async def process_data_chunks(ctx, msg) -> bytes:
    if INTENT_BYTES != msg.data_initial_chunk[:3]:
        raise wire.DataError("Invalid raw tx")

    data = bytearray(msg.data_initial_chunk)
    data_left = msg.data_length - len(msg.data_initial_chunk)
    while data_left > 0:
        resp = await send_request_chunk(ctx, data_left)
        data_left -= len(resp.data_chunk)
        data += resp.data_chunk

    return bytes(data)


async def send_request_chunk(ctx: wire.Context, data_left: int) -> BenfenTxAck:
    req = BenfenTxRequest()
    if data_left <= 1024:
        req.data_length = data_left
    else:
        req.data_length = 1024
    return await ctx.call(req, BenfenTxAck)


def parse_transaction(parsed_tx: dict) -> Tuple[int | str, str, str, int]:
    tx_kind = parsed_tx["V1"]["TransactionKind"]["ProgrammableTransaction"]
    inputs = tx_kind["Inputs"]
    commands = tx_kind["Commands"]
    amount_input_index = None
    recipient_input_index = None
    for cmd in commands:
        if cmd["type"] == "TransferObjects":
            address_data = cmd["data"]["address"]
            if address_data["type"] == "Input":
                recipient_input_index = address_data["index"]
            object_data = cmd["data"]["objects"][0]
            if object_data["type"] == "GasCoin":
                amount_input_index = None
                break
            if object_data["type"] == "NestedResult":
                command_index = object_data["index"][0]
                referenced_command = commands[command_index]
                if referenced_command["type"] == "SplitCoin":
                    amount_input_index = referenced_command["data"]["amounts"][0][
                        "index"
                    ]
            elif object_data["type"] == "Input":
                amount_input_index = object_data["index"]
            elif object_data["type"] == "Result":
                command_index = object_data["index"]
                referenced_command = commands[command_index]
                if referenced_command["type"] == "SplitCoin":
                    amount_input_index = referenced_command["data"]["amounts"][0][
                        "index"
                    ]

    if recipient_input_index is None:
        raise wire.DataError("Required commands not found")

    try:
        if amount_input_index is None:
            amount_raw = "All"
        else:
            amount_hex = inputs[amount_input_index]["Pure"]
            amount_raw = int.from_bytes(binascii.unhexlify(amount_hex), "little")
    except Exception:
        raise wire.DataError("Invalid amount format")

    try:
        recipient_hex = inputs[recipient_input_index]["Pure"]
        recipient = "0x" + recipient_hex
        recipient_bfc = try_convert_to_bfc_address(recipient)
    except Exception:
        raise wire.DataError("Invalid recipient address")
    sender = tx_kind["Sender"]["Address"]
    sender_bfc = try_convert_to_bfc_address(sender)
    gas_data = parsed_tx["V1"]["GasData"]
    gas_budget = gas_data["budget"]
    if not isinstance(recipient_bfc, str):
        raise wire.DataError("Invalid recipient address type")
    if not isinstance(sender_bfc, str):
        raise wire.DataError("Invalid sender address type")
    if not isinstance(gas_budget, int):
        raise wire.DataError("Invalid gas budget type")
    return amount_raw, recipient_bfc, sender_bfc, gas_budget


def validate_transaction(parsed_tx: dict) -> bool:

    if "V1" not in parsed_tx:
        return False
    tx_data = parsed_tx["V1"]
    if "TransactionKind" not in tx_data:
        return False

    if "ProgrammableTransaction" not in tx_data["TransactionKind"]:
        return False

    tx_kind = tx_data["TransactionKind"]["ProgrammableTransaction"]
    required_fields = ["Inputs", "Commands", "Sender"]
    missing_fields = [field for field in required_fields if field not in tx_kind]
    if missing_fields:
        return False

    commands = tx_kind["Commands"]
    if not commands:
        return False

    transfer_objects_count = sum(
        1 for cmd in commands if cmd.get("type") == "TransferObjects"
    )
    if transfer_objects_count != 1:
        return False

    required_commands = {"TransferObjects"}
    found_commands = {cmd["type"] for cmd in commands if "type" in cmd}

    if not required_commands.issubset(found_commands):
        return False

    if "GasData" not in tx_data:
        return False

    gas_data = tx_data["GasData"]
    if not all(key in gas_data for key in ["budget", "price"]):
        return False

    return True
