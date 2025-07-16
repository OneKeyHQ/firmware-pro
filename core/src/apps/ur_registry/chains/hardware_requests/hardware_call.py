from apps.ur_registry.chains import MismatchError
from apps.ur_registry.registry_types import HARDWARE_CALL
from apps.ur_registry.ur_py.ur import cbor_lite


class HardwareCall:
    def __init__(self, xfp: str, method: str, requestId: str, params: list[dict]):
        self.xfp = xfp
        self.method = method
        self.requestId = requestId
        self.params = params

    @staticmethod
    def get_registry_type():
        return HARDWARE_CALL.get_registry_type()

    @staticmethod
    def get_tag() -> int | None:
        return HARDWARE_CALL.get_tag()

    def get_xfp(self):
        return self.xfp

    def get_method(self):
        return self.method

    def get_requestId(self):
        return self.requestId

    def get_params(self):
        return self.params

    @staticmethod
    def cbor_encode(encoder: cbor_lite.CBOREncoder, obj: "HardwareCall"):
        import ujson

        encoder.encodeMapSize(1)
        encoder.encodeEncodedBytes(ujson.encode(obj).encode())

    @staticmethod
    def _decode(cbor: bytes) -> "HardwareCall":
        import ujson

        payloads = ujson.loads(cbor.decode())

        if any(k not in payloads for k in ("requestId", "xfp", "method", "params")):
            raise ValueError("Invalid call params")
        xfp = payloads["xfp"]
        requestId = payloads["requestId"]
        method = payloads["method"]
        params = payloads["params"]
        return HardwareCall(xfp, method, requestId, params)

    @staticmethod
    def from_cbor(cbor):
        return HardwareCall._decode(cbor)

    async def common_check(self):

        from trezor.messages import GetPublicKey, Initialize
        from apps.bitcoin import get_public_key as bitcoin_get_public_key
        from trezor.wire import QR_CONTEXT
        from apps.base import handle_Initialize
        from apps.common import passphrase
        from binascii import hexlify

        if passphrase.is_enabled():
            QR_CONTEXT.passphrase = None
        # pyright: off
        await handle_Initialize(QR_CONTEXT, Initialize())
        btc_pubkey_msg = GetPublicKey(address_n=[2147483692, 2147483708, 2147483648])
        resp = await bitcoin_get_public_key.get_public_key(QR_CONTEXT, btc_pubkey_msg)
        # pyright: on
        expected_fingerprint = self.get_xfp()
        assert resp.root_fingerprint is not None, "Root fingerprint should not be None"
        xfp = hexlify(int.to_bytes(resp.root_fingerprint, 4, "big")).decode()
        if xfp != expected_fingerprint:
            raise MismatchError(
                f"Fingerprint mismatch: got {xfp} expected {expected_fingerprint}"
            )

    @staticmethod
    async def gen_request(ur):
        call = HardwareCall.from_cbor(ur.cbor)
        await call.common_check()
        method = call.get_method()
        if method == "getMultiAccounts":
            from .get_multi_accounts import GetMultiAccountsRequest

            return GetMultiAccountsRequest(call)
        elif method == "verifyAddress":
            from .verify_address import VerifyAddressRequest

            return VerifyAddressRequest(call)
        else:
            raise ValueError("Invalid method")
