from apps.ur_registry.chains.base_signature import BaseSignature
from apps.ur_registry.registry_types import SOL_SIGNATURE


class SolSignature(BaseSignature):
    @staticmethod
    def get_registry_type():
        return SOL_SIGNATURE.get_registry_type()

    @staticmethod
    def get_tag():
        return SOL_SIGNATURE.get_tag()
