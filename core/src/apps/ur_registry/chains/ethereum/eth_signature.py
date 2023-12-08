from apps.ur_registry.registry_types import UUID
from apps.ur_registry.ur_py.ur.cbor_lite import CBORDecoder, CBOREncoder
from apps.ur_registry.ur_py.ur.ur import UR

REQUEST_ID = 1
SIGNATURE = 2
ORIGIN = 3


class EthSignature:
    def __init__(self, request_id=None, signature=None, origin=None):
        self.request_id = request_id
        self.signature = signature
        self.origin = origin

    @staticmethod
    def get_registry_type():
        return "eth-signature"

    @staticmethod
    def get_tag():
        return 402

    @staticmethod
    def new(request_id, signature, origin):
        return EthSignature(request_id, signature, origin)

    def get_request_id(self):
        return self.request_id

    def get_signature(self):
        return self.signature

    def get_origin(self):
        return self.origin

    def set_request_id(self, request_id):
        self.request_id = request_id

    def set_signature(self, signature):
        self.signature = signature

    def set_origin(self, origin):
        self.origin = origin

    def get_map_size(self):
        size = 1 + sum(
            (
                self.request_id is not None,
                self.origin is not None,
            )
        )
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

        if self.origin is not None:
            encoder.encodeInteger(ORIGIN)
            encoder.encodeText(self.origin)

        return encoder.get_bytes()

    def ur_encode(self):
        data = self.cbor_encode()
        return UR(EthSignature.get_registry_type(), data)

    @staticmethod
    def from_cbor(cbor):
        decoder = CBORDecoder(cbor)
        return EthSignature.decode(decoder)

    @staticmethod
    def decode(decoder):
        eth_sign_req = EthSignature()
        size, _ = decoder.decodeMapSize()
        for _ in range(size):
            key, _ = decoder.decodeInteger()
            if key == REQUEST_ID:
                tag, _ = decoder.decodeTag()
                if tag != UUID.get_tag():
                    raise Exception(f"Expected Tag {tag}")
                eth_sign_req.request_id, _ = decoder.decodeBytes()
            if key == SIGNATURE:
                eth_sign_req.signature, _ = decoder.decodeBytes()
            if key == ORIGIN:
                eth_sign_req.origin, _ = decoder.decodeText()
        return eth_sign_req
