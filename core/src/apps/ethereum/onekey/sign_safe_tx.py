from typing import TYPE_CHECKING

from trezor.crypto.curve import secp256k1
from trezor.messages import EthereumGnosisSafeSignature, EthereumGnosisSafeTxRequest
from trezor.ui.layouts import confirm_final

from apps.common import paths

from .. import networks
from ..helpers import (
    address_from_bytes,
    bytes_from_address,
    get_color_and_icon,
    get_display_network_name,
)
from ..layout import require_confirm_safe_tx
from .keychain import PATTERNS_ADDRESS, with_keychain_from_path
from .sign_typed_data import get_hash_writer, keccak256, write_leftpad32

if TYPE_CHECKING:
    from apps.common.keychain import Keychain
    from trezor.wire import Context

#  keccak256(
#      "EIP712Domain(uint256 chainId,address verifyingContract)"
#  );
# 0x47e79534a245952e8b16893a336b85a3d9ea9fa8c573f3d803afb92a79469218
DOMAIN_SEPARATOR_TYPEHASH = b"G\xe7\x954\xa2E\x95.\x8b\x16\x89:3k\x85\xa3\xd9\xea\x9f\xa8\xc5s\xf3\xd8\x03\xaf\xb9*yF\x92\x18"

#  keccak256(
#     "SafeTx(address to,uint256 value,bytes data,uint8 operation,uint256 safeTxGas,uint256 baseGas,uint256 gasPrice,address gasToken,address refundReceiver,uint256 nonce)"
# );
# 0xbb8310d486368db6bd6f849402fdd73ad53d316b5a4b2644ad6efe0f941286d8
SAFE_TX_TYPEHASH = b"\xbb\x83\x10\xd4\x866\x8d\xb6\xbdo\x84\x94\x02\xfd\xd7:\xd5=1kZK&D\xadn\xfe\x0f\x94\x12\x86\xd8"


@with_keychain_from_path(*PATTERNS_ADDRESS)
async def sign_safe_tx(
    ctx: Context, msg: EthereumGnosisSafeTxRequest, keychain: Keychain
) -> EthereumGnosisSafeSignature:
    await paths.validate_path(ctx, keychain, msg.address_n, force_strict=False)

    if msg.chain_id:
        network = networks.by_chain_id(msg.chain_id)
    else:
        if len(msg.address_n) > 1:  # path has slip44 network identifier
            network = networks.by_slip44(msg.address_n[1] & 0x7FFF_FFFF)
        else:
            network = None
    ctx.primary_color, ctx.icon_path = get_color_and_icon(
        network.chain_id if network else None
    )
    if msg.operation == 1:
        from trezor.lvglui.scrs import lv_colors

        ctx.primary_color = lv_colors.ONEKEY_RED_1
    ctx.name = get_display_network_name(network)
    node = keychain.derive(msg.address_n, force_strict=False)
    from_address = address_from_bytes(node.ethereum_pubkeyhash())

    await require_confirm_safe_tx(ctx, from_address, msg)
    domain_hash = get_domain_separator_hash(msg.chain_id, msg.verifyingContract)
    safe_tx_hash = get_safe_tx_hash(msg)
    data_hash = keccak256(b"\x19\x01" + domain_hash + safe_tx_hash)

    await confirm_final(ctx, "Gnosis Safe", 2 if msg.operation == 1 else 0)
    signature = secp256k1.sign(
        node.private_key(), data_hash, False, secp256k1.CANONICAL_SIG_ETHEREUM
    )

    return EthereumGnosisSafeSignature(
        signature=signature[1:] + signature[0:1],
    )


def get_domain_separator_hash(chain_id: int, verifying_contract: str) -> bytes:
    h_w = get_hash_writer()
    h_w.extend(DOMAIN_SEPARATOR_TYPEHASH)
    h_w.extend(chain_id.to_bytes(32, "big"))
    write_leftpad32(h_w, bytes_from_address(verifying_contract))
    return h_w.get_digest()


def get_safe_tx_hash(msg: EthereumGnosisSafeTxRequest) -> bytes:
    calldata_hash = keccak256(msg.data if msg.data else b"")
    h_w = get_hash_writer()
    h_w.extend(SAFE_TX_TYPEHASH)
    write_leftpad32(h_w, bytes_from_address(msg.to))
    write_leftpad32(h_w, msg.value)
    h_w.extend(calldata_hash)
    h_w.extend(msg.operation.to_bytes(32, "big"))
    write_leftpad32(h_w, msg.safeTxGas)
    write_leftpad32(h_w, msg.baseGas)
    write_leftpad32(h_w, msg.gasPrice)
    write_leftpad32(h_w, bytes_from_address(msg.gasToken))
    write_leftpad32(h_w, bytes_from_address(msg.refundReceiver))
    write_leftpad32(h_w, msg.nonce)
    return h_w.get_digest()
