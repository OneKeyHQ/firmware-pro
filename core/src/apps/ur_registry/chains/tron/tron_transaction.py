from trezor.messages import TronSignTx

from .tron_sign_request import TronSignRequest


class TronTransaction:
    def __init__(self, req: TronSignRequest):
        self.req = req
        self.qr = None
        self.encoder = None

    def gen_request(self):
        from apps.tron.serialize import deserialize

        tx = deserialize(self.req.get_sign_data())
        return TronSignTx(
            ref_block_bytes=tx.ref_block_bytes,
            ref_block_hash=tx.ref_block_hash,
            expiration=tx.expiration,
            contract=tx.contract,
            timestamp=tx.timestamp,
            address_n=self.req.get_address_n(),
            data=tx.data,
            fee_limit=tx.fee_limit,
        )

    async def run(self):
        from trezor import wire
        from apps.tron.sign_tx import sign_tx
        from apps.ur_registry.chains.tron.tron_signature import TronSignature
        from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder

        # pyright: off
        req = self.gen_request()
        resp = await sign_tx(wire.QR_CONTEXT, req)
        tron_signature = TronSignature(
            request_id=self.req.get_request_id(),
            signature=resp.signature,
        )
        # pyright: on
        ur = tron_signature.ur_encode()
        encoded = UREncoder.encode(ur).upper()
        self.qr = encoded
