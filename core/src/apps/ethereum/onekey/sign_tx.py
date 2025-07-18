from typing import TYPE_CHECKING

from storage import device
from trezor import wire
from trezor.crypto import rlp
from trezor.crypto.curve import secp256k1
from trezor.crypto.hashlib import sha3_256
from trezor.messages import (
    EthereumSignTxOneKey as EthereumSignTx,
    EthereumTxAckOneKey as EthereumTxAck,
    EthereumTxRequestOneKey as EthereumTxRequest,
)
from trezor.ui.layouts import confirm_final
from trezor.utils import HashWriter

from apps.common import paths

from .. import networks, tokens
from ..helpers import (
    address_from_bytes,
    bytes_from_address,
    get_color_and_icon,
    get_display_network_name,
)
from ..layout import (
    require_confirm_data,
    require_confirm_fee,
    require_confirm_legacy_erc20_approve,
    require_show_approve_overview,
    require_show_overview,
)
from .keychain import with_keychain_from_chain_id

if TYPE_CHECKING:
    from apps.common.keychain import Keychain

    from .keychain import EthereumSignTxAny

# Maximum chain_id which returns the full signature_v (which must fit into an uint32).
# chain_ids larger than this will only return one bit and the caller must recalculate
# the full value: v = 2 * chain_id + 35 + v_bit
MAX_CHAIN_ID = (0xFFFF_FFFF - 36) // 2


@with_keychain_from_chain_id
async def sign_tx(
    ctx: wire.Context, msg: EthereumSignTx, keychain: Keychain
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
    _is_safe_tx = False
    is_nft_transfer = False
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

        if value == 0:
            title = _(i18n_keys.TITLE_REQUEST_CONFIRMATION)
        else:
            title = _(i18n_keys.LIST_VALUE__SEND) + suffix
        await confirm_turbo(ctx, title, network.name)

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
            int.from_bytes(msg.gas_price, "big"),
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

            await require_confirm_legacy_erc20_approve(
                ctx,
                approve_info.value,
                int.from_bytes(msg.gas_price, "big"),
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
        if _is_safe_tx:
            node = keychain.derive(msg.address_n, force_strict=False)

            from_str = address_from_bytes(node.ethereum_pubkeyhash(), network)
            await handle_safe_tx(ctx, msg, from_str, False)
        else:
            has_raw_data = token is None and token_id is None and msg.data_length > 0
            show_details = await require_show_overview(
                ctx,
                recipient,
                value,
                int.from_bytes(msg.gas_price, "big"),
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
                await require_confirm_fee(
                    ctx,
                    value,
                    int.from_bytes(msg.gas_price, "big"),
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
    rlp.write_header(sha, total_length, rlp.LIST_HEADER_BYTE)

    if msg.tx_type is not None:
        rlp.write(sha, msg.tx_type)

    for field in (msg.nonce, msg.gas_price, msg.gas_limit, address_bytes, msg.value):
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

    # eip 155 replay protection
    rlp.write(sha, msg.chain_id)
    rlp.write(sha, 0)
    rlp.write(sha, 0)

    digest = sha.get_digest()
    result = sign_digest(msg, keychain, digest)
    if not device.is_turbomode_enabled():
        await confirm_final(ctx, get_display_network_name(network))
    return result


async def handle_erc20(
    ctx: wire.Context, msg: EthereumSignTxAny
) -> tuple[tokens.EthereumTokenInfo | None, bytes, bytes, int]:
    token = None
    address_bytes = recipient = bytes_from_address(msg.to)
    value = int.from_bytes(msg.value, "big")
    if (
        len(msg.to) in (40, 42)
        and len(msg.value) == 0
        and msg.data_length == 68
        and len(msg.data_initial_chunk) == 68
        and msg.data_initial_chunk[:16]
        == b"\xa9\x05\x9c\xbb\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    ):
        token = tokens.token_by_chain_address(msg.chain_id, address_bytes)
        recipient = msg.data_initial_chunk[16:36]
        value = int.from_bytes(msg.data_initial_chunk[36:68], "big")

    return token, address_bytes, recipient, value


async def handle_erc_721_or_1155(
    ctx: wire.Context, msg: EthereumSignTxAny
) -> None | tuple[bytes, bytes, int, int]:

    from_addr = recipient = None
    token_id = 0
    value = 0
    if (
        len(msg.to) in (40, 42)
        and len(msg.value) == 0
        and msg.data_length
        in (196, 228)  # assume data is 00 aka the recipient is not a contract
        and len(msg.data_initial_chunk) in (196, 228)
        and msg.data_initial_chunk[:16]
        == b"\xf2\x42\x43\x2a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"  # erc1155 f242432a == keccak("safeTransferFrom(address,address,uint256,uint256,bytes)")[:4].hex()
    ):
        from_addr = msg.data_initial_chunk[16:36]
        recipient = msg.data_initial_chunk[48:68]
        token_id = int.from_bytes(msg.data_initial_chunk[68:100], "big")

        value = int.from_bytes(msg.data_initial_chunk[100:132], "big")
        assert (
            int.from_bytes(msg.data_initial_chunk[132:164], "big") == 0xA0
        )  # dyn data position
        data_len = int.from_bytes(msg.data_initial_chunk[164:196], "big")
        if data_len > 0:
            data = msg.data_initial_chunk[-data_len:]
            if not (data_len == 1 and data == b"\x00"):
                await require_confirm_data(ctx, data, data_len)
    elif (
        len(msg.to) in (40, 42)
        and len(msg.value) == 0
        and msg.data_length == 100
        and len(msg.data_initial_chunk) == 100
        and msg.data_initial_chunk[:16]
        == b"\x42\x84\x2e\x0e\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"  # erc721 42842e0e ==  keccak("safeTransferFrom(address,address,uint256)")[:4].hex()
    ):
        from_addr = msg.data_initial_chunk[16:36]
        recipient = msg.data_initial_chunk[48:68]
        token_id = int.from_bytes(msg.data_initial_chunk[68:100], "big")
        value = 1
    if from_addr:
        assert recipient is not None
        return from_addr, recipient, token_id, value
    else:
        return None


class ApproveInfo:
    def __init__(
        self,
        spender: bytes,
        value: int,
        token: tokens.EthereumTokenInfo,
        token_address: bytes,
    ):
        self.spender = spender
        self.value = value
        self.token = token
        self.token_address = token_address


async def handle_approve(
    ctx: wire.Context, msg: EthereumSignTxAny
) -> ApproveInfo | None:
    if (
        len(msg.to) in (40, 42)
        and len(msg.value) == 0
        and msg.data_length == 68
        and len(msg.data_initial_chunk) == 68
        and msg.data_initial_chunk[:16]
        in (
            b"\x09\x5e\xa7\xb3\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            b"\x39\x50\x93\x51\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
        )
    ):

        token_address = bytes_from_address(msg.to)
        spender = msg.data_initial_chunk[16:36]
        value = int.from_bytes(msg.data_initial_chunk[36:68], "big")
        token = tokens.token_by_chain_address(msg.chain_id, token_address)
        return ApproveInfo(spender, value, token, token_address)
    return None


def is_safe_tx(msg: EthereumSignTxAny) -> bool:
    return is_safe_approve_hash(msg) or is_safe_exec_transaction(msg)


def is_safe_approve_hash(msg: EthereumSignTxAny) -> bool:
    if (
        len(msg.to) in (40, 42)
        and len(msg.value) == 0
        and msg.data_length == 36
        and msg.data_initial_chunk[:4] == b"\xd4\xd9\xbd\xcd"
        # approveHash(bytes32 hashToApprove) 0xd4d9bdcd
    ):
        return True
    return False


def is_safe_exec_transaction(msg: EthereumSignTxAny) -> bool:
    if (
        len(msg.to) in (40, 42)
        and len(msg.value) == 0
        and 437 <= msg.data_length <= 1024
        and msg.data_initial_chunk[:16]
        == b"\x6a\x76\x12\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        # 0x6a761202 == keccak("execTransaction(address,uint256,bytes,uint8,uint256,uint256,uint256,address,address,bytes)")[:4].hex()
    ):
        return True
    return False


async def handle_safe_tx(
    ctx: wire.Context, msg: EthereumSignTxAny, from_addr: str, is_eip1559: bool = True
) -> None:
    from binascii import hexlify

    network = networks.by_chain_id(msg.chain_id)
    is_unknown_network = network is networks.UNKNOWN_NETWORK
    if is_safe_exec_transaction(msg):

        data = msg.data_initial_chunk[16:]
        to_addr = data[0:20]
        value = int.from_bytes(data[20:52], "big")
        operation = int.from_bytes(data[84:116], "big")
        safe_tx_gas = int.from_bytes(data[116:148], "big")
        base_gas = int.from_bytes(data[148:180], "big")
        gas_price = int.from_bytes(data[180:212], "big")
        gas_token = data[224:244]
        refund_receiver = data[256:276]
        signature_pos = int.from_bytes(data[276:308], "big")
        data_len = int.from_bytes(data[308:340], "big")
        call_data = None
        call_method = None
        if data_len > 0:
            nest_data = data[340 : 340 + data_len]
            if (
                len(nest_data) == 68
                and nest_data[:16]
                == b"\xa9\x05\x9c\xbb\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            ):  # erc20 transfer
                from ..layout import format_ethereum_amount

                token = tokens.token_by_chain_address(msg.chain_id, to_addr)
                recipient = nest_data[16:36]
                safe_value = int.from_bytes(nest_data[36:68], "big")
                call_data = {
                    "Recipient": address_from_bytes(recipient, network),
                    "Amount": format_ethereum_amount(safe_value, token, msg.chain_id),
                }
                call_method = "[Transfer]"
            elif (
                len(nest_data) in (196, 228)
                and nest_data[:16]
                == b"\xf2\x42\x43\x2a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            ):  # erc1155 safeTransferFrom
                addr_from = nest_data[16:36]
                recipient = nest_data[48:68]
                token_id = int.from_bytes(nest_data[68:100], "big")
                safe_value = int.from_bytes(nest_data[100:132], "big")
                call_data = {
                    "From": address_from_bytes(addr_from, network),
                    "Recipient": address_from_bytes(recipient, network),
                    "Token ID": str(token_id),
                    "Amount": str(safe_value),
                }
                call_method = "[Transfer]"
            elif (
                len(nest_data) >= 100
                and nest_data[:16]
                == b"\x42\x84\x2e\x0e\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                # or nest_data[:16]
                # == b"\xb8\x8d\x4f\xde\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            ):  # erc721 safeTransferFrom
                addr_from = nest_data[16:36]
                recipient = nest_data[48:68]
                token_id = int.from_bytes(nest_data[68:100], "big")
                call_data = {
                    "From": address_from_bytes(addr_from, network),
                    "Recipient": address_from_bytes(recipient, network),
                    "Token ID": str(token_id),
                }
                call_method = "[Transfer]"
            elif (
                len(nest_data) == 68
                and nest_data[:16]
                == b"\x09\x5e\xa7\xb3\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            ):  # erc20/erc721 approve 0x095ea7b3
                spender = nest_data[16:36]
                safe_value = int.from_bytes(nest_data[36:68], "big")
                call_data = {
                    "Spender": address_from_bytes(spender, network),
                    "Amount/ID": str(safe_value),
                }
                call_method = "[Approve]"
            else:
                call_data = hexlify(nest_data).decode()
                call_method = None
        assert signature_pos >= 340 + data_len
        signatures_len = int.from_bytes(data[signature_pos : signature_pos + 20], "big")
        signatures = data[signature_pos + 20 : signature_pos + 20 + signatures_len]
        if not is_eip1559:
            from ..layout import require_confirm_safe_exec_transaction

            # pyright: off
            await require_confirm_safe_exec_transaction(
                ctx,
                from_addr,
                address_from_bytes(bytes_from_address(msg.to), network),
                address_from_bytes(to_addr, network),
                value,
                operation,
                safe_tx_gas,
                base_gas,
                gas_price,
                f"0x{hexlify(gas_token).decode()}",
                f"0x{hexlify(refund_receiver).decode()}",
                f"0x{hexlify(signatures).decode()}",
                int.from_bytes(msg.gas_price, "big"),
                int.from_bytes(msg.gas_limit, "big"),
                int.from_bytes(msg.nonce, "big"),
                msg.chain_id,
                call_data,
                call_method,
                is_unknown_network=is_unknown_network,
            )
            # pyright: on
        else:
            from ..layout import require_confirm_safe_exec_transaction_eip1559

            # pyright: off
            await require_confirm_safe_exec_transaction_eip1559(
                ctx,
                from_addr,
                address_from_bytes(bytes_from_address(msg.to), network),
                address_from_bytes(to_addr, network),
                value,
                operation,
                safe_tx_gas,
                base_gas,
                gas_price,
                f"0x{hexlify(gas_token).decode()}",
                f"0x{hexlify(refund_receiver).decode()}",
                f"0x{hexlify(signatures).decode()}",
                int.from_bytes(msg.nonce, "big"),
                msg.chain_id,
                int.from_bytes(msg.gas_limit, "big"),
                int.from_bytes(msg.max_priority_fee, "big"),
                int.from_bytes(msg.max_gas_fee, "big"),
                call_data,
                call_method,
                is_unknown_network=is_unknown_network,
            )
            # pyright: on
    elif is_safe_approve_hash(msg):
        data = msg.data_initial_chunk[4:]
        hash_to_approve = data[0:32]
        from trezor.ui.layouts import should_show_details

        show_details = await should_show_details(ctx, msg.to, "Safe transaction")
        if show_details:
            if is_eip1559:
                from ..layout import require_confirm_safe_approve_hash_eip1559

                # pyright: off
                await require_confirm_safe_approve_hash_eip1559(
                    ctx,
                    address_from_bytes(bytes_from_address(msg.to), network),
                    from_addr,
                    f"0x{hexlify(hash_to_approve).decode()}",
                    int.from_bytes(msg.nonce, "big"),
                    int.from_bytes(msg.max_priority_fee, "big"),
                    int.from_bytes(msg.max_gas_fee, "big"),
                    int.from_bytes(msg.gas_limit, "big"),
                    msg.chain_id,
                    is_unknown_network=is_unknown_network,
                )
                # pyright: on
            else:
                from ..layout import require_confirm_safe_approve_hash

                # pyright: off
                await require_confirm_safe_approve_hash(
                    ctx,
                    address_from_bytes(bytes_from_address(msg.to), network),
                    from_addr,
                    f"0x{hexlify(hash_to_approve).decode()}",
                    int.from_bytes(msg.nonce, "big"),
                    int.from_bytes(msg.gas_price, "big"),
                    int.from_bytes(msg.gas_limit, "big"),
                    msg.chain_id,
                    is_unknown_network=is_unknown_network,
                )
                # pyright: on
    return


def get_total_length(msg: EthereumSignTx, data_total: int) -> int:
    length = 0
    if msg.tx_type is not None:
        length += rlp.length(msg.tx_type)

    fields: tuple[rlp.RLPItem, ...] = (
        msg.nonce,
        msg.gas_price,
        msg.gas_limit,
        bytes_from_address(msg.to),
        msg.value,
        msg.chain_id,
        0,
        0,
    )

    for field in fields:
        length += rlp.length(field)

    length += rlp.header_length(data_total, msg.data_initial_chunk)
    length += data_total

    return length


async def send_request_chunk(ctx: wire.Context, data_left: int) -> EthereumTxAck:
    # TODO: layoutProgress ?
    req = EthereumTxRequest()
    if data_left <= 1024:
        req.data_length = data_left
    else:
        req.data_length = 1024

    return await ctx.call(req, EthereumTxAck)


def sign_digest(
    msg: EthereumSignTx, keychain: Keychain, digest: bytes
) -> EthereumTxRequest:
    node = keychain.derive(msg.address_n, force_strict=False)
    signature = secp256k1.sign(
        node.private_key(), digest, False, secp256k1.CANONICAL_SIG_ETHEREUM
    )

    req = EthereumTxRequest()
    req.signature_v = signature[0]
    if msg.chain_id > MAX_CHAIN_ID:
        req.signature_v -= 27
    else:
        req.signature_v += 2 * msg.chain_id + 8

    req.signature_r = signature[1:33]
    req.signature_s = signature[33:]

    return req


def check(msg: EthereumSignTx) -> None:
    if msg.tx_type not in [1, 6, None]:
        raise wire.DataError("tx_type out of bounds")

    if len(msg.gas_price) + len(msg.gas_limit) > 30:
        raise wire.DataError("Fee overflow")

    check_common_fields(msg)


def check_common_fields(msg: EthereumSignTxAny) -> None:
    if msg.data_length > 0:
        if not msg.data_initial_chunk:
            raise wire.DataError("Data length provided, but no initial chunk")
        # Our encoding only supports transactions up to 2^24 bytes. To
        # prevent exceeding the limit we use a stricter limit on data length.
        if msg.data_length > 16_000_000:
            raise wire.DataError("Data length exceeds limit")
        if len(msg.data_initial_chunk) > msg.data_length:
            raise wire.DataError("Invalid size of initial chunk")

    if len(msg.to) not in (0, 40, 42):
        raise wire.DataError("Invalid recipient address")

    if not msg.to and msg.data_length == 0:
        # sending transaction to address 0 (contract creation) without a data field
        raise wire.DataError("Contract creation without data")

    if msg.chain_id == 0:
        raise wire.DataError("Chain ID out of bounds")
