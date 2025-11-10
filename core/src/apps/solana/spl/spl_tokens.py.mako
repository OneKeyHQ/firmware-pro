# generated from tokens.py.mako
# do not edit manually!
# flake8: noqa
# fmt: off

from typing import Iterator


class SPLToken:
    def __init__(self, symbol: str, mint: str, decimals: int):
        self.symbol = symbol
        self.mint = mint
        self.decimals = decimals


def get_spl_token(mint: str) -> SPLToken | None:
    for symbol, _mint, decimals in _spl_tokens_iterator():
        if mint == _mint:
            return SPLToken(symbol, _mint, decimals)
    return None


def _spl_tokens_iterator() -> Iterator[tuple[str, str, int]]:
    # symbol, mint, decimals  # name
% for t in sorted(spl, key=lambda t: t.symbol):
    yield (${black_repr(t.symbol)}, ${black_repr(t.address)}, ${t.decimals}) # ${black_repr(t.name)}
% endfor
