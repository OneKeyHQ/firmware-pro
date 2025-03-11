from apps.ur_registry.registry_types import CRYPTO_PSBT

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
        return CRYPTO_PSBT.get_registry_type()

    @staticmethod
    def get_tag() -> int:
        return CRYPTO_PSBT.get_tag()

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

    @staticmethod
    async def gen_request(ur):
        req = CryptoPSBT.from_cbor(ur.cbor)
        from apps.ur_registry.chains.bitcoin.transaction import SignPsbt

        return SignPsbt(req)
