# Automatically generated by pb2py
import protobuf as p


class DebugLinkMemoryWrite(p.MessageType):
    FIELDS = {
        1: ('address', p.UVarintType, 0),
        2: ('memory', p.BytesType, 0),
        3: ('flash', p.BoolType, 0),
    }
    MESSAGE_WIRE_TYPE = 112
