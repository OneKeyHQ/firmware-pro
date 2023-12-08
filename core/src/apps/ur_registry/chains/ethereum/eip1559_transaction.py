from ubinascii import hexlify

from trezor.messages import EthereumSignTxEIP1559
from trezor.wire import DUMMY_CONTEXT

from apps.ur_registry.rlp import decode

from . import get_derivation_path
from .eth_sign_request import EthSignRequest

TRANSACTION_TYPE = 2


class FeeMarketEIP1559Transaction:
    def __init__(self, req: EthSignRequest):
        self.req = req
        self.resp = None
        self.qr = None

    async def initial_tx(self):
        pass

    # Format: `0x02 || rlp([chainId, nonce, maxPriorityFeePerGas, maxFeePerGas, gasLimit, to, value, data,
    # accessList, signatureYParity, signatureR, signatureS])`
    @staticmethod
    def fromSerializedTx(serialized):
        if serialized[0] != TRANSACTION_TYPE:
            raise Exception("Invalid serialized tx input: not an EIP-1559 transaction")

        tx = decode(serialized[1:])
        if tx is None:
            raise Exception("Decode error")
        if len(tx) != 9:
            raise Exception("Invalid EIP-1559 transaction. Only expecting 9 values")

        chainId = tx[0] if type(tx[0]) is bytes else b""
        nonce = tx[1] if type(tx[1]) is bytes else b""
        maxPriorityFeePerGas = tx[2]
        maxFeePerGas = tx[3]
        gasLimit = tx[4]
        to = tx[5]
        value = tx[6]
        data_initial_chunk = tx[7]
        accessList = tx[8]

        # pyright: off
        return EthereumSignTxEIP1559(
            nonce=nonce,
            max_gas_fee=maxFeePerGas,
            max_priority_fee=maxPriorityFeePerGas,
            gas_limit=gasLimit,
            value=value,
            data_length=len(data_initial_chunk),
            data_initial_chunk=data_initial_chunk,
            chain_id=int.from_bytes(chainId, "big"),
            address_n=get_derivation_path(),
            to=hexlify(to).decode(),
            access_list=accessList,
        )
        # pyright: on

    @staticmethod
    def get_tx(req: EthSignRequest):
        return FeeMarketEIP1559Transaction.fromSerializedTx(req.get_sign_data())

    async def run(self):
        from apps.ethereum.sign_tx_eip1559 import sign_tx_eip1559
        from apps.ur_registry.chains.ethereum.eth_signature import EthSignature
        from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder

        # pyright: off
        req = self.get_tx(self.req)
        self.resp = await sign_tx_eip1559(DUMMY_CONTEXT, req)
        self.signature = (
            self.resp.signature_r
            + self.resp.signature_s
            + self.resp.signature_v.to_bytes(1, "big")
        )
        eth_signature = EthSignature(
            request_id=self.req.get_request_id(),
            signature=self.signature,
            origin="OneKey".encode(),
        )
        ur = eth_signature.ur_encode()
        encoded = UREncoder.encode(ur).upper()
        self.qr = encoded
        # pyright: on
