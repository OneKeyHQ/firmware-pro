from micropython import const
from ubinascii import hexlify, unhexlify

from trezor import wire
from trezor.crypto import rlp
from trezor.utils import HashWriter
from trezor.crypto.hashlib import sha3_256

DECIMALS = const(8)

def bytes_from_address(address: str) -> bytes:
    if len(address) == 42:
        if address[0:2] != "1S":
            raise wire.ProcessError("Scdo: invalid beginning of an address")
        return unhexlify(address[2:])

    elif len(address) == 0:
        return bytes()

    raise wire.ProcessError("Ethereum: Invalid address length")

def address_from_public_key(pubkey: bytes, shard = 1) -> str:
    
    sha = HashWriter(sha3_256(keccak=True))
    rlp.write(sha, pubkey)

    digest = sha.get_digest()

    address = bytearray(digest)[-20:]
    address[0] = shard
    address[19] = address[19] & 0xF0 | 1
    address_hex = hexlify(address).decode()
    return str(shard) + "S" + address_hex