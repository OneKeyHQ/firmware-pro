from typing import TYPE_CHECKING

from storage import device
from trezor import wire
from trezor.crypto import rlp
from trezor.crypto.curve import secp256k1
from trezor.crypto.hashlib import sha3_256
from trezor.messages import (
    EthereumAccessListOneKey as EthereumAccessList,
    EthereumTxRequestOneKey as EthereumTxRequest,
)
from trezor.ui.layouts import confirm_final
from trezor.utils import HashWriter

from apps.common import paths

from .. import networks
from ..helpers import (
    address_from_bytes,
    bytes_from_address,
    get_color_and_icon,
    get_display_network_name,
)
from ..layout import (
    require_confirm_eip1559_erc20_approve,
    require_confirm_eip1559_fee,
    require_show_approve_overview,
    require_show_overview,
)
from .keychain import with_keychain_from_chain_id
from .sign_tx import (
    check_common_fields,
    handle_approve,
    handle_erc20,
    handle_erc_721_or_1155,
    handle_safe_tx,
    is_safe_tx,
    send_request_chunk,
)

if TYPE_CHECKING:
    from trezor.messages import EthereumSignTxEIP1559OneKey as EthereumSignTxEIP1559

    from apps.common.keychain import Keychain

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
    ctx: wire.Context, msg: EthereumSignTxEIP1559, keychain: Keychain
) -> EthereumTxRequest:
    check(msg)

    await paths.validate_path(ctx, keychain, msg.address_n, force_strict=False)

    approve_info = await handle_approve(ctx, msg)

    if approve_info:
        token = None
        recipient = address_bytes = bytes_from_address(msg.to)
        value = 0
    else:
        token, address_bytes, recipient, value = await handle_erc20(ctx, msg)

    data_total = msg.data_length
    if msg.chain_id:
        network = networks.by_chain_id(msg.chain_id)
    else:
        if len(msg.address_n) > 1:  # path has slip44 network identifier
            network = networks.by_slip44(msg.address_n[1] & 0x7FFF_FFFF)
        else:
            network = networks.UNKNOWN_NETWORK

    ctx.primary_color, ctx.icon_path = get_color_and_icon(
        network.chain_id if network else None
    )

    is_nft_transfer = False
    _is_safe_tx = False
    token_id = None
    from_addr = None
    if token is None:
        res = await handle_erc_721_or_1155(ctx, msg)
        if res is not None:
            is_nft_transfer = True
            from_addr, recipient, token_id, value = res
        else:
            _is_safe_tx = is_safe_tx(msg)

    if device.is_turbomode_enabled():
        from trezor.lvglui.i18n import gettext as _, keys as i18n_keys

        if is_nft_transfer:
            suffix = f"{value} NFT"
        elif token:
            suffix = (
                token.symbol
                if token.symbol != "Wei UNKN"
                else _(i18n_keys.TITLE__UNKNOWN_TOKEN)
            )
        else:
            suffix = networks.shortcut_by_chain_id(msg.chain_id)

        from trezor.ui.layouts.lvgl import confirm_turbo

        await confirm_turbo(ctx, (_(i18n_keys.LIST_VALUE__SEND) + suffix), network.name)

    elif approve_info:
        from .providers import provider_by_chain_address

        provider = provider_by_chain_address(
            msg.chain_id, address_from_bytes(approve_info.spender, network)
        )

        show_details = await require_show_approve_overview(
            ctx,
            approve_info.spender,
            approve_info.value,
            approve_info.token,
            approve_info.token_address,
            int.from_bytes(msg.max_gas_fee, "big"),
            int.from_bytes(msg.gas_limit, "big"),
            msg.chain_id,
            provider_name=provider.name if provider else None,
            provider_icon_path=provider.icon_path if provider else None,
        )

        if show_details:
            node = keychain.derive(msg.address_n, force_strict=False)
            from_str = address_from_bytes(
                from_addr or node.ethereum_pubkeyhash(), network
            )

            await require_confirm_eip1559_erc20_approve(
                ctx,
                approve_info.value,
                int.from_bytes(msg.max_priority_fee, "big"),
                int.from_bytes(msg.max_gas_fee, "big"),
                int.from_bytes(msg.gas_limit, "big"),
                msg.chain_id,
                approve_info.token,
                from_address=from_str,
                to_address=address_from_bytes(approve_info.spender, network),
                token_address=address_from_bytes(approve_info.token_address, network),
                token_id=None,
                evm_chain_id=None
                if network is not networks.UNKNOWN_NETWORK
                else msg.chain_id,
                raw_data=None,
                provider_name=provider.name if provider else None,
                provider_icon=provider.icon_path if provider else None,
            )

    else:
        if __debug__:
            print("is_safe_tx", _is_safe_tx)
        if _is_safe_tx:
            node = keychain.derive(msg.address_n, force_strict=False)

            from_str = address_from_bytes(node.ethereum_pubkeyhash(), network)
            await handle_safe_tx(ctx, msg, from_str, True)
        else:
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
                from_str = address_from_bytes(
                    from_addr or node.ethereum_pubkeyhash(), network
                )
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
                    token_address=address_from_bytes(address_bytes, network)
                    if token
                    else None,
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

    if not device.is_turbomode_enabled():
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
