from typing import Iterator

REVOKE_DELEGATOR = "0x0000000000000000000000000000000000000000"


def is_registered_delegator(chain_id: int, address: str) -> bool:
    for _name, addr, _initial_data in _delegator_iterator(chain_id):
        if address.lower() == addr.lower():
            return True
    return False


def get_delegator_info(chain_id: int, address: str) -> tuple[str, str, str]:
    for name, addr, initial_data in _delegator_iterator(chain_id):
        if address.lower() == addr.lower():
            return name, _get_delegator_provider_icon(name), initial_data
    raise ValueError("Invalid delegator address")


def _get_delegator_provider_icon(delegator_name: str) -> str:
    if delegator_name == "OKX":
        return "A:/res/provider-okx.png"
    elif delegator_name == "MetaMask":
        return "A:/res/mm-logo-96.png"
    elif delegator_name in ("Revoke", "Simple7702"):
        return "A:/res/icon-send.png"
    return "A:/res/icon-send.png"


def is_revoke_delegator(address: str) -> bool:
    return address.lower() == REVOKE_DELEGATOR


def _delegator_iterator(chain_id: int) -> Iterator[tuple[str, str, str]]:
    yield (
        "Revoke",
        REVOKE_DELEGATOR,
        "",  # can be any value
    )
    # if chain_id == 1:  # ETH
    yield (  # delegator provider name, delegator address, initial hex data
        "OKX",
        "0x80296FF8D1ED46f8e3C7992664D13B833504c2Bb",
        "8129fc1c",  # method id of `initialize()`
    )
    yield (
        "MetaMask",
        "0x63c0c19a282a1B52b07dD5a65b58948A07DAE32B",
        "",  # can only be empty value
    )
    yield (
        "Simple7702",
        "0x4Cd241E8d1510e30b2076397afc7508Ae59C66c9",
        "",  # can only be empty value
    )
