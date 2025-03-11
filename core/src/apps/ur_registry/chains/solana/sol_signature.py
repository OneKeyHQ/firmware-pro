from apps.ur_registry.registry_types import SOL_SIGNATURE, UUID
from apps.ur_registry.ur_py.ur.cbor_lite import CBORDecoder, CBOREncoder
from apps.ur_registry.ur_py.ur.ur import UR

REQUEST_ID = 1
SIGNATURE = 2


class SolSignature:
    def __init__(self, request_id=None, signature=None):
        self.request_id = request_id
        self.signature = signature

    @staticmethod
    def get_registry_type():
        return SOL_SIGNATURE.get_registry_type()

    @staticmethod
    def get_tag():
        return SOL_SIGNATURE.get_tag()

    @staticmethod
    def new(request_id, signature):
        return SolSignature(request_id, signature)

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
        return UR(SolSignature.get_registry_type(), data)

    @staticmethod
    def from_cbor(cbor):
        decoder = CBORDecoder(cbor)
        return SolSignature.decode(decoder)

    @staticmethod
    def decode(decoder):
        sol_sign_req = SolSignature()
        size, _ = decoder.decodeMapSize()
        for _ in range(size):
            key, _ = decoder.decodeInteger()
            if key == REQUEST_ID:
                tag, _ = decoder.decodeTag()
                if tag != UUID.get_tag():
                    raise Exception(f"Expected Tag {tag}")
                sol_sign_req.request_id, _ = decoder.decodeBytes()
            if key == SIGNATURE:
                sol_sign_req.signature, _ = decoder.decodeBytes()

        return sol_sign_req
