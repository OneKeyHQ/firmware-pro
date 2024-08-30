from typing import TYPE_CHECKING

from trezor import wire
from trezor.crypto.curve import ed25519
from trezor.enums import TonWalletVersion, TonWorkChain
from trezor.lvglui.scrs import lv
from trezor.messages import TonSignedMessage, TonSignMessage

from apps.common import paths, seed
from apps.common.keychain import Keychain, auto_keychain

from . import ICON, PRIMARY_COLOR, tokens
from .layout import require_confirm_fee, require_show_overview
from .tonsdk.contract.token.ft import JettonWallet
from .tonsdk.contract.wallet import Wallets, WalletVersionEnum
from .tonsdk.utils._address import Address

if TYPE_CHECKING:
    from trezor.wire import Context


@auto_keychain(__name__)
async def sign_message(
    ctx: Context, msg: TonSignMessage, keychain: Keychain
) -> TonSignedMessage:
    await paths.validate_path(ctx, keychain, msg.address_n)

    node = keychain.derive(msg.address_n)
    public_key = seed.remove_ed25519_prefix(node.public_key())
    workchain = (
        -1 if msg.workchain == TonWorkChain.MASTERCHAIN else TonWorkChain.BASECHAIN
    )

    if msg.wallet_version == TonWalletVersion.V4R2:
        wallet_version = WalletVersionEnum.v4r2
    else:
        raise wire.DataError("Invalid wallet version.")

    is_jetton_transfer = check_jetton_transfer(msg)

    wallet = Wallets.ALL[wallet_version](
        public_key=public_key, wallet_id=msg.wallet_id, wc=workchain
    )
    address = wallet.address.to_string(
        is_user_friendly=True,
        is_url_safe=True,
        is_bounceable=msg.is_bounceable,
        is_test_only=msg.is_testnet_only,
    )

    # display
    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
    from trezor.ui.layouts import confirm_final, confirm_unknown_token_transfer

    token = None
    recipient = Address(msg.destination).to_string(True, True)

    if is_jetton_transfer:
        token = tokens.token_by_address("TON_TOKEN", msg.jetton_master_address)

        if token is tokens.UNKNOWN_TOKEN:
            # unknown token, confirm contract address
            if msg.jetton_master_address is None:
                raise ValueError("Address cannot be None")
            await confirm_unknown_token_transfer(ctx, msg.jetton_master_address)

    show_details = await require_show_overview(
        ctx,
        recipient,
        msg.jetton_amount if is_jetton_transfer else msg.ton_amount,
        token,
    )

    if show_details:
        comment = msg.comment.encode("utf-8") if msg.comment else None
        await require_confirm_fee(
            ctx,
            from_address=address,
            to_address=recipient,
            value=msg.jetton_amount if is_jetton_transfer else msg.ton_amount,
            token=token,
            raw_data=comment if comment else None,
        )

    await confirm_final(ctx, token.symbol if token else "TON")

    if is_jetton_transfer:
        if msg.jetton_amount is None:
            raise ValueError("Jetton amount cannot be None")

        body = JettonWallet().create_transfer_body(
            Address(msg.destination),
            msg.jetton_amount,
            msg.fwd_fee,
            msg.comment,
            wallet.address,
        )
        payload = body
    else:
        payload = msg.comment

    digest, boc = wallet.create_transaction_digest(
        to_addr=msg.jetton_wallet_address if is_jetton_transfer else msg.destination,
        amount=msg.ton_amount,
        seqno=msg.seqno,
        expire_at=msg.expire_at,
        payload=payload,
        send_mode=msg.mode,
        ext_to=None if is_jetton_transfer else msg.ext_destination,
        ext_amount=None if is_jetton_transfer else msg.ext_ton_amount,
        ext_payload=None if is_jetton_transfer else msg.ext_payload,
    )

    signature = ed25519.sign(node.private_key(), digest)

    return TonSignedMessage(signature=signature, signning_message=boc)


def check_jetton_transfer(msg: TonSignMessage):
    if msg.jetton_amount is None and msg.jetton_master_address is None:
        return False
    elif msg.jetton_amount is not None and msg.jetton_master_address is not None:
        return True
    else:
        raise wire.DataError("Invalid jetton transfer message.")
