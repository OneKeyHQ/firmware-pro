from apps.ur_registry.chains import MismatchError
from apps.ur_registry.crypto_key_path import CryptoKeyPath
from apps.ur_registry.registry_types import SOL_SIGN_REQUEST, UUID
from apps.ur_registry.ur_py.ur.cbor_lite import CBORDecoder, CBOREncoder
from apps.ur_registry.ur_py.ur.ur import UR

REQUEST_ID = 1
SIGN_DATA = 2
DERIVATION_PATH = 3
ADDRESS = 4
ORIGIN = 5
REQUEST_TYPE = 6


RequestType_Transaction = 1
RequestType_UnsafeMessage = 2
RequestType_OffChainMessage_Legacy = 3
RequestType_OffChainMessage_Standard = 4


class SolSignRequest:
    def __init__(
        self,
        request_id=None,
        sign_data=None,
        derivation_path=None,
        address=None,
        origin=None,
        request_type=None,
    ):
        self.request_id = request_id
        self.sign_data = sign_data
        self.derivation_path = derivation_path
        self.address = address
        self.origin = origin
        self.request_type = request_type

    @staticmethod
    def get_registry_type():
        return SOL_SIGN_REQUEST.get_registry_type()

    @staticmethod
    def get_tag():
        return SOL_SIGN_REQUEST.get_tag()

    @staticmethod
    def new(request_id, sign_data, derivation_path, address, origin, request_type):
        return SolSignRequest(
            request_id, sign_data, derivation_path, address, origin, request_type
        )

    def get_request_id(self):
        return self.request_id

    def get_sign_data(self):
        return self.sign_data

    def get_request_type(self):
        return self.request_type

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

    def set_derivation_path(self, derivation_path):
        self.derivation_path = derivation_path

    def set_address(self, address):
        self.address = address

    def set_origin(self, origin):
        self.origin = origin

    def set_request_type(self, request_type):
        self.request_type = request_type

    def get_map_size(self):
        size = 3 + sum(
            (
                self.request_id is not None,
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

        if self.derivation_path is not None:
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

        if self.request_type is not None:
            encoder.encodeInteger(REQUEST_TYPE)
            encoder.encodeInteger(self.request_type)

        return encoder.get_bytes()

    def ur_encode(self):
        data = self.cbor_encode()
        return UR(SolSignRequest.get_registry_type(), data)

    @staticmethod
    def from_cbor(cbor):
        decoder = CBORDecoder(cbor)
        return SolSignRequest.decode(decoder)

    @staticmethod
    def decode(decoder):
        sol_sign_req = SolSignRequest()
        size, _ = decoder.decodeMapSize()
        for _ in range(size):
            key, _ = decoder.decodeInteger()
            if key == REQUEST_ID:
                tag, _ = decoder.decodeTag()
                if tag != UUID.get_tag():
                    raise Exception(f"Expected Tag {tag}")
                sol_sign_req.request_id, _ = decoder.decodeBytes()
            if key == SIGN_DATA:
                sol_sign_req.sign_data, _ = decoder.decodeBytes()
            if key == DERIVATION_PATH:
                tag, _ = decoder.decodeTag()
                if tag != CryptoKeyPath.get_tag():
                    raise Exception(f"Expected Tag {tag}")
                sol_sign_req.derivation_path = CryptoKeyPath.decode(decoder)
            if key == ADDRESS:
                sol_sign_req.address, _ = decoder.decodeBytes()
            if key == ORIGIN:
                sol_sign_req.origin, _ = decoder.decodeText()
            if key == REQUEST_TYPE:
                sol_sign_req.request_type, _ = decoder.decodeInteger()
        return sol_sign_req

    def get_address_n(self) -> list[int]:
        path = self.derivation_path.get_path() if self.derivation_path else ""
        if "*" in path:
            raise Exception("Invalid derivation path")
        from apps.common import paths

        return paths.parse_path(path)

    async def common_check(self):
        key_path: CryptoKeyPath | None = self.derivation_path
        assert key_path is not None
        if not key_path.source_fingerprint:
            raise Exception("Missing source_fingerprint")

        if not self.get_address_n():
            raise Exception("Invalid derivation path")

        from trezor.messages import GetPublicKey, Initialize
        from apps.bitcoin import get_public_key as bitcoin_get_public_key
        from trezor.wire import QR_CONTEXT
        from apps.base import handle_Initialize
        from apps.common import passphrase

        if passphrase.is_enabled():
            QR_CONTEXT.passphrase = None
        # pyright: off
        await handle_Initialize(QR_CONTEXT, Initialize())
        btc_pubkey_msg = GetPublicKey(address_n=[2147483692, 2147483708, 2147483648])
        resp = await bitcoin_get_public_key.get_public_key(QR_CONTEXT, btc_pubkey_msg)
        # pyright: on
        expected_fingerprint = key_path.source_fingerprint
        if resp.root_fingerprint != expected_fingerprint:
            raise MismatchError(
                f"Fingerprint mismatch: got {resp.root_fingerprint} expected {expected_fingerprint}"
            )

    @staticmethod
    async def gen_request(ur):
        req = SolSignRequest.from_cbor(ur.cbor)
        await req.common_check()
        if req.get_request_type() == RequestType_Transaction:
            from .sol_transaction import (
                SolTransaction,
            )

            return SolTransaction(req)
        elif req.get_request_type() == RequestType_UnsafeMessage:
            from .sol_unsafe_message import (
                SolUnsafeMessage,
            )

            return SolUnsafeMessage(req)
        elif req.get_request_type() in [
            RequestType_OffChainMessage_Legacy,
            RequestType_OffChainMessage_Standard,
        ]:
            from .sol_offchain_message import (
                SolOffChainMessage,
            )

            return SolOffChainMessage(
                req, req.get_request_type() == RequestType_OffChainMessage_Standard
            )
        else:
            raise Exception(f"Unexpected Request Type {req.get_request_type()}")
