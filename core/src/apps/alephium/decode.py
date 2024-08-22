import binascii

from apps.alephium.get_address import b58encode


def generate_address_from_output(lockup_script_type, lockup_script_hash):
    if lockup_script_type == 0:  # P2PKH
        return generate_p2pkh_address(lockup_script_hash)
    elif lockup_script_type == 1:  # P2MPKH
        return generate_p2mpkh_address(lockup_script_hash)
    elif lockup_script_type == 2:  # P2SH
        return generate_p2sh_address(lockup_script_hash)
    else:
        raise ValueError(f"Unsupported lockup script type: {lockup_script_type}")


def generate_p2pkh_address(lockup_script_hash: str) -> str:
    address_bytes = bytes([0x00]) + binascii.unhexlify(lockup_script_hash)
    return b58encode(address_bytes)


def generate_p2mpkh_address(lockup_script_hash: str) -> str:
    address_bytes = bytes([0x01]) + binascii.unhexlify(lockup_script_hash)
    return b58encode(address_bytes)


def generate_p2sh_address(lockup_script_hash: str) -> str:
    address_bytes = bytes([0x02]) + binascii.unhexlify(lockup_script_hash)
    return b58encode(address_bytes)


def decode_unlock_script(data):
    script_type = data[0]
    if script_type == 0x00:  # P2PKH
        return binascii.hexlify(data[:34]).decode(), 34
    elif script_type == 0x01:  # P2MPKH
        length, bytes_read = decode_compact_int(data[1:])
        total_length = 1 + bytes_read + length * 37
        return binascii.hexlify(data[:total_length]).decode(), total_length
    elif script_type == 0x02:  # P2SH
        script_length, bytes_read = decode_compact_int(data[1:])
        params_length, params_bytes_read = decode_compact_int(
            data[1 + bytes_read + script_length :]
        )
        total_length = (
            1 + bytes_read + script_length + params_bytes_read + params_length
        )
        return binascii.hexlify(data[:total_length]).decode(), total_length
    elif script_type == 0x03:
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


def decode_i32(data):
    return int.from_bytes(data[:4], "big"), 4


def decode_u256(data):
    first_byte = data[0]
    if first_byte < 0x40:
        return first_byte, 1
    elif first_byte < 0x80:
        length = (first_byte - 0x40) + 1
        return int.from_bytes(data[1 : length + 1], "big"), length + 1
    elif first_byte < 0xC0:
        length = (first_byte - 0x80) + 3
        return int.from_bytes(data[1 : length + 1], "big"), length + 1
    else:
        length = (first_byte - 0xC0) + 4
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
    if gas_amount & 0x80000000:
        gas_amount &= 0x7FFFFFFF

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

    return {
        "version": version,
        "networkId": network_id,
        "scriptOption": script_opt,
        "gasAmount": gas_amount,
        "gasPrice": str(gas_price),
        "inputs": inputs,
        "outputs": outputs,
    }
