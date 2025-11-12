# generated from tokens.py.mako
# (by running `make templates` in `core`)
# do not edit manually!
# fmt: off

# NOTE: returning a tuple instead of `TokenInfo` from the "data" function
# saves 5600 bytes of flash size. Implementing the `_token_iterator`
# instead of if-tree approach saves another 5600 bytes.

# NOTE: interestingly, it did not save much flash size to use smaller
# parts of the address, for example address length of 10 bytes saves
# 1 byte per entry, so 1887 bytes overall (and further decrease does not help).
# (The idea was not having to store the whole address, even a smaller part
# of it has enough collision-resistance.)
# (In the if-tree approach the address length did not have any effect whatsoever.)

from typing import Iterator

from trezor.messages import EthereumTokenInfo

UNKNOWN_TOKEN = EthereumTokenInfo(
    symbol="Token",
    decimals=0,
    address=b"",
    chain_id=0,
    name="Unknown token",
)


def token_by_chain_address(chain_id: int, address: bytes) -> EthereumTokenInfo:
    for addr, symbol, decimal, name in _token_iterator(chain_id):
        if address == addr:
            return EthereumTokenInfo(
                symbol=symbol,
                decimals=decimal,
                address=address,
                chain_id=chain_id,
                name=name,
            )
    return UNKNOWN_TOKEN


def _token_iterator(chain_id: int) -> Iterator[tuple[bytes, str, int, str]]:
    if chain_id == 1:  # ETH
        yield (  # address, symbol, decimals, name
            b"\x7f\xc6\x65\x00\xc8\x4a\x76\xad\x7e\x9c\x93\x43\x7b\xfc\x5a\xc3\x3e\x2d\xda\xe9",
            "AAVE",
            18,
            "Aave Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xb5\x07\x21\xbc\xf8\xd6\x64\xc3\x04\x12\xcf\xbc\x6c\xf7\xa1\x51\x45\x23\x4a\xd1",
            "ARB",
            18,
            "Arbitrum",
        )
        yield (  # address, symbol, decimals, name
            b"\x54\xd2\x25\x27\x57\xe1\x67\x2e\xea\xd2\x34\xd2\x7b\x12\x70\x72\x8f\xf9\x05\x81",
            "BGB",
            18,
            "BitgetToken",
        )
        yield (  # address, symbol, decimals, name
            b"\xb8\xc7\x74\x82\xe4\x5f\x1f\x44\xde\x17\x45\xf5\x2c\x74\x42\x6c\x63\x1b\xdd\x52",
            "BNB",
            18,
            "BNB",
        )
        yield (  # address, symbol, decimals, name
            b"\x11\x51\xcb\x3d\x86\x19\x20\xe0\x7a\x38\xe0\x3e\xea\xd1\x2c\x32\x17\x85\x67\xf6",
            "Bonk",
            5,
            "Bonk",
        )
        yield (  # address, symbol, decimals, name
            b"\xc6\x69\x92\x81\x85\xdb\xce\x49\xd2\x23\x0c\xc9\xb0\x97\x9b\xe6\xdc\x79\x79\x57",
            "BTT",
            18,
            "BitTorrent",
        )
        yield (  # address, symbol, decimals, name
            b"\x15\x26\x49\xea\x73\xbe\xab\x28\xc5\xb4\x9b\x26\xeb\x48\xf7\xea\xd6\xd4\xc8\x98",
            "Cake",
            18,
            "PancakeSwap Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xcb\xb7\xc0\x00\x0a\xb8\x8b\x47\x3b\x1f\x5a\xfd\x9e\xf8\x08\x44\x0e\xed\x33\xbf",
            "cbBTC",
            8,
            "Coinbase Wrapped BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\xbe\x98\x95\x14\x6f\x7a\xf4\x30\x49\xca\x1c\x1a\xe3\x58\xb0\x54\x1e\xa4\x97\x04",
            "cbETH",
            18,
            "Coinbase Wrapped Staked ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\xc6\x0a\x91\x45\xd9\xe9\xf1\x15\x22\x18\xe7\xda\x6d\xf6\x34\xb7\xa7\x4a\xe4\x44",
            "cgETH.hashkey",
            18,
            "cgETH Hashkey Cloud",
        )
        yield (  # address, symbol, decimals, name
            b"\xe7\xae\x30\xc0\x33\x95\xd6\x6f\x30\xa2\x6c\x49\xc9\x1e\xda\xe1\x51\x74\x79\x11",
            "clBTC",
            18,
            "clBTC",
        )
        yield (  # address, symbol, decimals, name
            b"\xa0\xb7\x3e\x1f\xf0\xb8\x09\x14\xab\x6f\xe0\x44\x4e\x65\x84\x8c\x4c\x34\x45\x0b",
            "CRO",
            8,
            "CRO",
        )
        yield (  # address, symbol, decimals, name
            b"\xd5\x33\xa9\x49\x74\x0b\xb3\x30\x6d\x11\x9c\xc7\x77\xfa\x90\x0b\xa0\x34\xcd\x52",
            "CRV",
            18,
            "Curve DAO Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x6b\x17\x54\x74\xe8\x90\x94\xc4\x4d\xa9\x8b\x95\x4e\xed\xea\xc4\x95\x27\x1d\x0f",
            "DAI",
            18,
            "Dai Stablecoin",
        )
        yield (  # address, symbol, decimals, name
            b"\x19\x6c\x20\xda\x81\xfb\xc3\x24\xec\xdf\x55\x50\x1e\x95\xce\x9f\x0b\xd8\x4d\x14",
            "DOT",
            10,
            "Polkadot",
        )
        yield (  # address, symbol, decimals, name
            b"\x35\xfa\x16\x47\x35\x18\x2d\xe5\x08\x11\xe8\xe2\xe8\x24\xcf\xb9\xb6\x11\x8a\xc2",
            "eETH",
            18,
            "ether.fi ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x57\xe1\x14\xb6\x91\xdb\x79\x0c\x35\x20\x7b\x2e\x68\x5d\x4a\x43\x18\x1e\x60\x61",
            "ENA",
            18,
            "ENA",
        )
        yield (  # address, symbol, decimals, name
            b"\xc1\x83\x60\x21\x7d\x8f\x7a\xb5\xe7\xc5\x16\x56\x67\x61\xea\x12\xce\x7f\x9d\x72",
            "ENS",
            18,
            "Ethereum Name Service",
        )
        yield (  # address, symbol, decimals, name
            b"\xfe\x0c\x30\x06\x5b\x38\x4f\x05\x76\x1f\x15\xd0\xcc\x89\x9d\x4f\x9f\x9c\xc0\xeb",
            "ETHFI",
            18,
            "ether.fi governance token",
        )
        yield (  # address, symbol, decimals, name
            b"\xa3\x5b\x1b\x31\xce\x00\x2f\xbf\x20\x58\xd2\x2f\x30\xf9\x5d\x40\x52\x00\xa1\x5b",
            "ETHx",
            18,
            "ETHx",
        )
        yield (  # address, symbol, decimals, name
            b"\xbf\x54\x95\xef\xe5\xdb\x9c\xe0\x0f\x80\x36\x4c\x8b\x42\x35\x67\xe5\x8d\x21\x10",
            "ezETH",
            18,
            "Renzo Restaked ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\xc5\xf0\xf7\xb6\x67\x64\xf6\xec\x8c\x8d\xff\x7b\xa6\x83\x10\x22\x95\xe1\x64\x09",
            "FDUSD",
            18,
            "First Digital USD",
        )
        yield (  # address, symbol, decimals, name
            b"\xae\xa4\x6a\x60\x36\x8a\x7b\xd0\x60\xee\xc7\xdf\x8c\xba\x43\xb7\xef\x41\xad\x85",
            "FET",
            18,
            "Fetch",
        )
        yield (  # address, symbol, decimals, name
            b"\xcf\x0c\x12\x2c\x6b\x73\xff\x80\x9c\x69\x3d\xb7\x61\xe7\xba\xeb\xe6\x2b\x6a\x2e",
            "FLOKI",
            9,
            "FLOKI",
        )
        yield (  # address, symbol, decimals, name
            b"\xae\xdf\x38\x6b\x75\x54\x65\x87\x1f\xf8\x74\xe3\xe3\x7a\xf5\x97\x6e\x24\x70\x64",
            "FTN",
            18,
            "Fasttoken",
        )
        yield (  # address, symbol, decimals, name
            b"\xd1\xd2\xeb\x1b\x1e\x90\xb6\x38\x58\x87\x28\xb4\x13\x01\x37\xd2\x62\xc8\x7c\xae",
            "GALA",
            8,
            "Gala",
        )
        yield (  # address, symbol, decimals, name
            b"\xc9\x44\xe9\x0c\x64\xb2\xc0\x76\x62\xa2\x92\xbe\x62\x44\xbd\xf0\x5c\xda\x44\xa7",
            "GRT",
            18,
            "Graph Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xe6\x67\x47\xa1\x01\xbf\xf2\xdb\xa3\x69\x71\x99\xdc\xce\x5b\x74\x3b\x45\x47\x59",
            "GT",
            18,
            "GateChainToken",
        )
        yield (  # address, symbol, decimals, name
            b"\xcf\x51\x04\xd0\x94\xe3\x86\x4c\xfc\xbd\xa4\x3b\x82\xe1\xce\xfd\x26\xa0\x16\xeb",
            "H",
            18,
            "Humanity",
        )
        yield (  # address, symbol, decimals, name
            b"\x61\xec\x85\xab\x89\x37\x7d\xb6\x57\x62\xe2\x34\xc9\x46\xb5\xc2\x5a\x56\xe9\x9e",
            "HTX",
            18,
            "HTX",
        )
        yield (  # address, symbol, decimals, name
            b"\x00\xf3\xc4\x28\x33\xc3\x17\x01\x59\xaf\x4e\x92\xdb\xb4\x51\xfb\x3f\x70\x89\x17",
            "ICP",
            8,
            "ICP",
        )
        yield (  # address, symbol, decimals, name
            b"\xf5\x7e\x7e\x7c\x23\x97\x8c\x3c\xae\xc3\xc3\x54\x8e\x3d\x61\x5c\x34\x6e\x79\xff",
            "IMX",
            18,
            "Immutable X",
        )
        yield (  # address, symbol, decimals, name
            b"\xe2\x8b\x3b\x32\xb6\xc3\x45\xa3\x4f\xf6\x46\x74\x60\x61\x24\xdd\x5a\xce\xca\x30",
            "INJ",
            18,
            "Injective Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x74\x20\xb4\xb9\xa0\x11\x0c\xdc\x71\xfb\x72\x09\x08\x34\x0c\x03\xf9\xbc\x03\xec",
            "JASMY",
            18,
            "JasmyCoin",
        )
        yield (  # address, symbol, decimals, name
            b"\x82\x36\xa8\x70\x84\xf8\xb8\x43\x06\xf7\x20\x07\xf3\x6f\x26\x18\xa5\x63\x44\x94",
            "LBTC",
            8,
            "Lombard Staked Bitcoin",
        )
        yield (  # address, symbol, decimals, name
            b"\x5a\x98\xfc\xbe\xa5\x16\xcf\x06\x85\x72\x15\x77\x9f\xd8\x12\xca\x3b\xef\x1b\x32",
            "LDO",
            18,
            "Lido DAO Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x2a\xf5\xd2\xad\x76\x74\x11\x91\xd1\x5d\xfe\x7b\xf6\xac\x92\xd4\xbd\x91\x2c\xa3",
            "LEO",
            18,
            "Bitfinex LEO Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x51\x49\x10\x77\x1a\xf9\xca\x65\x6a\xf8\x40\xdf\xf8\x3e\x82\x64\xec\xf9\x86\xca",
            "LINK",
            18,
            "ChainLink Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xf0\xbb\x20\x86\x52\x77\xab\xd6\x41\xa3\x07\xec\xe5\xee\x04\xe7\x90\x73\x41\x6c",
            "liquidETH",
            18,
            "Ether.Fi Liquid ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x8c\x1b\xed\x5b\x9a\x09\x28\x46\x7c\x9b\x13\x41\xda\x1d\x7b\xd5\xe1\x0b\x65\x49",
            "LsETH",
            18,
            "Liquid Staked ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\xd5\xf7\x83\x8f\x5c\x46\x1f\xef\xf7\xfe\x49\xea\x5e\xba\xf7\x72\x8b\xb0\xad\xfa",
            "mETH",
            18,
            "mETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x3c\x3a\x81\xe8\x1d\xc4\x9a\x52\x2a\x59\x2e\x76\x22\xa7\xe7\x11\xc0\x6b\xf3\x54",
            "MNT",
            18,
            "Mantle",
        )
        yield (  # address, symbol, decimals, name
            b"\x58\xd9\x7b\x57\xbb\x95\x32\x0f\x9a\x05\xdc\x91\x8a\xef\x65\x43\x49\x69\xc2\xb2",
            "MORPHO",
            18,
            "Morpho Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x85\xf1\x7c\xf9\x97\x93\x4a\x59\x70\x31\xb2\xe1\x8a\x9a\xb6\xeb\xd4\xb9\xf6\xa4",
            "NEAR",
            24,
            "NEAR",
        )
        yield (  # address, symbol, decimals, name
            b"\xb6\x21\x32\xe3\x5a\x6c\x13\xee\x1e\xe0\xf8\x4d\xc5\xd4\x0b\xad\x8d\x81\x52\x06",
            "NEXO",
            18,
            "Nexo",
        )
        yield (  # address, symbol, decimals, name
            b"\x75\x23\x1f\x58\xb4\x32\x40\xc9\x71\x8d\xd5\x8b\x49\x67\xc5\x11\x43\x42\xa8\x6c",
            "OKB",
            18,
            "OKB",
        )
        yield (  # address, symbol, decimals, name
            b"\xfa\xba\x6f\x8e\x4a\x5e\x8a\xb8\x2f\x62\xfe\x7c\x39\x85\x9f\xa5\x77\x26\x9b\xe3",
            "ONDO",
            18,
            "Ondo",
        )
        yield (  # address, symbol, decimals, name
            b"\xf1\xc9\xac\xdc\x66\x97\x4d\xfb\x6d\xec\xb1\x2a\xa3\x85\xb9\xcd\x01\x19\x0e\x38",
            "osETH",
            18,
            "Staked ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x1b\x19\xc1\x93\x93\xe2\xd0\x34\xd8\xff\x31\xff\x34\xc8\x12\x52\xfc\xbb\xee\x92",
            "OUSG",
            18,
            "Ondo Short-Term U.S. Government Bond Fund",
        )
        yield (  # address, symbol, decimals, name
            b"\x45\x80\x48\x80\xde\x22\x91\x3d\xaf\xe0\x9f\x49\x80\x84\x8e\xce\x6e\xcb\xaf\x78",
            "PAXG",
            18,
            "Paxos Gold",
        )
        yield (  # address, symbol, decimals, name
            b"\x80\x85\x07\x12\x1b\x80\xc0\x23\x88\xfa\xd1\x47\x26\x48\x2e\x06\x1b\x8d\xa8\x27",
            "PENDLE",
            18,
            "Pendle",
        )
        yield (  # address, symbol, decimals, name
            b"\x64\x18\xc0\xdd\x09\x9a\x9f\xda\x39\x7c\x76\x63\x04\xcd\xd9\x18\x23\x3e\x88\x47",
            "PENGU",
            18,
            "Pudgy Penguins",
        )
        yield (  # address, symbol, decimals, name
            b"\x69\x82\x50\x81\x45\x45\x4c\xe3\x25\xdd\xbe\x47\xa2\x5d\x4e\xc3\xd2\x31\x19\x33",
            "PEPE",
            18,
            "Pepe",
        )
        yield (  # address, symbol, decimals, name
            b"\x45\x5e\x53\xcb\xb8\x60\x18\xac\x2b\x80\x92\xfd\xcd\x39\xd8\x44\x4a\xff\xc3\xf6",
            "POL",
            18,
            "Polygon Ecosystem Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x6c\x3e\xa9\x03\x64\x06\x85\x20\x06\x29\x07\x70\xbe\xdf\xca\xba\x0e\x23\xa0\xe8",
            "PYUSD",
            6,
            "PayPal USD",
        )
        yield (  # address, symbol, decimals, name
            b"\x4a\x22\x0e\x60\x96\xb2\x5e\xad\xb8\x83\x58\xcb\x44\x06\x8a\x32\x48\x25\x46\x75",
            "QNT",
            18,
            "Quant",
        )
        yield (  # address, symbol, decimals, name
            b"\xae\x78\x73\x6c\xd6\x15\xf3\x74\xd3\x08\x51\x23\xa2\x10\x44\x8e\x74\xfc\x63\x93",
            "rETH",
            18,
            "Rocket Pool ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x82\x92\xbb\x45\xbf\x1e\xe4\xd1\x40\x12\x70\x49\x75\x7c\x2e\x0f\xf0\x63\x17\xed",
            "RLUSD",
            18,
            "RLUSD",
        )
        yield (  # address, symbol, decimals, name
            b"\x6d\xe0\x37\xef\x9a\xd2\x72\x5e\xb4\x01\x18\xbb\x17\x02\xeb\xb2\x7e\x4a\xeb\x24",
            "RNDR",
            18,
            "Render Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xa1\x29\x0d\x69\xc6\x5a\x6f\xe4\xdf\x75\x2f\x95\x82\x3f\xae\x25\xcb\x99\xe5\xa7",
            "rsETH",
            18,
            "rsETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x38\x45\xba\xda\xde\x8e\x6d\xff\x04\x98\x20\x68\x0d\x1f\x14\xbd\x39\x03\xa5\xd0",
            "SAND",
            18,
            "SAND",
        )
        yield (  # address, symbol, decimals, name
            b"\x95\xad\x61\xb0\xa1\x50\xd7\x92\x19\xdc\xf6\x4e\x1e\x6c\xc0\x1f\x0b\x64\xc4\xce",
            "SHIB",
            18,
            "SHIBA INU",
        )
        yield (  # address, symbol, decimals, name
            b"\x56\x07\x2c\x95\xfa\xa7\x01\x25\x60\x59\xaa\x12\x26\x97\xb1\x33\xad\xed\x92\x79",
            "SKY",
            18,
            "SKY Governance Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x7a\x56\xe1\xc5\x7c\x74\x75\xcc\xf7\x42\xa1\x83\x2b\x02\x8f\x04\x56\x65\x2f\x97",
            "SolvBTC",
            18,
            "Solv BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\xe0\xf6\x3a\x42\x4a\x44\x39\xcb\xe4\x57\xd8\x0e\x4f\x4b\x51\xad\x25\xb2\xc5\x6c",
            "SPX",
            8,
            "SPX6900",
        )
        yield (  # address, symbol, decimals, name
            b"\xbe\xef\x01\x73\x5c\x13\x2a\xda\x46\xaa\x9a\xa4\xc5\x46\x23\xca\xa9\x2a\x64\xcb",
            "steakUSDC",
            18,
            "Steakhouse USDC",
        )
        yield (  # address, symbol, decimals, name
            b"\xae\x7a\xb9\x65\x20\xde\x3a\x18\xe5\xe1\x11\xb5\xea\xab\x09\x53\x12\xd7\xfe\x84",
            "stETH",
            18,
            "Liquid staked Ether 2.0",
        )
        yield (  # address, symbol, decimals, name
            b"\xca\x14\x00\x7e\xff\x0d\xb1\xf8\x13\x5f\x4c\x25\xb3\x4d\xe4\x9a\xb0\xd4\x27\x66",
            "STRK",
            18,
            "StarkNet Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x9d\x39\xa5\xde\x30\xe5\x74\x43\xbf\xf2\xa8\x30\x7a\x42\x56\xc8\x79\x7a\x34\x97",
            "sUSDe",
            18,
            "Staked USDe",
        )
        yield (  # address, symbol, decimals, name
            b"\xa3\x93\x1d\x71\x87\x7c\x0e\x7a\x31\x48\xcb\x7e\xb4\x46\x35\x24\xfe\xc2\x7f\xbd",
            "sUSDS",
            18,
            "Savings USDS",
        )
        yield (  # address, symbol, decimals, name
            b"\xf9\x51\xe3\x35\xaf\xb2\x89\x35\x3d\xc2\x49\xe8\x29\x26\x17\x8e\xac\x7d\xed\x78",
            "swETH",
            18,
            "swETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x64\x3c\x4e\x15\xd7\xd6\x2a\xd0\xab\xec\x4a\x9b\xd4\xb0\x01\xaa\x3e\xf5\x2d\x66",
            "SYRUP",
            18,
            "Syrup Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x80\xac\x24\xaa\x92\x9e\xaf\x50\x13\xf6\x43\x6c\xda\x2a\x7b\xa1\x90\xf5\xcc\x0b",
            "syrupUSDC",
            6,
            "Syrup USDC",
        )
        yield (  # address, symbol, decimals, name
            b"\x35\x6b\x8d\x89\xc1\xe1\x23\x9c\xbb\xb9\xde\x48\x15\xc3\x9a\x14\x74\xd5\xba\x7d",
            "syrupUSDT",
            6,
            "Syrup USDT",
        )
        yield (  # address, symbol, decimals, name
            b"\x18\x08\x4f\xba\x66\x6a\x33\xd3\x75\x92\xfa\x26\x33\xfd\x49\xa7\x4d\xd9\x3a\x88",
            "tBTC",
            18,
            "tBTC v2",
        )
        yield (  # address, symbol, decimals, name
            b"\x58\x2d\x87\x2a\x1b\x09\x4f\xc4\x8f\x5d\xe3\x1d\x3b\x73\xf2\xd9\xbe\x47\xde\xf1",
            "TONCOIN",
            9,
            "Wrapped TON Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\x00\x00\x00\x00\x00\x08\x5d\x47\x80\xb7\x31\x19\xb6\x44\xae\x5e\xcd\x22\xb3\x76",
            "TUSD",
            18,
            "TrueUSD",
        )
        yield (  # address, symbol, decimals, name
            b"\x1f\x98\x40\xa8\x5d\x5a\xf5\xbf\x1d\x17\x62\xf9\x25\xbd\xad\xdc\x42\x01\xf9\x84",
            "UNI",
            18,
            "Uniswap",
        )
        yield (  # address, symbol, decimals, name
            b"\x73\xa1\x5f\xed\x60\xbf\x67\x63\x1d\xc6\xcd\x7b\xc5\xb6\xe8\xda\x81\x90\xac\xf5",
            "USD0",
            18,
            "Usual USD",
        )
        yield (  # address, symbol, decimals, name
            b"\x8d\x0d\x00\x0e\xe4\x49\x48\xfc\x98\xc9\xb9\x8a\x4f\xa4\x92\x14\x76\xf0\x8b\x0d",
            "USD1",
            18,
            "World Liberty Financial USD",
        )
        yield (  # address, symbol, decimals, name
            b"\xa0\xb8\x69\x91\xc6\x21\x8b\x36\xc1\xd1\x9d\x4a\x2e\x9e\xb0\xce\x36\x06\xeb\x48",
            "USDC",
            6,
            "USD Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\x4c\x9e\xdd\x58\x52\xcd\x90\x5f\x08\x6c\x75\x9e\x83\x83\xe0\x9b\xff\x1e\x68\xb3",
            "USDe",
            18,
            "USDe",
        )
        yield (  # address, symbol, decimals, name
            b"\xfa\x2b\x94\x7e\xec\x36\x8f\x42\x19\x5f\x24\xf3\x6d\x2a\xf2\x9f\x7c\x24\xce\xc2",
            "USDf",
            18,
            "Falcon USD",
        )
        yield (  # address, symbol, decimals, name
            b"\xe3\x43\x16\x76\x31\xd8\x9b\x6f\xfc\x58\xb8\x8d\x6b\x7f\xb0\x22\x87\x95\x49\x1d",
            "USDG",
            6,
            "Global Dollar",
        )
        yield (  # address, symbol, decimals, name
            b"\xdc\x03\x5d\x45\xd9\x73\xe3\xec\x16\x9d\x22\x76\xdd\xab\x16\xf1\xe4\x07\x38\x4f",
            "USDS",
            18,
            "USDS Stablecoin",
        )
        yield (  # address, symbol, decimals, name
            b"\xda\xc1\x7f\x95\x8d\x2e\xe5\x23\xa2\x20\x62\x06\x99\x45\x97\xc1\x3d\x83\x1e\xc7",
            "USDT",
            6,
            "Tether USD",
        )
        yield (  # address, symbol, decimals, name
            b"\xc1\x39\x19\x0f\x44\x7e\x92\x9f\x09\x0e\xde\xb5\x54\xd9\x5a\xbb\x8b\x18\xac\x1c",
            "USDtb",
            18,
            "USDtb",
        )
        yield (  # address, symbol, decimals, name
            b"\xf3\x52\x7e\xf8\xde\x26\x5e\xaa\x37\x16\xfb\x31\x2c\x12\x84\x7b\xfb\xa6\x6c\xef",
            "USDX",
            18,
            "USDX",
        )
        yield (  # address, symbol, decimals, name
            b"\x96\xf6\xef\x95\x18\x40\x72\x1a\xdb\xf4\x6a\xc9\x96\xb5\x9e\x02\x35\xcb\x98\x5c",
            "USDY",
            18,
            "Ondo U.S. Dollar Yield",
        )
        yield (  # address, symbol, decimals, name
            b"\x43\x41\x5e\xb6\xff\x9d\xb7\xe2\x6a\x15\xb7\x04\xe7\xa3\xed\xce\x97\xd3\x1c\x4e",
            "USTB",
            6,
            "Superstate Short Duration US Government Securities Fund",
        )
        yield (  # address, symbol, decimals, name
            b"\x44\xff\x86\x20\xb8\xca\x30\x90\x23\x95\xa7\xbd\x3f\x24\x07\xe1\xa0\x91\xbf\x73",
            "VIRTUAL",
            18,
            "Virtual Protocol",
        )
        yield (  # address, symbol, decimals, name
            b"\xa2\xe3\x35\x66\x10\x84\x07\x01\xbd\xf5\x61\x1a\x53\x97\x45\x10\xae\x27\xe2\xe1",
            "wBETH",
            18,
            "Wrapped Binance Beacon ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x92\x52\x06\xb8\xa7\x07\x09\x6e\xd2\x6a\xe4\x7c\x84\x74\x7f\xe0\xbb\x73\x4f\x59",
            "WBT",
            8,
            "WBT",
        )
        yield (  # address, symbol, decimals, name
            b"\x22\x60\xfa\xc5\xe5\x54\x2a\x77\x3a\xa4\x4f\xbc\xfe\xdf\x7c\x19\x3b\xc2\xc5\x99",
            "WBTC",
            8,
            "Wrapped BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\xcd\x5f\xe2\x3c\x85\x82\x0f\x7b\x72\xd0\x92\x6f\xc9\xb0\x5b\x43\xe3\x59\xb7\xee",
            "weETH",
            18,
            "Wrapped eETH",
        )
        yield (  # address, symbol, decimals, name
            b"\xc0\x2a\xaa\x39\xb2\x23\xfe\x8d\x0a\x0e\x5c\x4f\x27\xea\xd9\x08\x3c\x75\x6c\xc2",
            "WETH",
            18,
            "Wrapped Ether",
        )
        yield (  # address, symbol, decimals, name
            b"\x16\x3f\x8c\x24\x67\x92\x4b\xe0\xae\x7b\x53\x47\x22\x8c\xab\xf2\x60\x31\x87\x53",
            "WLD",
            18,
            "Worldcoin",
        )
        yield (  # address, symbol, decimals, name
            b"\xda\x5e\x19\x88\x09\x72\x97\xdc\xdc\x1f\x90\xd4\xdf\xe7\x90\x9e\x84\x7c\xbe\xf6",
            "WLFI",
            18,
            "World Liberty Financial",
        )
        yield (  # address, symbol, decimals, name
            b"\x7f\x39\xc5\x81\xf5\x95\xb5\x3c\x5c\xb1\x9b\xd0\xb3\xf8\xda\x6c\x93\x5e\x2c\xa0",
            "wstETH",
            18,
            "Wrapped liquid staked Ether 2.0",
        )
        yield (  # address, symbol, decimals, name
            b"\x68\x74\x96\x65\xff\x8d\x2d\x11\x2f\xa8\x59\xaa\x29\x3f\x07\xa6\x22\x78\x2f\x38",
            "XAUt",
            6,
            "Tether Gold",
        )
    if chain_id == 10:  # OPT
        yield (  # address, symbol, decimals, name
            b"\x3e\x7e\xf8\xf5\x02\x46\xf7\x25\x88\x51\x02\xe8\x23\x8c\xbb\xa3\x3f\x27\x67\x47",
            "BOND",
            18,
            "BarnBridge Governance Token (Optimism)",
        )
        yield (  # address, symbol, decimals, name
            b"\xda\x10\x00\x9c\xbd\x5d\x07\xdd\x0c\xec\xc6\x61\x61\xfc\x93\xd7\xc9\x00\x0d\xa1",
            "DAI",
            18,
            "Dai Stablecoin",
        )
        yield (  # address, symbol, decimals, name
            b"\x35\x0a\x79\x1b\xfc\x2c\x21\xf9\xed\x5d\x10\x98\x0d\xad\x2e\x26\x38\xff\xa7\xf6",
            "LINK",
            18,
            "ChainLink Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x42\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x42",
            "OP",
            18,
            "Optimism",
        )
        yield (  # address, symbol, decimals, name
            b"\x9e\x10\x28\xf5\xf1\xd5\xed\xe5\x97\x48\xff\xce\xe5\x53\x25\x09\x97\x68\x40\xe0",
            "PERP",
            18,
            "Perpetual",
        )
        yield (  # address, symbol, decimals, name
            b"\x7f\xb6\x88\xcc\xf6\x82\xd5\x8f\x86\xd7\xe3\x8e\x03\xf9\xd2\x2e\x77\x05\x44\x8b",
            "RAI",
            18,
            "Rai Reflex Index",
        )
        yield (  # address, symbol, decimals, name
            b"\x87\x00\xda\xec\x35\xaf\x8f\xf8\x8c\x16\xbd\xf0\x41\x87\x74\xcb\x3d\x75\x99\xb4",
            "SNX",
            18,
            "Synthetix Network Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x6f\xd9\xd7\xad\x17\x24\x2c\x41\xf7\x13\x1d\x25\x72\x12\xc5\x4a\x0e\x81\x66\x91",
            "UNI",
            18,
            "Uniswap",
        )
        yield (  # address, symbol, decimals, name
            b"\x0b\x2c\x63\x9c\x53\x38\x13\xf4\xaa\x9d\x78\x37\xca\xf6\x26\x53\xd0\x97\xff\x85",
            "USDC",
            6,
            "USD Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\x7f\x5c\x76\x4c\xbc\x14\xf9\x66\x9b\x88\x83\x7c\xa1\x49\x0c\xca\x17\xc3\x16\x07",
            "USDC.e",
            6,
            "USD Coin (Bridged from Ethereum)",
        )
        yield (  # address, symbol, decimals, name
            b"\x94\xb0\x08\xaa\x00\x57\x9c\x13\x07\xb0\xef\x2c\x49\x9a\xd9\x8a\x8c\xe5\x8e\x58",
            "USDT",
            6,
            "Tether",
        )
        yield (  # address, symbol, decimals, name
            b"\x68\xf1\x80\xfc\xce\x68\x36\x68\x8e\x90\x84\xf0\x35\x30\x9e\x29\xbf\x0a\x20\x95",
            "WBTC",
            8,
            "Wrapped BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x42\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x06",
            "WETH",
            18,
            "Wrapped Ether",
        )
    if chain_id == 25:  # CRO
        yield (  # address, symbol, decimals, name
            b"\x98\x93\x6b\xde\x1c\xf1\xbf\xf1\xe7\xa8\x01\x2c\xee\x5e\x25\x83\x85\x1f\x20\x67",
            "ANN",
            18,
            "Annex",
        )
        yield (  # address, symbol, decimals, name
            b"\xad\xbd\x12\x31\xfb\x36\x00\x47\x52\x5b\xed\xf9\x62\x58\x1f\x3e\xee\x7b\x49\xfe",
            "CRONA",
            18,
            "CronaSwap Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xe2\x43\xcc\xab\x9e\x66\xe6\xcf\x12\x15\x37\x69\x80\x81\x1d\xdf\x1e\xb7\xf6\x89",
            "CRX",
            18,
            "Crodex Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xf2\x00\x1b\x14\x5b\x43\x03\x2a\xaf\x5e\xe2\x88\x4e\x45\x6c\xcd\x80\x5f\x67\x7d",
            "DAI",
            18,
            "Dai Stablecoin",
        )
        yield (  # address, symbol, decimals, name
            b"\x65\x4b\xac\x3e\xc7\x7d\x6d\xb4\x97\x89\x24\x78\xf8\x54\xcf\x6e\x82\x45\xdc\xa9",
            "SVN",
            18,
            "Savanna Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xc2\x12\x23\x24\x9c\xa2\x83\x97\xb4\xb6\x54\x1d\xff\xae\xcc\x53\x9b\xff\x0c\x59",
            "USDC",
            6,
            "USD Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\x66\xe4\x28\xc3\xf6\x7a\x68\x87\x85\x62\xe7\x9a\x02\x34\xc1\xf8\x3c\x20\x87\x70",
            "USDT",
            6,
            "Tether",
        )
        yield (  # address, symbol, decimals, name
            b"\x2d\x03\xbe\xce\x67\x47\xad\xc0\x0e\x1a\x13\x1b\xba\x14\x69\xc1\x5f\xd1\x1e\x03",
            "VVS",
            18,
            "VVSToken",
        )
        yield (  # address, symbol, decimals, name
            b"\x06\x2e\x66\x47\x7f\xaf\x21\x9f\x25\xd2\x7d\xce\xd6\x47\xbf\x57\xc3\x10\x7d\x52",
            "WBTC",
            8,
            "Wrapped BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x5c\x7f\x8a\x57\x0d\x57\x8e\xd8\x4e\x63\xfd\xfa\x7b\x1e\xe7\x2d\xea\xe1\xae\x23",
            "WCRO",
            18,
            "Wrapped CRO",
        )
        yield (  # address, symbol, decimals, name
            b"\xe4\x4f\xd7\xfc\xb2\xb1\x58\x18\x22\xd0\xc8\x62\xb6\x82\x22\x99\x8a\x0c\x29\x9a",
            "WETH",
            18,
            "Wrapped Ether",
        )
    if chain_id == 30:  # Rootstock
        yield (  # address, symbol, decimals, name
            b"\xe7\x00\x69\x1d\xa7\xb9\x85\x1f\x2f\x35\xf8\xb8\x18\x2c\x69\xc5\x3c\xca\xd9\xdb",
            "DOC",
            18,
            "Dollar on Chain",
        )
        yield (  # address, symbol, decimals, name
            b"\xef\x85\x25\x4a\xa4\xa8\x49\x0b\xcc\x9c\x02\xae\x38\x51\x3c\xae\x83\x03\xfb\x53",
            "mBTC",
            18,
            "Midas BTC Yield Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x9a\xc7\xfe\x28\x96\x7b\x30\xe3\xa4\xe6\xe0\x32\x86\xd7\x15\xb4\x2b\x45\x3d\x10",
            "MOC",
            18,
            "MOC",
        )
        yield (  # address, symbol, decimals, name
            b"\xdd\x62\x9e\x52\x41\xcb\xc5\x91\x98\x47\x78\x3e\x6c\x96\xb2\xde\x47\x54\xe4\x38",
            "mTBILL",
            18,
            "Midas US Treasury Bill Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x2a\xcc\x95\x75\x8f\x8b\x5f\x58\x34\x70\xba\x26\x5e\xb6\x85\xa8\xf4\x5f\xc9\xd5",
            "RIF",
            18,
            "RIF",
        )
        yield (  # address, symbol, decimals, name
            b"\x74\xc9\xf2\xb0\x05\x81\xf1\xb1\x1a\xa7\xff\x05\xaa\x9f\x60\x8b\x73\x89\xde\x67",
            "USDC.e",
            6,
            "Bridged USDC (Stargate)",
        )
        yield (  # address, symbol, decimals, name
            b"\x3a\x15\x46\x1d\x8a\xe0\xf0\xfb\x5f\xa2\x62\x9e\x9d\xa7\xd6\x6a\x79\x4a\x6e\x37",
            "USDRIF",
            18,
            "USDRIF",
        )
        yield (  # address, symbol, decimals, name
            b"\xaf\x36\x8c\x91\x79\x3c\xb2\x27\x39\x38\x6d\xfc\xbb\xb2\xf1\xa9\xe4\xbc\xbe\xbf",
            "USDT",
            6,
            "USDT",
        )
        yield (  # address, symbol, decimals, name
            b"\x2f\x6f\x07\xcd\xcf\x35\x88\x94\x4b\xf4\xc4\x2a\xc7\x4f\xf2\x4b\xf5\x6e\x75\x90",
            "WETH",
            18,
            "WETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x54\x2f\xda\x31\x73\x18\xeb\xf1\xd3\xde\xaf\x76\xe0\xb6\x32\x74\x1a\x7e\x67\x7d",
            "WRBTC",
            18,
            "Wrapped BTC",
        )
    if chain_id == 56:  # BSC
        yield (  # address, symbol, decimals, name
            b"\xfb\x61\x15\x44\x5b\xff\x7b\x52\xfe\xb9\x86\x50\xc8\x7f\x44\x90\x7e\x58\xf8\x02",
            "AAVE",
            18,
            "Aave Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x95\x03\x4f\x65\x3d\x5d\x16\x18\x90\x83\x6a\xd2\xb6\xb8\xcc\x49\xd1\x4e\x02\x9a",
            "AB",
            18,
            "AB",
        )
        yield (  # address, symbol, decimals, name
            b"\x77\x73\x4e\x70\xb6\xe8\x8b\x4d\x82\xfe\x63\x2a\x16\x8e\xdf\x6e\x70\x09\x12\xb6",
            "asBNB",
            18,
            "Astherus BNB",
        )
        yield (  # address, symbol, decimals, name
            b"\x00\x0a\xe3\x14\xe2\xa2\x17\x2a\x03\x9b\x26\x37\x88\x14\xc2\x52\x73\x4f\x55\x6a",
            "ASTER",
            18,
            "Aster",
        )
        yield (  # address, symbol, decimals, name
            b"\x0e\xb3\xa7\x05\xfc\x54\x72\x50\x37\xcc\x9e\x00\x8b\xde\xde\x69\x7f\x62\xf3\x35",
            "ATOM",
            18,
            "Cosmos Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x1c\xe0\xc2\x82\x7e\x2e\xf1\x4d\x5c\x4f\x29\xa0\x91\xd7\x35\xa2\x04\x79\x40\x41",
            "AVAX",
            18,
            "Avalanche",
        )
        yield (  # address, symbol, decimals, name
            b"\x8f\xf7\x95\xa6\xf4\xd9\x7e\x78\x87\xc7\x9b\xea\x79\xab\xa5\xcc\x76\x44\x4a\xdf",
            "BCH",
            18,
            "Bitcoin Cash Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xa6\x97\xe2\x72\xa7\x37\x44\xb3\x43\x52\x8c\x3b\xc4\x70\x2f\x25\x65\xb2\xf4\x22",
            "Bonk",
            5,
            "Bonk",
        )
        yield (  # address, symbol, decimals, name
            b"\x35\x2c\xb5\xe1\x9b\x12\xfc\x21\x65\x48\xa2\x67\x7b\xd0\xfc\xe8\x3b\xae\x43\x4b",
            "BTT",
            18,
            "BitTorrent",
        )
        yield (  # address, symbol, decimals, name
            b"\x0e\x09\xfa\xbb\x73\xbd\x3a\xde\x0a\x17\xec\xc3\x21\xfd\x13\xa1\x9e\x81\xce\x82",
            "Cake",
            18,
            "PancakeSwap Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xba\x2a\xe4\x24\xd9\x60\xc2\x62\x47\xdd\x6c\x32\xed\xc7\x0b\x29\x5c\x74\x4c\x43",
            "DOGE",
            8,
            "Dogecoin",
        )
        yield (  # address, symbol, decimals, name
            b"\x70\x83\x60\x9f\xce\x4d\x1d\x8d\xc0\xc9\x79\xaa\xb8\xc8\x69\xea\x2c\x87\x34\x02",
            "DOT",
            18,
            "Polkadot Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x8d\x01\x0b\xf9\xc2\x68\x81\x78\x8b\x4e\x6b\xf5\xfd\x1b\xdc\x35\x8c\x8f\x90\xb8",
            "DOT",
            18,
            "Polkadot Token (Relay Chain)",
        )
        yield (  # address, symbol, decimals, name
            b"\x21\x70\xed\x08\x80\xac\x9a\x75\x5f\xd2\x9b\x26\x88\x95\x6b\xd9\x59\xf9\x33\xf8",
            "ETH",
            18,
            "Ethereum Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x24\x16\x09\x2f\x14\x33\x78\x75\x0b\xb2\x9b\x79\xed\x96\x1a\xb1\x95\xcc\xee\xa5",
            "ezETH",
            18,
            "Renzo Restaked ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\xc5\xf0\xf7\xb6\x67\x64\xf6\xec\x8c\x8d\xff\x7b\xa6\x83\x10\x22\x95\xe1\x64\x09",
            "FDUSD",
            18,
            "First Digital USD",
        )
        yield (  # address, symbol, decimals, name
            b"\x03\x1b\x41\xe5\x04\x67\x78\x79\x37\x0e\x9d\xbc\xf9\x37\x28\x3a\x86\x91\xfa\x7f",
            "FET",
            18,
            "Fetch",
        )
        yield (  # address, symbol, decimals, name
            b"\xfb\x5b\x83\x8b\x6c\xfe\xed\xc2\x87\x3a\xb2\x78\x66\x07\x9a\xc5\x53\x63\xd3\x7e",
            "FLOKI",
            9,
            "FLOKI",
        )
        yield (  # address, symbol, decimals, name
            b"\x10\x45\x97\x1c\x16\x8b\x52\x94\xac\xbc\x87\x27\xa4\xf1\xc9\xe1\xaf\x99\xf6\xd0",
            "FTN",
            18,
            "Bridged FTN (OrtakSea)",
        )
        yield (  # address, symbol, decimals, name
            b"\x44\xf1\x61\xae\x29\x36\x1e\x33\x2d\xea\x03\x9d\xfa\x2f\x40\x4e\x0b\xc5\xb5\xcc",
            "H",
            18,
            "Humanity",
        )
        yield (  # address, symbol, decimals, name
            b"\x61\xec\x85\xab\x89\x37\x7d\xb6\x57\x62\xe2\x34\xc9\x46\xb5\xc2\x5a\x56\xe9\x9e",
            "HTX",
            18,
            "HTX",
        )
        yield (  # address, symbol, decimals, name
            b"\xa2\xb7\x26\xb1\x14\x5a\x47\x73\xf6\x85\x93\xcf\x17\x11\x87\xd8\xeb\xe4\xd4\x95",
            "INJ",
            18,
            "Injective Protocol",
        )
        yield (  # address, symbol, decimals, name
            b"\xec\xac\x9c\x5f\x70\x4e\x95\x49\x31\x34\x9d\xa3\x7f\x60\xe3\x9f\x51\x5c\x11\xc1",
            "LBTC",
            8,
            "Lombard Staked Bitcoin",
        )
        yield (  # address, symbol, decimals, name
            b"\xf8\xa0\xbf\x9c\xf5\x4b\xb9\x2f\x17\x37\x4d\x9e\x9a\x32\x1e\x6a\x11\x1a\x51\xbd",
            "LINK",
            18,
            "ChainLink Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x43\x38\x66\x5c\xbb\x7b\x24\x85\xa8\x85\x5a\x13\x9b\x75\xd5\xe3\x4a\xb0\xdb\x94",
            "LTC",
            18,
            "Litecoin Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x22\xb1\x45\x8e\x78\x0f\x8f\xa7\x1e\x2f\x84\x50\x2c\xee\x8b\x5a\x3c\xc7\x31\xfa",
            "M",
            18,
            "MemeCore",
        )
        yield (  # address, symbol, decimals, name
            b"\xd8\x25\x44\xbf\x0d\xfe\x83\x85\xef\x8f\xa3\x4d\x67\xe6\xe4\x94\x0c\xc6\x3e\x16",
            "MYX",
            18,
            "MYX",
        )
        yield (  # address, symbol, decimals, name
            b"\xb3\xed\x0a\x42\x61\x55\xb7\x9b\x89\x88\x49\x80\x3e\x3b\x36\x55\x2f\x7e\xd5\x07",
            "PENDLE",
            18,
            "Pendle",
        )
        yield (  # address, symbol, decimals, name
            b"\x64\x18\xc0\xdd\x09\x9a\x9f\xda\x39\x7c\x76\x63\x04\xcd\xd9\x18\x23\x3e\x88\x47",
            "PENGU",
            18,
            "Pudgy Penguins",
        )
        yield (  # address, symbol, decimals, name
            b"\x25\xd8\x87\xce\x7a\x35\x17\x2c\x62\xfe\xbf\xd6\x7a\x18\x56\xf2\x0f\xae\xbb\x00",
            "PEPE",
            18,
            "Pepe",
        )
        yield (  # address, symbol, decimals, name
            b"\x4a\xae\x82\x3a\x6a\x0b\x37\x6d\xe6\xa7\x8e\x74\xec\xc5\xb0\x79\xd3\x8c\xbc\xf7",
            "SolvBTC",
            18,
            "Solv BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x21\x1c\xc4\xdd\x07\x37\x34\xda\x05\x5f\xbf\x44\xa2\xb4\x66\x7d\x5e\x5f\xe5\xd2",
            "sUSDe",
            18,
            "Staked USDe",
        )
        yield (  # address, symbol, decimals, name
            b"\x76\xa7\x97\xa5\x9b\xa2\xc1\x77\x26\x89\x69\x76\xb7\xb3\x74\x7b\xfd\x1d\x22\x0f",
            "TONCOIN",
            9,
            "Wrapped TON Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\x40\xaf\x38\x27\xf3\x9d\x0e\xac\xbf\x4a\x16\x8f\x8d\x4e\xe6\x7c\x12\x1d\x11\xc9",
            "TUSD",
            18,
            "TrueUSD",
        )
        yield (  # address, symbol, decimals, name
            b"\x4b\x0f\x18\x12\xe5\xdf\x2a\x09\x79\x64\x81\xff\x14\x01\x7e\x60\x05\x50\x80\x03",
            "TWT",
            18,
            "Trust Wallet",
        )
        yield (  # address, symbol, decimals, name
            b"\xbf\x51\x40\xa2\x25\x78\x16\x8f\xd5\x62\xdc\xcf\x23\x5e\x5d\x43\xa0\x2c\xe9\xb1",
            "UNI",
            18,
            "Uniswap",
        )
        yield (  # address, symbol, decimals, name
            b"\x75\x8a\x3e\x0b\x1f\x84\x2c\x93\x06\xb7\x83\xf8\xa4\x07\x8c\x6c\x8c\x03\xa2\x70",
            "USD0",
            18,
            "Usual USD",
        )
        yield (  # address, symbol, decimals, name
            b"\x8d\x0d\x00\x0e\xe4\x49\x48\xfc\x98\xc9\xb9\x8a\x4f\xa4\x92\x14\x76\xf0\x8b\x0d",
            "USD1",
            18,
            "World Liberty Financial USD",
        )
        yield (  # address, symbol, decimals, name
            b"\x8a\xc7\x6a\x51\xcc\x95\x0d\x98\x22\xd6\x8b\x83\xfe\x1a\xd9\x7b\x32\xcd\x58\x0d",
            "USDC",
            18,
            "USD Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\x39\x20\x04\xbe\xe2\x13\xf1\xff\x58\x0c\x86\x73\x59\xc2\x46\x92\x4f\x21\xe6\xad",
            "USDD",
            18,
            "Decentralized USD",
        )
        yield (  # address, symbol, decimals, name
            b"\x5d\x3a\x1f\xf2\xb6\xba\xb8\x3b\x63\xcd\x9a\xd0\x78\x70\x74\x08\x1a\x52\xef\x34",
            "USDe",
            18,
            "USDe",
        )
        yield (  # address, symbol, decimals, name
            b"\xb3\xb0\x2e\x4a\x9f\xb2\xbd\x28\xcc\x2f\xf9\x7b\x0a\xb3\xf6\xb3\xec\x1e\xe9\xd2",
            "USDf",
            18,
            "Falcon USD",
        )
        yield (  # address, symbol, decimals, name
            b"\x55\xd3\x98\x32\x6f\x99\x05\x9f\xf7\x75\x48\x52\x46\x99\x90\x27\xb3\x19\x79\x55",
            "USDT",
            18,
            "Tether USD",
        )
        yield (  # address, symbol, decimals, name
            b"\xf3\x52\x7e\xf8\xde\x26\x5e\xaa\x37\x16\xfb\x31\x2c\x12\x84\x7b\xfb\xa6\x6c\xef",
            "USDX",
            18,
            "USDX",
        )
        yield (  # address, symbol, decimals, name
            b"\xa2\xe3\x35\x66\x10\x84\x07\x01\xbd\xf5\x61\x1a\x53\x97\x45\x10\xae\x27\xe2\xe1",
            "wBETH",
            18,
            "Wrapped Binance Beacon ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\xbb\x4c\xdb\x9c\xbd\x36\xb0\x1b\xd1\xcb\xae\xbf\x2d\xe0\x8d\x91\x73\xbc\x09\x5c",
            "WBNB",
            18,
            "Wrapped BNB",
        )
        yield (  # address, symbol, decimals, name
            b"\x05\x55\xe3\x0d\xa8\xf9\x83\x08\xed\xb9\x60\xaa\x94\xc0\xdb\x47\x23\x0d\x2b\x9c",
            "WBTC",
            8,
            "Wrapped BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x04\xc0\x59\x9a\xe5\xa4\x47\x57\xc0\xaf\x6f\x9e\xc3\xb9\x3d\xa8\x97\x6c\x15\x0a",
            "weETH",
            18,
            "Wrapped eETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x47\x47\x47\x47\x47\x7b\x19\x92\x88\xbf\x72\xa1\xd7\x02\xf7\xfe\x0f\xb1\xde\xea",
            "WLFI",
            18,
            "World Liberty Financial",
        )
        yield (  # address, symbol, decimals, name
            b"\x40\x5f\xbc\x90\x04\xd8\x57\x90\x3b\xfd\x6b\x33\x57\x79\x2d\x71\xa5\x07\x26\xb0",
            "XPL",
            18,
            "Plasma",
        )
    if chain_id == 66:  # okexchain
        yield (  # address, symbol, decimals, name
            b"\x77\xdf\x6e\xbe\xc3\x31\x67\x92\xd4\xea\x5b\xc0\xf8\x28\x6c\x27\x90\x5a\xa8\xe8",
            "AUCTIONK",
            18,
            "AUCTIONK",
        )
        yield (  # address, symbol, decimals, name
            b"\x18\xd1\x03\xb7\x06\x6a\xee\xdb\x60\x05\xb7\x8a\x46\x2e\xf9\x02\x73\x29\xb1\xea",
            "BCHK",
            18,
            "BCHK",
        )
        yield (  # address, symbol, decimals, name
            b"\x54\xe4\x62\x2d\xc5\x04\x17\x6b\x3b\xb4\x32\xdc\xca\xf5\x04\x56\x96\x99\xa7\xff",
            "BTCK",
            18,
            "BTCK",
        )
        yield (  # address, symbol, decimals, name
            b"\x33\x27\x30\xa4\xf6\xe0\x3d\x9c\x55\x82\x94\x35\xf1\x03\x60\xe1\x3c\xfa\x41\xff",
            "BUSD",
            18,
            "Binance-Peg BUSD Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x81\x79\xd9\x7e\xb6\x48\x88\x60\xd8\x16\xe3\xec\xaf\xe6\x94\xa4\x15\x3f\x21\x6c",
            "CHE",
            18,
            "CherrySwap Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x21\xcd\xe7\xe3\x2a\x6c\xaf\x47\x42\xd0\x0d\x44\xb0\x72\x79\xe7\x59\x6d\x26\xb9",
            "DAIK",
            18,
            "DAIK",
        )
        yield (  # address, symbol, decimals, name
            b"\x99\x97\x07\x78\xe2\x71\x5b\xbc\x9c\xf8\xfb\x83\xd1\x0d\xcb\xc2\xd2\xd5\x51\xa3",
            "ETCK",
            18,
            "ETCK",
        )
        yield (  # address, symbol, decimals, name
            b"\xef\x71\xca\x2e\xe6\x8f\x45\xb9\xad\x6f\x72\xfb\xdb\x33\xd7\x07\xb8\x72\x31\x5c",
            "ETHK",
            18,
            "ETHK",
        )
        yield (  # address, symbol, decimals, name
            b"\x3f\x89\x69\xbe\x2f\xc0\x77\x0d\xcc\x17\x49\x68\xe4\xb4\xff\x14\x6e\x0a\xcd\xaf",
            "FILK",
            18,
            "FILK",
        )
        yield (  # address, symbol, decimals, name
            b"\xd0\xc6\x82\x1a\xba\x4f\xcc\x65\xe8\xf1\x54\x25\x89\xe6\x4b\xae\x9d\xe1\x12\x28",
            "FLUXK",
            18,
            "Flux Protocol",
        )
        yield (  # address, symbol, decimals, name
            b"\xc0\x57\x60\xd7\x5e\x7f\x5a\xd4\x28\xa9\x06\x67\x4c\xe7\xc7\xc8\x2d\x00\x3d\x01",
            "KINEK",
            18,
            "KINEK",
        )
        yield (  # address, symbol, decimals, name
            b"\xfa\x52\x0e\xfc\x34\xc8\x1b\xfc\x1e\x3d\xd4\x8b\x7f\xe9\xff\x32\x60\x49\xf9\x86",
            "LTCK",
            18,
            "LTCK",
        )
        yield (  # address, symbol, decimals, name
            b"\xdf\x54\xb6\xc6\x19\x5e\xa4\xd9\x48\xd0\x3b\xfd\x81\x8d\x36\x5c\xf1\x75\xcf\xc2",
            "OKB",
            18,
            "OKB",
        )
        yield (  # address, symbol, decimals, name
            b"\x32\x12\x60\x6f\x74\xcc\x59\x65\x6e\x1e\xc6\xf5\x87\xfc\xa6\x1b\xa3\xb8\x5e\xb0",
            "SFGK",
            18,
            "SFGK",
        )
        yield (  # address, symbol, decimals, name
            b"\xaa\x27\xba\xda\xa3\xc9\xec\x90\x81\xb8\xf6\xc9\xcd\xd2\xbf\x37\x5f\x14\x37\x80",
            "SHIBK",
            18,
            "SHIBK",
        )
        yield (  # address, symbol, decimals, name
            b"\x22\x18\xe0\xd5\xe0\x17\x37\x69\xf5\xb4\x93\x9a\x3a\xe4\x23\xf7\xe5\xe4\xea\xb7",
            "SUSHIK",
            18,
            "SUSHIK",
        )
        yield (  # address, symbol, decimals, name
            b"\x59\xd2\x26\xbb\x0a\x4d\x74\x27\x4d\x43\x54\xeb\xb6\xa0\xe1\xa1\xaa\x51\x75\xb6",
            "UNIK",
            18,
            "UNIK",
        )
        yield (  # address, symbol, decimals, name
            b"\xc9\x46\xda\xf8\x1b\x08\x14\x6b\x1c\x7a\x8d\xa2\xa8\x51\xdd\xf2\xb3\xea\xaf\x85",
            "USDC",
            18,
            "USD Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\xdc\xac\x52\xe0\x01\xf5\xbd\x41\x3a\xa6\xea\x83\x95\x64\x38\xf2\x90\x98\x16\x6b",
            "USDK",
            18,
            "USDK",
        )
        yield (  # address, symbol, decimals, name
            b"\x38\x2b\xb3\x69\xd3\x43\x12\x5b\xfb\x21\x17\xaf\x9c\x14\x97\x95\xc6\xc6\x5c\x50",
            "USDT",
            18,
            "Tether",
        )
        yield (  # address, symbol, decimals, name
            b"\x8f\x85\x26\xdb\xfd\x6e\x38\xe3\xd8\x30\x77\x02\xca\x84\x69\xba\xe6\xc5\x6c\x15",
            "WOKT",
            18,
            "Wrapped OKT",
        )
        yield (  # address, symbol, decimals, name
            b"\xcd\x08\xd3\x21\xf6\xbc\x10\xa1\x0f\x09\x4e\x4b\x2e\x6c\x9b\x8b\xf9\x90\x64\x01",
            "ZKSK",
            18,
            "ZKSK",
        )
    if chain_id == 100:  # Gnosis
        yield (  # address, symbol, decimals, name
            b"\xd3\xd4\x7d\x55\x78\xe5\x5c\x88\x05\x05\xdc\x40\x64\x8f\x7f\x93\x07\xc3\xe7\xa8",
            "DPI",
            18,
            "DefiPulse Index on xDai",
        )
        yield (  # address, symbol, decimals, name
            b"\x9c\x58\xba\xcc\x33\x1c\x9a\xa8\x71\xaf\xd8\x02\xdb\x63\x79\xa9\x8e\x80\xce\xdb",
            "GNO",
            18,
            "Gnosis Token on xDai",
        )
        yield (  # address, symbol, decimals, name
            b"\xe2\xe7\x3a\x1c\x69\xec\xf8\x3f\x46\x4e\xfc\xe6\xa5\xbe\x35\x3a\x37\xca\x09\xb2",
            "LINK",
            18,
            "ChainLink Token on xDai",
        )
        yield (  # address, symbol, decimals, name
            b"\xfa\x57\xaa\x7b\xee\xd6\x3d\x03\xaa\xf8\x5f\xfd\x17\x53\xf5\xf6\x24\x25\x88\xfb",
            "MPS",
            0,
            "MtPelerin Shares",
        )
        yield (  # address, symbol, decimals, name
            b"\xb7\xd3\x11\xe2\xeb\x55\xf2\xf6\x8a\x94\x40\xda\x38\xe7\x98\x92\x10\xb9\xa0\x5e",
            "STAKE",
            18,
            "STAKE on xDai",
        )
        yield (  # address, symbol, decimals, name
            b"\xdd\xaf\xbb\x50\x5a\xd2\x14\xd7\xb8\x0b\x1f\x83\x0f\xcc\xc8\x9b\x60\xfb\x7a\x83",
            "USDC",
            6,
            "USD Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\x8e\x5b\xbb\xb0\x9e\xd1\xeb\xde\x86\x74\xcd\xa3\x9a\x0c\x16\x94\x01\xdb\x42\x52",
            "WBTC",
            8,
            "Wrapped BTC on xDai",
        )
        yield (  # address, symbol, decimals, name
            b"\x6a\x02\x3c\xcd\x1f\xf6\xf2\x04\x5c\x33\x09\x76\x8e\xad\x9e\x68\xf9\x78\xf6\xe1",
            "WETH",
            18,
            "Wrapped Ether on xDai",
        )
        yield (  # address, symbol, decimals, name
            b"\xe9\x1d\x15\x3e\x0b\x41\x51\x8a\x2c\xe8\xdd\x3d\x79\x44\xfa\x86\x34\x63\xa9\x7d",
            "WXDAI",
            18,
            "Wrapped XDAI",
        )
    if chain_id == 128:  # Heco
        yield (  # address, symbol, decimals, name
            b"\x20\x2b\x49\x36\xfe\x1a\x82\xa4\x96\x52\x20\x86\x0a\xe4\x6d\x7d\x39\x39\xbb\x25",
            "AAVE",
            18,
            "Heco-Peg AAVE Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xa0\x42\xfb\x0e\x60\x12\x5a\x40\x22\x67\x00\x14\xac\x12\x19\x31\xe7\x50\x1a\xf4",
            "BAG",
            18,
            "BAG",
        )
        yield (  # address, symbol, decimals, name
            b"\x04\x5d\xe1\x5c\xa7\x6e\x76\x42\x6e\x8f\xc7\xcb\xa8\x39\x2a\x31\x38\x07\x8d\x0f",
            "BAL",
            18,
            "Heco-Peg BAL Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xb1\xf8\x08\x44\xa1\xb8\x4c\x61\xab\x80\xca\xfd\x88\xb1\xf8\xc0\x9f\x93\x42\xe1",
            "BEE",
            8,
            "BEE",
        )
        yield (  # address, symbol, decimals, name
            b"\xb6\xf4\xc4\x18\x51\x4d\xd4\x68\x0f\x76\xd5\xca\xa3\xbb\x42\xdb\x4a\x89\x3a\xcb",
            "BETH",
            18,
            "Heco-Peg BETH Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x4f\x99\xd1\x0e\x16\x97\x2f\xf2\xfe\x31\x5e\xee\x53\xa9\x5f\xc5\xa5\x87\x0c\xe3",
            "BNB",
            18,
            "Poly-Peg BNB",
        )
        yield (  # address, symbol, decimals, name
            b"\x1e\x63\x95\xe6\xb0\x59\xfc\x97\xa4\xdd\xa9\x25\xb6\xc5\xeb\xf1\x9e\x05\xc6\x9f",
            "CAN",
            18,
            "Channels",
        )
        yield (  # address, symbol, decimals, name
            b"\x24\xab\x27\xa7\x27\x4d\xe0\xba\x57\x60\xba\xb8\x04\xfe\x87\x0b\xb5\x72\xc5\x10",
            "CETF",
            18,
            "CellETF",
        )
        yield (  # address, symbol, decimals, name
            b"\xee\xf1\x32\x43\x43\xca\x7b\xf6\xe7\x43\xe2\x1d\xd9\xe9\x2d\xfa\x4e\xfc\x3a\x56",
            "CON",
            18,
            "CON",
        )
        yield (  # address, symbol, decimals, name
            b"\x74\x18\x98\x62\xb0\x69\xe2\xbe\x5f\x7c\x8e\x6f\xf0\x8e\xa8\xe1\xb1\x94\x85\x19",
            "COOK",
            18,
            "Poly-Peg COOK",
        )
        yield (  # address, symbol, decimals, name
            b"\x3d\x76\x0a\x45\xd0\x88\x7d\xfd\x89\xa2\xf5\x38\x5a\x23\x6b\x29\xcb\x46\xed\x2a",
            "DAI-HECO",
            18,
            "Heco-Peg DAIHECO Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x48\xc8\x59\x53\x12\x54\xf2\x5e\x57\xd1\xc1\xa8\xe0\x30\xef\x0b\x1c\x89\x5c\x27",
            "DEP",
            18,
            "Depth Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x57\xa7\xbc\xdf\xab\x16\x31\xac\xa9\xd6\xe0\xf3\x99\x59\x47\x71\x82\xcf\xae\x12",
            "DMT",
            18,
            "DMT token",
        )
        yield (  # address, symbol, decimals, name
            b"\x09\x96\x26\x78\x38\x42\xd3\x5c\x22\x1e\x5d\x01\x69\x4c\x2b\x92\x8e\xb3\xb0\xad",
            "DOG",
            18,
            "DOG",
        )
        yield (  # address, symbol, decimals, name
            b"\xa1\xec\xfc\x2b\xec\x06\xe4\xb4\x3d\xdd\x42\x3b\x94\xfe\xf8\x4d\x0d\xbc\x8f\x5c",
            "ELA",
            18,
            "ELA on HuobiChain",
        )
        yield (  # address, symbol, decimals, name
            b"\x64\xff\x63\x7f\xb4\x78\x86\x3b\x74\x68\xbc\x97\xd3\x0a\x5b\xf3\xa4\x28\xa1\xfd",
            "ETH",
            18,
            "Heco-Peg ETH Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x91\x4b\x63\x30\x38\xf3\x6d\x03\xfe\xf5\xaf\x7f\x12\xe5\x19\x87\x95\x76\x77\x1a",
            "FCN",
            18,
            "FEICHANG NIU",
        )
        yield (  # address, symbol, decimals, name
            b"\xe3\x6f\xfd\x17\xb2\x66\x1e\xb5\x71\x44\xce\xae\xf9\x42\xd9\x52\x95\xe6\x37\xf0",
            "FILDA",
            18,
            "FilDA on Heco",
        )
        yield (  # address, symbol, decimals, name
            b"\x2a\xaf\xe3\xc9\x11\x8d\xb3\x6a\x20\xdd\x4a\x94\x2b\x6f\xf3\xe7\x89\x81\xdc\xe1",
            "GOF",
            18,
            "Heco-Peg GOF Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x89\x4b\x29\x17\xc7\x83\x51\x4c\x9e\x4c\x24\x22\x9b\xf6\x0f\x3c\xb4\xc9\xc9\x05",
            "HBC",
            18,
            "Heco-Peg HBC Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xef\x3c\xeb\xd7\x7e\x0c\x52\xcb\x6f\x60\x87\x5d\x93\x06\x39\x7b\x5c\xac\xa3\x75",
            "HBCH",
            18,
            "Heco-Peg HBCH Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x87\x64\xbd\x4f\xcc\x02\x7f\xaf\x72\xba\x83\xc0\xb2\x02\x8a\x3b\xae\x0d\x2d\x57",
            "HBO",
            18,
            "Hash Bridge Oracle",
        )
        yield (  # address, symbol, decimals, name
            b"\xc2\xcb\x6b\x53\x57\xcc\xce\x1b\x99\xcd\x22\x23\x29\x42\xd9\xa2\x25\xea\x4e\xb1",
            "HBSV",
            18,
            "Heco-Peg HBSV Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x66\xa7\x9d\x23\xe5\x84\x75\xd2\x73\x81\x79\xca\x52\xcd\x0b\x41\xd7\x3f\x0b\xea",
            "HBTC",
            18,
            "Heco-Peg HBTC Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xa2\xc4\x9c\xee\x16\xa5\xe5\xbd\xef\xde\x93\x11\x07\xdc\x1f\xae\x9f\x77\x73\xe3",
            "HDOT",
            18,
            "Heco-Peg HDOT Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xae\x3a\x76\x8f\x9a\xb1\x04\xc6\x9a\x7c\xd6\x04\x1f\xe1\x6f\xfa\x23\x5d\x18\x10",
            "HFIL",
            18,
            "Heco-Peg HFIL Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xd1\x08\x52\xdf\x03\xea\x8b\x8a\xf0\xcc\x0b\x09\xca\xc3\xf7\xdb\xb1\x5e\x04\x33",
            "hFLUX",
            18,
            "Flux Protocol",
        )
        yield (  # address, symbol, decimals, name
            b"\xec\xb5\x6c\xf7\x72\xb5\xc9\xa6\x90\x7f\xb7\xd3\x23\x87\xda\x2f\xcb\xfb\x63\xb4",
            "HLTC",
            18,
            "Heco-Peg HLTC Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xe4\x99\xef\x46\x16\x99\x37\x30\xce\xd0\xf3\x1f\xa2\x70\x3b\x92\xb5\x0b\xb5\x36",
            "HPT",
            18,
            "Heco-Peg HPT Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x80\xc6\x6d\x46\x0e\x2b\xb9\xd3\x1a\x8d\xe5\x4b\x40\x16\xfc\xa9\x86\xd0\x81\x1f",
            "HTM",
            18,
            "\u706b\u5e01\u751f\u6001\u9690\u79c1\u62d3\u5c55\u94fe",
        )
        yield (  # address, symbol, decimals, name
            b"\x02\x98\xc2\xb3\x2e\xae\x4d\xa0\x02\xa1\x5f\x36\xfd\xf7\x61\x5b\xea\x3d\xa0\x47",
            "HUSD",
            8,
            "Heco-Peg HUSD Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x45\xe9\x7d\xad\x82\x8a\xd7\x35\xaf\x1d\xf0\x47\x3f\xc2\x73\x5f\x0f\xd5\x33\x0c",
            "HXTZ",
            18,
            "Heco-Peg HXTZ Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xe1\x31\xf0\x48\xd8\x5f\x03\x91\xa2\x44\x35\xee\xfb\x77\x63\x19\x9b\x58\x7d\x0e",
            "LAMB",
            18,
            "Heco-Peg LAMB Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x8f\x67\x85\x44\x97\x21\x80\x43\xe1\xf7\x29\x08\xff\xe3\x8d\x0e\xd7\xf2\x47\x21",
            "LHB",
            18,
            "LendHub",
        )
        yield (  # address, symbol, decimals, name
            b"\x9e\x00\x45\x45\xc5\x9d\x35\x9f\x6b\x7b\xfb\x06\xa2\x63\x90\xb0\x87\x71\x7b\x42",
            "LINK",
            18,
            "Heco-Peg LINK Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x8b\x70\x51\x2b\x52\x48\xe7\xc1\xf0\xf6\x99\x6e\x2f\xde\x2e\x95\x27\x08\xc4\xc9",
            "NT",
            18,
            "NEXTYPE",
        )
        yield (  # address, symbol, decimals, name
            b"\xaa\xae\x74\x6b\x5e\x55\xd1\x43\x98\x87\x93\x12\x66\x0e\x9f\xde\x07\xfb\xc1\xdc",
            "PIPI",
            18,
            "Pippi Shrimp Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x52\xee\x54\xdd\x7a\x68\xe9\xcf\x13\x1b\x0a\x57\xfd\x60\x15\xc7\x4d\x71\x40\xe2",
            "PTD",
            18,
            "P.TD",
        )
        yield (  # address, symbol, decimals, name
            b"\xb6\xcc\xfa\x7e\xf3\xa2\x95\x93\x25\x36\xe0\x98\x8c\xff\xd8\x52\x28\xcb\x17\x7c",
            "sCASH",
            18,
            "sCASH",
        )
        yield (  # address, symbol, decimals, name
            b"\x77\x78\x50\x28\x17\x19\xd5\xa9\x6c\x29\x81\x2a\xb7\x2f\x82\x2e\x0e\x09\xf3\xda",
            "SNX",
            18,
            "Heco-Peg SNX Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x49\xe1\x65\x63\xa2\xba\x84\xe5\x60\x78\x09\x46\xf0\xfb\x73\xa8\xb3\x2c\x84\x1e",
            "SOVI",
            18,
            "Sovi Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x32\x9d\xda\x64\xcb\xc4\xdf\xd5\xfa\x50\x72\xb4\x47\xb3\x94\x1c\xe0\x54\xeb\xb3",
            "SWFTC",
            8,
            "Heco-Peg SWFTC Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xae\x88\xbe\xdc\xed\xac\xa5\x6a\x3d\x9a\x56\xb6\x56\x80\x44\x60\xca\x16\xad\x86",
            "TLOD",
            18,
            "The Legend of Deification",
        )
        yield (  # address, symbol, decimals, name
            b"\x22\xc5\x4c\xe8\x32\x1a\x40\x15\x74\x0e\xe1\x10\x9d\x9c\xbc\x25\x81\x5c\x46\xe6",
            "UNI",
            18,
            "Heco-Peg UNI Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x93\x62\xbb\xef\x4b\x83\x13\xa8\xaa\x9f\x0c\x98\x08\xb8\x05\x77\xaa\x26\xb7\x3b",
            "USDC-HECO",
            6,
            "Heco-Peg USDCHECO Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xa7\x1e\xdc\x38\xd1\x89\x76\x75\x82\xc3\x8a\x31\x45\xb5\x87\x30\x52\xc3\xe4\x7a",
            "USDT",
            18,
            "Tether",
        )
        yield (  # address, symbol, decimals, name
            b"\x55\x45\x15\x3c\xcf\xca\x01\xfb\xd7\xdd\x11\xc0\xb2\x3b\xa6\x94\xd9\x50\x9a\x6f",
            "WHT",
            18,
            "Wrapped HT",
        )
        yield (  # address, symbol, decimals, name
            b"\xe0\xfe\x25\xee\xfc\xfc\xad\xde\xf8\x44\xfe\x30\xb8\xbe\x1d\x68\xac\x6b\x7a\xf3",
            "XF",
            18,
            "xFarmer",
        )
        yield (  # address, symbol, decimals, name
            b"\xe5\x94\x4b\x50\xdf\x84\x00\x1a\x36\xc7\xde\x0d\x5c\xb4\xda\x7a\xb2\x14\x07\xd2",
            "XNFT",
            18,
            "XNFT",
        )
        yield (  # address, symbol, decimals, name
            b"\xb4\xf0\x19\xbe\xac\x75\x8a\xbb\xee\x2f\x90\x60\x33\xaa\xa2\xf0\xf6\xda\xcb\x35",
            "YFI",
            18,
            "Heco-Peg YFI Token",
        )
    if chain_id == 137:  # Polygon
        yield (  # address, symbol, decimals, name
            b"\xd6\xdf\x93\x2a\x45\xc0\xf2\x55\xf8\x51\x45\xf2\x86\xea\x0b\x29\x2b\x21\xc9\x0b",
            "AAVE",
            18,
            "Aave (PoS)",
        )
        yield (  # address, symbol, decimals, name
            b"\xfc\xe6\x0b\xbc\x52\xa5\x70\x5c\xec\x5b\x44\x55\x01\xfb\xaf\x32\x74\xdc\x43\xd0",
            "ACRED",
            6,
            "Apollo Diversified Credit Securitize Fund",
        )
        yield (  # address, symbol, decimals, name
            b"\xa3\xf7\x51\x66\x2e\x28\x2e\x83\xec\x3c\xbc\x38\x7d\x22\x5c\xa5\x6d\xd6\x3d\x3a",
            "APEPE",
            18,
            "Ape and Pepe",
        )
        yield (  # address, symbol, decimals, name
            b"\x00\x00\x00\x00\xef\xe3\x02\xbe\xaa\x2b\x3e\x6e\x1b\x18\xd0\x8d\x69\xa9\x01\x2a",
            "AUSD",
            6,
            "AUSD",
        )
        yield (  # address, symbol, decimals, name
            b"\x2c\x89\xbb\xc9\x2b\xd8\x6f\x80\x75\xd1\xde\xcc\x58\xc7\xf4\xe0\x10\x7f\x28\x6b",
            "AVAX",
            18,
            "Avalanche Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x6e\x4e\x62\x41\x06\xcb\x12\xe1\x68\xe6\x53\x3f\x8e\xc7\xc8\x22\x63\x35\x89\x40",
            "AXL",
            6,
            "Axelar",
        )
        yield (  # address, symbol, decimals, name
            b"\x3c\xef\x98\xbb\x43\xd7\x32\xe2\xf2\x85\xee\x60\x5a\x81\x58\xcd\xe9\x67\xd2\x19",
            "BAT",
            18,
            "Basic Attention Token (PoS)",
        )
        yield (  # address, symbol, decimals, name
            b"\xe5\xb4\x98\x20\xe5\xa1\x06\x3f\x6f\x4d\xdf\x85\x13\x27\xb5\xe8\xb2\x30\x10\x48",
            "Bonk",
            5,
            "Bonk",
        )
        yield (  # address, symbol, decimals, name
            b"\x85\x05\xb9\xd2\x25\x4a\x7a\xe4\x68\xc0\xe9\xdd\x10\xcc\xea\x3a\x83\x7a\xef\x5c",
            "COMP",
            18,
            "(PoS) Compound",
        )
        yield (  # address, symbol, decimals, name
            b"\x2f\x4e\xfd\x3a\xa4\x2e\x15\xa1\xec\x61\x14\x54\x71\x51\xb6\x3e\xe5\xd3\x99\x58",
            "COW",
            18,
            "CoW Protocol Token(PoS)",
        )
        yield (  # address, symbol, decimals, name
            b"\x17\x23\x70\xd5\xcd\x63\x27\x9e\xfa\x6d\x50\x2d\xab\x29\x17\x19\x33\xa6\x10\xaf",
            "CRV",
            18,
            "CRV (PoS)",
        )
        yield (  # address, symbol, decimals, name
            b"\xc4\xce\x1d\x6f\x5d\x98\xd6\x5e\xe2\x5c\xf8\x5e\x9f\x2e\x9d\xcf\xee\x6c\xb5\xd6",
            "crvUSD",
            18,
            "Curve.Fi USD Stablecoin(PoS)",
        )
        yield (  # address, symbol, decimals, name
            b"\x8f\x3c\xf7\xad\x23\xcd\x3c\xad\xbd\x97\x35\xaf\xf9\x58\x02\x32\x39\xc6\xa0\x63",
            "DAI",
            18,
            "(PoS) Dai Stablecoin",
        )
        yield (  # address, symbol, decimals, name
            b"\xe1\x11\x17\x8a\x87\xa3\xbf\xf0\xc8\xd1\x8d\xec\xba\x57\x98\x82\x75\x39\xae\x99",
            "EURS",
            2,
            "STASIS EURS Token (PoS)",
        )
        yield (  # address, symbol, decimals, name
            b"\xa0\x76\x9f\x7a\x8f\xc6\x5e\x47\xde\x93\x79\x7b\x4e\x21\xc0\x73\xc1\x17\xfc\x80",
            "EUTBL",
            5,
            "Spiko EU T-Bills Money Market Fund",
        )
        yield (  # address, symbol, decimals, name
            b"\x45\xc3\x2f\xa6\xdf\x82\xea\xd1\xe2\xef\x74\xd1\x7b\x76\x54\x7e\xdd\xfa\xff\x89",
            "FRAX",
            18,
            "Frax",
        )
        yield (  # address, symbol, decimals, name
            b"\xee\x32\x7f\x88\x9d\x59\x47\xc1\xdc\x19\x34\xbb\x20\x8a\x1e\x79\x2f\x95\x3e\x96",
            "frxETH",
            18,
            "Frax Ether",
        )
        yield (  # address, symbol, decimals, name
            b"\x80\xee\xde\x49\x66\x55\xfb\x90\x47\xdd\x39\xd9\xf4\x18\xd5\x48\x3e\xd6\x00\xdf",
            "frxUSD",
            18,
            "Frax USD",
        )
        yield (  # address, symbol, decimals, name
            b"\x4b\x43\x27\xdb\x16\x00\xb8\xb1\x44\x01\x63\xf6\x67\xe1\x99\xce\xf3\x53\x85\xf5",
            "fxcbETH",
            18,
            "Coinbase Wrapped Staked ETH (FXERC20)",
        )
        yield (  # address, symbol, decimals, name
            b"\x1a\x3a\xcf\x6d\x19\x26\x7e\x2d\x3e\x7f\x89\x8f\x42\x80\x3e\x90\xc9\x21\x90\x62",
            "FXS",
            18,
            "Frax Share",
        )
        yield (  # address, symbol, decimals, name
            b"\x5f\xe2\xb5\x8c\x01\x3d\x76\x01\x14\x7d\xcd\xd6\x8c\x14\x3a\x77\x49\x9f\x55\x31",
            "GRT",
            18,
            "Graph Token (PoS)",
        )
        yield (  # address, symbol, decimals, name
            b"\xf5\x0d\x05\xa1\x40\x2d\x0a\xda\xfa\x88\x0d\x36\x05\x07\x36\xf9\xf6\xee\x7d\xee",
            "INST",
            18,
            "Instadapp (PoS)",
        )
        yield (  # address, symbol, decimals, name
            b"\xc3\xc7\xd4\x22\x80\x98\x52\x03\x1b\x44\xab\x29\xee\xc9\xf1\xef\xf2\xa5\x87\x56",
            "LDO",
            18,
            "Lido DAO Token (PoS)",
        )
        yield (  # address, symbol, decimals, name
            b"\x53\xe0\xbc\xa3\x5e\xc3\x56\xbd\x5d\xdd\xfe\xbb\xd1\xfc\x0f\xd0\x3f\xab\xad\x39",
            "LINK",
            18,
            "ChainLink Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xa1\xc5\x7f\x48\xf0\xde\xb8\x9f\x56\x9d\xfb\xe6\xe2\xb7\xf4\x6d\x33\x60\x6f\xd4",
            "MANA",
            18,
            "(PoS) Decentraland MANA",
        )
        yield (  # address, symbol, decimals, name
            b"\x41\xb3\x96\x6b\x4f\xf7\xb4\x27\x96\x9d\xdf\x5d\xa3\x62\x7d\x6a\xea\xe9\xa4\x8e",
            "NEXO",
            18,
            "Nexo (PoS)",
        )
        yield (  # address, symbol, decimals, name
            b"\xc3\xec\x80\x34\x3d\x2b\xae\x2f\x8e\x68\x0f\xda\xdd\xe7\xc1\x7e\x71\xe1\x14\xea",
            "OM",
            18,
            "MANTRA DAO (PoS)",
        )
        yield (  # address, symbol, decimals, name
            b"\xba\x11\xc5\xef\xfa\x33\xc4\xd6\xf8\xf5\x93\xcf\xa3\x94\x24\x1c\xfe\x92\x58\x11",
            "OUSG",
            18,
            "Ondo Short-Term U.S. Government Bond Fund",
        )
        yield (  # address, symbol, decimals, name
            b"\x02\x66\xf4\xf0\x8d\x82\x37\x2c\xf0\xfc\xbc\xcc\x0f\xf7\x43\x09\x08\x9c\x74\xd1",
            "rETH",
            18,
            "Rocket Pool ETH (PoS)",
        )
        yield (  # address, symbol, decimals, name
            b"\xbb\xba\x07\x3c\x31\xbf\x03\xb8\xac\xf7\xc2\x8e\xf0\x73\x8d\xec\xf3\x69\x56\x83",
            "SAND",
            18,
            "SAND",
        )
        yield (  # address, symbol, decimals, name
            b"\x6d\x1f\xdb\xb2\x66\xfc\xc0\x9a\x16\xa2\x20\x16\x36\x92\x10\xa1\x5b\xb9\x57\x61",
            "sfrxETH",
            18,
            "Staked Frax Ether",
        )
        yield (  # address, symbol, decimals, name
            b"\x50\xb7\x28\xd8\xd9\x64\xfd\x00\xc2\xd0\xaa\xd8\x17\x18\xb7\x13\x11\xfe\xf6\x8a",
            "SNX",
            18,
            "Synthetix Network Token (PoS)",
        )
        yield (  # address, symbol, decimals, name
            b"\xae\x4e\xfb\xc7\x73\x6f\x96\x39\x82\xaa\xcb\x17\xef\xa3\x7f\xcb\xab\x92\x4c\xb3",
            "SolvBTC",
            18,
            "Solv BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x2f\x6f\x07\xcd\xcf\x35\x88\x94\x4b\xf4\xc4\x2a\xc7\x4f\xf2\x4b\xf5\x6e\x75\x90",
            "STG",
            18,
            "StargateToken",
        )
        yield (  # address, symbol, decimals, name
            b"\xa1\x42\x81\x74\xf5\x16\xf5\x27\xfa\xfd\xd1\x46\xb8\x83\xbb\x44\x28\x68\x27\x37",
            "SUPER",
            18,
            "SuperFarm (PoS)",
        )
        yield (  # address, symbol, decimals, name
            b"\x23\x6a\xa5\x09\x79\xd5\xf3\xde\x3b\xd1\xee\xb4\x0e\x81\x13\x7f\x22\xab\x79\x4b",
            "tBTC",
            18,
            "Polygon tBTC v2",
        )
        yield (  # address, symbol, decimals, name
            b"\xdf\x78\x37\xde\x1f\x2f\xa4\x63\x1d\x71\x6c\xf2\x50\x2f\x8b\x23\x0f\x1d\xcc\x32",
            "TEL",
            2,
            "Telcoin (PoS)",
        )
        yield (  # address, symbol, decimals, name
            b"\xb3\x3e\xaa\xd8\xd9\x22\xb1\x08\x34\x46\xdc\x23\xf6\x10\xc2\x56\x7f\xb5\x18\x0f",
            "UNI",
            18,
            "Uniswap (PoS)",
        )
        yield (  # address, symbol, decimals, name
            b"\x3c\x49\x9c\x54\x2c\xef\x5e\x38\x11\xe1\x19\x2c\xe7\x0d\x8c\xc0\x3d\x5c\x33\x59",
            "USDC",
            6,
            "USD Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\x27\x91\xbc\xa1\xf2\xde\x46\x61\xed\x88\xa3\x0c\x99\xa7\xa9\x44\x9a\xa8\x41\x74",
            "USDC.e",
            6,
            "Bridged USDC",
        )
        yield (  # address, symbol, decimals, name
            b"\xc2\x13\x2d\x05\xd3\x1c\x91\x4a\x87\xc6\x61\x1c\x10\x74\x8a\xeb\x04\xb5\x8e\x8f",
            "USDT",
            6,
            "Tether",
        )
        yield (  # address, symbol, decimals, name
            b"\xe4\x88\x02\x49\x74\x5e\xac\x5f\x1e\xd9\xd8\xf7\xdf\x84\x47\x92\xd5\x60\xe7\x50",
            "USTBL",
            5,
            "Spiko US T-Bills Money Market Fund",
        )
        yield (  # address, symbol, decimals, name
            b"\x8a\x16\xd4\xbf\x8a\x0a\x71\x60\x17\xe8\xd2\x26\x2c\x4a\xc3\x29\x27\x79\x7a\x2f",
            "VCNT",
            18,
            "ViciCoin",
        )
        yield (  # address, symbol, decimals, name
            b"\x1b\xfd\x67\x03\x7b\x42\xcf\x73\xac\xf2\x04\x70\x67\xbd\x4f\x2c\x47\xd9\xbf\xd6",
            "WBTC",
            8,
            "(PoS) Wrapped BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x7c\xeb\x23\xfd\x6b\xc0\xad\xd5\x9e\x62\xac\x25\x57\x82\x70\xcf\xf1\xb9\xf6\x19",
            "WETH",
            18,
            "Wrapped Ether",
        )
        yield (  # address, symbol, decimals, name
            b"\x0d\x50\x0b\x1d\x8e\x8e\xf3\x1e\x21\xc9\x9d\x1d\xb9\xa6\x44\x4d\x3a\xdf\x12\x70",
            "WMATIC",
            18,
            "Wrapped Matic",
        )
        yield (  # address, symbol, decimals, name
            b"\xc9\x9f\x5c\x92\x2d\xae\x05\xb6\xe2\xff\x83\x46\x3c\xe7\x05\xef\x7c\x91\xf0\x77",
            "xSolvBTC",
            18,
            "xSolvBTC",
        )
        yield (  # address, symbol, decimals, name
            b"\xda\x53\x71\x04\xd6\xa5\xed\xd5\x3c\x6f\xbb\xa9\xa8\x98\x70\x8e\x46\x52\x60\xb6",
            "YFI",
            18,
            "(PoS) yearn.finance",
        )
        yield (  # address, symbol, decimals, name
            b"\x7b\xeb\xd2\x26\x15\x4e\x86\x59\x54\xa8\x76\x50\xfa\xef\xa8\xf4\x85\xd3\x60\x81",
            "ZIG",
            18,
            "ZigCoin",
        )
        yield (  # address, symbol, decimals, name
            b"\x69\x85\x88\x4c\x43\x92\xd3\x48\x58\x7b\x19\xcb\x9e\xaa\xf1\x57\xf1\x32\x71\xcd",
            "ZRO",
            18,
            "LayerZero",
        )
    if chain_id == 177:  # HashKey Chain
        yield (  # address, symbol, decimals, name
            b"\xf1\xb5\x0e\xd6\x7a\x9e\x2c\xc9\x4a\xd3\xc4\x77\x77\x9e\x2d\x4c\xbf\xff\x90\x29",
            "USDT",
            6,
            "Tether USD",
        )
        yield (  # address, symbol, decimals, name
            b"\x61\x19\xca\x49\xa7\x9f\x58\x25\xc8\xb3\x45\xf8\xd7\xac\x36\xb2\x72\x56\x5b\x14",
            "WBTC",
            8,
            "Wrapped BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\xef\xd4\xbc\x9a\xfd\x21\x05\x17\x80\x3f\x29\x3a\xba\xbd\x70\x1c\xae\xec\xdf\xd0",
            "WETH",
            18,
            "Wrapped Ether",
        )
        yield (  # address, symbol, decimals, name
            b"\xb2\x10\xd2\x12\x0d\x57\xb7\x58\xee\x16\x3c\xff\xb4\x3e\x73\x72\x8c\x47\x1c\xf1",
            "WHSK",
            18,
            "Wrapped HSK",
        )
    if chain_id == 250:  # FTM
        yield (  # address, symbol, decimals, name
            b"\x6a\x07\xa7\x92\xab\x29\x65\xc7\x2a\x5b\x80\x88\xd3\xa0\x69\xa7\xac\x3a\x99\x3b",
            "AAVE",
            18,
            "Aave",
        )
        yield (  # address, symbol, decimals, name
            b"\x46\xe7\x62\x8e\x8b\x43\x50\xb2\x71\x6a\xb4\x70\xee\x0b\xa1\xfa\x9e\x76\xc6\xc5",
            "BAND",
            18,
            "Band",
        )
        yield (  # address, symbol, decimals, name
            b"\x32\x11\x62\xcd\x93\x3e\x2b\xe4\x98\xcd\x22\x67\xa9\x05\x34\xa8\x04\x05\x1b\x11",
            "BTC",
            8,
            "Bitcoin",
        )
        yield (  # address, symbol, decimals, name
            b"\xb0\x1e\x84\x19\xd8\x42\xbe\xeb\xf1\xb7\x0a\x7b\x5f\x71\x42\xab\xba\xf7\x15\x9d",
            "COVER",
            18,
            "Cover Protocol Governance",
        )
        yield (  # address, symbol, decimals, name
            b"\x65\x7a\x18\x61\xc1\x5a\x3d\xed\x9a\xf0\xb6\x79\x9a\x19\x5a\x24\x9e\xbd\xcb\xc6",
            "CREAM",
            18,
            "Cream",
        )
        yield (  # address, symbol, decimals, name
            b"\x1e\x4f\x97\xb9\xf9\xf9\x13\xc4\x6f\x16\x32\x78\x17\x32\x92\x7b\x90\x19\xc6\x8b",
            "CRV",
            18,
            "Curve DAO",
        )
        yield (  # address, symbol, decimals, name
            b"\x8d\x11\xec\x38\xa3\xeb\x5e\x95\x6b\x05\x2f\x67\xda\x8b\xdc\x9b\xef\x8a\xbf\x3e",
            "DAI",
            18,
            "Dai Stablecoin",
        )
        yield (  # address, symbol, decimals, name
            b"\x74\xb2\x38\x82\xa3\x02\x90\x45\x1a\x17\xc4\x4f\x4f\x05\x24\x3b\x6b\x58\xc7\x6d",
            "ETH",
            18,
            "Ethereum",
        )
        yield (  # address, symbol, decimals, name
            b"\x07\x8e\xef\x5a\x2f\xb5\x33\xe1\xa4\xd4\x87\xef\x64\xb2\x7d\xf1\x13\xd1\x2c\x32",
            "FBAND",
            18,
            "fBAND",
        )
        yield (  # address, symbol, decimals, name
            b"\x27\xf2\x6f\x00\xe1\x60\x59\x03\x64\x5b\xba\xbc\x0a\x73\xe3\x50\x27\xdc\xcd\x45",
            "FBNB",
            18,
            "fBNB",
        )
        yield (  # address, symbol, decimals, name
            b"\xe1\x14\x6b\x9a\xc4\x56\xfc\xbb\x60\x64\x4c\x36\xfd\x3f\x86\x8a\x90\x72\xfc\x6e",
            "FBTC",
            18,
            "fBTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x65\x8b\x0c\x76\x13\xe8\x90\xee\x50\xb8\xc4\xbc\x6a\x3f\x41\xef\x41\x12\x08\xad",
            "FETH",
            18,
            "fETH",
        )
        yield (  # address, symbol, decimals, name
            b"\xa6\x49\xa1\x94\x23\x05\x2d\xc6\xb3\x20\x36\x0b\x3c\x76\x08\x84\xe0\x95\xac\x57",
            "FLINK",
            18,
            "fLINK",
        )
        yield (  # address, symbol, decimals, name
            b"\xdc\x30\x16\x22\xe6\x21\x16\x6b\xd8\xe8\x2f\x2c\xa0\xa2\x6c\x13\xad\x0b\xe3\x55",
            "FRAX",
            18,
            "Frax",
        )
        yield (  # address, symbol, decimals, name
            b"\x04\x9d\x68\x02\x96\x88\xea\xbf\x47\x30\x97\xa2\xfc\x38\xef\x61\x63\x3a\x3c\x7a",
            "fUSDT",
            18,
            "Frapped USDT",
        )
        yield (  # address, symbol, decimals, name
            b"\x7d\x01\x6e\xec\x9c\x25\x23\x2b\x01\xf2\x3e\xf9\x92\xd9\x8c\xa9\x7f\xc2\xaf\x5a",
            "FXS",
            18,
            "Frax Share",
        )
        yield (  # address, symbol, decimals, name
            b"\x44\xb2\x6e\x83\x9e\xb3\x57\x2c\x5e\x95\x9f\x99\x48\x04\xa5\xde\x66\x60\x03\x49",
            "HEGIC",
            18,
            "Hegic",
        )
        yield (  # address, symbol, decimals, name
            b"\x10\x01\x00\x78\xa5\x43\x96\xf6\x2c\x96\xdf\x85\x32\xdc\x2b\x48\x47\xd4\x7e\xd3",
            "HND",
            18,
            "Hundred Finance",
        )
        yield (  # address, symbol, decimals, name
            b"\xf1\x6e\x81\xdc\xe1\x5b\x08\xf3\x26\x22\x07\x42\x02\x03\x79\xb8\x55\xb8\x7d\xf9",
            "ICE",
            18,
            "IceToken",
        )
        yield (  # address, symbol, decimals, name
            b"\x2a\x50\x62\xd2\x2a\xdc\xfa\xaf\xbd\x5c\x54\x1d\x4d\xa8\x2e\x4b\x45\x0d\x42\x12",
            "KP3R",
            18,
            "Keep3r",
        )
        yield (  # address, symbol, decimals, name
            b"\xb3\x65\x4d\xc3\xd1\x0e\xa7\x64\x5f\x83\x19\x66\x8e\x8f\x54\xd2\x57\x4f\xbd\xc8",
            "LINK",
            18,
            "ChainLink",
        )
        yield (  # address, symbol, decimals, name
            b"\x82\xf0\xb8\xb4\x56\xc1\xa4\x51\x37\x84\x67\x39\x89\x82\xd4\x83\x4b\x68\x29\xc1",
            "MIM",
            18,
            "Magic Internet Money",
        )
        yield (  # address, symbol, decimals, name
            b"\x56\xee\x92\x6b\xd8\xc7\x2b\x2d\x5f\xa1\xaf\x4d\x9e\x4c\xbb\x51\x5a\x1e\x3a\xdc",
            "SNX",
            18,
            "Synthetix Network",
        )
        yield (  # address, symbol, decimals, name
            b"\x46\x80\x03\xb6\x88\x94\x39\x77\xe6\x13\x0f\x4f\x68\xf2\x3a\xad\x93\x9a\x10\x40",
            "SPELL",
            18,
            "Spell Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x0e\x16\x94\x48\x3e\xbb\x3b\x74\xd3\x05\x4e\x38\x38\x40\xc6\xcf\x01\x1e\x51\x8e",
            "sUSD",
            18,
            "Synth sUSD",
        )
        yield (  # address, symbol, decimals, name
            b"\xae\x75\xa4\x38\xb2\xe0\xcb\x8b\xb0\x1e\xc1\xe1\xe3\x76\xde\x11\xd4\x44\x77\xcc",
            "SUSHI",
            18,
            "Sushi",
        )
        yield (  # address, symbol, decimals, name
            b"\x04\x06\x8d\xa6\xc8\x3a\xfc\xfa\x0e\x13\xba\x15\xa6\x69\x66\x62\x33\x5d\x5b\x75",
            "USDC",
            6,
            "USD Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\x21\xbe\x37\x0d\x53\x12\xf4\x4c\xb4\x2c\xe3\x77\xbc\x9b\x8a\x0c\xef\x1a\x4c\x83",
            "WFTM",
            18,
            "Wrapped Fantom",
        )
        yield (  # address, symbol, decimals, name
            b"\x29\xb0\xda\x86\xe4\x84\xe1\xc0\x02\x9b\x56\xe8\x17\x91\x2d\x77\x8a\xc0\xec\x69",
            "YFI",
            18,
            "yearn.finance",
        )
    if chain_id == 288:  # Boba
        yield (  # address, symbol, decimals, name
            b"\x12\x16\x36\xc4\x3e\x96\xd9\x7a\xb0\x0b\x6c\x69\x94\xcd\xde\xbe\xf2\x7d\xe1\xc7",
            "BDoge",
            18,
            "BobaDoge",
        )
        yield (  # address, symbol, decimals, name
            b"\x3a\x93\xbd\x0f\xa1\x00\x50\xd2\x06\x37\x0e\xea\x53\x27\x65\x42\xa1\x05\xc8\x85",
            "BRE",
            18,
            "Brewery",
        )
        yield (  # address, symbol, decimals, name
            b"\x61\x8c\xc6\x54\x9d\xdf\x12\xde\x63\x7d\x46\xcd\xda\xda\xfc\x0c\x29\x51\x13\x1c",
            "KYO",
            18,
            "K\u014dy\u014d Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xf0\x8a\xd7\xc3\xf6\xb1\xc6\x84\x3b\xa0\x27\xad\x54\xed\x8d\xdb\x6d\x71\x16\x9b",
            "SB",
            18,
            "Shibui",
        )
    if chain_id == 8453:  # Base
        yield (  # address, symbol, decimals, name
            b"\x63\x70\x6e\x40\x1c\x06\xac\x85\x13\x14\x5b\x76\x87\xa1\x48\x04\xd1\x7f\x81\x4b",
            "AAVE",
            18,
            "Aave Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x94\x01\x81\xa9\x4a\x35\xa4\x56\x9e\x45\x29\xa3\xcd\xfb\x74\xe3\x8f\xd9\x86\x31",
            "AERO",
            18,
            "Aerodrome",
        )
        yield (  # address, symbol, decimals, name
            b"\x30\x55\x91\x3c\x90\xfc\xc1\xa6\xce\x9a\x35\x89\x11\x72\x1e\xeb\x94\x20\x13\xa1",
            "Cake",
            18,
            "PancakeSwap Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xcb\xb7\xc0\x00\x0a\xb8\x8b\x47\x3b\x1f\x5a\xfd\x9e\xf8\x08\x44\x0e\xed\x33\xbf",
            "cbBTC",
            8,
            "Coinbase Wrapped BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x2a\xe3\xf1\xec\x7f\x1f\x50\x12\xcf\xea\xb0\x18\x5b\xfc\x7a\xa3\xcf\x0d\xec\x22",
            "cbETH",
            18,
            "Coinbase Wrapped Staked ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\xf5\x87\xb7\x11\x68\x79\xa5\x29\x35\x3c\xc7\x1e\xe9\x59\xcd\x69\xfd\x5c\xae\x48",
            "cgETH.hashkey",
            18,
            "cgETH Hashkey Cloud",
        )
        yield (  # address, symbol, decimals, name
            b"\x8d\x27\x57\xea\x27\xaa\xbf\x17\x2d\xa4\xcc\xa4\xe5\x47\x4c\x76\x01\x6e\x3d\xc5",
            "clBTC",
            18,
            "clBTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x9e\x10\x28\xf5\xf1\xd5\xed\xe5\x97\x48\xff\xce\xe5\x53\x25\x09\x97\x68\x40\xe0",
            "COMP",
            18,
            "Compound",
        )
        yield (  # address, symbol, decimals, name
            b"\x8e\xe7\x3c\x48\x4a\x26\xe0\xa5\xdf\x2e\xe2\xa4\x96\x0b\x78\x99\x67\xdd\x04\x15",
            "CRV",
            18,
            "Curve DAO Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x8d\x01\x0b\xf9\xc2\x68\x81\x78\x8b\x4e\x6b\xf5\xfd\x1b\xdc\x35\x8c\x8f\x90\xb8",
            "DOT",
            18,
            "Polkadot Token (Relay Chain)",
        )
        yield (  # address, symbol, decimals, name
            b"\x20\x81\xab\x0d\x9e\xc9\xe4\x30\x32\x34\xab\x26\xd8\x6b\x20\xb3\x36\x79\x46\xee",
            "EIGEN",
            18,
            "Eigen",
        )
        yield (  # address, symbol, decimals, name
            b"\x58\x53\x8e\x6a\x46\xe0\x74\x34\xd7\xe7\x37\x5b\xc2\x68\xd3\xcb\x83\x9c\x01\x33",
            "ENA",
            18,
            "ENA",
        )
        yield (  # address, symbol, decimals, name
            b"\x6c\x24\x0d\xda\x6b\x5c\x33\x6d\xf0\x9a\x4d\x01\x11\x39\xbe\xaa\xa1\xea\x2a\xa2",
            "ETHFI",
            18,
            "ether.fi governance token",
        )
        yield (  # address, symbol, decimals, name
            b"\xa0\x76\x9f\x7a\x8f\xc6\x5e\x47\xde\x93\x79\x7b\x4e\x21\xc0\x73\xc1\x17\xfc\x80",
            "EUTBL",
            5,
            "Spiko EU T-Bills Money Market Fund",
        )
        yield (  # address, symbol, decimals, name
            b"\x24\x16\x09\x2f\x14\x33\x78\x75\x0b\xb2\x9b\x79\xed\x96\x1a\xb1\x95\xcc\xee\xa5",
            "ezETH",
            18,
            "Renzo Restaked ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x61\xe0\x30\xa5\x6d\x33\xe8\x26\x0f\xdd\x81\xf0\x3b\x16\x2a\x79\xfe\x34\x49\xcd",
            "FLUID",
            18,
            "Fluid",
        )
        yield (  # address, symbol, decimals, name
            b"\x10\x45\x97\x1c\x16\x8b\x52\x94\xac\xbc\x87\x27\xa4\xf1\xc9\xe1\xaf\x99\xf6\xd0",
            "FTN",
            18,
            "Bridged FTN (OrtakSea)",
        )
        yield (  # address, symbol, decimals, name
            b"\x6b\xb7\xa2\x12\x91\x06\x82\xdc\xfd\xbd\x5b\xcb\xb3\xe2\x8f\xb4\xe8\xda\x10\xee",
            "GHO",
            18,
            "Gho Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x00\xf3\xc4\x28\x33\xc3\x17\x01\x59\xaf\x4e\x92\xdb\xb4\x51\xfb\x3f\x70\x89\x17",
            "ICP",
            8,
            "ICP",
        )
        yield (  # address, symbol, decimals, name
            b"\xec\xac\x9c\x5f\x70\x4e\x95\x49\x31\x34\x9d\xa3\x7f\x60\xe3\x9f\x51\x5c\x11\xc1",
            "LBTC",
            8,
            "Lombard Staked Bitcoin",
        )
        yield (  # address, symbol, decimals, name
            b"\x88\xfb\x15\x0b\xdc\x53\xa6\x5f\xe9\x4d\xea\x0c\x9b\xa0\xa6\xda\xf8\xc6\xe1\x96",
            "LINK",
            18,
            "ChainLink Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xb2\x97\x49\x49\x89\x54\xa3\xa8\x21\xec\x37\xbd\xe8\x6e\x38\x6d\xf3\xce\x30\xb6",
            "LsETH",
            18,
            "Liquid Staked ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\xba\xa5\xcc\x21\xfd\x48\x7b\x8f\xcc\x2f\x63\x2f\x3f\x4e\x8d\x37\x26\x2a\x08\x42",
            "MORPHO",
            18,
            "Morpho Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x06\x0c\xb0\x87\xa9\x73\x0e\x13\xaa\x19\x1f\x31\xa6\xd8\x6b\xff\x8d\xfc\xdc\xc0",
            "OHM",
            9,
            "Olympus",
        )
        yield (  # address, symbol, decimals, name
            b"\xa9\x9f\x6e\x67\x85\xda\x0f\x5d\x6f\xb4\x24\x95\xfe\x42\x4b\xce\x02\x9e\xeb\x3e",
            "PENDLE",
            18,
            "Pendle",
        )
        yield (  # address, symbol, decimals, name
            b"\xb6\xfe\x22\x1f\xe9\xee\xf5\xab\xa2\x21\xc3\x48\xba\x20\xa1\xbf\x5e\x73\x62\x4c",
            "rETH",
            18,
            "Rocket Pool ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x70\x65\x4a\xad\x8b\x77\x34\xdc\x31\x9d\x0c\x36\x08\xec\x7b\x32\xe0\x3f\xa1\x62",
            "satUSD",
            18,
            "Satoshi Stablecoin V2",
        )
        yield (  # address, symbol, decimals, name
            b"\x99\xac\x44\x84\xe8\xa1\xdb\xd6\xa1\x85\x38\x0b\x3a\x81\x19\x13\xac\x88\x4d\x87",
            "sDAI",
            18,
            "Savings Dai",
        )
        yield (  # address, symbol, decimals, name
            b"\x22\xe6\x96\x6b\x79\x9c\x4d\x5b\x13\xbe\x96\x2e\x1d\x11\x7b\x56\x32\x7f\xda\x66",
            "SNX",
            18,
            "Synthetix Network Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x3b\x86\xad\x95\x85\x9b\x6a\xb7\x73\xf5\x5f\x8d\x94\xb4\xb9\xd4\x43\xee\x93\x1f",
            "SolvBTC",
            18,
            "Solv BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\xc2\x6c\x90\x99\xbd\x37\x89\x10\x78\x88\xc3\x5b\xb4\x11\x78\x07\x9b\x28\x25\x61",
            "SolvBTC.BBN",
            18,
            "SolvBTC Babylon",
        )
        yield (  # address, symbol, decimals, name
            b"\x50\xda\x64\x5f\x14\x87\x98\xf6\x8e\xf2\xd7\xdb\x7c\x1c\xb2\x2a\x68\x19\xbb\x2c",
            "SPX",
            8,
            "SPX6900",
        )
        yield (  # address, symbol, decimals, name
            b"\x21\x1c\xc4\xdd\x07\x37\x34\xda\x05\x5f\xbf\x44\xa2\xb4\x66\x7d\x5e\x5f\xe5\xd2",
            "sUSDe",
            18,
            "Staked USDe",
        )
        yield (  # address, symbol, decimals, name
            b"\x58\x75\xee\xe1\x1c\xf8\x39\x81\x02\xfd\xad\x70\x4c\x9e\x96\x60\x76\x75\x46\x7a",
            "sUSDS",
            18,
            "Savings USDS",
        )
        yield (  # address, symbol, decimals, name
            b"\x68\x8a\xee\x02\x2a\xa5\x44\xf1\x50\x67\x8b\x8e\x57\x20\xb6\xb9\x6a\x9e\x9a\x2f",
            "SYRUP",
            18,
            "Syrup Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x23\x6a\xa5\x09\x79\xd5\xf3\xde\x3b\xd1\xee\xb4\x0e\x81\x13\x7f\x22\xab\x79\x4b",
            "tBTC",
            18,
            "Base tBTC v2",
        )
        yield (  # address, symbol, decimals, name
            b"\x09\xbe\x16\x92\xca\x16\xe0\x6f\x53\x6f\x00\x38\xff\x11\xd1\xda\x85\x24\xad\xb1",
            "TEL",
            2,
            "Telcoin",
        )
        yield (  # address, symbol, decimals, name
            b"\xd0\x9a\xcb\x80\xc1\xe8\xf2\x29\x18\x62\xc4\x97\x8a\x00\x87\x91\xc9\x16\x70\x03",
            "tETH",
            18,
            "Treehouse ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\xa4\xa2\xe2\xca\x3f\xbf\xe2\x1a\xed\x83\x47\x1d\x28\xb6\xf6\x5a\x23\x3c\x6e\x00",
            "TIBBIR",
            18,
            "Ribbita by Virtuals",
        )
        yield (  # address, symbol, decimals, name
            b"\x75\x8a\x3e\x0b\x1f\x84\x2c\x93\x06\xb7\x83\xf8\xa4\x07\x8c\x6c\x8c\x03\xa2\x70",
            "USD0",
            18,
            "Usual USD",
        )
        yield (  # address, symbol, decimals, name
            b"\xd9\xaa\xec\x86\xb6\x5d\x86\xf6\xa7\xb5\xb1\xb0\xc4\x2f\xfa\x53\x17\x10\xb6\xca",
            "USDbC",
            6,
            "USD Base Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\x83\x35\x89\xfc\xd6\xed\xb6\xe0\x8f\x4c\x7c\x32\xd4\xf7\x1b\x54\xbd\xa0\x29\x13",
            "USDC",
            6,
            "USD Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\x5d\x3a\x1f\xf2\xb6\xba\xb8\x3b\x63\xcd\x9a\xd0\x78\x70\x74\x08\x1a\x52\xef\x34",
            "USDe",
            18,
            "USDe",
        )
        yield (  # address, symbol, decimals, name
            b"\x82\x0c\x13\x7f\xa7\x0c\x86\x91\xf0\xe4\x4d\xc4\x20\xa5\xe5\x3c\x16\x89\x21\xdc",
            "USDS",
            18,
            "USDS Stablecoin",
        )
        yield (  # address, symbol, decimals, name
            b"\xf3\x52\x7e\xf8\xde\x26\x5e\xaa\x37\x16\xfb\x31\x2c\x12\x84\x7b\xfb\xa6\x6c\xef",
            "USDX",
            18,
            "Wrapped USDX",
        )
        yield (  # address, symbol, decimals, name
            b"\x0b\x3e\x32\x84\x55\xc4\x05\x9e\xeb\x9e\x3f\x84\xb5\x54\x3f\x74\xe2\x4e\x7e\x1b",
            "VIRTUAL",
            18,
            "Virtual Protocol",
        )
        yield (  # address, symbol, decimals, name
            b"\x05\x55\xe3\x0d\xa8\xf9\x83\x08\xed\xb9\x60\xaa\x94\xc0\xdb\x47\x23\x0d\x2b\x9c",
            "WBTC",
            8,
            "Wrapped BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x04\xc0\x59\x9a\xe5\xa4\x47\x57\xc0\xaf\x6f\x9e\xc3\xb9\x3d\xa8\x97\x6c\x15\x0a",
            "weETH",
            18,
            "Wrapped eETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x42\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x06",
            "WETH",
            18,
            "Wrapped Ether",
        )
        yield (  # address, symbol, decimals, name
            b"\x11\x11\x11\x11\x11\x16\x6b\x7f\xe7\xbd\x91\x42\x77\x24\xb4\x87\x98\x0a\xfc\x69",
            "ZORA",
            18,
            "Zora",
        )
    if chain_id == 9798:  # Data Trade Chain
        yield (  # address, symbol, decimals, name
            b"\x8e\x79\x85\x0c\x50\xe5\x25\xeb\x6b\xa6\x3e\x60\x1e\x7b\x41\x88\x8a\x1c\x91\x02",
            "BV",
            2,
            "BV",
        )
        yield (  # address, symbol, decimals, name
            b"\x89\x9f\x0b\x9d\x67\xdd\x1b\x83\x3f\xda\xa9\x0c\x8b\x09\xea\x61\x6d\x0e\x9e\x98",
            "CNV",
            2,
            "CNV",
        )
        yield (  # address, symbol, decimals, name
            b"\xe8\x95\xc5\x77\xd7\x47\xbb\x5d\xbb\xc1\xf0\x6c\xb4\x4d\x60\x67\x68\x0b\xe4\xbe",
            "dBTC",
            8,
            "dBTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x8b\x71\x60\xc1\xe9\xfd\xb6\x89\xa0\x60\xff\x09\x19\xe8\x49\x15\xb0\xdf\xa0\x4a",
            "dETH",
            18,
            "dETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x74\x5c\x11\xfb\x47\x83\xbd\x00\xa8\x8a\x0b\x99\x42\x02\x62\xf4\x09\xfa\x8b\xb8",
            "DOS",
            2,
            "DOS",
        )
        yield (  # address, symbol, decimals, name
            b"\x36\xe6\x50\x4c\x96\x8f\x5c\x2a\x31\x0b\x6a\xf7\xb9\x7b\xc2\x2c\xdd\x34\x02\xcc",
            "dUSDT",
            6,
            "dUSDT",
        )
        yield (  # address, symbol, decimals, name
            b"\xb8\x8a\xd7\x67\xb4\x16\x19\x7e\x62\x93\x9d\xec\x20\x74\x31\xb5\x61\xa9\x38\x3b",
            "FEC",
            4,
            "FEC",
        )
        yield (  # address, symbol, decimals, name
            b"\xe5\x2a\x73\x68\x28\xc7\x82\xc2\xa4\xa3\x45\xbb\xe8\x05\x2a\xed\x01\x0f\xc8\x2d",
            "HLT",
            2,
            "HLT",
        )
        yield (  # address, symbol, decimals, name
            b"\x6d\x88\x5b\x0b\x37\xc6\x2b\xe0\xc7\x2e\xcd\x6a\x61\xaf\x2b\xff\xf6\x81\x41\x9e",
            "STC08375",
            0,
            "STC08375",
        )
    if chain_id == 42161:  # Arbitrum
        yield (  # address, symbol, decimals, name
            b"\xba\x5d\xdd\x1f\x9d\x7f\x57\x0d\xc9\x4a\x51\x47\x9a\x00\x0e\x3b\xce\x96\x71\x96",
            "AAVE",
            18,
            "Aave Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x91\x2c\xe5\x91\x44\x19\x1c\x12\x04\xe6\x45\x59\xfe\x82\x53\xa0\xe4\x9e\x65\x48",
            "ARB",
            18,
            "Arbitrum",
        )
        yield (  # address, symbol, decimals, name
            b"\xc8\x7b\x37\xa5\x81\xec\x32\x57\xb7\x34\x88\x6d\x9d\x3a\x58\x1f\x5a\x9d\x05\x6c",
            "ATH",
            18,
            "Aethir Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x09\x19\x9d\x9a\x5f\x44\x48\xd0\x84\x8e\x43\x95\xd0\x65\xe1\xad\x9c\x4a\x1f\x74",
            "Bonk",
            5,
            "Bonk",
        )
        yield (  # address, symbol, decimals, name
            b"\x1b\x89\x68\x93\xdf\xc8\x6b\xb6\x7c\xf5\x77\x67\x29\x8b\x90\x73\xd2\xc1\xba\x2c",
            "Cake",
            18,
            "PancakeSwap Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xcb\xb7\xc0\x00\x0a\xb8\x8b\x47\x3b\x1f\x5a\xfd\x9e\xf8\x08\x44\x0e\xed\x33\xbf",
            "cbBTC",
            8,
            "Coinbase Wrapped BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x1d\xeb\xd7\x3e\x75\x2b\xea\xf7\x98\x65\xfd\x64\x46\xb0\xc9\x70\xea\xe7\x73\x2f",
            "cbETH",
            18,
            "Coinbase Wrapped Staked ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x0c\xe4\x5d\xd5\x3a\xff\xbb\x01\x18\x84\xef\x18\x66\xe0\x73\x8f\x58\xab\x79\x69",
            "cgETH.hashkey",
            18,
            "cgETH Hashkey Cloud",
        )
        yield (  # address, symbol, decimals, name
            b"\x17\x92\x86\x5d\x49\x3f\xe4\xdf\xdd\x50\x40\x10\xd3\xc0\xf6\xda\x11\xe8\x04\x6d",
            "clBTC",
            18,
            "clBTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x11\xcd\xb4\x2b\x0e\xb4\x6d\x95\xf9\x90\xbe\xdd\x46\x95\xa6\xe3\xfa\x03\x49\x78",
            "CRV",
            18,
            "Curve DAO Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x8d\x01\x0b\xf9\xc2\x68\x81\x78\x8b\x4e\x6b\xf5\xfd\x1b\xdc\x35\x8c\x8f\x90\xb8",
            "DOT",
            18,
            "Polkadot Token (Relay Chain)",
        )
        yield (  # address, symbol, decimals, name
            b"\x58\x53\x8e\x6a\x46\xe0\x74\x34\xd7\xe7\x37\x5b\xc2\x68\xd3\xcb\x83\x9c\x01\x33",
            "ENA",
            18,
            "ENA",
        )
        yield (  # address, symbol, decimals, name
            b"\x71\x89\xfb\x5b\x65\x04\xbb\xff\x6a\x85\x2b\x13\xb7\xb8\x2a\x3c\x11\x8f\xdc\x27",
            "ETHFI",
            18,
            "ether.fi governance token",
        )
        yield (  # address, symbol, decimals, name
            b"\xcb\xeb\x19\x54\x90\x54\xcc\x0a\x62\x57\xa7\x77\x36\xfc\x78\xc3\x67\x21\x6c\xe7",
            "EUTBL",
            5,
            "Spiko EU T-Bills Money Market Fund",
        )
        yield (  # address, symbol, decimals, name
            b"\x24\x16\x09\x2f\x14\x33\x78\x75\x0b\xb2\x9b\x79\xed\x96\x1a\xb1\x95\xcc\xee\xa5",
            "ezETH",
            18,
            "Renzo Restaked ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x93\xc9\x93\x2e\x4a\xfa\x59\x20\x1f\x0b\x5e\x63\xf7\xd8\x16\x51\x6f\x16\x69\xfe",
            "FDUSD",
            18,
            "First Digital USD",
        )
        yield (  # address, symbol, decimals, name
            b"\x46\x85\x0a\xd6\x1c\x2b\x7d\x64\xd0\x8c\x9c\x75\x4f\x45\x25\x45\x96\x69\x69\x84",
            "FLX",
            6,
            "Flux USD",
        )
        yield (  # address, symbol, decimals, name
            b"\x17\x84\x12\xe7\x9c\x25\x96\x8a\x32\xe8\x9b\x11\xf6\x3b\x33\xf7\x33\x77\x0c\x2a",
            "frxETH",
            18,
            "Frax Ether",
        )
        yield (  # address, symbol, decimals, name
            b"\x10\x45\x97\x1c\x16\x8b\x52\x94\xac\xbc\x87\x27\xa4\xf1\xc9\xe1\xaf\x99\xf6\xd0",
            "FTN",
            18,
            "Bridged FTN (OrtakSea)",
        )
        yield (  # address, symbol, decimals, name
            b"\x7d\xff\x72\x69\x3f\x6a\x41\x49\xb1\x7e\x7c\x63\x14\x65\x5f\x6a\x9f\x7c\x8b\x33",
            "GHO",
            18,
            "Gho Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x96\x23\x06\x33\x77\xad\x1b\x27\x54\x4c\x96\x5c\xcd\x73\x42\xf7\xea\x7e\x88\xc7",
            "GRT",
            18,
            "Graph Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x13\xad\x51\xed\x4f\x1b\x7e\x9d\xc1\x68\xd8\xa0\x0c\xb3\xf4\xdd\xd8\x5e\xfa\x60",
            "LDO",
            18,
            "Lido DAO Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xf9\x7f\x4d\xf7\x51\x17\xa7\x8c\x1a\x5a\x0d\xbb\x81\x4a\xf9\x24\x58\x53\x9f\xb4",
            "LINK",
            18,
            "ChainLink Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x40\xbd\x67\x0a\x58\x23\x8e\x6e\x23\x0c\x43\x0b\xbb\x5c\xe6\xec\x0d\x40\xdf\x48",
            "MORPHO",
            18,
            "Morpho Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xf0\xcb\x2d\xc0\xdb\x5e\x6c\x66\xb9\xa7\x0a\xc2\x7b\x06\xb8\x78\xda\x01\x70\x28",
            "OHM",
            9,
            "Olympus",
        )
        yield (  # address, symbol, decimals, name
            b"\xf7\xd4\xe7\x27\x3e\x50\x15\xc9\x67\x28\xa6\xb0\x2f\x31\xc5\x05\xee\x18\x46\x03",
            "osETH",
            18,
            "Staked ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x0c\x88\x0f\x67\x61\xf1\xaf\x8d\x9a\xa9\xc4\x66\x98\x4b\x80\xda\xb9\xa8\xc9\xe8",
            "PENDLE",
            18,
            "Pendle",
        )
        yield (  # address, symbol, decimals, name
            b"\x25\xd8\x87\xce\x7a\x35\x17\x2c\x62\xfe\xbf\xd6\x7a\x18\x56\xf2\x0f\xae\xbb\x00",
            "PEPE",
            18,
            "Pepe",
        )
        yield (  # address, symbol, decimals, name
            b"\xec\x70\xdc\xb4\xa1\xef\xa4\x6b\x8f\x2d\x97\xc3\x10\xc9\xc4\x79\x0b\xa5\xff\xa8",
            "rETH",
            18,
            "Rocket Pool ETH",
        )
        yield (  # address, symbol, decimals, name
            b"\xb4\x81\x8b\xb6\x94\x78\x73\x0e\xf4\xe3\x3c\xc0\x68\xdd\x94\x27\x8e\x27\x66\xcb",
            "satUSD",
            18,
            "Satoshi Stablecoin V2",
        )
        yield (  # address, symbol, decimals, name
            b"\x36\x47\xc5\x4c\x4c\x2c\x65\xbc\x7a\x2d\x63\xc0\xda\x28\x09\xb3\x99\xdb\xbd\xc0",
            "SolvBTC",
            18,
            "Solv BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x21\x1c\xc4\xdd\x07\x37\x34\xda\x05\x5f\xbf\x44\xa2\xb4\x66\x7d\x5e\x5f\xe5\xd2",
            "sUSDe",
            18,
            "Staked USDe",
        )
        yield (  # address, symbol, decimals, name
            b"\xdd\xb4\x69\x99\xf8\x89\x16\x63\xa8\xf2\x82\x8d\x25\x29\x8f\x70\x41\x6d\x76\x10",
            "sUSDS",
            18,
            "Savings USDS",
        )
        yield (  # address, symbol, decimals, name
            b"\xbc\x01\x1a\x12\xda\x28\xe8\xf0\xf5\x28\xd9\xee\x5e\x70\x39\xe2\x2f\x91\xcf\x18",
            "swETH",
            18,
            "swETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x41\xca\x75\x86\xcc\x13\x11\x80\x7b\x46\x05\xfb\xb7\x48\xa3\xb8\x86\x2b\x42\xb5",
            "syrupUSDC",
            6,
            "Syrup USDC",
        )
        yield (  # address, symbol, decimals, name
            b"\x6c\x84\xa8\xf1\xc2\x91\x08\xf4\x7a\x79\x96\x4b\x5f\xe8\x88\xd4\xf4\xd0\xde\x40",
            "tBTC",
            18,
            "Arbitrum tBTC v2",
        )
        yield (  # address, symbol, decimals, name
            b"\xfa\x7f\x89\x80\xb0\xf1\xe6\x4a\x20\x62\x79\x1c\xc3\xb0\x87\x15\x72\xf1\xf7\xf0",
            "UNI",
            18,
            "Uniswap",
        )
        yield (  # address, symbol, decimals, name
            b"\x35\xf1\xc5\xcb\x7f\xb9\x77\xe6\x69\xfd\x24\x4c\x56\x7d\xa9\x9d\x8a\x3a\x68\x50",
            "USD0",
            18,
            "Usual USD",
        )
        yield (  # address, symbol, decimals, name
            b"\x0a\x1a\x1a\x10\x7e\x45\xb7\xce\xd8\x68\x33\x86\x3f\x48\x2b\xc5\xf4\xed\x82\xef",
            "USDai",
            18,
            "USDai",
        )
        yield (  # address, symbol, decimals, name
            b"\xaf\x88\xd0\x65\xe7\x7c\x8c\xc2\x23\x93\x27\xc5\xed\xb3\xa4\x32\x26\x8e\x58\x31",
            "USDC",
            6,
            "USD Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\xff\x97\x0a\x61\xa0\x4b\x1c\xa1\x48\x34\xa4\x3f\x5d\xe4\x53\x3e\xbd\xdb\x5c\xc8",
            "USDC.e",
            6,
            "Bridged USDC",
        )
        yield (  # address, symbol, decimals, name
            b"\x68\x04\x47\x59\x5e\x8b\x7b\x3a\xa1\xb4\x3b\xeb\x9f\x60\x98\xc7\x9a\xc2\xab\x3f",
            "USDD",
            18,
            "Decentralized USD",
        )
        yield (  # address, symbol, decimals, name
            b"\x5d\x3a\x1f\xf2\xb6\xba\xb8\x3b\x63\xcd\x9a\xd0\x78\x70\x74\x08\x1a\x52\xef\x34",
            "USDe",
            18,
            "USDe",
        )
        yield (  # address, symbol, decimals, name
            b"\x64\x91\xc0\x5a\x82\x21\x9b\x8d\x14\x79\x05\x73\x61\xff\x16\x54\x74\x9b\x87\x6b",
            "USDS",
            18,
            "USDS Stablecoin",
        )
        yield (  # address, symbol, decimals, name
            b"\xfd\x08\x6b\xc7\xcd\x5c\x48\x1d\xcc\x9c\x85\xeb\xe4\x78\xa1\xc0\xb6\x9f\xcb\xb9",
            "USDT",
            6,
            "Tether USD",
        )
        yield (  # address, symbol, decimals, name
            b"\xf3\x52\x7e\xf8\xde\x26\x5e\xaa\x37\x16\xfb\x31\x2c\x12\x84\x7b\xfb\xa6\x6c\xef",
            "USDX",
            18,
            "USDX",
        )
        yield (  # address, symbol, decimals, name
            b"\x35\xe0\x50\xd3\xc0\xec\x2d\x29\xd2\x69\xa8\xec\xea\x76\x3a\x18\x3b\xdf\x9a\x9d",
            "USDY",
            18,
            "Ondo U.S. Dollar Yield",
        )
        yield (  # address, symbol, decimals, name
            b"\x2f\x2a\x25\x43\xb7\x6a\x41\x66\x54\x9f\x7a\xab\x2e\x75\xbe\xf0\xae\xfc\x5b\x0f",
            "WBTC",
            8,
            "Wrapped BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x35\x75\x10\x07\xa4\x07\xca\x6f\xef\xfe\x80\xb3\xcb\x39\x77\x36\xd2\xcf\x4d\xbe",
            "weETH",
            18,
            "Wrapped eETH",
        )
        yield (  # address, symbol, decimals, name
            b"\x82\xaf\x49\x44\x7d\x8a\x07\xe3\xbd\x95\xbd\x0d\x56\xf3\x52\x41\x52\x3f\xba\xb1",
            "WETH",
            18,
            "Wrapped Ether",
        )
    if chain_id == 42220:  # Celo
        yield (  # address, symbol, decimals, name
            b"\x63\x9a\x64\x7f\xbe\x20\xb6\xc8\xac\x19\xe4\x8e\x2d\xe4\x4e\xa7\x92\xc6\x2c\x5c",
            "BIFI",
            18,
            "beefy.finance",
        )
        yield (  # address, symbol, decimals, name
            b"\xd6\x29\xeb\x00\xde\xce\xd2\xa0\x80\xb7\xec\x63\x0e\xf6\xac\x11\x7e\x61\x4f\x1b",
            "BTC",
            18,
            "Wrapped Bitcoin",
        )
        yield (  # address, symbol, decimals, name
            b"\x47\x1e\xce\x37\x50\xda\x23\x7f\x93\xb8\xe3\x39\xc5\x36\x98\x9b\x89\x78\xa4\x38",
            "CELO",
            18,
            "Celo native asset",
        )
        yield (  # address, symbol, decimals, name
            b"\x2d\xef\x42\x85\x78\x7d\x58\xa2\xf8\x11\xaf\x24\x75\x5a\x81\x50\x62\x2f\x43\x61",
            "cETH",
            18,
            "Wrapped Ethereum",
        )
        yield (  # address, symbol, decimals, name
            b"\xd8\x76\x3c\xba\x27\x6a\x37\x38\xe6\xde\x85\xb4\xb3\xbf\x5f\xde\xd6\xd6\xca\x73",
            "cEUR",
            18,
            "Celo Euro",
        )
        yield (  # address, symbol, decimals, name
            b"\x59\x27\xfd\x24\x4e\x11\xdb\x1c\x7b\x12\x15\x61\x91\x44\xd2\xaa\xba\xc8\x0a\x4f",
            "cLA",
            18,
            "celoLaunch",
        )
        yield (  # address, symbol, decimals, name
            b"\xf3\x60\x8f\x84\x6c\xa7\x31\x47\xf0\x8f\xde\x8d\x57\xf4\x5e\x27\xce\xea\x4d\x61",
            "cMETA",
            18,
            "metaCelo Game NFT",
        )
        yield (  # address, symbol, decimals, name
            b"\x76\x5d\xe8\x16\x84\x58\x61\xe7\x5a\x25\xfc\xa1\x22\xbb\x68\x98\xb8\xb1\x28\x2a",
            "cUSD",
            18,
            "Celo Dollar",
        )
        yield (  # address, symbol, decimals, name
            b"\x70\x37\xf7\x29\x6b\x2f\xc7\x90\x8d\xe7\xb5\x7a\x89\xef\xaa\x83\x19\xf0\xc5\x00",
            "mCELO",
            18,
            "Moola CELO AToken",
        )
        yield (  # address, symbol, decimals, name
            b"\x73\xa2\x10\x63\x7f\x6f\x6b\x70\x05\x51\x26\x77\xba\x6b\x3c\x96\xbb\x4a\xa4\x4b",
            "MOBI",
            18,
            "Mobius DAO Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x46\xc9\x75\x7c\x54\x97\xc5\xb1\xf2\xeb\x73\xae\x79\xb6\xb6\x7d\x11\x9b\x0b\x58",
            "PACT",
            18,
            "impactMarket",
        )
        yield (  # address, symbol, decimals, name
            b"\x74\xc0\xc5\x8b\x99\xb6\x8c\xf1\x6a\x71\x72\x79\xac\x2d\x05\x6a\x34\xba\x2b\xfe",
            "SOURCE",
            18,
            "Source",
        )
        yield (  # address, symbol, decimals, name
            b"\xd1\x5e\xc7\x21\xc2\xa8\x96\x51\x2a\xd2\x9c\x67\x19\x97\xdd\x68\xf9\x59\x32\x26",
            "SUSHI",
            18,
            "SushiToken",
        )
        yield (  # address, symbol, decimals, name
            b"\x00\xbe\x91\x5b\x9d\xcf\x56\xa3\xcb\xe7\x39\xd9\xb9\xc2\x02\xca\x69\x24\x09\xec",
            "UBE",
            18,
            "Ubeswap",
        )
    if chain_id == 43114:  # AVAX
        yield (  # address, symbol, decimals, name
            b"\x63\xa7\x28\x06\x09\x8b\xd3\xd9\x52\x0c\xc4\x33\x56\xdd\x78\xaf\xe5\xd3\x86\xd9",
            "AAVE.e",
            18,
            "Aave Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x21\x47\xef\xff\x67\x5e\x4a\x4e\xe1\xc2\xf9\x18\xd1\x81\xcd\xbd\x7a\x8e\x20\x8f",
            "ALPHA.e",
            18,
            "AlphaToken",
        )
        yield (  # address, symbol, decimals, name
            b"\x48\xf8\x8a\x3f\xe8\x43\xcc\xb0\xb5\x00\x3e\x70\xb4\x19\x2c\x1d\x74\x48\xbe\xf0",
            "CAI",
            18,
            "Colony Avalanche Index",
        )
        yield (  # address, symbol, decimals, name
            b"\x63\x7a\xfe\xff\x75\xca\x66\x9f\xf9\x2e\x45\x70\xb1\x4d\x63\x99\xa6\x58\x90\x2f",
            "COOK",
            18,
            "Poly-Peg COOK",
        )
        yield (  # address, symbol, decimals, name
            b"\xd5\x86\xe7\xf8\x44\xce\xa2\xf8\x7f\x50\x15\x26\x65\xbc\xbc\x2c\x27\x9d\x8d\x70",
            "DAI.e",
            18,
            "Dai Stablecoin",
        )
        yield (  # address, symbol, decimals, name
            b"\x50\x85\x43\x42\x27\xab\x73\x15\x1f\xad\x2d\xe5\x46\x21\x0c\xbc\x86\x63\xdf\x96",
            "DBY",
            18,
            "Metaderby token",
        )
        yield (  # address, symbol, decimals, name
            b"\xfc\xc6\xce\x74\xf4\xcd\x7e\xde\xf0\xc5\x42\x9b\xb9\x9d\x38\xa3\x60\x80\x43\xa5",
            "FIRE",
            18,
            "FIRE",
        )
        yield (  # address, symbol, decimals, name
            b"\x02\x61\x87\xbd\xbc\x6b\x75\x10\x03\x51\x7b\xcb\x30\xac\x78\x17\xd5\xb7\x66\xf8",
            "H2O",
            18,
            "Defrost Finance H2O",
        )
        yield (  # address, symbol, decimals, name
            b"\x82\xfe\x03\x8e\xa4\xb5\x0f\x9c\x95\x7d\xa3\x26\xc4\x12\xeb\xd7\x34\x62\x07\x7c",
            "HAT",
            18,
            "Joe Hat Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x65\x37\x8b\x69\x78\x53\x56\x8d\xa9\xff\x8e\xab\x60\xc1\x3e\x1e\xe9\xf4\xa6\x54",
            "HUSKY",
            18,
            "Husky",
        )
        yield (  # address, symbol, decimals, name
            b"\x3e\xef\xb1\x80\x03\xd0\x33\x66\x1f\x84\xe4\x83\x60\xeb\xec\xd1\x81\xa8\x47\x09",
            "ISA",
            18,
            "Islander",
        )
        yield (  # address, symbol, decimals, name
            b"\x59\x47\xbb\x27\x5c\x52\x10\x40\x05\x1d\x82\x39\x61\x92\x18\x1b\x41\x32\x27\xa3",
            "LINK.e",
            18,
            "Chainlink Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x44\x96\x74\xb8\x2f\x05\xd4\x98\xe1\x26\xdd\x66\x15\xa1\x05\x7a\x9c\x08\x8f\x2c",
            "LOST",
            18,
            "LostToken",
        )
        yield (  # address, symbol, decimals, name
            b"\x47\xeb\x6f\x75\x25\xc1\xaa\x99\x9f\xbc\x9e\xe9\x27\x15\xf5\x23\x1e\xb1\x24\x1d",
            "MELT",
            18,
            "Defrost Finance Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x13\x09\x66\x62\x88\x46\xbf\xd3\x6f\xf3\x1a\x82\x27\x05\x79\x6e\x8c\xb8\xc1\x8d",
            "MIM",
            18,
            "Magic Internet Money",
        )
        yield (  # address, symbol, decimals, name
            b"\xd9\xd9\x0f\x88\x2c\xdd\xd6\x06\x39\x59\xa9\xd8\x37\xb0\x5c\xb7\x48\x71\x8a\x05",
            "MORE",
            18,
            "More Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x60\x78\x1c\x25\x86\xd6\x82\x29\xfd\xe4\x75\x64\x54\x67\x84\xab\x3f\xac\xa9\x82",
            "PNG",
            18,
            "Pangolin",
        )
        yield (  # address, symbol, decimals, name
            b"\x87\x29\x43\x8e\xb1\x5e\x2c\x8b\x57\x6f\xcc\x6a\xec\xda\x6a\x14\x87\x76\xc0\xf5",
            "QI",
            18,
            "BENQI",
        )
        yield (  # address, symbol, decimals, name
            b"\xce\x1b\xff\xbd\x53\x74\xda\xc8\x6a\x28\x93\x11\x96\x83\xf4\x91\x1a\x2f\x78\x14",
            "SPELL",
            18,
            "Spell Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xb2\x79\xf8\xdd\x15\x2b\x99\xec\x1d\x84\xa4\x89\xd3\x2c\x35\xbc\x0c\x7f\x56\x74",
            "STEAK",
            18,
            "STEAK",
        )
        yield (  # address, symbol, decimals, name
            b"\x37\xb6\x08\x51\x9f\x91\xf7\x0f\x2e\xeb\x0e\x5e\xd9\xaf\x40\x61\x72\x2e\x4f\x76",
            "SUSHI.e",
            18,
            "SushiToken",
        )
        yield (  # address, symbol, decimals, name
            b"\xb9\x7e\xf9\xef\x87\x34\xc7\x19\x04\xd8\x00\x2f\x8b\x6b\xc6\x6d\xd9\xc4\x8a\x6e",
            "USDC",
            6,
            "USD Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\xa7\xd7\x07\x9b\x0f\xea\xd9\x1f\x3e\x65\xf8\x6e\x89\x15\xcb\x59\xc1\xa4\xc6\x64",
            "USDC.e",
            6,
            "USD Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\x97\x02\x23\x0a\x8e\xa5\x36\x01\xf5\xcd\x2d\xc0\x0f\xdb\xc1\x3d\x4d\xf4\xa8\xc7",
            "USDT",
            6,
            "Tether",
        )
        yield (  # address, symbol, decimals, name
            b"\xc7\x19\x84\x37\x98\x0c\x04\x1c\x80\x5a\x1e\xdc\xba\x50\xc1\xce\x5d\xb9\x51\x18",
            "USDT.e",
            6,
            "Tether USD",
        )
        yield (  # address, symbol, decimals, name
            b"\x84\x6d\x50\x24\x8b\xaf\x8b\x7c\xea\xa9\xd9\xb5\x3b\xfd\x12\xd7\xd7\xfb\xb2\x5a",
            "VSO",
            18,
            "VersoToken",
        )
        yield (  # address, symbol, decimals, name
            b"\xb3\x1f\x66\xaa\x3c\x1e\x78\x53\x63\xf0\x87\x5a\x1b\x74\xe2\x7b\x85\xfd\x66\xc7",
            "WAVAX",
            18,
            "Wrapped AVAX",
        )
        yield (  # address, symbol, decimals, name
            b"\x40\x8d\x4c\xd0\xad\xb7\xce\xbd\x1f\x1a\x1c\x33\xa0\xba\x20\x98\xe1\x29\x5b\xab",
            "WBTC",
            8,
            "Wrapped BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x50\xb7\x54\x56\x27\xa5\x16\x2f\x82\xa9\x92\xc3\x3b\x87\xad\xc7\x51\x87\xb2\x18",
            "WBTC.e",
            8,
            "Wrapped BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\x49\xd5\xc2\xbd\xff\xac\x6c\xe2\xbf\xdb\x66\x40\xf4\xf8\x0f\x22\x6b\xc1\x0b\xab",
            "WETH.e",
            18,
            "Wrapped Ether",
        )
        yield (  # address, symbol, decimals, name
            b"\xd1\xc3\xf9\x4d\xe7\xe5\xb4\x5f\xa4\xed\xbb\xa4\x72\x49\x1a\x9f\x4b\x16\x6f\xc4",
            "XAVA",
            18,
            "Avalaunch",
        )
        yield (  # address, symbol, decimals, name
            b"\x59\x41\x4b\x30\x89\xce\x2a\xf0\x01\x0e\x75\x23\xde\xa7\xe2\xb3\x5d\x77\x6e\xc7",
            "YAK",
            18,
            "Yak Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x77\x77\x77\x77\x77\x7d\x45\x54\xc3\x92\x23\xc3\x54\xa0\x58\x25\xb2\xe8\xfa\xa3",
            "YETI",
            18,
            "Yeti Finance",
        )
    if chain_id == 1313161554:  # NEAR
        yield (  # address, symbol, decimals, name
            b"\xc2\x1f\xf0\x12\x29\xe9\x82\xd7\xc8\xb8\x69\x11\x63\xb0\xa3\xcb\x8f\x35\x74\x53",
            "$META",
            24,
            "Meta Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x2b\xae\x00\xc8\xbc\x18\x68\xa5\xf7\xa2\x16\xe8\x81\xba\xe9\xe6\x62\x63\x01\x11",
            "ABR",
            18,
            "Allbridge - Allbridge",
        )
        yield (  # address, symbol, decimals, name
            b"\xc4\xbd\xd2\x7c\x33\xec\x7d\xaa\x6f\xcf\xd8\x53\x2d\xdb\x52\x4b\xf4\x03\x80\x96",
            "atLUNA",
            18,
            "Luna Terra - Allbridge",
        )
        yield (  # address, symbol, decimals, name
            b"\x2a\xb9\x8d\x9e\xa8\x1a\xf2\x00\x37\xaf\x1a\x4f\x43\xcc\x3e\x69\x77\x54\x58\x40",
            "ATO",
            18,
            "Atocha Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\x5c\xe9\xf0\xb6\xaf\xb3\x61\x35\xb5\xdd\xbf\x11\x70\x5c\xeb\x65\xe6\x34\xa9\xdc",
            "atUST",
            18,
            "UST Terra - Allbridge",
        )
        yield (  # address, symbol, decimals, name
            b"\x8b\xec\x47\x86\x5a\xde\x3b\x17\x2a\x92\x8d\xf8\xf9\x90\xbc\x7f\x2a\x3b\x9f\x79",
            "AURORA",
            18,
            "Aurora",
        )
        yield (  # address, symbol, decimals, name
            b"\x89\x73\xc9\xec\x7b\x79\xfe\x88\x06\x97\xcd\xbc\xa7\x44\x89\x26\x82\x76\x4c\x37",
            "BAKED",
            18,
            "BakedToken",
        )
        yield (  # address, symbol, decimals, name
            b"\x41\x48\xd2\xce\x78\x16\xf0\xae\x37\x8d\x98\xb4\x0e\xb3\xa7\x21\x1e\x1f\xcf\x0d",
            "BBT",
            18,
            "BlueBit Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x9f\x1f\x93\x3c\x66\x0a\x1d\xc8\x56\xf0\xe0\xfe\x05\x84\x35\x87\x9c\x5c\xce\xf0",
            "BSTN",
            18,
            "Bastion",
        )
        yield (  # address, symbol, decimals, name
            b"\xe3\x52\x03\x49\xf4\x77\xa5\xf6\xeb\x06\x10\x70\x66\x04\x85\x08\x49\x8a\x29\x1b",
            "DAI",
            18,
            "Dai Stablecoin",
        )
        yield (  # address, symbol, decimals, name
            b"\xe3\x01\xed\x8c\x76\x30\xc9\x67\x8c\x39\xe4\xe4\x51\x93\xd1\xe7\xdf\xb9\x14\xf7",
            "DODO",
            18,
            "DODO bird",
        )
        yield (  # address, symbol, decimals, name
            b"\x17\xcb\xd9\xc2\x74\xe9\x0c\x53\x77\x90\xc5\x1b\x40\x15\xa6\x5c\xd0\x15\x49\x7e",
            "ETHERNAL",
            18,
            "ETHERNAL",
        )
        yield (  # address, symbol, decimals, name
            b"\xd5\xc9\x97\x72\x4e\x4b\x57\x56\xd0\x8e\x64\x64\xc0\x1a\xfb\xc5\xf6\x39\x72\x36",
            "FAME",
            18,
            "FAME",
        )
        yield (  # address, symbol, decimals, name
            b"\xea\x62\x79\x1a\xa6\x82\xd4\x55\x61\x4e\xaa\x2a\x12\xba\x3d\x9a\x2f\xd1\x97\xaf",
            "FLX",
            18,
            "Flux Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xe4\xeb\x03\x59\x8f\x4d\xca\xb7\x40\x33\x1f\xa4\x32\xf4\xb8\x5f\xf5\x8a\xa9\x7e",
            "KSW",
            18,
            "KillSwitchToken",
        )
        yield (  # address, symbol, decimals, name
            b"\x91\x8d\xbe\x08\x70\x40\xa4\x1b\x78\x6f\x0d\xa8\x31\x90\xc2\x93\xda\xe2\x47\x49",
            "LINEAR",
            24,
            "LiNEAR",
        )
        yield (  # address, symbol, decimals, name
            b"\x25\xe8\x01\xeb\x75\x85\x9b\xa4\x05\x2c\x4a\xc4\x23\x3c\xec\x02\x64\xea\xdf\x8c",
            "LUNAR",
            18,
            "LUNAR",
        )
        yield (  # address, symbol, decimals, name
            b"\xa3\x3c\x3b\x53\x69\x44\x19\x82\x47\x22\xc1\x0d\x99\xad\x7c\xb1\x6e\xa6\x27\x54",
            "MECHA",
            18,
            "Mecha",
        )
        yield (  # address, symbol, decimals, name
            b"\x3a\xc5\x5e\xa8\xd2\x08\x2f\xab\xda\x67\x42\x70\xcd\x23\x67\xda\x96\x09\x28\x89",
            "ORBITAL",
            18,
            "ORBITAL",
        )
        yield (  # address, symbol, decimals, name
            b"\x34\xf2\x91\x93\x4b\x88\xc7\x87\x0b\x7a\x17\x83\x5b\x92\x6b\x26\x4f\xc1\x3a\x81",
            "PAD",
            18,
            "SmartPad token",
        )
        yield (  # address, symbol, decimals, name
            b"\x88\x5f\x8c\xf6\xe4\x5b\xdd\x3f\xdc\xdc\x64\x4e\xfd\xcd\x0a\xc9\x38\x80\xc7\x81",
            "PAD",
            18,
            "NearPad Token",
        )
        yield (  # address, symbol, decimals, name
            b"\x29\x1c\x8f\xce\xac\xa3\x34\x2b\x29\xcc\x36\x17\x1d\xeb\x98\x10\x6f\x71\x2c\x66",
            "PICKLE",
            18,
            "PickleToken",
        )
        yield (  # address, symbol, decimals, name
            b"\x09\xc9\xd4\x64\xb5\x8d\x96\x83\x7f\x8d\x8b\x6f\x4d\x9f\xe4\xad\x40\x8d\x3a\x4f",
            "PLY",
            18,
            "Aurigami Token",
        )
        yield (  # address, symbol, decimals, name
            b"\xf0\xf3\xb9\xee\xe3\x2b\x1f\x49\x0a\x4b\x87\x20\xcf\x6f\x00\x5d\x4a\xe9\xea\x86",
            "POLAR",
            18,
            "POLAR",
        )
        yield (  # address, symbol, decimals, name
            b"\x9d\x6f\xc9\x0b\x25\x97\x6e\x40\xad\xad\x5a\x3e\xdd\x08\xaf\x9e\xd7\xa2\x17\x29",
            "SPOLAR",
            18,
            "SPOLAR",
        )
        yield (  # address, symbol, decimals, name
            b"\x07\xf9\xf7\xf9\x63\xc5\xcd\x2b\xbf\xfd\x30\xcc\xfb\x96\x4b\xe1\x14\x33\x2e\x30",
            "STNEAR",
            24,
            "Staked NEAR",
        )
        yield (  # address, symbol, decimals, name
            b"\xfa\x94\x34\x84\x67\xf6\x4d\x5a\x45\x7f\x75\xf8\xbc\x40\x49\x5d\x33\xc6\x5a\xbb",
            "TRI",
            18,
            "Trisolaris",
        )
        yield (  # address, symbol, decimals, name
            b"\x60\x52\x7a\x27\x51\xa8\x27\xec\x0a\xdf\x86\x1e\xfc\xac\xbf\x11\x15\x87\xd7\x48",
            "TRIPOLAR",
            18,
            "TRIPOLAR",
        )
        yield (  # address, symbol, decimals, name
            b"\x98\x4c\x25\x05\xa1\x4d\xa7\x32\xd7\x27\x14\x16\x35\x6f\x53\x59\x53\x61\x03\x40",
            "UMINT",
            18,
            "YouMinter",
        )
        yield (  # address, symbol, decimals, name
            b"\xb1\x2b\xfc\xa5\xa5\x58\x06\xaa\xf6\x4e\x99\x52\x19\x18\xa4\xbf\x0f\xc4\x08\x02",
            "USDC",
            6,
            "USD Coin",
        )
        yield (  # address, symbol, decimals, name
            b"\x49\x88\xa8\x96\xb1\x22\x72\x18\xe4\xa6\x86\xfd\xe5\xea\xbd\xca\xbd\x91\x57\x1f",
            "USDT",
            6,
            "Tether",
        )
        yield (  # address, symbol, decimals, name
            b"\x51\x83\xe1\xb1\x09\x18\x04\xbc\x26\x02\x58\x69\x19\xe6\x88\x0a\xc1\xcf\x28\x96",
            "USN",
            18,
            "USN",
        )
        yield (  # address, symbol, decimals, name
            b"\xa6\x9d\x9b\xa0\x86\xd4\x14\x25\xf3\x59\x88\x61\x3c\x15\x6d\xb9\xa8\x8a\x1a\x96",
            "USP",
            18,
            "USP",
        )
        yield (  # address, symbol, decimals, name
            b"\x7f\xaa\x64\xfa\xf5\x47\x50\xa2\xe3\xee\x62\x11\x66\x63\x5f\xea\xf4\x06\xab\x22",
            "WANNA",
            18,
            "WannaSwap",
        )
        yield (  # address, symbol, decimals, name
            b"\xf4\xeb\x21\x7b\xa2\x45\x46\x13\xb1\x5d\xbd\xea\x6e\x5f\x22\x27\x64\x10\xe8\x9e",
            "WBTC",
            8,
            "Wrapped BTC",
        )
        yield (  # address, symbol, decimals, name
            b"\xc4\x2c\x30\xac\x6c\xc1\x5f\xac\x9b\xd9\x38\x61\x8b\xca\xa1\xa1\xfa\xe8\x50\x1d",
            "wNEAR",
            24,
            "Wrapped NEAR fungible token",
        )
        yield (  # address, symbol, decimals, name
            b"\x7c\xa1\xc2\x86\x63\xb7\x6c\xfd\xe4\x24\xa9\x49\x45\x55\xb9\x48\x46\x20\x55\x85",
            "XNL",
            18,
            "Chronicle",
        )
        yield (  # address, symbol, decimals, name
            b"\x80\x21\x19\xe4\xe2\x53\xd5\xc1\x9a\xa0\x6a\x5d\x56\x7c\x5a\x41\x59\x6d\x68\x03",
            "xTRI",
            18,
            "TriBar",
        )
