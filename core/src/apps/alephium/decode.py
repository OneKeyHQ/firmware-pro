import binascii

from trezor.crypto import base58

SCRIPT_TYPE_P2PKH = 0
SCRIPT_TYPE_P2MPKH = 1
SCRIPT_TYPE_P2SH = 2
SINGLE_BYTE_LIMIT = 0x40
TWO_BYTE_LIMIT = 0x80
MULTI_BYTE_LIMIT = 0xC0


def generate_address_from_output(lockup_script_type, lockup_script_hash):
    if lockup_script_type not in [
        SCRIPT_TYPE_P2PKH,
        SCRIPT_TYPE_P2MPKH,
        SCRIPT_TYPE_P2SH,
    ]:
        raise ValueError(f"Unsupported lockup script type: {lockup_script_type}")

    address_bytes = bytes([lockup_script_type]) + binascii.unhexlify(lockup_script_hash)
    return base58.encode(address_bytes)


def decode_unlock_script(data):
    script_type = data[0]
    if script_type == SCRIPT_TYPE_P2PKH:  # P2PKH
        return binascii.hexlify(data[:34]).decode(), 34
    elif script_type == SCRIPT_TYPE_P2MPKH:  # P2MPKH
        length, bytes_read = decode_compact_int(data[1:])
        total_length = 1 + bytes_read + length * 37
        return binascii.hexlify(data[:total_length]).decode(), total_length
    elif script_type == SCRIPT_TYPE_P2SH:  # P2SH
        script_length, bytes_read = decode_compact_int(data[1:])
        params_length, params_bytes_read = decode_compact_int(
            data[1 + bytes_read + script_length :]
        )
        total_length = (
            1 + bytes_read + script_length + params_bytes_read + params_length
        )
        return binascii.hexlify(data[:total_length]).decode(), total_length
    elif script_type == 3:
        return "03", 1
    else:
        raise ValueError(f"Unknown unlock script type: {script_type}")


def decode_compact_int(data):
    first_byte = data[0]
    if first_byte < 0xFD:
        return first_byte, 1
    elif first_byte == 0xFD:
        return int.from_bytes(data[1:3], "little"), 3
    elif first_byte == 0xFE:
        return int.from_bytes(data[1:5], "little"), 5
    else:
        return int.from_bytes(data[1:9], "little"), 9


def decode_i32(data: bytes) -> tuple[int, int]:
    if not data:
        raise ValueError("Empty input")

    first_byte = data[0]
    prefix = first_byte & 0xC0

    if prefix == 0x00:
        if first_byte & 0x20:
            return -(64 - first_byte), 1
        else:
            return first_byte, 1
    elif prefix == 0x40:
        if len(data) < 2:
            raise ValueError("Insufficient bytes for two-byte encoding")
        value = ((first_byte & 0x3F) << 8) | data[1]
        if first_byte & 0x20:
            return -(16384 - value), 2
        else:
            return value, 2
    elif prefix == 0x80:
        if len(data) < 4:
            raise ValueError("Insufficient bytes for four-byte encoding")
        value = ((first_byte & 0x3F) << 24) | (data[1] << 16) | (data[2] << 8) | data[3]
        if first_byte & 0x20:
            return -(1073741824 - value), 4
        else:
            return value, 4
    else:
        length = (first_byte & 0x3F) + 5
        if len(data) < length:
            raise ValueError(f"Insufficient bytes for {length}-byte encoding")
        value = int.from_bytes(data[1:length], byteorder="big")
        if first_byte & 0x20:
            return -value, length
        else:
            return value, length


def decode_u256(data):
    if not data:
        raise ValueError("data is empty")
    first_byte = data[0]
    if first_byte < SINGLE_BYTE_LIMIT:
        return first_byte, 1
    elif first_byte < TWO_BYTE_LIMIT:
        return ((first_byte & 0x3F) << 8) | data[1], 2
    elif first_byte < MULTI_BYTE_LIMIT:
        length = (first_byte - TWO_BYTE_LIMIT) + 3
        return int.from_bytes(data[1 : length + 1], "big"), length + 1
    else:
        length = (first_byte - MULTI_BYTE_LIMIT) + 4
        return int.from_bytes(data[1 : length + 1], "big"), length + 1


def decode_tx(encoded_tx):

    if isinstance(encoded_tx, str):
        try:
            data = binascii.unhexlify(encoded_tx)
        except binascii.Error as e:
            raise ValueError(f"Invalid hex string: {e}")
    elif isinstance(encoded_tx, bytes):
        data = encoded_tx
    else:
        raise ValueError("Input must be a hex string or bytes")

    index = 0
    version = data[index]
    index += 1
    network_id = data[index]
    index += 1
    script_opt = data[index]
    index += 1

    gas_amount, bytes_read = decode_i32(data[index:])
    index += bytes_read

    gas_price, bytes_read = decode_u256(data[index:])
    index += bytes_read

    inputs_count, bytes_read = decode_compact_int(data[index:])
    index += bytes_read

    inputs = []
    for i in range(inputs_count):
        hint = int.from_bytes(data[index : index + 4], "big")
        index += 4
        key = binascii.hexlify(data[index : index + 32]).decode()
        index += 32
        unlock_script, script_length = decode_unlock_script(data[index:])
        index += script_length
        inputs.append({"hint": hint, "key": key, "unlockScript": unlock_script})

    outputs_count, bytes_read = decode_compact_int(data[index:])
    index += bytes_read

    outputs = []
    for i in range(outputs_count):
        if index >= len(data):
            break

        if i > 0 and data[index] in [0x00, 0x01]:
            index += 1

        amount, bytes_read = decode_u256(data[index:])
        index += bytes_read

        lockup_script_type = data[index]
        index += 1
        lockup_script_hash = binascii.hexlify(data[index : index + 32]).decode()
        index += 32

        address = generate_address_from_output(lockup_script_type, lockup_script_hash)

        lock_time = int.from_bytes(data[index : index + 4], "big")
        index += 4

        message_length = int.from_bytes(data[index : index + 4], "big")
        index += 4

        message = binascii.hexlify(data[index : index + message_length]).decode()
        index += message_length

        tokens_count, bytes_read = decode_compact_int(data[index:])
        index += bytes_read

        tokens = []
        for _ in range(tokens_count):
            token_id = binascii.hexlify(data[index : index + 32]).decode()
            index += 32
            token_amount, bytes_read = decode_u256(data[index:])
            index += bytes_read
            tokens.append({"id": token_id, "amount": str(token_amount)})

        outputs.append(
            {
                "amount": str(amount),
                "lockupScriptType": lockup_script_type,
                "address": address,
                "lockTime": lock_time,
                "message": message,
                "tokens": tokens,
            }
        )
    if index + 1 < len(data):
        raise ValueError("Extra unparsed data: Transaction decoding failed")
    return {
        "version": version,
        "networkId": network_id,
        "scriptOption": script_opt,
        "gasAmount": gas_amount,
        "gasPrice": str(gas_price),
        "inputs": inputs,
        "outputs": outputs,
    }
