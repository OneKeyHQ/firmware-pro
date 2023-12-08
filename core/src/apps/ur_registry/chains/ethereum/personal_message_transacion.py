from trezor.messages import EthereumSignMessage

from . import get_derivation_path
from .eth_sign_request import EthSignRequest


class EthereumPersonalMessageTransacion:
    def __init__(self, req: EthSignRequest):
        self.req = req
        self.resp = None
        self.qr = None

    async def initial_tx(self):
        pass

    def get_tx(self, msg):
        return EthereumSignMessage(
            address_n=get_derivation_path(),
            message=msg,
        )

    async def run(self):
        from trezor import wire
        from apps.ethereum.sign_message import sign_message
        from apps.ur_registry.chains.ethereum.eth_signature import EthSignature
        from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder

        # pyright: off
        tx = self.get_tx(self.req.get_sign_data())
        self.resp = await sign_message(wire.DUMMY_CONTEXT, tx)
        self.signature = self.resp.signature
        eth_signature = EthSignature(
            request_id=self.req.get_request_id(),
            signature=self.signature,
            origin="OneKey".encode(),
        )
        ur = eth_signature.ur_encode()
        encoded = UREncoder.encode(ur).upper()
        self.qr = encoded
        # pyright: on
