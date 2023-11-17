from apps.ur_registry.rlp import decode
from trezor.messages import EthereumSignTx
from .eth_sign_request import EthSignRequest
from ubinascii import hexlify


class EthereumSignTxTransacion:
    def __init__(self):
        pass

    # Format: rlp([nonce, gasPrice, gasLimit, to, value, data, v, r, s])
    @staticmethod
    def fromSerializedTx(serialized, chainId):
        tx = decode(serialized)
        if len(tx) != 9:
            raise Exception("Invalid transaction. Only expecting 9 values")

        nonce = tx[0]
        gasPrice = tx[1]
        gasLimit = tx[2]
        to = tx[3]
        gasLimit = tx[4]
        to = tx[5]
        value = tx[6]
        data = tx[7]

        return EthereumSignTx(
            address_n = [2147483692, 2147483708, 2147483648, 0, 0], # todo
            nonce = nonce,
            gas_price = gasPrice,
            gas_limit = gasLimit,
            to = hexlify(to).decode(),
            value = value,
            data_length = len(data),
            data = data,
            chain_id = int.from_bytes(chainId, "big"),
        )

    @staticmethod
    def get_tx(req: EthSignRequest):
        return EthereumSignTxTransacion.fromSerializedTx(req.get_sign_data(), req.get_chain_id())
