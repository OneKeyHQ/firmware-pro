# Automatically generated by pb2py
import protobuf as p
from .HDNodeType import HDNodeType


class DebugLinkState(p.MessageType):
    FIELDS = {
        1: ('layout', p.BytesType, 0),
        2: ('pin', p.UnicodeType, 0),
        3: ('matrix', p.UnicodeType, 0),
        4: ('mnemonic', p.UnicodeType, 0),
        5: ('node', HDNodeType, 0),
        6: ('passphrase_protection', p.BoolType, 0),
        7: ('reset_word', p.UnicodeType, 0),
        8: ('reset_entropy', p.BytesType, 0),
        9: ('recovery_fake_word', p.UnicodeType, 0),
        10: ('recovery_word_pos', p.UVarintType, 0),
    }
    MESSAGE_WIRE_TYPE = 102
