from trezor.messages import SolanaSignTx

from .sol_sign_request import SolSignRequest


class SolTransaction:
    def __init__(self, req: SolSignRequest):
        self.req = req
        self.qr = None
        self.encoder = None

    def gen_request(self):
        return SolanaSignTx(
            raw_tx=self.req.get_sign_data(),
            address_n=self.req.get_address_n(),
        )

    async def run(self):
        from trezor import wire
        from apps.solana.sign_tx import sign_tx
        from apps.ur_registry.chains.solana.sol_signature import SolSignature
        from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder

        # pyright: off
        req = self.gen_request()
        resp = await sign_tx(wire.QR_CONTEXT, req)
        sol_signature = SolSignature(
            request_id=self.req.get_request_id(),
            signature=resp.signature,
        )
        # pyright: on
        ur = sol_signature.ur_encode()
        encoded = UREncoder.encode(ur).upper()
        self.qr = encoded
