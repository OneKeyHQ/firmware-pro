from trezor import wire
from trezor.crypto.curve import nist256p1
from trezor.lvglui.scrs import lv
from trezor.messages import NeoSignedTx, NeoSignTx
from trezor.utils import BufferReader

from apps.common import paths
from apps.common.keychain import Keychain, auto_keychain

from . import ICON, PRIMARY_COLOR
from .helpers import make_digest, neo_address_from_pubkey, retrieve_network
from .transaction import RawTransaction


@auto_keychain(__name__)
async def sign_tx(ctx: wire.Context, msg: NeoSignTx, keychain: Keychain) -> NeoSignedTx:
    address_n = msg.address_n

    await paths.validate_path(ctx, keychain, address_n)

    node = keychain.derive(address_n)
    pubkey = node.public_key()
    raw_tx = msg.raw_tx
    address = neo_address_from_pubkey(pubkey)

    tx = RawTransaction()
    try:
        tx.deserialize(BufferReader(raw_tx))
    except Exception as e:
        if __debug__:
            import sys

            sys.print_exception(e)  # type: ignore["print_exception" is not a known member of module]
        raise wire.DataError(f"Invalid transaction: {e}")

    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON

    network_magic = msg.network_magic
    network, is_unknown_network = retrieve_network(network_magic)
    if tx.is_asset_transfer():
        if tx.is_unknown_token():
            from trezor.ui.layouts import confirm_unknown_token_transfer

            await confirm_unknown_token_transfer(ctx, tx.token_contract_hash())

        from .layout import require_show_overview

        show_overview = await require_show_overview(
            ctx, tx.destination(), tx.display_amount()
        )
        if show_overview:
            from .layout import confirm_neo_transfer

            await confirm_neo_transfer(
                ctx,
                tx.source(),
                tx.destination(),
                tx.display_amount(),
                tx.total_fee(),
                None if is_unknown_network else network_magic,
            )
    elif tx.is_vote() or tx.is_remove_vote():
        from .layout import require_confirm_neo_vote

        await require_confirm_neo_vote(
            ctx,
            tx.sender(),
            tx.vote_to(),
            tx.is_remove_vote(),
            None if is_unknown_network else network_magic,
        )
    else:
        from trezor.ui.layouts import confirm_blind_sign_common

        await confirm_blind_sign_common(ctx, address, raw_tx)

    from trezor.ui.layouts import confirm_final

    await confirm_final(ctx, network)

    digest = make_digest(raw_tx, network_magic)
    signature = nist256p1.sign(node.private_key(), digest)

    return NeoSignedTx(public_key=pubkey, signature=signature[1:])
