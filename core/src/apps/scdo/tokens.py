# generated from tokens.py.mako
# do not edit manually!
# flake8: noqa
# fmt: off

class TokenInfo:
    def __init__(self, symbol: str, decimals: int) -> None:
        self.symbol = symbol
        self.decimals = decimals


UNKNOWN_TOKEN = TokenInfo("UNKN", 0)

# ToDo: Add more tokens
def token_by_address(token_type, address) -> TokenInfo:
    if token_type == "SRC20":
        if address == "1S01438368e87b5609e6323eb5816795bc79530032":
            return TokenInfo("sTEST", 8)
    return UNKNOWN_TOKEN
