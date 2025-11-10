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
    yield ("$WIF", "EKpQGSJtjMFqKZ9KQanSqYXRcF8fBopzLHYxdM65zcjm", 6) # "dogwifhat"
    yield ("2Z", "J6pQQ3FAcJQeWPPGppWRb4nM8jU3wLyYbRrLh7feMfvd", 8) # "DoubleZero"
    yield ("ATH", "Dm5BxyMetG3Aq5PaG1BrG7rBYqEMtnkjvPNMExfacVk7", 9) # "Aethir Token"
    yield ("AXYC", "4TaBYbgQyErDQx67rJRnbABd2tqHCzBvMN1a2M4vY8fc", 9) # "AxyCoin"
    yield ("BNSOL", "BNso1VUJnh4zcfpZa6986Ea66P6TCp59hvtNJ8b1X85", 9) # "Binance Staked SOL"
    yield ("BORG", "3dQTr7ror2QPKQ3GbBCokJUmjErGg8kTJzdnYjNfvi3Z", 9) # "SwissBorg Token"
    yield ("Bonk", "DezXAZ8z7PnrnRJjz3wXBoRgixCa6xjnB7YaB1pPB263", 5) # "Bonk"
    yield ("Cake", "4qQeZ5LwSz6HuupUu8jCtgXyW1mYQcNbFAW1sWZp89HL", 9) # "PancakeSwap Token"
    yield ("FDUSD", "9zNQRsGLjNKwCUU5Gq5LR8beUCPzQMVMqKAi3SSZh54u", 6) # "First Digital USD"
    yield ("FLUID", "DuEy8wWrzCUun5ZbbG9hkVqXqqicpTQw8gB7nEAzpCHQ", 9) # "FLUID"
    yield ("Fartcoin ", "9BB6NFEcjBCtnNLFko2FqVQBq8HHM13kCyYcdQbgpump", 6) # "Fartcoin "
    yield ("HNT", "hntyVP6YFm1Hg25TN9WGLqM12b8TQmcknKrdu1oxWux", 8) # "Helium Network Token"
    yield ("JLP", "27G8MtK7VtTcCHkpASjSDdkWWYfoqT6ggEuKidVJidD4", 6) # "Jupiter Perps LP"
    yield ("JTO", "jtojtomepa8beP8AuQc6eXt5FriJwfFMwQx2v2f9mCL", 9) # "JITO"
    yield ("JUP", "JUPyiwrYJFskUPiHa7hkeR8VUtAeFoSYbKedZNsDvCN", 6) # "Jupiter"
    yield ("JitoSOL", "J1toso1uCk3RLmjorhTtrVwY9HJ7X8V9yYac6Y7kGCPn", 9) # "Jito Staked SOL"
    yield ("JupSOL", "jupSoLaHXQiZZTSfEWMTRRgpnyFm8f6sZdosWBjx93v", 9) # "Jupiter Staked SOL"
    yield ("LBTC", "LBTCgU4b3wsFKsPwBn1rRZDx5DoFutM6RPiEt1TPDsY", 8) # "Lombard Staked BTC"
    yield ("LINK", "LinkhB3afbBKb2EQQu7s7umdZceV3wcvAUJhQAfQ23L", 9) # "Chainlink Token"
    yield ("LION", "7kN5FQMD8ja4bzysEgc5FXmryKd6gCgjiWnhksjHCFb3", 9) # "Loaded Lions"
    yield ("OUSG", "i7u4r16TcsJTgq1kAG8opmVZyVnAKBwLKu6ZPMwzxNc", 6) # "Ondo Short-Term US Gov Bond Fund"
    yield ("PENGU", "2zMMhcVQEXDtdE6vsFS7S7D5oUodfJHE8vd1gnBouauv", 6) # "Pudgy Penguins"
    yield ("PUMP", "pumpCmXqMfrsAkQ5r49WcJnRayYRqmXz6ae8H7H9Dfn", 6) # "Pump"
    yield ("PYTH", "HZ1JovNiVvGrGNiiYvEozEVgZ58xaU3RKwX8eACQBCt3", 6) # "Pyth Network"
    yield ("PYUSD", "2b1kV6DkPAnxd5ixfnxCpjxmKwqjjaYmCZfHsFu24GXo", 6) # "PayPal USD"
    yield ("RAY", "4k3Dyjzvzp8eMZWUXbBCjEvwSkkk59S5iCNLY3QrkX6R", 6) # "Raydium"
    yield ("RENDER", "rndrizKT3MK1iimdxRdWabcF7Zg7AR5T4nud4EkHBof", 8) # "Render Token"
    yield ("SPX", "J3NKxxXZcnNiMjKw9hYb2K4LUxgwB6t1FtPtQVsv3KFr", 8) # "SPX6900 (Wormhole)"
    yield ("SolvBTC", "SoLvHDFVstC74Jr9eNLTDoG4goSUsn1RENmjNtFKZvW", 8) # "SolvBTC"
    yield ("TRUMP", "6p6xgHyF7AeE6TZkSmFsko444wqoP15icUSqi2jfGiPN", 6) # "OFFICIAL TRUMP"
    yield ("USD1", "USD1ttGY1N17NEEHLmELoaybftRBUSErhqYiQzvEmuB", 6) # "World Liberty Financial USD"
    yield ("USDC", "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v", 6) # "USD Coin"
    yield ("USDG", "2u1tszSeqZ3qBWF3uNGPFc8TzMk2tdiwknnRMWGWjGWH", 6) # "Global Dollar"
    yield ("USDS", "USDSwr9ApdHk5bvJKMjzff41FfuX8bSxdKcR81vTwcA", 6) # "USDS"
    yield ("USDT", "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB", 6) # "USDT"
    yield ("USDY", "A1KLoBrKBde8Ty9qtNQUtq3C2ortoC3u7twggz7sEto6", 6) # "Ondo US Dollar Yield"
    yield ("USDe", "DEkqHyPN7GMRJ5cArtQFAWefqbZb33Hyf6s5iCwjEonT", 9) # "USDe"
    yield ("VIRTUAL", "3iQL8BFS2vE7mww4ehAqQHAsbmRNCrPxizWAT2Zfyr9y", 9) # "Virtual Protocol"
    yield ("W", "85VBFQZC9TZkfaptBWjvUw7YbZjy52A6mjtPGjstQAmQ", 6) # "Wormhole Token"
    yield ("WBTC", "5XZw2LKTyrfvfiskJ78AMpackRjPcyCif1WhUsPDuVqQ", 8) # "Wrapped BTC"
    yield ("WLFI", "WLFinEv6ypjkczcS83FZqFpgFZYwQXutRbxGe7oC16g", 6) # "World Liberty Financial"
    yield ("ZBCN", "ZBCNpuD7YMXzTHB2fhGkGi78MNsHGLRXUhRewNRm9RU", 6) # "Zebec Network"
    yield ("bbSOL", "Bybit2vBJGhPF52GBdNaQfUJ6ZpThSgHBobjWZpLPb4B", 9) # "BybitSOL"
    yield ("cbBTC", "cbbtcf3aa214zXHbiAZQwf4122FBYbraNdFqgw4iMij", 8) # "Coinbase Wrapped BTC"
    yield ("dSOL", "Dso1bDeDjCQxTrWHqUUi63oBvV7Mdm6WaobLbQ7gnPQ", 9) # "Drift Staked Sol"
    yield ("mSOL", "mSoLzYCxHdYgdzU16g5QSh3i5K3z3KZK7ytfqcJm7So", 9) # "Marinade staked SOL (mSOL)"
    yield ("sUSDe", "Eh6XEPhSwoLv5wFApukmnaVSHQ6sAnoD9BmgmwQoN2sN", 9) # "Staked USDe"
    yield ("syrupUSDC", "AvZZF1YaZDziPY2RCK4oJrRVrbN3mTD9NL24hPeaZeUj", 6) # "Syrup USDC"
    yield ("tBTC", "6DNSN2BJsaPFdFFc1zP37kkeNe4Usc1Sqkzr9C9vPWcU", 8) # "tBTC v2"
    yield ("wSOL", "So11111111111111111111111111111111111111112", 9) # "Wrapped SOL"
