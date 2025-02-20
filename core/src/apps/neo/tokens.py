from typing import Iterator

NEO_SCRIPT_HASH = b"\xf5\x63\xea\x40\xbc\x28\x3d\x4d\x0e\x05\xc4\x8e\xa3\x05\xb3\xf2\xa0\x73\x40\xef"  # the real script hash is reversed


class NeoTokenInfo:
    def __init__(self, symbol: str, decimals: int, contract_script_hash: bytes):
        self.symbol = symbol
        self.decimals = decimals
        self.contract_script_hash = contract_script_hash

    # def is_gas(self) -> bool:
    #     return self.symbol == "GAS"


UNKNOWN_TOKEN = NeoTokenInfo("UNK", 0, b"")


def token_by_contract_script_hash(contract_script_hash: bytes) -> NeoTokenInfo:
    for symbol, decimals, script_hash in _token_iterator():
        if contract_script_hash == script_hash:
            return NeoTokenInfo(symbol, decimals, contract_script_hash)
    return UNKNOWN_TOKEN


def _token_iterator() -> Iterator[tuple[str, int, bytes]]:
    yield (
        "NEO",
        0,
        NEO_SCRIPT_HASH,
    )
    yield (
        "GAS",
        8,
        (
            b"\xcf\x76\xe2\x8b\xd0\x06\x2c\x4a\x47\x8e\xe3\x55\x61\x01\x13\x19\xf3\xcf\xa4\xd2"
        ),
    )
    yield (
        "FLM",
        8,
        (
            b"\x28\xab\x18\x74\xda\x47\xaa\xd8\x2c\x9c\xb3\x51\x88\x55\x27\x81\x52\x1f\x15\xf0"
        ),
    )
