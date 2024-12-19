import binascii

ADDRESS_LENGTH = 32
SEQUENCE_NUMBER_LENGTH = 8
DIGEST_LENGTH = 32

INPUT_TYPE_PURE = 0x00
INPUT_TYPE_OBJECT = 0x01

ARG_TYPE_INPUT = 0x01
ARG_TYPE_GAS_COIN = 0x00
ARG_TYPE_RESULT = 0x02
ARG_TYPE_NESTED_RESULT = 0x03


class ObjectArgType:
    IMM_OR_OWNED_OBJECT = 0x00


class CommandType:
    MOVE_CALL = 0
    TRANSFER_OBJECTS = 1
    SPLIT_COIN = 2
    MERGE_COINS = 3
    PUBLISH = 4
    MAKE_MOVE_VEC = 5
    UPGRADE = 6


class Address:
    def __init__(self, data):
        self.data = data

    def to_hex(self):
        return f"0x{binascii.hexlify(self.data).decode()}"


class ObjectReference:
    def __init__(self, address, sequence_number, digest):
        self.address = address
        self.sequence_number = sequence_number
        self.digest = digest

    def to_dict(self):
        return {
            "objectId": self.address.to_hex(),
            "sequenceNumber": self.sequence_number,
            "digest": binascii.hexlify(self.digest).decode(),
        }


class PureData:
    def __init__(self, data):
        self.data = data


class ParsedInput:
    def __init__(self, kind, value, index, type):
        self.kind = kind
        self.value = value
        self.index = index
        self.type = type


class Argument:
    def __init__(self, arg_type, index):
        self.arg_type = arg_type
        self.index = index


class Command:
    def __init__(self, type, data):
        self.type = type
        self.data = data


class BCSParser:
    def __init__(self, data):
        self.data = data
        self.cursor = 0

    def read_bytes(self, length: int) -> bytes | None:
        if self.cursor + length > len(self.data):
            return None
        result = self.data[self.cursor : self.cursor + length]
        self.cursor += length
        return result

    def read_u8(self) -> int | None:
        data = self.read_bytes(1)
        if data is None:
            return None
        return int.from_bytes(data, "little")

    def read_u64(self) -> int | None:
        data = self.read_bytes(8)
        if data is None:
            return None
        return int.from_bytes(data, "little")

    def read_address(self) -> Address | None:
        data = self.read_bytes(ADDRESS_LENGTH)
        if data is None:
            return None
        return Address(data)

    def read_object_reference(self) -> ObjectReference | None:
        address = self.read_address()
        if address is None:
            return None

        sequence_number = self.read_u64()
        if sequence_number is None:
            return None

        digest_length = self.read_u8()
        if digest_length is None or digest_length != DIGEST_LENGTH:
            return None

        digest = self.read_bytes(DIGEST_LENGTH)
        if digest is None:
            return None

        if address.data is None:
            return None

        return ObjectReference(address, sequence_number, digest)

    def read_pure_data(self) -> PureData | None:
        length = self.read_u8()
        if length is None:
            return None
        data = self.read_bytes(length)
        if data is None:
            return None
        return PureData(list(data))

    def read_argument(self) -> dict | None:
        arg_type = self.read_u8()
        if arg_type is None:
            return None

        if arg_type == ARG_TYPE_INPUT:
            data = self.read_bytes(2)
            if data is None:
                return None
            index = int.from_bytes(data, "little")
            return {"type": "Input", "index": index}
        elif arg_type == ARG_TYPE_GAS_COIN:
            return {"type": "GasCoin"}
        elif arg_type == ARG_TYPE_RESULT:
            data = self.read_bytes(2)
            if data is None:
                return None
            index = int.from_bytes(data, "little")
            return {"type": "Result", "index": index}
        elif arg_type == ARG_TYPE_NESTED_RESULT:
            data1 = self.read_bytes(2)
            if data1 is None:
                return None
            data2 = self.read_bytes(2)
            if data2 is None:
                return None
            index1 = int.from_bytes(data1, "little")
            index2 = int.from_bytes(data2, "little")
            return {"type": "NestedResult", "index": [index1, index2]}
        else:
            return None

    def read_argument_vector(self) -> list | None:
        count = self.read_u8()
        if count is None:
            return None

        arguments = []
        for _ in range(count):
            arg = self.read_argument()
            if arg is None:
                return None
            arguments.append(arg)

        return arguments

    def read_split_coin_command(self) -> dict | None:
        from_coin = self.read_argument()
        if from_coin is None:
            return None

        amount_count = self.read_u8()
        if amount_count is None:
            return None

        amounts = []
        for _ in range(amount_count):
            amount = self.read_argument()
            if amount is None:
                return None
            amounts.append(amount)

        return {"type": "SplitCoin", "data": {"coin": from_coin, "amounts": amounts}}

    def read_transfer_objects_command(self) -> dict | None:
        objects_count = self.read_u8()
        if objects_count is None:
            return None

        objects = []
        for _ in range(objects_count):
            obj = self.read_argument()
            if obj is None:
                return None
            objects.append(obj)

        if len(objects) != 1:
            return None

        address = self.read_argument()
        if address is None:
            return None

        return {
            "type": "TransferObjects",
            "data": {"objects": objects, "address": address},
        }

    def read_merge_coins_command(self) -> dict | None:

        to_coin = self.read_argument()
        if to_coin is None:
            return None

        from_coins_count = self.read_u8()
        if from_coins_count is None:
            return None

        from_coins = []
        for _ in range(from_coins_count):
            coin = self.read_argument()
            if coin is None:
                return None
            from_coins.append(coin)

        return {
            "type": "MergeCoins",
            "data": {"to_coin": to_coin, "from_coins": from_coins},
        }

    def read_commands(self) -> list | None:
        command_count = self.read_u8()
        if command_count is None:
            return None

        commands = []
        for _ in range(command_count):
            command_type = self.read_u8()
            if command_type is None:
                return None

            command = None
            if command_type == CommandType.SPLIT_COIN:
                command = self.read_split_coin_command()
            elif command_type == CommandType.TRANSFER_OBJECTS:
                command = self.read_transfer_objects_command()
            elif command_type == CommandType.MERGE_COINS:
                command = self.read_merge_coins_command()
            else:
                return None

            if command is None:
                return None
            commands.append(command)

        return commands


def parse_transaction_inputs(parser):
    input_count = parser.read_u8()

    inputs = []

    for i in range(input_count):
        input_type = parser.read_u8()
        if input_type == INPUT_TYPE_OBJECT:
            sub_type = parser.read_u8()
            if sub_type != ObjectArgType.IMM_OR_OWNED_OBJECT:
                return None
            obj_ref = parser.read_object_reference()
            inputs.append(
                ParsedInput(
                    kind="Input",
                    value={
                        "type": "ImmOrOwnedObject",
                        "objectId": f"0x{binascii.hexlify(obj_ref.address.data).decode()}",
                        "sequenceNumber": obj_ref.sequence_number,
                        "digest": binascii.hexlify(obj_ref.digest).decode(),
                    },
                    index=i,
                    type="object",
                )
            )

        elif input_type == INPUT_TYPE_PURE:
            pure_data = parser.read_pure_data()
            inputs.append(
                ParsedInput(
                    kind="Input", value={"Pure": pure_data.data}, index=i, type="pure"
                )
            )
        else:
            return None

    return inputs


def parse_gas_data(parser):
    payment_count = parser.read_u8()
    payments = []
    for _ in range(payment_count):
        obj_ref = parser.read_object_reference()
        payments.append(
            {
                "objectId": obj_ref.address.to_hex(),
                "sequenceNumber": obj_ref.sequence_number,
            }
        )
    owner_bytes = parser.read_bytes(ADDRESS_LENGTH)
    owner = f"0x{binascii.hexlify(owner_bytes).decode()}"
    price = parser.read_u64()
    budget = parser.read_u64()
    return {"payment": payments, "owner": owner, "price": price, "budget": budget}


def parse_transaction_expiration(parser):
    expiration_type = parser.read_u8()
    if expiration_type == 0:
        return {"type": "None", "value": None}
    elif expiration_type == 1:
        epoch = parser.read_u64()
        return {"type": "Epoch", "value": epoch}
    else:
        return None


def parse_transaction(hex_data):
    try:
        data = binascii.unhexlify(hex_data)
        parser = BCSParser(data)

        version = parser.read_u8()
        if version is None:
            return None

        parser.read_bytes(3)
        kind_type = parser.read_u8()
        if kind_type is None:
            return None

        inputs = parse_transaction_inputs(parser)
        if inputs is None:
            return None

        commands = parser.read_commands()
        if commands is None:
            return None

        sender_bytes = parser.read_bytes(ADDRESS_LENGTH)
        if sender_bytes is None:
            return None
        sender = "0x" + binascii.hexlify(sender_bytes).decode()

        gas_data = parse_gas_data(parser)
        if gas_data is None:
            return None

        expiration = parse_transaction_expiration(parser)

        return {
            "version": version,
            "kind_type": hex(kind_type),
            "sender": sender,
            "inputs": inputs,
            "commands": commands,
            "gas_data": gas_data,
            "expiration": expiration,
        }
    except Exception:
        return None


class TransactionParser:
    def parse_tx(self, tx_hex):
        try:
            if isinstance(tx_hex, bytes):
                tx_hex = binascii.hexlify(tx_hex).decode()
            if tx_hex.startswith("0x"):
                tx_hex = tx_hex[2:]

            result = parse_transaction(tx_hex)

            if result is None:
                return None

            formatted_inputs = []
            for _, input in enumerate(result["inputs"]):
                try:
                    if input.type == "pure":
                        hex_value = binascii.hexlify(
                            bytes(input.value["Pure"])
                        ).decode()
                        formatted_inputs.append({"Pure": hex_value})
                    else:
                        formatted_inputs.append(input.value)
                except Exception:
                    return None

            final_result = {
                "V1": {
                    "TransactionKind": {
                        "ProgrammableTransaction": {
                            "Inputs": formatted_inputs,
                            "Commands": result["commands"],
                            "Sender": {"Address": result["sender"]},
                        }
                    },
                    "GasData": result["gas_data"],
                    "Expiration": result["expiration"],
                }
            }
            return final_result

        except Exception:
            return None
