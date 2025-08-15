from apps.ur_registry.chains.base_signature import BaseSignature
from apps.ur_registry.registry_types import TRON_SIGNATURE


class TronSignature(BaseSignature):
    @staticmethod
    def get_registry_type():
        return TRON_SIGNATURE.get_registry_type()

    @staticmethod
    def get_tag():
        return TRON_SIGNATURE.get_tag()
