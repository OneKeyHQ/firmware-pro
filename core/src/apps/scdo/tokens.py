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
        if address == "1S01dc515d287d1dbdc98abe9c397e73c4680f0022":
            return TokenInfo("TEST", 8)
        elif address == "1S01f0daaf7a59fb5eb90256112bf5d080ff290022":
            return TokenInfo("TEST0", 8)
        elif address == "1S01f4fb4ae0d3c043ac0cdc93a3c54b9c62600022":
            return TokenInfo("TEST1", 8)
        elif address == "1S019829e1a6658054c03113678c52ca1510330002":
            return TokenInfo("TEST2", 8)
        elif address == "1S015acd40eb8e0dc87018926aed6bdae91c7d0012":
            return TokenInfo("TEST3", 8)
        elif address == "1S016f5d94e7050ba8281cf1b67306a1a7d7070002":
            return TokenInfo("TEST4", 8)
        elif address == "1S01f61937dfa9a1fb568454c43ce65cf164c60012":
            return TokenInfo("TEST5", 8)
        elif address == "1S01aaab0a1d03eb075e63ee02b8d4a126e20e0022":
            return TokenInfo("TEST6", 8)
        elif address == "1S014fe934f2383aa9d3bf1d57f35fd6735b600022":
            return TokenInfo("TEST7", 8)
        elif address == "1S01e21b02c41f23638fbffccc81ffccd2a5d70012":
            return TokenInfo("TEST8", 8)
        elif address == "1S01b85c9f5e8e8d2762586d5673e55d033f350012":
            return TokenInfo("TEST9", 8)
        elif address == "1S0140a5ba0d07a99492034beff9707d7c73040012":
            return TokenInfo("USDO TEST", 8)
        elif address == "1S017b992068ae58386922056a01c792cb4e0a0032":
            return TokenInfo("WIN", 8)
    return UNKNOWN_TOKEN
