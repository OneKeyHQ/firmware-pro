from ubinascii import hexlify

from trezor import loop, messages, wire

from apps.ur_registry.rlp import decode

from .eth_sign_request import EthSignRequest


class EthereumSignTxTransacion:
    CALL_DATA = None

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
        data = bytes(tx[5])
        total_data_length = len(data)
        if total_data_length > 1024:
            EthereumSignTxTransacion.CALL_DATA = data[1024:]
            data = data[:1024]
        else:
            EthereumSignTxTransacion.CALL_DATA = None
        # pyright: off
        return messages.EthereumSignTxOneKey(
            address_n=address_n,
            nonce=nonce,
            gas_price=gasPrice,
            gas_limit=gasLimit,
            to=hexlify(to).decode(),
            value=value,
            data_length=total_data_length,
            data_initial_chunk=data,
            chain_id=chainId,
        )
        # pyright: on

    @staticmethod
    def gen_request(req: EthSignRequest):
        return EthereumSignTxTransacion.fromSerializedTx(
            req.get_sign_data(), req.get_chain_id(), req.get_address_n()
        )

    async def run(self):
        from apps.ethereum.onekey.sign_tx import sign_tx
        from apps.ur_registry.chains.ethereum.eth_signature import EthSignature
        from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder

        # pyright: off
        tx = self.gen_request(self.req)
        task = sign_tx(wire.QR_CONTEXT, tx)
        if EthereumSignTxTransacion.CALL_DATA:
            loop.spawn(self.interact())
            try:
                resp = await loop.spawn(task)
            except Exception as e:
                if __debug__:
                    print(f"Error: {e}")
                raise e
            finally:
                await wire.QR_CONTEXT.interact_stop()
        else:
            resp = await task
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

    async def interact(self):

        while True:
            response = await wire.QR_CONTEXT.qr_receive()
            if response is None:
                if __debug__:
                    print("eth sign type data interaction finished")
                break
            try:
                if messages.EthereumTxRequestOneKey.is_type_of(response):
                    assert (
                        EthereumSignTxTransacion.CALL_DATA is not None
                    ), "CALL_DATA is None"
                    request_data_length = response.data_length
                    response = messages.EthereumTxAckOneKey(
                        data_chunk=EthereumSignTxTransacion.CALL_DATA[
                            :request_data_length
                        ]
                    )
                    EthereumSignTxTransacion.CALL_DATA = (
                        EthereumSignTxTransacion.CALL_DATA[request_data_length:]
                    )
            except Exception as e:
                if __debug__:
                    print(f"Data error: {e}")
                response = messages.Failure(
                    code=messages.FailureType.DataError, message=f"Error: {e}"
                )
            finally:
                await wire.QR_CONTEXT.qr_send(response)
