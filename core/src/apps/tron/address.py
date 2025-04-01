from trezor.crypto import base58
from trezor.crypto.hashlib import sha3_256


def get_address_from_public_key(pubkey: bytes) -> str:
    address = b"\x41" + sha3_256(pubkey[1:65], keccak=True).digest()[12:32]
    return address_base58(address)


def address_base58(address: bytes) -> str:
    return base58.encode_check(address)


def address_to_bytes(address: str) -> bytes:
    return base58.decode_check(address)
