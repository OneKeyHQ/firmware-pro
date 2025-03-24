from trezor import utils

from apps.ur_registry.registry_types import (
    CRYPTO_PSBT,
    ETH_SIGN_REQUEST,
    HARDWARE_CALL,
    SOL_SIGN_REQUEST,
)


def get_request_class(registry_type: str):
    if registry_type == CRYPTO_PSBT.get_registry_type():
        from apps.ur_registry.chains.bitcoin.crypto_psbt import CryptoPSBT

        return CryptoPSBT
    elif registry_type == HARDWARE_CALL.get_registry_type():
        from apps.ur_registry.chains.hardware_requests.hardware_call import HardwareCall

        return HardwareCall
    if not utils.BITCOIN_ONLY:
        if registry_type == ETH_SIGN_REQUEST.get_registry_type():
            from apps.ur_registry.chains.ethereum.eth_sign_request import EthSignRequest

            return EthSignRequest

        elif registry_type == SOL_SIGN_REQUEST.get_registry_type():
            from apps.ur_registry.chains.solana.sol_sign_request import SolSignRequest

            return SolSignRequest

    return None
