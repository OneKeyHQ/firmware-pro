from ubinascii import hexlify

from trezor.messages import EthereumSignTx

from apps.ur_registry.rlp import decode

from .eth_sign_request import EthSignRequest


class EthereumSignTxTransacion:
    def __init__(self, req: EthSignRequest):
        self.req = req
        self.qr = None
        self.encoder = None

    # Format: rlp([nonce, gasPrice, gasLimit, to, value, data, v, r, s])
    @staticmethod
    def fromSerializedTx(serialized, chainId, address_n):
        tx = decode(serialized)
        if tx is None:
            raise Exception("Decode error")
        if len(tx) != 9:
            raise Exception("Invalid transaction. Only expecting 9 values")

        nonce = tx[0]
        gasPrice = tx[1]
        gasLimit = tx[2]
        to = tx[3]
        value = tx[4]
        data = tx[5]
        # pyright: off
        return EthereumSignTx(
            address_n=address_n,
            nonce=nonce,
            gas_price=gasPrice,
            gas_limit=gasLimit,
            to=hexlify(to).decode(),
            value=value,
            data_length=len(data),
            data_initial_chunk=data,
            chain_id=chainId,
        )
        # pyright: on

    @staticmethod
    def get_tx(req: EthSignRequest):
        return EthereumSignTxTransacion.fromSerializedTx(
            req.get_sign_data(), req.get_chain_id(), req.get_address_n()
        )

    async def run(self):
        from apps.ethereum.sign_tx import sign_tx
        from apps.ur_registry.chains.ethereum.eth_signature import EthSignature
        from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder
        from trezor import wire

        # pyright: off
        tx = self.get_tx(self.req)
        resp = await sign_tx(wire.QR_CONTEXT, tx)
        self.signature = (
            resp.signature_r + resp.signature_s + resp.signature_v.to_bytes(4, "big")
        )
        eth_signature = EthSignature(
            request_id=self.req.get_request_id(),
            signature=self.signature,
            origin="OneKey Pro",
        )
        ur = eth_signature.ur_encode()
        encoded = UREncoder.encode(ur).upper()
        self.qr = encoded
        # pyright: on
