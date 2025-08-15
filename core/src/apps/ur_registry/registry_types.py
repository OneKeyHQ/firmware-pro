class RegistryType:
    def __init__(self, registry_type: str, tag: int):
        self.registry_type = registry_type
        self.tag = tag

    def get_registry_type(self) -> str:
        return self.registry_type

    def get_tag(self) -> int:
        return self.tag


UUID = RegistryType("uuid", 37)

SOL_SIGN_REQUEST = RegistryType("sol-sign-request", 1101)
SOL_SIGNATURE = RegistryType("sol-signature", 1102)

TRON_SIGN_REQUEST = RegistryType("tron-sign-request", 5201)
TRON_SIGNATURE = RegistryType("tron-signature", 5202)

ETH_SIGN_REQUEST = RegistryType("eth-sign-request", 401)
ETH_SIGNATURE = RegistryType("eth-signature", 402)

CRYPTO_PSBT = RegistryType("crypto-psbt", 310)

HARDWARE_CALL = RegistryType("onekey-app-call-device", 0)
