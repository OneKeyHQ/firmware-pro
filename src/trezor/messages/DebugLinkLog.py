# Automatically generated by pb2py
import protobuf as p


class DebugLinkLog(p.MessageType):
    FIELDS = {
        1: ('level', p.UVarintType, 0),
        2: ('bucket', p.UnicodeType, 0),
        3: ('text', p.UnicodeType, 0),
    }
    MESSAGE_WIRE_TYPE = 104
