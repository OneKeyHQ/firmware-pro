from typing import TYPE_CHECKING

from trezor import wire
from trezor.crypto import rlp
from trezor.crypto.curve import secp256k1
from trezor.crypto.hashlib import sha3_256
from trezor.messages import EthereumAccessList, EthereumTxRequest
from trezor.ui.layouts import confirm_final
from trezor.utils import HashWriter

from apps.common import paths

from . import networks
from .helpers import (
    address_from_bytes,
    bytes_from_address,
    get_color_and_icon,
    get_display_network_name,
)
from .keychain import with_keychain_from_chain_id
from .layout import require_confirm_eip1559_fee, require_show_overview
from .sign_tx import (
    check_common_fields,
    handle_erc20,
    handle_erc_721_or_1155,
    send_request_chunk,
)

if TYPE_CHECKING:
    from trezor.messages import EthereumSignTxEIP1559

    from apps.common.keychain import Keychain
    from .definitions import Definitions

TX_TYPE = 2


def access_list_item_length(item: EthereumAccessList) -> int:
    address_length = rlp.length(bytes_from_address(item.address))
    keys_length = rlp.length(item.storage_keys)
    return (
        rlp.header_length(address_length + keys_length) + address_length + keys_length
    )


def access_list_length(access_list: list[EthereumAccessList]) -> int:
    payload_length = sum(access_list_item_length(i) for i in access_list)
    return rlp.header_length(payload_length) + payload_length


def write_access_list(w: HashWriter, access_list: list[EthereumAccessList]) -> None:
    payload_length = sum(access_list_item_length(i) for i in access_list)
    rlp.write_header(w, payload_length, rlp.LIST_HEADER_BYTE)
    for item in access_list:
        address_bytes = bytes_from_address(item.address)
        address_length = rlp.length(address_bytes)
        keys_length = rlp.length(item.storage_keys)
        rlp.write_header(w, address_length + keys_length, rlp.LIST_HEADER_BYTE)
        rlp.write(w, address_bytes)
        rlp.write(w, item.storage_keys)


@with_keychain_from_chain_id
async def sign_tx_eip1559(
    ctx: wire.Context, msg: EthereumSignTxEIP1559, keychain: Keychain, defs: Definitions
) -> EthereumTxRequest:
    check(msg)

    await paths.validate_path(ctx, keychain, msg.address_n, force_strict=False)

    # Handle ERC20s
    token, address_bytes, recipient, value = await handle_erc20(ctx, msg)

    data_total = msg.data_length
    network = defs.network
    ctx.primary_color, ctx.icon_path = get_color_and_icon(
        network.chain_id if network else None
    )
    is_nft_transfer = False
    token_id = None
    from_addr = None
    if token is None:
        res = await handle_erc_721_or_1155(ctx, msg)
        if res is not None:
            is_nft_transfer = True
            from_addr, recipient, token_id, value = res
    has_raw_data = token is None and token_id is None and msg.data_length > 0
    show_details = await require_show_overview(
        ctx,
        recipient,
        value,
        int.from_bytes(msg.max_gas_fee, "big"),
        int.from_bytes(msg.gas_limit, "big"),
        msg.chain_id,
        token,
        address_from_bytes(address_bytes, network) if token else None,
        is_nft_transfer,
        has_raw_data,
    )
    if show_details:
        node = keychain.derive(msg.address_n, force_strict=False)

        recipient_str = address_from_bytes(recipient, network)
        from_str = address_from_bytes(from_addr or node.ethereum_pubkeyhash(), network)
        await require_confirm_eip1559_fee(
            ctx,
            value,
            int.from_bytes(msg.max_priority_fee, "big"),
            int.from_bytes(msg.max_gas_fee, "big"),
            int.from_bytes(msg.gas_limit, "big"),
            msg.chain_id,
            token,
            from_address=from_str,
            to_address=recipient_str,
            contract_addr=address_from_bytes(address_bytes, network)
            if token_id is not None
            else None,
            token_id=token_id,
            evm_chain_id=None
            if network is not networks.UNKNOWN_NETWORK
            else msg.chain_id,
            raw_data=msg.data_initial_chunk if has_raw_data else None,
        )
    data = bytearray()
    data += msg.data_initial_chunk
    data_left = data_total - len(msg.data_initial_chunk)

    total_length = get_total_length(msg, data_total)

    sha = HashWriter(sha3_256(keccak=True))

    rlp.write(sha, TX_TYPE)

    rlp.write_header(sha, total_length, rlp.LIST_HEADER_BYTE)

    fields: tuple[rlp.RLPItem, ...] = (
        msg.chain_id,
        msg.nonce,
        msg.max_priority_fee,
        msg.max_gas_fee,
        msg.gas_limit,
        address_bytes,
        msg.value,
    )
    for field in fields:
        rlp.write(sha, field)

    if data_left == 0:
        rlp.write(sha, data)
    else:
        rlp.write_header(sha, data_total, rlp.STRING_HEADER_BYTE, data)
        sha.extend(data)

    while data_left > 0:
        resp = await send_request_chunk(ctx, data_left)
        data_left -= len(resp.data_chunk)
        sha.extend(resp.data_chunk)

    write_access_list(sha, msg.access_list)

    digest = sha.get_digest()
    result = sign_digest(msg, keychain, digest)
    await confirm_final(ctx, get_display_network_name(network))
    return result


def get_total_length(msg: EthereumSignTxEIP1559, data_total: int) -> int:
    length = 0

    fields: tuple[rlp.RLPItem, ...] = (
        msg.nonce,
        msg.gas_limit,
        bytes_from_address(msg.to),
        msg.value,
        msg.chain_id,
        msg.max_gas_fee,
        msg.max_priority_fee,
    )
    for field in fields:
        length += rlp.length(field)

    length += rlp.header_length(data_total, msg.data_initial_chunk)
    length += data_total

    length += access_list_length(msg.access_list)

    return length


def sign_digest(
    msg: EthereumSignTxEIP1559, keychain: Keychain, digest: bytes
) -> EthereumTxRequest:
    node = keychain.derive(msg.address_n, force_strict=False)
    signature = secp256k1.sign(
        node.private_key(), digest, False, secp256k1.CANONICAL_SIG_ETHEREUM
    )

    req = EthereumTxRequest()
    req.signature_v = signature[0] - 27
    req.signature_r = signature[1:33]
    req.signature_s = signature[33:]

    return req


def check(msg: EthereumSignTxEIP1559) -> None:
    if len(msg.max_gas_fee) + len(msg.gas_limit) > 30:
        raise wire.DataError("Fee overflow")
    if len(msg.max_priority_fee) + len(msg.gas_limit) > 30:
        raise wire.DataError("Fee overflow")

    check_common_fields(msg)
