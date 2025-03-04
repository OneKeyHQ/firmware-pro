from typing import TYPE_CHECKING

import trezorproto

decode = trezorproto.decode
encode = trezorproto.encode
encoded_length = trezorproto.encoded_length
type_for_name = trezorproto.type_for_name
type_for_wire = trezorproto.type_for_wire

if TYPE_CHECKING:
    # XXX
    # Note that MessageType "subclasses" are not true subclasses, but instead instances
    # of the built-in metaclass MsgDef. MessageType instances are in fact instances of
    # the built-in type Msg. That is why isinstance checks do not work, and instead the
    # MessageTypeSubclass.is_type_of() method must be used.
    from typing import TypeGuard, TypeVar

    T = TypeVar("T", bound="MessageType")

    class MessageType:
        MESSAGE_NAME: str = "MessageType"
        MESSAGE_WIRE_TYPE: int | None = None

        @classmethod
        def is_type_of(cls: type[T], msg: "MessageType") -> TypeGuard[T]:
            """Identify if the provided message belongs to this type."""
            raise NotImplementedError


def load_message_buffer(
    buffer: bytes,
    msg_wire_type: int,
    experimental_enabled: bool = True,
) -> MessageType:
    msg_type = type_for_wire(msg_wire_type)
    return decode(buffer, msg_type, experimental_enabled)


def dump_message_buffer(msg: MessageType) -> bytearray:
    buffer = bytearray(encoded_length(msg))
    encode(buffer, msg)
    return buffer


def print_message(msg: MessageType, indent: int = 0, drop_none: bool = False):
    if indent == 0:
        print(
            f"========================= BEGIN --- Msg: {msg.MESSAGE_NAME} Type: {msg.MESSAGE_WIRE_TYPE} ========================="
        )

    try:
        for key, value in msg.__dict__.items():
            if value:
                if type(value) is type(msg):
                    print(
                        f"{'    ' * (indent)}{str(key)}:{value.MESSAGE_NAME}:{type(value)} = "
                    )
                    print_message(value, indent + 1)
                else:
                    print(f"{'    ' * (indent)}{str(key)}:{type(value)} = ", end="")
                    if type(value) is not bytes:
                        print(
                            str(value)
                        )  # have to use otherwise may corrupt the output
                    else:
                        print("".join(f"{x:02x}" for x in value))
            elif not drop_none:
                print(f"{'    ' * (indent)}{str(key)}:{type(value)} = {str(value)}")
            else:
                continue

    except Exception as ex:
        from traceback import print_exception

        print(
            f"Error while handling Msg: {msg.MESSAGE_NAME} Type: {msg.MESSAGE_WIRE_TYPE}"
        )
        print_exception(ex)
        pass

    if indent == 0:
        print(
            f"========================= END --- Msg: {msg.MESSAGE_NAME} Type: {msg.MESSAGE_WIRE_TYPE} ========================="
        )
