from apps.ur_registry.chains.base_sign_request import BaseSignRequest
from apps.ur_registry.registry_types import TRON_SIGN_REQUEST

RequestType_Transaction = 1
RequestType_MessageV1 = 2
RequestType_MessageV2 = 3


class TronSignRequest(BaseSignRequest):
    @staticmethod
    def get_registry_type():
        return TRON_SIGN_REQUEST.get_registry_type()

    @staticmethod
    def get_tag():
        return TRON_SIGN_REQUEST.get_tag()

    @staticmethod
    async def gen_request(ur):
        req = TronSignRequest.from_cbor(ur.cbor)
        await req.common_check()
        if req.get_request_type() == RequestType_Transaction:
            from .tron_transaction import (
                TronTransaction,
            )

            return TronTransaction(req)
        elif req.get_request_type() in [RequestType_MessageV1, RequestType_MessageV2]:
            from .tron_message import (
                TronMessage,
            )
            from trezor.enums import TronMessageType

            if req.get_request_type() == RequestType_MessageV1:
                version = TronMessageType.V1
            else:
                version = TronMessageType.V2
            return TronMessage(req, version)
        else:
            raise Exception(f"Unexpected Request Type {req.get_request_type()}")
