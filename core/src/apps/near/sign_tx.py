from ubinascii import hexlify

from trezor import wire
from trezor.crypto.curve import ed25519
from trezor.crypto.hashlib import sha256
from trezor.lvglui.scrs import lv
from trezor.messages import NearSignedTx, NearSignTx

from apps.common import paths
from apps.common.keychain import Keychain, auto_keychain

from . import ICON, PRIMARY_COLOR
from .transaction import Action_Transfer, Transaction


@auto_keychain(__name__)
async def sign_tx(
    ctx: wire.Context, msg: NearSignTx, keychain: Keychain
) -> NearSignedTx:

    await paths.validate_path(ctx, keychain, msg.address_n)
    node = keychain.derive(msg.address_n)
    pubkey = node.public_key()[1:]
    address = hexlify(pubkey).decode()
    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
    # parse message
    try:
        tx = Transaction.deserialize(msg.raw_tx)
    except BaseException as e:
        raise wire.DataError(f"Invalid message {e}")

    if tx.action.action_type == Action_Transfer:
        from trezor.ui.layouts.lvgl import confirm_near_transfer
        from trezor.strings import format_amount

        await confirm_near_transfer(
            ctx, address, tx.receiverId, format_amount(tx.action.amount, 24) + " NEAR"
        )
    else:
        from trezor.ui.layouts.lvgl import confirm_blind_sign_common

        await confirm_blind_sign_common(ctx, address, msg.raw_tx)

    signature = ed25519.sign(node.private_key(), sha256(msg.raw_tx).digest())
    from trezor.ui.layouts.lvgl import confirm_final

    await confirm_final(ctx, "NEAR")
    return NearSignedTx(signature=signature)
