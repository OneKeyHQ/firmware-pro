from trezor.enums import TronMessageType
from trezor.messages import TronSignMessage

from .tron_sign_request import TronSignRequest


class TronMessage:
    def __init__(self, req: TronSignRequest, version: TronMessageType):
        self.req = req
        self.qr = None
        self.encoder = None
        self.version = version

    def gen_request(self):
        return TronSignMessage(
            address_n=self.req.get_address_n(),
            message=self.req.get_sign_data(),
            message_type=self.version,
        )

    async def run(self):
        from trezor import wire
        from apps.tron.sign_message import sign_message
        from apps.ur_registry.chains.tron.tron_signature import TronSignature
        from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder

        # pyright: off
        req = self.gen_request()
        resp = await sign_message(wire.QR_CONTEXT, req)
        tron_signature = TronSignature(
            request_id=self.req.get_request_id(),
            signature=resp.signature,
        )
        # pyright: on
        ur = tron_signature.ur_encode()
        encoded = UREncoder.encode(ur).upper()
        self.qr = encoded
