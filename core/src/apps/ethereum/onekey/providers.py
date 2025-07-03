from typing import Iterator


class ProviderInfo:
    def __init__(
        self,
        name: str | None,
        approve_address: str | None,
        swap_address: str | None = None,
        icon_path: str | None = None,
    ):
        self.name = name
        self.approve_address = approve_address
        self.swap_address = swap_address
        self.icon_path = icon_path


PROVIDER_META = {
    "1inch": "A:/res/provider-1inch.png",
    "OKX": "A:/res/provider-okx.png",
    "0x": "A:/res/provider-0x.png",
    "CoW": "A:/res/provider-cowswap.png",
    "Socket": "A:/res/provider-socket.png",
}


UNKNOWN_PROVIDER = ProviderInfo(
    name=None,
    approve_address=None,
    swap_address=None,
    icon_path=None,
)


def provider_by_chain_address(chain_id: int, address: str) -> ProviderInfo:
    for approve_addr, name, swap_addr in _provider_iterator(chain_id):
        if address.lower() == approve_addr.lower():
            icon_path = PROVIDER_META.get(name)
            return ProviderInfo(
                name=name,
                approve_address=approve_addr,
                swap_address=swap_addr,
                icon_path=icon_path,
            )
    return UNKNOWN_PROVIDER


def get_provider_icon(provider_name: str) -> str | None:
    return PROVIDER_META.get(provider_name)


def _provider_iterator(chain_id: int) -> Iterator[tuple[str, str, str | None]]:
    if chain_id == 1:  # Ethereum Mainnet
        yield (
            "0x111111125421cA6dc452d289314280a0f8842A65",
            "1inch",
            None,
        )
        yield (
            "0x40aA958dd87FC8305b97f2BA922CDdCa374bcD7f",
            "OKX",
            None,
        )
        yield (
            "0x0000000000001fF3684f28c67538d4D072C22734",
            "0x",
            None,
        )
        yield (
            "0xC92E8bdf79f0507f65a392b0ab4667716BFE0110",
            "CoW",
            None,
        )
        yield (
            "0x3a23F943181408EAC424116Af7b7790c94Cb97a5",
            "Socket",
            None,
        )

    if chain_id == 10:  # Optimism
        yield (
            "0x111111125421cA6dc452d289314280a0f8842A65",
            "1inch",
            None,
        )
        yield (
            "0x68D6B739D2020067D1e2F713b999dA97E4d54812",
            "OKX",
            None,
        )
        yield (
            "0x0000000000001fF3684f28c67538d4D072C22734",
            "0x",
            None,
        )
        yield (
            "0x3a23F943181408EAC424116Af7b7790c94Cb97a5",
            "Socket",
            None,
        )

    if chain_id == 56:  # BSC/BNB Chain
        yield (
            "0x111111125421cA6dc452d289314280a0f8842A65",
            "1inch",
            None,
        )
        yield (
            "0x2c34A2Fb1d0b4f55de51E1d0bDEfaDDce6b7cDD6",
            "OKX",
            None,
        )
        yield (
            "0x0000000000001fF3684f28c67538d4D072C22734",
            "0x",
            None,
        )
        yield (
            "0x3a23F943181408EAC424116Af7b7790c94Cb97a5",
            "Socket",
            None,
        )

    if chain_id == 66:  # OKC (OKX Chain)
        yield (
            "0x70cBb871E8f30Fc8Ce23609E9E0Ea87B6b222F58",
            "OKX",
            None,
        )

    if chain_id == 100:  # Gnosis Chain
        yield (
            "0x3a23F943181408EAC424116Af7b7790c94Cb97a5",
            "Socket",
            None,
        )

    if chain_id == 137:  # Polygon
        yield (
            "0x111111125421cA6dc452d289314280a0f8842A65",
            "1inch",
            None,
        )
        yield (
            "0x3B86917369B83a6892f553609F3c2F439C184e31",
            "OKX",
            None,
        )
        yield (
            "0x0000000000001fF3684f28c67538d4D072C22734",
            "0x",
            None,
        )
        yield (
            "0x3a23F943181408EAC424116Af7b7790c94Cb97a5",
            "Socket",
            None,
        )

    if chain_id == 146:  # Sonic
        yield (
            "0xd321ab5589d3e8fa5df985ccfef625022e2dd910",
            "OKX",
            None,
        )

    if chain_id == 250:  # Fantom
        yield (
            "0x111111125421cA6dc452d289314280a0f8842A65",
            "1inch",
            None,
        )
        yield (
            "0x70cBb871E8f30Fc8Ce23609E9E0Ea87B6b222F58",
            "OKX",
            None,
        )
        yield (
            "0x3a23F943181408EAC424116Af7b7790c94Cb97a5",
            "Socket",
            None,
        )

    if chain_id == 324:  # zkSync Era
        yield (
            "0xc67879F4065d3B9fe1C09EE990B891Aa8E3a4c2f",
            "OKX",
            None,
        )

    if chain_id == 1088:  # Metis
        yield (
            "0x57df6092665eb6058DE53939612413ff4B09114E",
            "OKX",
            None,
        )

    if chain_id == 1101:  # Polygon zkEVM
        yield (
            "0x57df6092665eb6058DE53939612413ff4B09114E",
            "OKX",
            None,
        )
        yield (
            "0x3a23F943181408EAC424116Af7b7790c94Cb97a5",
            "Socket",
            None,
        )

    if chain_id == 1329:  # SEI
        yield (
            "0x801D8ED849039007a7170830623180396492c7ED",
            "OKX",
            None,
        )

    if chain_id == 5000:  # Mantle
        yield (
            "0x57df6092665eb6058DE53939612413ff4B09114E",
            "OKX",
            None,
        )
        yield (
            "0x3a23F943181408EAC424116Af7b7790c94Cb97a5",
            "Socket",
            None,
        )

    if chain_id == 7000:  # Zeta
        yield (
            "0x03B5ACdA01207824cc7Bc21783Ee5aa2B8d1D2fE",
            "OKX",
            None,
        )

    if chain_id == 8453:  # Base
        yield (
            "0x111111125421cA6dc452d289314280a0f8842A65",
            "1inch",
            None,
        )
        yield (
            "0x57df6092665eb6058DE53939612413ff4B09114E",
            "OKX",
            None,
        )
        yield (
            "0x0000000000001fF3684f28c67538d4D072C22734",
            "0x",
            None,
        )
        yield (
            "0xC92E8bdf79f0507f65a392b0ab4667716BFE0110",
            "CoW",
            None,
        )
        yield (
            "0x3a23F943181408EAC424116Af7b7790c94Cb97a5",
            "Socket",
            None,
        )

    if chain_id == 34443:  # Mode
        yield (
            "0xbd0EBE49779E154E5042B34D5BcfBc498e4B3249",
            "OKX",
            None,
        )
        yield (
            "0x3a23F943181408EAC424116Af7b7790c94Cb97a5",
            "Socket",
            None,
        )

    if chain_id == 42161:  # Arbitrum
        yield (
            "0x111111125421cA6dc452d289314280a0f8842A65",
            "1inch",
            None,
        )
        yield (
            "0x70cBb871E8f30Fc8Ce23609E9E0Ea87B6b222F58",
            "OKX",
            None,
        )
        yield (
            "0x0000000000001fF3684f28c67538d4D072C22734",
            "0x",
            None,
        )
        yield (
            "0xC92E8bdf79f0507f65a392b0ab4667716BFE0110",
            "CoW",
            None,
        )
        yield (
            "0x3a23F943181408EAC424116Af7b7790c94Cb97a5",
            "Socket",
            None,
        )

    if chain_id == 43114:  # Avalanche C-Chain
        yield (
            "0x111111125421cA6dc452d289314280a0f8842A65",
            "1inch",
            None,
        )
        yield (
            "0x40aA958dd87FC8305b97f2BA922CDdCa374bcD7f",
            "OKX",
            None,
        )
        yield (
            "0x0000000000001fF3684f28c67538d4D072C22734",
            "0x",
            None,
        )
        yield (
            "0x3a23F943181408EAC424116Af7b7790c94Cb97a5",
            "Socket",
            None,
        )

    if chain_id == 59144:  # Linea
        yield (
            "0x57df6092665eb6058DE53939612413ff4B09114E",
            "OKX",
            None,
        )
        yield (
            "0x3a23F943181408EAC424116Af7b7790c94Cb97a5",
            "Socket",
            None,
        )

    if chain_id == 81457:  # Blast
        yield (
            "0x5fD2Dc91FF1dE7FF4AEB1CACeF8E9911bAAECa68",
            "OKX",
            None,
        )
        yield (
            "0x3a23F943181408EAC424116Af7b7790c94Cb97a5",
            "Socket",
            None,
        )

    if chain_id == 130:  # UniChain
        yield (
            "0x2e28281Cf3D58f475cebE27bec4B8a23dFC7782c",
            "OKX",
            None,
        )

    if chain_id == 534352:  # Scroll
        yield (
            "0x57df6092665eb6058DE53939612413ff4B09114E",
            "OKX",
            None,
        )

    if chain_id == 1313161554:  # Aurora (NEAR ecosystem)
        yield (
            "0x3a23F943181408EAC424116Af7b7790c94Cb97a5",
            "Socket",
            None,
        )
