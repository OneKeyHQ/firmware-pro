import ustruct

from trezor.crypto import base32
from trezor.wire import ProcessError

from . import consts


class InvokeHostFunctionOpSummary:
    def __init__(
        self,
        contract_address: str,
        function_name: str,
        call_args_hash: bytes,
        soroban_auth_hash: bytes,
        soroban_tx_ext_hash: bytes,
    ) -> None:
        self.contract_address = contract_address
        self.function_name = function_name
        self.call_args_hash = call_args_hash
        self.soroban_auth_hash = soroban_auth_hash
        self.soroban_tx_ext_hash = soroban_tx_ext_hash


def public_key_from_address(address: str) -> bytes:
    return _raw_payload_from_address(
        address, version=consts.STELLAR_STRKEY_VERSION_ED25519_PUBLIC_KEY
    )


def contract_id_from_address(c_address: str) -> bytes:
    return _raw_payload_from_address(
        c_address, version=consts.STELLAR_STRKEY_VERSION_CONTRACT
    )


def address_from_public_key(pubkey: bytes) -> str:
    """Returns the base32-encoded version of public key bytes (G...)"""
    address = bytearray()
    address.append(6 << 3)  # version -> 'G'
    address.extend(pubkey)
    address.extend(_crc16_checksum(bytes(address)))  # checksum

    return base32.encode(address)


def _crc16_checksum_verify(data: bytes, checksum: bytes) -> None:
    if _crc16_checksum(data) != checksum:
        raise ProcessError("Invalid address checksum")


def _crc16_checksum(data: bytes) -> bytes:
    """Returns the CRC-16 checksum of bytearray bytes

    Ported from Java implementation at: http://introcs.cs.princeton.edu/java/61data/CRC16CCITT.java.html

    Initial value changed to 0x0000 to match Stellar configuration.
    """
    crc = 0x0000
    polynomial = 0x1021

    for byte in data:
        for i in range(8):
            bit = (byte >> (7 - i) & 1) == 1
            c15 = (crc >> 15 & 1) == 1
            crc <<= 1
            if c15 ^ bit:
                crc ^= polynomial

    return ustruct.pack("<H", crc & 0xFFFF)


def _raw_payload_from_address(address: str, version: int) -> bytes:
    """Extracts raw payload from an address
    Stellar address is in format:
    <1-byte version> <32-bytes raw payload> <2-bytes CRC-16 checksum>
    """
    b = base32.decode(address)
    if b[0] != version:
        raise ProcessError("Invalid address version")
    _crc16_checksum_verify(b[:-2], b[-2:])
    return b[1:-2]
