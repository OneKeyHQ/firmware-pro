from apps.ur_registry.rlp import decode
from trezor.messages import EthereumSignTxEIP1559
from .eth_sign_request import EthSignRequest

from ubinascii import hexlify

TRANSACTION_TYPE = 2

class FeeMarketEIP1559Transaction:
    def __init__(self):
        pass

    # Format: `0x02 || rlp([chainId, nonce, maxPriorityFeePerGas, maxFeePerGas, gasLimit, to, value, data,
    # accessList, signatureYParity, signatureR, signatureS])`
    def fromSerializedTx(serialized):
        if serialized[0] != TRANSACTION_TYPE:
            raise Exception("Invalid serialized tx input: not an EIP-1559 transaction")

        tx = decode(serialized[1:])
        if len(tx) != 9:
            raise Exception("Invalid EIP-1559 transaction. Only expecting 9 values")

        chainId = tx[0]
        nonce = tx[1]
        maxPriorityFeePerGas = tx[2]
        maxFeePerGas = tx[3]
        gasLimit = tx[4]
        to = tx[5]
        value = tx[6]
        data = tx[7]
        accessList = tx[8]

        return EthereumSignTxEIP1559(
            nonce = nonce,
            max_gas_fee = maxFeePerGas,
            max_priority_fee = maxPriorityFeePerGas,
            gas_limit = gasLimit,
            value = value,
            data_length = len(data),
            data = data,
            chain_id = int.from_bytes(chainId, "big"),
            address_n = [2147483692, 2147483708, 2147483648, 0, 0], # todo
            to = hexlify(to).decode(),
            access_list = accessList,
        )

    @staticmethod
    def get_tx(req: EthSignRequest):
        return FeeMarketEIP1559Transaction.fromSerializedTx(req.get_sign_data())
