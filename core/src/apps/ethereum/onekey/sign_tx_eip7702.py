from typing import TYPE_CHECKING

from storage import device
from trezor import wire
from trezor.crypto import rlp
from trezor.crypto.curve import secp256k1
from trezor.crypto.hashlib import sha3_256
from trezor.messages import (
    EthereumAuthorizationOneKey as EthereumAuthorizationOneKey,
    EthereumAuthorizationSignature as EthereumAuthorizationSignature,
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
    require_confirm_eip7702,
    require_show_overview_eip7702,
    show_invalid_delegate,
)
from .eip7702_delegators import (
    get_delegator_info,
    is_registered_delegator,
    is_revoke_delegator,
)
from .keychain import with_keychain_from_chain_id
from .sign_tx_eip1559 import access_list_length, send_request_chunk, write_access_list

if TYPE_CHECKING:
    from trezor.messages import EthereumSignTxEIP7702OneKey as EthereumSignTxEIP7702

    from apps.common.keychain import Keychain

TX_TYPE = 4
AUTHORIZATION_MAGIC = 0x05


def authorization_list_item_length(item: EthereumAuthorizationOneKey) -> int:
    assert item.signature is not None, "Authorization signature is required"
    return rlp.length(
        [
            item.chain_id,
            bytes_from_address(item.address),
            item.nonce,
            item.signature.y_parity,
            item.signature.r,
            item.signature.s,
        ]
    )


def authorization_list_length(
    authorization_list: list[EthereumAuthorizationOneKey],
) -> int:
    payload_length = sum(
        authorization_list_item_length(item) for item in authorization_list
    )
    return rlp.header_length(payload_length) + payload_length


def write_authorization_list(
    w: HashWriter, authorization_list: list[EthereumAuthorizationOneKey]
) -> None:
    payload_length = sum(
        authorization_list_item_length(item) for item in authorization_list
    )
    rlp.write_header(w, payload_length, rlp.LIST_HEADER_BYTE)
    for item in authorization_list:
        assert item.signature is not None, "Authorization signature is required"
        rlp.write(
            w,
            [
                item.chain_id,
                bytes_from_address(item.address),
                item.nonce,
                item.signature.y_parity,
                item.signature.r,
                item.signature.s,
            ],
        )


@with_keychain_from_chain_id
async def sign_tx_eip7702(
    ctx: wire.Context, msg: EthereumSignTxEIP7702, keychain: Keychain
) -> EthereumTxRequest:
    check(msg)
    delegate_addr = msg.authorization_list[0].address
    delegate_chain_id = msg.authorization_list[0].chain_id
    await validate_delegate(ctx, delegate_addr, delegate_chain_id)
    await paths.validate_path(ctx, keychain, msg.address_n, force_strict=False)

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
    recipient = address_bytes = bytes_from_address(msg.to)
    value = int.from_bytes(msg.value, "big")
    node = keychain.derive(msg.address_n, force_strict=False)
    from_str = address_from_bytes(node.ethereum_pubkeyhash(), network)

    if delegate_chain_id == 0:
        delegate_network_name = "ALL"
    else:
        delegate_network_name = get_display_network_name(network)
        if delegate_network_name == "EVM":
            delegate_network_name = str(delegate_chain_id)
    delegator_name, delegator_icon_path, initial_data = get_delegator_info(
        delegate_chain_id, delegate_addr
    )
    if not is_revoke_delegator(delegate_addr):
        from binascii import unhexlify

        if initial_data:
            calldata = unhexlify(initial_data)
            if calldata != msg.data_initial_chunk:
                raise wire.DataError("Invalid calldata provided")
        elif msg.data_initial_chunk:
            raise wire.DataError("Calldata is expected to be empty")
    show_details = await require_show_overview_eip7702(
        ctx,
        from_str,
        delegate_addr,
        delegate_network_name,
        delegator_name,
        delegator_icon_path,
    )
    if show_details:
        node = keychain.derive(msg.address_n, force_strict=False)
        _recipient_str = address_from_bytes(recipient, network)  # noqa: F841
        await require_confirm_eip7702(
            ctx,
            from_str,
            delegate_addr,
            delegate_network_name,
            value,
            int.from_bytes(msg.nonce, "big"),
            int.from_bytes(msg.max_priority_fee, "big"),
            int.from_bytes(msg.max_gas_fee, "big"),
            int.from_bytes(msg.gas_limit, "big"),
            msg.chain_id,
            delegator_icon_path,
        )
    authorization_signatures = sign_authorization(msg, keychain)
    authorization_list = [
        EthereumAuthorizationOneKey(
            address=authorization.address,
            chain_id=authorization.chain_id,
            nonce=authorization.nonce,
            signature=signature,
        )
        for authorization, signature in zip(
            msg.authorization_list, authorization_signatures
        )
    ]
    data = bytearray()
    data += msg.data_initial_chunk
    data_left = data_total - len(msg.data_initial_chunk)

    total_length = get_total_length(msg, data_total, authorization_list)

    sha = HashWriter(sha3_256(keccak=True))

    sha.append(TX_TYPE)

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
    write_authorization_list(sha, authorization_list)

    digest = sha.get_digest()
    result = sign_digest(msg, keychain, digest, authorization_signatures)

    if not device.is_turbomode_enabled():
        await confirm_final(ctx, get_display_network_name(network))

    return result


def get_total_length(
    msg: EthereumSignTxEIP7702,
    data_total: int,
    authorization_list: list[EthereumAuthorizationOneKey],
) -> int:
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
    length += authorization_list_length(authorization_list)
    return length


def sign_digest(
    msg: EthereumSignTxEIP7702,
    keychain: Keychain,
    digest: bytes,
    authorization_signatures: list[EthereumAuthorizationSignature],
) -> EthereumTxRequest:
    node = keychain.derive(msg.address_n, force_strict=False)
    signature = secp256k1.sign(
        node.private_key(), digest, False, secp256k1.CANONICAL_SIG_ETHEREUM
    )

    req = EthereumTxRequest()
    req.signature_v = signature[0] - 27
    req.signature_r = signature[1:33]
    req.signature_s = signature[33:]
    req.authorization_signatures = authorization_signatures

    return req


def check(msg: EthereumSignTxEIP7702) -> None:
    if len(msg.max_gas_fee) + len(msg.gas_limit) > 30:
        raise wire.DataError("Fee overflow")
    if len(msg.max_priority_fee) + len(msg.gas_limit) > 30:
        raise wire.DataError("Fee overflow")

    check_common_fields(msg)


def make_authorization_digest(msg: EthereumAuthorizationOneKey) -> bytes:
    keccak_256 = HashWriter(sha3_256(keccak=True))
    keccak_256.append(AUTHORIZATION_MAGIC)
    rlp.write(keccak_256, [msg.chain_id, bytes_from_address(msg.address), msg.nonce])
    digest = keccak_256.get_digest()
    return digest


def sign_authorization(
    msg: EthereumSignTxEIP7702, keychain: Keychain
) -> list[EthereumAuthorizationSignature]:
    authorization_signatures: list[EthereumAuthorizationSignature] = []
    for authorization in msg.authorization_list:
        if authorization.address_n:
            node = keychain.derive(authorization.address_n, force_strict=False)
        else:
            node = keychain.derive(msg.address_n, force_strict=False)
        if not authorization.signature:
            digest = make_authorization_digest(authorization)
            signature = secp256k1.sign(
                node.private_key(), digest, False, secp256k1.CANONICAL_SIG_ETHEREUM
            )
            authorization_signatures.append(
                EthereumAuthorizationSignature(
                    y_parity=signature[0] - 27, r=signature[1:33], s=signature[33:]
                )
            )
        else:
            authorization_signatures.append(authorization.signature)

    return authorization_signatures


def check_common_fields(msg: EthereumSignTxEIP7702) -> None:
    if msg.data_length > 0:
        if not msg.data_initial_chunk:
            raise wire.DataError("Data length provided, but no initial chunk")
        # Our encoding only supports transactions up to 2^24 bytes. To
        # prevent exceeding the limit we use a stricter limit on data length.
        if msg.data_length > 16_000_000:
            raise wire.DataError("Data length exceeds limit")
        if len(msg.data_initial_chunk) > msg.data_length:
            raise wire.DataError("Invalid size of initial chunk")

    if len(msg.to) not in (40, 42):
        raise wire.DataError("Invalid recipient address")

    if msg.chain_id == 0:
        raise wire.DataError("Chain ID out of bounds")
    if len(msg.authorization_list) == 0:
        raise wire.DataError("Authorization list is empty")
    if len(msg.authorization_list) > 1:
        raise wire.DataError("Only support Self-sponsoring transaction now")
    for authorization in msg.authorization_list:
        if authorization.chain_id not in (0, msg.chain_id):
            raise wire.DataError("Authorization chain ID invalid")
        if len(authorization.address) != 42:
            raise wire.DataError("Authorization address invalid, should start with 0x")
        nonce = int.from_bytes(authorization.nonce, "big")
        if nonce >= 2**64:
            raise wire.DataError("Authorization nonce invalid")
        if len(authorization.address_n) > 0 and authorization.signature:
            raise wire.DataError("Authorization contains both address_n and signature")
        elif authorization.signature:
            if authorization.signature.y_parity >= 2**8:
                raise wire.DataError("Authorization signature y parity invalid")
            if len(authorization.signature.r) != 32:
                raise wire.DataError("Authorization signature r invalid")
            if len(authorization.signature.s) != 32:
                raise wire.DataError("Authorization signature s invalid")
        else:
            if len(authorization.address_n) == 0:
                sender_nonce = int.from_bytes(msg.nonce, "big")
                if nonce != sender_nonce + 1:
                    raise wire.DataError("Authority nonce invalid")


async def validate_delegate(
    ctx: wire.Context, delegate_addr: str, delegate_chain_id: int
) -> None:
    if not is_registered_delegator(delegate_chain_id, delegate_addr):
        await show_invalid_delegate(ctx)
        raise wire.DataError("Authorization address not in registered delegators")
