from apps.ur_registry.ur_py.ur.cbor_lite import CBORDecoder, CBOREncoder
from apps.ur_registry.ur_py.ur.ur import UR
from apps.ur_registry.registry_types import UUID
from apps.ur_registry.crypto_key_path import CryptoKeyPath


REQUEST_ID = 1
SIGN_DATA = 2
DATA_TYPE = 3
CHAIN_ID = 4
DERIVATION_PATH = 5
ADDRESS = 6
ORIGIN = 7

RequestType_Transaction = 1
RequestType_TypedData = 2
RequestType_PersonalMessage = 3
RequestType_TypedTransaction = 4


class EthSignRequest:
    def __init__(self, request_id=None, 
                sign_data=None, 
                data_type=None, 
                chain_id=None,
                derivation_path=None,
                address=None, origin=None):
        self.coin_type = None
        self.network = None
        self.request_id = request_id
        self.sign_data = sign_data
        self.data_type = data_type
        self.chain_id = chain_id
        self.derivation_path = derivation_path
        self.address = address
        self.origin = origin

    @staticmethod
    def get_registry_type():
        return "eth-sign-request"

    @staticmethod
    def get_tag():
        return 401

    @staticmethod
    def new(request_id, sign_data, data_type, chain_id, derivation_path, address, origin):
        return EthSignRequest(request_id, sign_data, data_type, chain_id, derivation_path, address, origin)

    def get_request_id(self):
        return self.request_id

    def get_sign_data(self):
        return self.sign_data

    def get_data_type(self):
        return self.data_type

    def get_chain_id(self):
        return self.chain_id

    def get_derivation_path(self):
        return self.derivation_path

    def get_address(self):
        return self.address

    def get_origin(self):
        return self.origin

    def set_request_id(self, request_id):
        self.request_id = request_id

    def set_sign_data(self, sign_data):
        self.sign_data = sign_data

    def set_data_type(self, data_type):
        self.data_type = data_type

    def set_chain_id(self, chain_id):
        self.chain_id = chain_id

    def set_derivation_path(self, derivation_path):
        self.derivation_path = derivation_path

    def set_address(self, address):
        self.address = address

    def set_origin(self, origin):
        self.origin = origin

    def get_map_size(self):
        size = 3 + sum(
            (
                self.request_id is not None,
                self.chain_id is not None,
                self.address is not None,
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

        if self.sign_data is not None:
            encoder.encodeInteger(SIGN_DATA)
            encoder.encodeBytes(self.sign_data)

        encoder.encodeInteger(DATA_TYPE)
        encoder.encodeInteger(self.data_type)

        if self.chain_id is not None:
            encoder.encodeInteger(CHAIN_ID)
            encoder.encodeInteger(self.chain_id)

        encoder.encodeInteger(DERIVATION_PATH)
        encoder.encodeTag(CryptoKeyPath.get_tag())
        cbor = self.derivation_path.cbor_encode()
        encoder.cborExtend(cbor)

        if self.address is not None:
            encoder.encodeInteger(ADDRESS)
            encoder.encodeBytes(self.address)

        if self.origin is not None:
            encoder.encodeInteger(ORIGIN)
            encoder.encodeText(self.origin)
    
        return encoder.get_bytes()

    def ur_encode(self):
        data = self.cbor_encode()
        return UR(EthSignRequest.get_registry_type(), data)

    @staticmethod
    def from_cbor(cbor):
        decoder = CBORDecoder(cbor)
        return EthSignRequest.decode(decoder)

    @staticmethod
    def decode(decoder):
        eth_sign_req = EthSignRequest()
        size, _ = decoder.decodeMapSize()
        for _ in range(size):
            key, _ = decoder.decodeInteger()
            if key == REQUEST_ID:
                tag, _ = decoder.decodeTag()
                if tag != UUID.get_tag():
                    raise Exception("Expected Tag {}".format(tag))
                eth_sign_req.request_id, _ = decoder.decodeBytes()
            if key == SIGN_DATA:
                eth_sign_req.sign_data, _ = decoder.decodeBytes()
            if key == DATA_TYPE:
                eth_sign_req.data_type, _ = decoder.decodeInteger()
            if key == CHAIN_ID:
                eth_sign_req.chain_id, _ = decoder.decodeInteger()
            if key == DERIVATION_PATH:
                tag, _ = decoder.decodeTag()
                if tag != CryptoKeyPath.get_tag():
                    raise Exception("Expected Tag {}".format(tag))
                eth_sign_req.derivation_path = CryptoKeyPath.decode(decoder)
            if key == ADDRESS:
                eth_sign_req.address, _ = decoder.decodeBytes()
            if key == ORIGIN:
                eth_sign_req.origin, _ = decoder.decodeText()
        return eth_sign_req
