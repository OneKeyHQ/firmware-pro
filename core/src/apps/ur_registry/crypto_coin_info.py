from .ur_py.ur.cbor_lite import CBORDecoder, CBOREncoder
from .ur_py.ur.ur import UR

COIN_TYPE = 1
NETWORK = 2

# CoinType
Bitcoin = 0
Ethereum = 60

# Network
MainNet = 0
TestNet = 1


class CryptoCoinInfo:
    def __init__(self, coin_type=None, network=None):
        self.coin_type = coin_type
        self.network = network

    @staticmethod
    def get_registry_type():
        return "crypto-coin-info"

    @staticmethod
    def get_tag():
        return 305

    def get_coin_type(self):
        return self.coin_type if self.coin_type is not None else Bitcoin

    def get_network(self):
        return self.network if self.network is not None else MainNet

    def set_coin_type(self, coin_type):
        if coin_type == 0:
            self.coin_type = Bitcoin
        elif coin_type == 60:
            self.coin_type = Ethereum
        else:
            self.coin_type = Bitcoin

    def set_network(self, id):
        if id == 0:
            self.network = MainNet
        elif id == 1:
            self.network = TestNet
        else:
            self.network = TestNet

    def cbor_encode(self):
        encoder = CBOREncoder()
        size = 0
        if self.coin_type is not None:
            size += 1
        if self.network is not None:
            size += 1
        encoder.encodeMapSize(size)
        if self.coin_type is not None:
            encoder.encodeInteger(COIN_TYPE)
            encoder.encodeInteger(self.coin_type)
        if self.network is not None:
            encoder.encodeInteger(NETWORK)
            encoder.encodeInteger(self.network)

        return encoder.get_bytes()

    def ur_encode(self):
        data = self.cbor_encode()
        return UR(CryptoCoinInfo.get_registry_type(), data)

    @staticmethod
    def from_cbor(cbor):
        decoder = CBORDecoder(cbor)
        return CryptoCoinInfo.decode(decoder)

    @staticmethod
    def decode(decoder):
        coin_info = CryptoCoinInfo()
        size, _ = decoder.decodeMapSize()
        for _ in range(size):
            value, _ = decoder.decodeInteger()
            if value == COIN_TYPE:
                value, _ = decoder.decodeInteger()
                coin_info.coin_type = value
            if value == NETWORK:
                value, _ = decoder.decodeInteger()
                coin_info.network = value
        return coin_info
