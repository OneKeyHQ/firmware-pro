# Serialize/deserialize TRON transactions
from typing import Any

from trezor.crypto import base58
from trezor.messages import TronContract, TronSignTx
from trezor.utils import BufferReader

from apps.common.writers import write_bytes_fixed

# PROTOBUF3 types
TYPE_VARINT = 0
TYPE_DOUBLE = 1
TYPE_LEN = 2
TYPE_FLOAT = 5


def write_field(w: bytearray, fnumber: int, ftype: int):
    tag = fnumber << 3 | ftype
    write_varint(w, tag)


def read_field(r: BufferReader) -> tuple[int, int]:
    tag = read_varint(r)
    fnumber = tag >> 3
    ftype = tag & 0x07
    return fnumber, ftype


def write_varint(w: bytearray, value: int):
    """
    Implements Base 128 variant
    See: https://developers.google.com/protocol-buffers/docs/encoding#varints
    """
    while True:
        byte = value & 0x7F
        value = value >> 7
        if value == 0:
            w.append(byte)
            break
        else:
            w.append(byte | 0x80)


def read_varint(r: BufferReader) -> int:
    """
    Read a varint from a BufferReader
    """
    value = 0
    shift = 0
    while True:
        byte = r.get()
        value |= (byte & 0x7F) << shift
        if byte & 0x80 == 0:
            break
        shift += 7
    return value


def write_bytes_with_length(w, buf: bytes):
    write_varint(w, len(buf))
    write_bytes_fixed(w, buf, len(buf))


def read_bytes_fixed(r: BufferReader) -> bytes:
    length = read_varint(r)
    return r.read_memoryview(length)


def pack_contract(contract, owner_address):
    """
    Pack Tron Proto3 Contract
    See: https://github.com/tronprotocol/protocol/blob/master/core/Tron.proto
    and https://github.com/tronprotocol/protocol/blob/master/core/contract/smart_contract.proto
    """
    retc = bytearray()
    write_field(retc, 1, TYPE_VARINT)
    # contract message
    cmessage = bytearray()
    api = ""
    if contract.transfer_contract:
        write_varint(retc, 1)
        api = "TransferContract"

        write_field(cmessage, 1, TYPE_LEN)
        write_bytes_with_length(cmessage, base58.decode_check(owner_address))
        write_field(cmessage, 2, TYPE_LEN)
        write_bytes_with_length(
            cmessage, base58.decode_check(contract.transfer_contract.to_address)
        )
        write_field(cmessage, 3, TYPE_VARINT)
        write_varint(cmessage, contract.transfer_contract.amount)
    elif contract.vote_witness_contract:
        write_varint(retc, 4)
        api = "VoteWitnessContract"

        write_field(cmessage, 1, TYPE_LEN)
        write_bytes_with_length(cmessage, base58.decode_check(owner_address))
        for vote in contract.vote_witness_contract.votes:
            v_message = bytearray()
            write_field(cmessage, 2, TYPE_LEN)
            write_field(v_message, 1, TYPE_LEN)
            write_bytes_with_length(v_message, base58.decode_check(vote.vote_address))
            write_field(v_message, 2, TYPE_VARINT)
            write_varint(v_message, vote.vote_count)
            write_bytes_with_length(cmessage, v_message)
        if contract.vote_witness_contract.support is not None:
            write_field(cmessage, 3, TYPE_VARINT)
            write_varint(cmessage, int(contract.vote_witness_contract.support))

    elif contract.trigger_smart_contract:
        write_varint(retc, 31)
        api = "TriggerSmartContract"

        write_field(cmessage, 1, TYPE_LEN)
        write_bytes_with_length(cmessage, base58.decode_check(owner_address))
        write_field(cmessage, 2, TYPE_LEN)
        write_bytes_with_length(
            cmessage,
            base58.decode_check(contract.trigger_smart_contract.contract_address),
        )
        if contract.trigger_smart_contract.call_value:
            write_field(cmessage, 3, TYPE_VARINT)
            write_varint(cmessage, contract.trigger_smart_contract.call_value)

        # Contract data
        write_field(cmessage, 4, TYPE_LEN)
        write_bytes_with_length(cmessage, contract.trigger_smart_contract.data)

        if contract.trigger_smart_contract.call_token_value:
            write_field(cmessage, 5, TYPE_VARINT)
            write_varint(cmessage, contract.trigger_smart_contract.call_token_value)
            write_field(cmessage, 6, TYPE_VARINT)
            write_varint(cmessage, contract.trigger_smart_contract.asset_id)

    elif contract.freeze_balance_contract:
        write_varint(retc, 11)
        api = "FreezeBalanceContract"

        write_field(cmessage, 1, TYPE_LEN)
        write_bytes_with_length(cmessage, base58.decode_check(owner_address))
        write_field(cmessage, 2, TYPE_VARINT)
        write_varint(cmessage, contract.freeze_balance_contract.frozen_balance)
        write_field(cmessage, 3, TYPE_VARINT)
        write_varint(cmessage, contract.freeze_balance_contract.frozen_duration)
        if contract.freeze_balance_contract.resource is not None:
            write_field(cmessage, 10, TYPE_VARINT)
            write_varint(cmessage, contract.freeze_balance_contract.resource)
        if contract.freeze_balance_contract.receiver_address is not None:
            write_field(cmessage, 15, TYPE_LEN)
            write_bytes_with_length(
                cmessage,
                base58.decode_check(contract.freeze_balance_contract.receiver_address),
            )

    elif contract.unfreeze_balance_contract:
        write_varint(retc, 12)
        api = "UnfreezeBalanceContract"

        write_field(cmessage, 1, TYPE_LEN)
        write_bytes_with_length(cmessage, base58.decode_check(owner_address))

        if contract.unfreeze_balance_contract.resource is not None:
            write_field(cmessage, 10, TYPE_VARINT)
            write_varint(cmessage, contract.unfreeze_balance_contract.resource)
        if contract.unfreeze_balance_contract.receiver_address is not None:
            write_field(cmessage, 15, TYPE_LEN)
            write_bytes_with_length(
                cmessage,
                base58.decode_check(
                    contract.unfreeze_balance_contract.receiver_address
                ),
            )

    elif contract.withdraw_balance_contract:
        write_varint(retc, 13)
        api = "WithdrawBalanceContract"
        write_field(cmessage, 1, TYPE_LEN)
        write_bytes_with_length(cmessage, base58.decode_check(owner_address))

    elif contract.freeze_balance_v2_contract:
        write_varint(retc, 54)
        api = "FreezeBalanceV2Contract"
        write_field(cmessage, 1, TYPE_LEN)
        write_bytes_with_length(cmessage, base58.decode_check(owner_address))

        write_field(cmessage, 2, TYPE_VARINT)
        write_varint(cmessage, contract.freeze_balance_v2_contract.frozen_balance)
        if contract.freeze_balance_v2_contract.resource is not None:
            write_field(cmessage, 3, TYPE_VARINT)
            write_varint(cmessage, contract.freeze_balance_v2_contract.resource)

    elif contract.unfreeze_balance_v2_contract:
        write_varint(retc, 55)
        api = "UnfreezeBalanceV2Contract"
        write_field(cmessage, 1, TYPE_LEN)
        write_bytes_with_length(cmessage, base58.decode_check(owner_address))

        write_field(cmessage, 2, TYPE_VARINT)
        write_varint(cmessage, contract.unfreeze_balance_v2_contract.unfreeze_balance)
        if contract.unfreeze_balance_v2_contract.resource is not None:
            write_field(cmessage, 3, TYPE_VARINT)
            write_varint(cmessage, contract.unfreeze_balance_v2_contract.resource)

    elif contract.withdraw_expire_unfreeze_contract:
        write_varint(retc, 56)
        api = "WithdrawExpireUnfreezeContract"
        write_field(cmessage, 1, TYPE_LEN)
        write_bytes_with_length(cmessage, base58.decode_check(owner_address))

    elif contract.delegate_resource_contract:
        write_varint(retc, 57)
        api = "DelegateResourceContract"
        write_field(cmessage, 1, TYPE_LEN)
        write_bytes_with_length(cmessage, base58.decode_check(owner_address))
        if contract.delegate_resource_contract.resource is not None:
            write_field(cmessage, 2, TYPE_VARINT)
            write_varint(cmessage, contract.delegate_resource_contract.resource)
        write_field(cmessage, 3, TYPE_VARINT)
        write_varint(cmessage, contract.delegate_resource_contract.balance)
        write_field(cmessage, 4, TYPE_LEN)
        write_bytes_with_length(
            cmessage,
            base58.decode_check(contract.delegate_resource_contract.receiver_address),
        )
        if contract.delegate_resource_contract.lock is not None:
            write_field(cmessage, 5, TYPE_VARINT)
            write_varint(cmessage, contract.delegate_resource_contract.lock)
        if contract.delegate_resource_contract.lock_period is not None:
            write_field(cmessage, 6, TYPE_VARINT)
            write_varint(cmessage, contract.delegate_resource_contract.lock_period)

    elif contract.undelegate_resource_contract:
        write_varint(retc, 58)
        api = "UnDelegateResourceContract"
        write_field(cmessage, 1, TYPE_LEN)
        write_bytes_with_length(cmessage, base58.decode_check(owner_address))
        if contract.undelegate_resource_contract.resource is not None:
            write_field(cmessage, 2, TYPE_VARINT)
            write_varint(cmessage, contract.undelegate_resource_contract.resource)
        write_field(cmessage, 3, TYPE_VARINT)
        write_varint(cmessage, contract.undelegate_resource_contract.balance)
        write_field(cmessage, 4, TYPE_LEN)
        write_bytes_with_length(
            cmessage,
            base58.decode_check(contract.undelegate_resource_contract.receiver_address),
        )
    elif contract.cancel_all_unfreeze_v2_contract:
        write_varint(retc, 59)
        api = "CancelAllUnfreezeV2Contract"
        write_field(cmessage, 1, TYPE_LEN)
        write_bytes_with_length(cmessage, base58.decode_check(owner_address))
    else:
        raise ValueError("Unsupported contract type")

    # write API
    capi = bytearray()
    write_field(capi, 1, TYPE_LEN)
    # write_bytes_with_length(capi, "type.googleapis.com/protocol." + api)
    write_bytes_with_length(capi, bytes("type.googleapis.com/protocol." + api, "ascii"))

    # extend to capi
    write_field(capi, 2, TYPE_LEN)
    write_bytes_with_length(capi, cmessage)

    # extend to contract
    write_field(retc, 2, TYPE_LEN)
    write_bytes_with_length(retc, capi)

    if contract.provider:
        write_field(retc, 3, TYPE_LEN)
        write_bytes_with_length(retc, contract.provider)
    if contract.contract_name:
        write_field(retc, 4, TYPE_LEN)
        write_bytes_with_length(retc, contract.contract_name)
    if contract.permission_id is not None:
        write_field(retc, 5, TYPE_VARINT)
        write_varint(retc, contract.permission_id)
    return retc


def serialize(transaction: TronSignTx, owner_address: str):
    # transaction parameters
    ret = bytearray()
    write_field(ret, 1, TYPE_LEN)
    write_bytes_with_length(ret, transaction.ref_block_bytes)
    write_field(ret, 4, TYPE_LEN)
    write_bytes_with_length(ret, transaction.ref_block_hash)
    write_field(ret, 8, TYPE_VARINT)
    write_varint(ret, transaction.expiration)
    if transaction.data is not None:
        write_field(ret, 10, TYPE_LEN)
        write_bytes_with_length(ret, transaction.data)

    # add Contract
    retc = pack_contract(transaction.contract, owner_address)

    write_field(ret, 11, TYPE_LEN)
    write_bytes_with_length(ret, retc)
    # add timestamp
    write_field(ret, 14, TYPE_VARINT)
    write_varint(ret, transaction.timestamp)
    # add fee_limit if any
    if transaction.fee_limit:
        write_field(ret, 18, TYPE_VARINT)
        write_varint(ret, transaction.fee_limit)

    return ret


def _parse_any(r: bytes) -> tuple[str, bytes]:

    param_reader = BufferReader(r)
    type_url = _ensure_non_none(
        _read_field_len(param_reader, 1, "type_url"), "type_url"
    )
    value = _ensure_non_none(_read_field_len(param_reader, 2, "value"), "value")
    return type_url.decode(), value


def unpack_contract(contract_data: bytes) -> "TronContract":
    r = BufferReader(contract_data)

    contract_type = _ensure_non_none(_read_field_varint(r, 1, "type"), "contract_type")
    parameter = _ensure_non_none(_read_field_len(r, 2, "parameter"), "parameter")
    _, value = _parse_any(parameter)
    provider = _read_field_len(r, 3, "provider", required=False)
    contract_name = _read_field_len(r, 4, "contract_name", required=False)
    permission_id = _read_field_varint(r, 5, "permission_id", required=False)

    contract = TronContract()
    contract_parsers = {
        1: ("transfer_contract", _parse_transfer_contract),
        4: ("vote_witness_contract", _parse_vote_witness_contract),
        11: ("freeze_balance_contract", _parse_freeze_balance_contract),
        12: ("unfreeze_balance_contract", _parse_unfreeze_balance_contract),
        13: ("withdraw_balance_contract", _parse_withdraw_balance_contract),
        31: ("trigger_smart_contract", _parse_trigger_smart_contract),
        54: ("freeze_balance_v2_contract", _parse_freeze_balance_v2_contract),
        55: ("unfreeze_balance_v2_contract", _parse_unfreeze_balance_v2_contract),
        56: (
            "withdraw_expire_unfreeze_contract",
            _parse_withdraw_expire_unfreeze_contract,
        ),
        57: ("delegate_resource_contract", _parse_delegate_resource_contract),
        58: ("undelegate_resource_contract", _parse_undelegate_resource_contract),
        59: ("cancel_all_unfreeze_v2_contract", _parse_cancel_all_unfreeze_v2_contract),
    }

    if (contract_info := contract_parsers.get(contract_type, None)) is not None:
        field_name, parser_func = contract_info
        parsed_contract = parser_func(value)
        setattr(contract, field_name, parsed_contract)
    else:
        raise ValueError(f"Unsupported contract type: {contract_type}")

    contract.provider = provider
    contract.contract_name = contract_name
    contract.permission_id = permission_id

    return contract


def _parse_contract_fields(r: BufferReader, field_definitions: list):
    parsed_fields = {}

    for (
        field_number,
        field_type,
        field_name,
        required,
        transform_func,
        repeated,
    ) in field_definitions:
        if r.remaining_count() == 0:
            if required:
                raise ValueError(f"Missing required field: {field_name}")
            break
        if repeated:
            parsed_fields[field_name] = []
        while True:
            if __debug__:
                print(
                    f"field_name: {field_name}, field_number: {field_number}, field_type: {field_type}, required: {required}, repeated: {repeated}"
                )
            required = required and (
                not repeated or len(parsed_fields[field_name]) == 0
            )
            value = _read_field(r, field_number, field_type, field_name, required)
            if value is None:
                break
            if transform_func == "base58":
                if not isinstance(value, bytes):
                    raise ValueError(
                        f"Expected bytes, got {type(value)} for field {field_name}"
                    )
                value = base58.encode_check(value)
            elif transform_func == "bool":
                value = bool(value)
            # elif transform_func == "skip":
            #     pass
            elif callable(transform_func):
                value = transform_func(value)

            if not repeated:
                parsed_fields[field_name] = value
            else:
                parsed_fields[field_name].append(value)
                continue
            break

    return parsed_fields


def _parse_transfer_contract(data: bytes):
    from trezor.messages import TronTransferContract

    r = BufferReader(data)

    field_definitions = [
        (1, TYPE_LEN, "owner_address", True, None, False),
        (2, TYPE_LEN, "to_address", True, "base58", False),
        (3, TYPE_VARINT, "amount", True, None, False),
    ]

    parsed_fields = _parse_contract_fields(r, field_definitions)

    transfer = TronTransferContract()
    transfer.to_address = parsed_fields["to_address"]
    transfer.amount = parsed_fields["amount"]

    return transfer


def _parse_vote_witness_contract(data: bytes):
    from trezor.messages import TronVoteWitnessContract

    r = BufferReader(data)

    field_definitions = [
        (1, TYPE_LEN, "owner_address", True, None, False),
        (2, TYPE_LEN, "votes", True, lambda data: _parse_vote(data), True),
        (3, TYPE_VARINT, "support", False, "bool", False),
    ]
    parsed_fields = _parse_contract_fields(r, field_definitions)

    vote_contract = TronVoteWitnessContract()
    vote_contract.votes = parsed_fields["votes"]

    vote_contract.support = parsed_fields.get("support", None)

    return vote_contract


def _parse_vote(data: bytes):
    from trezor.messages import Vote

    r = BufferReader(data)

    field_definitions = [
        (1, TYPE_LEN, "vote_address", True, "base58", False),
        (2, TYPE_VARINT, "vote_count", True, None, False),
    ]

    parsed_fields = _parse_contract_fields(r, field_definitions)

    vote = Vote(
        vote_address=parsed_fields["vote_address"],
        vote_count=parsed_fields["vote_count"],
    )

    return vote


def _parse_trigger_smart_contract(data: bytes):
    from trezor.messages import TronTriggerSmartContract

    r = BufferReader(data)

    field_definitions = [
        (1, TYPE_LEN, "owner_address", True, None, False),
        (2, TYPE_LEN, "contract_address", True, "base58", False),
        (3, TYPE_VARINT, "call_value", False, None, False),
        (4, TYPE_LEN, "data", True, None, False),
        (5, TYPE_VARINT, "call_token_value", False, None, False),
        (6, TYPE_VARINT, "asset_id", False, None, False),
    ]

    parsed_fields = _parse_contract_fields(r, field_definitions)

    trigger_contract = TronTriggerSmartContract()
    trigger_contract.contract_address = parsed_fields["contract_address"]
    trigger_contract.data = parsed_fields["data"]
    trigger_contract.call_value = parsed_fields.get("call_value", None)
    trigger_contract.call_token_value = parsed_fields.get("call_token_value", None)
    trigger_contract.asset_id = parsed_fields.get("asset_id", None)

    return trigger_contract


def _parse_freeze_balance_contract(data: bytes):
    from trezor.messages import TronFreezeBalanceContract

    r = BufferReader(data)

    field_definitions = [
        (1, TYPE_LEN, "owner_address", True, None, False),
        (2, TYPE_VARINT, "frozen_balance", True, None, False),
        (3, TYPE_VARINT, "frozen_duration", True, None, False),
        (10, TYPE_VARINT, "resource", False, None, False),
        (15, TYPE_LEN, "receiver_address", False, "base58", False),
    ]

    parsed_fields = _parse_contract_fields(r, field_definitions)

    freeze_contract = TronFreezeBalanceContract()
    freeze_contract.frozen_balance = parsed_fields["frozen_balance"]
    freeze_contract.frozen_duration = parsed_fields["frozen_duration"]
    freeze_contract.resource = parsed_fields.get("resource", None)
    freeze_contract.receiver_address = parsed_fields.get("receiver_address", None)

    return freeze_contract


def _parse_unfreeze_balance_contract(data: bytes):
    from trezor.messages import TronUnfreezeBalanceContract

    r = BufferReader(data)

    field_definitions = [
        (1, TYPE_LEN, "owner_address", True, None, False),
        (10, TYPE_VARINT, "resource", False, None, False),
        (15, TYPE_LEN, "receiver_address", False, "base58", False),
    ]

    parsed_fields = _parse_contract_fields(r, field_definitions)

    unfreeze_contract = TronUnfreezeBalanceContract()
    unfreeze_contract.resource = parsed_fields.get("resource", None)
    unfreeze_contract.receiver_address = parsed_fields.get("receiver_address", None)

    return unfreeze_contract


def _parse_withdraw_balance_contract(data: bytes):
    from trezor.messages import TronWithdrawBalanceContract

    r = BufferReader(data)

    field_definitions = [
        (1, TYPE_LEN, "owner_address", True, None, False),
    ]

    parsed_fields = _parse_contract_fields(r, field_definitions)

    withdraw_contract = TronWithdrawBalanceContract()
    withdraw_contract.owner_address = parsed_fields["owner_address"]

    return withdraw_contract


def _parse_freeze_balance_v2_contract(data: bytes):
    from trezor.messages import TronFreezeBalanceV2Contract

    r = BufferReader(data)

    field_definitions = [
        (1, TYPE_LEN, "owner_address", True, None, False),
        (2, TYPE_VARINT, "frozen_balance", True, None, False),
        (3, TYPE_VARINT, "resource", False, None, False),
    ]

    parsed_fields = _parse_contract_fields(r, field_definitions)

    freeze_contract = TronFreezeBalanceV2Contract()
    freeze_contract.frozen_balance = parsed_fields["frozen_balance"]
    freeze_contract.resource = parsed_fields.get("resource", None)

    return freeze_contract


def _parse_unfreeze_balance_v2_contract(data: bytes):
    from trezor.messages import TronUnfreezeBalanceV2Contract

    r = BufferReader(data)

    field_definitions = [
        (1, TYPE_LEN, "owner_address", True, None, False),
        (2, TYPE_VARINT, "unfreeze_balance", False, None, False),
        (3, TYPE_VARINT, "resource", False, None, False),
    ]

    parsed_fields = _parse_contract_fields(r, field_definitions)

    unfreeze_contract = TronUnfreezeBalanceV2Contract()
    unfreeze_contract.unfreeze_balance = parsed_fields["unfreeze_balance"]
    unfreeze_contract.resource = parsed_fields.get("resource", None)

    return unfreeze_contract


def _parse_withdraw_expire_unfreeze_contract(data: bytes):
    from trezor.messages import TronWithdrawExpireUnfreezeContract

    r = BufferReader(data)

    field_definitions = [
        (1, TYPE_LEN, "owner_address", True, None, False),
    ]

    _ = _parse_contract_fields(r, field_definitions)

    return TronWithdrawExpireUnfreezeContract()


def _parse_delegate_resource_contract(data: bytes):
    from trezor.messages import TronDelegateResourceContract

    r = BufferReader(data)

    field_definitions = [
        (1, TYPE_LEN, "owner_address", True, None, False),
        (2, TYPE_VARINT, "resource", False, None, False),
        (3, TYPE_VARINT, "balance", True, None, False),
        (4, TYPE_LEN, "receiver_address", True, "base58", False),
        (5, TYPE_VARINT, "lock", False, "bool", False),
        (6, TYPE_VARINT, "lock_period", False, None, False),
    ]

    parsed_fields = _parse_contract_fields(r, field_definitions)

    delegate_contract = TronDelegateResourceContract()
    delegate_contract.resource = parsed_fields.get("resource", None)
    delegate_contract.balance = parsed_fields["balance"]
    delegate_contract.receiver_address = parsed_fields["receiver_address"]

    delegate_contract.lock = parsed_fields.get("lock", None)
    delegate_contract.lock_period = parsed_fields.get("lock_period", None)

    return delegate_contract


def _parse_undelegate_resource_contract(data: bytes):
    from trezor.messages import TronUnDelegateResourceContract

    r = BufferReader(data)

    field_definitions = [
        (1, TYPE_LEN, "owner_address", True, None, False),
        (2, TYPE_VARINT, "resource", False, None, False),
        (3, TYPE_VARINT, "balance", True, None, False),
        (4, TYPE_LEN, "receiver_address", True, "base58", False),
    ]

    parsed_fields = _parse_contract_fields(r, field_definitions)

    undelegate_contract = TronUnDelegateResourceContract()
    undelegate_contract.resource = parsed_fields.get("resource", None)
    undelegate_contract.balance = parsed_fields["balance"]
    undelegate_contract.receiver_address = parsed_fields["receiver_address"]

    return undelegate_contract


def _parse_cancel_all_unfreeze_v2_contract(data: bytes):
    from trezor.messages import TronCancelAllUnfreezeV2Contract

    r = BufferReader(data)

    field_definitions = [
        (1, TYPE_LEN, "owner_address", True, None, False),
    ]

    _ = _parse_contract_fields(r, field_definitions)

    return TronCancelAllUnfreezeV2Contract()


def _read_field_varint(
    r: BufferReader, expected_number: int, field_name: str = None, required: bool = True
) -> int | None:
    result = _read_field(r, expected_number, TYPE_VARINT, field_name, required)
    if result is None:
        return None
    if isinstance(result, int):
        return result
    else:
        raise ValueError(f"Expected int, got {type(result)}")


def _read_field_len(
    r: BufferReader, expected_number: int, field_name: str = None, required: bool = True
) -> bytes | None:
    result = _read_field(r, expected_number, TYPE_LEN, field_name, required)
    if result is None:
        return None
    if isinstance(result, bytes):
        return result
    else:
        raise ValueError(f"Expected bytes, got {type(result)}")


def _read_field(
    r: BufferReader,
    expected_number: int,
    expected_type: int,
    field_name: str = None,
    required: bool = True,
):
    offset_backup = r.offset
    if not required and r.remaining_count() == 0:
        return None

    field_number, field_type = read_field(r)

    if field_number == expected_number and field_type == expected_type:
        if expected_type == TYPE_LEN:
            return bytes(read_bytes_fixed(r))
        else:
            return read_varint(r)

    if required:
        field_desc = (
            f"{field_name} (field {expected_number}, type {expected_type})"
            if field_name
            else f"field {expected_number}, type {expected_type}"
        )
        raise ValueError(
            f"Invalid data: expected {field_desc}, got field {field_number}, type {field_type}"
        )
    else:
        r.seek(offset_backup)
        return None


def _ensure_non_none(value: Any, name: str) -> Any:
    if value is None:
        raise ValueError(f"Missing required field: {name}")
    return value


def deserialize(data: bytes) -> "TronSignTx":
    if not data:
        raise ValueError("Empty data")

    r = BufferReader(data)
    ref_block_bytes = _ensure_non_none(
        _read_field_len(r, 1, "ref_block_bytes"), "ref_block_bytes"
    )
    ref_block_hash = _ensure_non_none(
        _read_field_len(r, 4, "ref_block_hash"), "ref_block_hash"
    )
    expiration = _ensure_non_none(_read_field_varint(r, 8, "expiration"), "expiration")
    tx_data = _read_field_len(r, 10, "data", required=False)
    contract = _ensure_non_none(_read_field_len(r, 11, "contract"), "contract")
    parsed_contract = unpack_contract(contract)

    timestamp = _ensure_non_none(_read_field_varint(r, 14, "timestamp"), "timestamp")
    fee_limit = _read_field_varint(r, 18, "fee_limit", required=False)

    return TronSignTx(
        ref_block_bytes=ref_block_bytes,
        ref_block_hash=ref_block_hash,
        expiration=expiration,
        data=tx_data,
        contract=parsed_contract,
        timestamp=timestamp,
        fee_limit=fee_limit,
    )
