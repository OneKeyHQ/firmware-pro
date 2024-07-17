from ...ur_py.ur import cbor_lite
from ...ur_py.ur.ur import UR


class CryptoPSBT:
    def __init__(self, psbt: bytes):
        self.psbt = psbt

    def get_psbt(self) -> bytes:
        return self.psbt

    def set_psbt(self, psbt: bytes):
        self.psbt = psbt

    @staticmethod
    def get_registry_type() -> str:
        return "crypto-psbt"

    @staticmethod
    def get_tag() -> int:
        return 310  # 40310

    def cbor_encode(self) -> bytes:
        encoder = cbor_lite.CBOREncoder()
        encoder.encodeBytes(self.psbt)
        return encoder.get_bytes()

    def to_ur(self) -> "UR":
        data = self.cbor_encode()
        return UR(CryptoPSBT.get_registry_type(), data)

    @staticmethod
    def cbor_decode(decoder: cbor_lite.CBORDecoder) -> "CryptoPSBT":
        psbt, _ = decoder.decodeBytes()
        return CryptoPSBT(psbt)

    @staticmethod
    def from_cbor(cbor: bytes) -> "CryptoPSBT":
        decoder = cbor_lite.CBORDecoder(cbor)
        return CryptoPSBT.cbor_decode(decoder)
