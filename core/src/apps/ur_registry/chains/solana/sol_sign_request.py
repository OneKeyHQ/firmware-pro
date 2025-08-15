from apps.ur_registry.chains.base_sign_request import BaseSignRequest
from apps.ur_registry.registry_types import SOL_SIGN_REQUEST

RequestType_Transaction = 1
RequestType_UnsafeMessage = 2
RequestType_OffChainMessage_Legacy = 3
RequestType_OffChainMessage_Standard = 4


class SolSignRequest(BaseSignRequest):
    @staticmethod
    def get_registry_type():
        return SOL_SIGN_REQUEST.get_registry_type()

    @staticmethod
    def get_tag():
        return SOL_SIGN_REQUEST.get_tag()

    @staticmethod
    async def gen_request(ur):
        req = SolSignRequest.from_cbor(ur.cbor)
        await req.common_check()
        if req.get_request_type() == RequestType_Transaction:
            from .sol_transaction import (
                SolTransaction,
            )

            return SolTransaction(req)
        elif req.get_request_type() == RequestType_UnsafeMessage:
            from .sol_unsafe_message import (
                SolUnsafeMessage,
            )

            return SolUnsafeMessage(req)
        elif req.get_request_type() in [
            RequestType_OffChainMessage_Legacy,
            RequestType_OffChainMessage_Standard,
        ]:
            from .sol_offchain_message import (
                SolOffChainMessage,
            )

            return SolOffChainMessage(
                req, req.get_request_type() == RequestType_OffChainMessage_Standard
            )
        else:
            raise Exception(f"Unexpected Request Type {req.get_request_type()}")
