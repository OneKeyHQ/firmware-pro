from trezor.messages import EthereumSignMessageOneKey

from .eth_sign_request import EthSignRequest


class EthereumPersonalMessageTransacion:
    def __init__(self, req: EthSignRequest):
        self.req = req
        self.qr = None
        self.encoder = None

    def gen_request(self):
        return EthereumSignMessageOneKey(
            address_n=self.req.get_address_n(),
            message=self.req.get_sign_data(),
        )

    async def run(self):
        from trezor import wire
        from apps.ethereum.onekey.sign_message import sign_message
        from apps.ur_registry.chains.ethereum.eth_signature import EthSignature
        from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder

        # pyright: off
        tx = self.gen_request()
        resp = await sign_message(wire.QR_CONTEXT, tx)
        self.signature = resp.signature
        eth_signature = EthSignature(
            request_id=self.req.get_request_id(),
            signature=self.signature,
            origin="OneKey Pro",
        )
        ur = eth_signature.ur_encode()
        encoded = UREncoder.encode(ur).upper()
        self.qr = encoded
        # pyright: on
