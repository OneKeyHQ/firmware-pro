from apps.ur_registry.registry_types import UUID
from apps.ur_registry.ur_py.ur.cbor_lite import CBORDecoder, CBOREncoder
from apps.ur_registry.ur_py.ur.ur import UR

REQUEST_ID = 1
SIGNATURE = 2


class BaseSignature:
    def __init__(self, request_id=None, signature=None):
        self.request_id = request_id
        self.signature = signature

    @staticmethod
    def get_registry_type():
        raise NotImplementedError

    @staticmethod
    def get_tag():
        raise NotImplementedError

    @classmethod
    def new(cls, request_id, signature):
        return cls(request_id, signature)

    def get_request_id(self):
        return self.request_id

    def get_signature(self):
        return self.signature

    def set_request_id(self, request_id):
        self.request_id = request_id

    def set_signature(self, signature):
        self.signature = signature

    def get_map_size(self):
        size = 1
        if self.request_id is not None:
            size += 1
        return size

    def cbor_encode(self):
        encoder = CBOREncoder()
        size = self.get_map_size()
        encoder.encodeMapSize(size)
        if self.request_id is not None:
            encoder.encodeInteger(REQUEST_ID)
            encoder.encodeTag(UUID.get_tag())
            encoder.encodeBytes(self.request_id)

        encoder.encodeInteger(SIGNATURE)
        encoder.encodeBytes(self.signature)

        return encoder.get_bytes()

    def ur_encode(self):
        data = self.cbor_encode()
        return UR(self.get_registry_type(), data)

    @classmethod
    def from_cbor(cls, cbor):
        decoder = CBORDecoder(cbor)
        return cls.decode(decoder)

    @classmethod
    def decode(cls, decoder):
        base_signature = cls()
        size, _ = decoder.decodeMapSize()
        for _ in range(size):
            key, _ = decoder.decodeInteger()
            if key == REQUEST_ID:
                tag, _ = decoder.decodeTag()
                if tag != UUID.get_tag():
                    raise Exception(f"Expected Tag {tag}")
                base_signature.request_id, _ = decoder.decodeBytes()
            if key == SIGNATURE:
                base_signature.signature, _ = decoder.decodeBytes()

        return base_signature
