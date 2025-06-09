import re
from typing import Any, Optional

from trezor import messages
from trezor.enums import EthereumDataType, FailureType
from trezor.wire import QR_CONTEXT

from .eth_sign_request import EthSignRequest


class EthereumTypedDataTransacion:
    def __init__(self, req: EthSignRequest):
        self.req = req
        self.qr = None
        self.encoder = None

    def get_data(self):
        return self.req.get_sign_data().decode()

    @staticmethod
    def sanitize_typed_data(data: dict) -> dict:
        """Remove properties from a message object that are not defined per EIP-712."""
        REQUIRED_KEYS = ("types", "primaryType", "domain", "message")
        sanitized_data = {key: data[key] for key in REQUIRED_KEYS}
        sanitized_data["types"].setdefault("EIP712Domain", [])
        return sanitized_data

    @staticmethod
    def is_array(type_name: str) -> bool:
        return type_name[-1] == "]"

    @staticmethod
    def parse_type_n(type_name: str) -> int:
        """Parse N from type<N>. Example: "uint256" -> 256."""
        match = re.search(r"\d+$", type_name)
        if match:
            return int(match.group(0))
        else:
            raise ValueError(f"Could not parse type<N> from {type_name}.")

    @staticmethod
    def typeof_array(type_name: str) -> str:
        return type_name[: type_name.rindex("[")]

    @staticmethod
    def get_byte_size_for_int_type(int_type: str) -> int:
        return EthereumTypedDataTransacion.parse_type_n(int_type) // 8

    @staticmethod
    def parse_array_n(type_name: str) -> Optional[int]:
        """Parse N in type[<N>] where "type" can itself be an array type."""
        # sign that it is a dynamic array - we do not know <N>
        if type_name.endswith("[]"):
            return None

        start_idx = type_name.rindex("[") + 1
        return int(type_name[start_idx:-1])

    @staticmethod
    def get_field_type(type_name: str, types: dict) -> messages.EthereumFieldType:
        data_type = None
        size = None
        entry_type = None
        struct_name = None

        if EthereumTypedDataTransacion.is_array(type_name):
            data_type = EthereumDataType.ARRAY
            size = EthereumTypedDataTransacion.parse_array_n(type_name)
            member_typename = EthereumTypedDataTransacion.typeof_array(type_name)
            entry_type = EthereumTypedDataTransacion.get_field_type(
                member_typename, types
            )
            # Not supporting nested arrays currently
            if entry_type.data_type == EthereumDataType.ARRAY:
                raise NotImplementedError("Nested arrays are not supported")
        elif type_name.startswith("uint"):
            data_type = EthereumDataType.UINT
            size = EthereumTypedDataTransacion.get_byte_size_for_int_type(type_name)
        elif type_name.startswith("int"):
            data_type = EthereumDataType.INT
            size = EthereumTypedDataTransacion.get_byte_size_for_int_type(type_name)
        elif type_name.startswith("bytes"):
            data_type = EthereumDataType.BYTES
            size = (
                None
                if type_name == "bytes"
                else EthereumTypedDataTransacion.parse_type_n(type_name)
            )
        elif type_name == "string":
            data_type = EthereumDataType.STRING
        elif type_name == "bool":
            data_type = EthereumDataType.BOOL
        elif type_name == "address":
            data_type = EthereumDataType.ADDRESS
        elif type_name in types:
            data_type = EthereumDataType.STRUCT
            size = len(types[type_name])
            struct_name = type_name
        else:
            raise ValueError(f"Unsupported type name: {type_name}")

        return messages.EthereumFieldType(
            data_type=data_type,
            size=size,
            entry_type=entry_type,
            struct_name=struct_name,
        )

    @staticmethod
    def decode_hex(value: str) -> bytes:
        from binascii import unhexlify

        hex_str = value.replace("0x", "").replace("0X", "")

        if len(hex_str) % 2:
            hex_str = "0" + hex_str

        return unhexlify(hex_str)

    @staticmethod
    def encode_data(value: Any, type_name: str) -> bytes:
        if type_name.startswith("bytes"):
            return EthereumTypedDataTransacion.decode_hex(value)
        elif type_name == "string":
            return value.encode()
        # elif type_name.startswith(("int", "uint")):
        elif type_name.startswith("int") or type_name.startswith("uint"):
            byte_length = EthereumTypedDataTransacion.get_byte_size_for_int_type(
                type_name
            )
            return int(value).to_bytes(byte_length, "big")
        elif type_name == "bool":
            if not isinstance(value, bool):
                raise ValueError(f"Invalid bool value - {value}")
            return int(value).to_bytes(1, "big")
        elif type_name == "address":
            return EthereumTypedDataTransacion.decode_hex(value)

        # We should be receiving only atomic, non-array types
        raise ValueError(
            f"Unsupported data type for direct field encoding: {type_name}"
        )

    async def run(self):
        import ujson as json

        msg = json.loads(self.get_data())
        data = EthereumTypedDataTransacion.sanitize_typed_data(msg)

        request = messages.EthereumSignTypedData(
            address_n=self.req.get_address_n(),
            primary_type=data["primaryType"],
            metamask_v4_compat=True,
            definitions=None,
        )

        from apps.ethereum.sign_typed_data import sign_typed_data
        from trezor import loop
        from apps.ur_registry.chains.ethereum.eth_signature import EthSignature
        from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder

        # pyright: off
        task = sign_typed_data(QR_CONTEXT, request)
        loop.spawn(self.interact())
        try:
            resp = await loop.spawn(task)
        except Exception as e:
            if __debug__:
                print(f"Error: {e}")
            raise e
        finally:
            await QR_CONTEXT.interact_stop()
        # pyright: on
        self.signature = resp.signature
        eth_signature = EthSignature(
            request_id=self.req.get_request_id(),
            signature=self.signature,
            origin="OneKey Pro",
        )
        ur = eth_signature.ur_encode()
        encoded = UREncoder.encode(ur).upper()
        self.qr = encoded

    async def interact(self):
        import ujson as json

        msg = json.loads(self.get_data())
        data = EthereumTypedDataTransacion.sanitize_typed_data(msg)
        types = msg["types"]
        while True:
            response = await QR_CONTEXT.qr_receive()
            if response is None:
                if __debug__:
                    print("eth sign type data interaction finished")
                break
            try:
                if messages.EthereumTypedDataStructRequest.is_type_of(response):
                    struct_name = response.name
                    members: list["messages.EthereumStructMember"] = []
                    for field in types[struct_name]:
                        field_type = EthereumTypedDataTransacion.get_field_type(
                            field["type"], types
                        )
                        struct_member = messages.EthereumStructMember(
                            type=field_type,
                            name=field["name"],
                        )
                        members.append(struct_member)

                    response = messages.EthereumTypedDataStructAck(members=members)
                elif messages.EthereumTypedDataValueRequest.is_type_of(response):
                    root_index = response.member_path[0]
                    # Index 0 is for the domain data, 1 is for the actual message
                    if root_index == 0:
                        member_typename = "EIP712Domain"
                        member_data = data["domain"]
                    elif root_index == 1:
                        member_typename = data["primaryType"]
                        member_data = data["message"]
                    else:
                        raise ValueError("Root index can only be 0 or 1")

                    for index in response.member_path[1:]:
                        if isinstance(member_data, dict):
                            member_def = types[member_typename][index]
                            member_typename = member_def["type"]
                            member_data = member_data[member_def["name"]]
                        elif isinstance(member_data, list):
                            member_typename = EthereumTypedDataTransacion.typeof_array(
                                member_typename
                            )
                            member_data = member_data[index]

                    if isinstance(member_data, list):
                        # Sending the length as uint16
                        encoded_data = len(member_data).to_bytes(2, "big")
                    else:
                        encoded_data = EthereumTypedDataTransacion.encode_data(
                            member_data, member_typename
                        )

                    response = messages.EthereumTypedDataValueAck(value=encoded_data)
                elif messages.ButtonRequest.is_type_of(response):
                    response = messages.ButtonAck()
                elif messages.EthereumGnosisSafeTxRequest.is_type_of(response):
                    message = data["message"]
                    operation = int(message["operation"])
                    from trezor.enums import EthereumGnosisSafeTxOperation

                    if operation == 0:
                        operation = EthereumGnosisSafeTxOperation.CALL
                    elif operation == 1:
                        operation = EthereumGnosisSafeTxOperation.DELEGATE_CALL
                    else:
                        raise ValueError(f"Invalid operation: {operation}")
                    response = messages.EthereumGnosisSafeTxAck(
                        to=message["to"],
                        value=int(message["value"]).to_bytes(32, "big"),
                        data=EthereumTypedDataTransacion.decode_hex(message["data"]),
                        operation=operation,
                        safeTxGas=int(message["safeTxGas"]).to_bytes(32, "big"),
                        baseGas=int(message["baseGas"]).to_bytes(32, "big"),
                        gasPrice=int(message["gasPrice"]).to_bytes(32, "big"),
                        gasToken=message["gasToken"],
                        refundReceiver=message["refundReceiver"],
                        nonce=int(message["nonce"]).to_bytes(32, "big"),
                        chain_id=int.from_bytes(
                            EthereumTypedDataTransacion.decode_hex(
                                data["domain"]["chainId"]
                            ),
                            "big",
                        ),
                        verifyingContract=data["domain"]["verifyingContract"],
                    )
                else:
                    response = messages.Failure(
                        code=FailureType.UnexpectedMessage,
                        message=f"Unknown message {response.MESSAGE_NAME}",
                    )
                    if __debug__:
                        print(f"Message error: {response}.")
            except Exception as e:
                if __debug__:
                    print(f"Data error: {e}")
                response = messages.Failure(
                    code=FailureType.DataError, message=f"Error: {e}"
                )
            finally:
                await QR_CONTEXT.qr_send(response)
