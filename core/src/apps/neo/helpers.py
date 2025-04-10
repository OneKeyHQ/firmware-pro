from trezor.crypto import base58, scripts
from trezor.crypto.hashlib import sha256

ADDRESS_VERSION = b"\x35"

NETWORK_MAINNET = 860833102
NETWORK_TESTNET = 894710606


def build_check_sig_script_hash(pubkey: bytes) -> bytes:
    assert len(pubkey) == 33, "Compressed public key is expected"
    verification_script = b"\x0C\x21" + pubkey + b"\x41\x56\xe7\xb3\x27"
    return scripts.sha256_ripemd160(verification_script).digest()


def neo_address_from_pubkey(pubkey: bytes, version: bytes = ADDRESS_VERSION) -> str:
    script_hash = build_check_sig_script_hash(pubkey)
    return neo_address_from_script_hash(script_hash, version)


def neo_address_from_script_hash(
    script_hash: bytes, version: bytes = ADDRESS_VERSION
) -> str:
    assert len(script_hash) == 20, "Script hash is expected to be 20 bytes"
    return base58.encode_check(version + script_hash)


def is_mainnet(network: int) -> bool:
    return network == NETWORK_MAINNET


def is_testnet(network: int) -> bool:
    return network == NETWORK_TESTNET


def is_known_network(network: int) -> bool:
    return is_mainnet(network) or is_testnet(network)


def make_digest(raw_tx: bytes, network_magic: int = NETWORK_MAINNET) -> bytes:
    return sha256(
        network_magic.to_bytes(4, "little") + sha256(raw_tx).digest()
    ).digest()


def retrieve_network(network_magic: int) -> tuple[str, bool]:
    if is_mainnet(network_magic):
        return "Neo N3", True
    elif is_testnet(network_magic):
        return "Neo Testnet", True
    else:
        return "Neo UNK", False
