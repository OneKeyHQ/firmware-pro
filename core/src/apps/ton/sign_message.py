
from typing import TYPE_CHECKING

from trezor.crypto.curve import ed25519

from trezor import wire
from trezor.lvglui.scrs import lv
from trezor.enums import TonWalletVersion, TonWorkChain
from trezor.messages import TonSignMessage, TonSignedMessage
from trezor.strings import format_amount

from apps.common import paths,seed
from apps.common.keychain import Keychain, auto_keychain

from .tonsdk.contract.wallet import Wallets, WalletVersionEnum
from .tonsdk.utils._address import Address
from .tonsdk.contract.token.ft import JettonWallet
from .import ICON, PRIMARY_COLOR, tokens
if TYPE_CHECKING:
    from trezor.wire import Context

@auto_keychain(__name__)
async def sign_message(
    ctx: Context, msg: TonSignMessage, keychain: Keychain
) -> TonSignedMessage:
    await paths.validate_path(ctx, keychain, msg.address_n)

    node = keychain.derive(msg.address_n)
    public_key = seed.remove_ed25519_prefix(node.public_key())
    workchain = -1 if msg.workchain == TonWorkChain.MASTERCHAIN else TonWorkChain.BASECHAIN

    if msg.wallet_version == TonWalletVersion.V3R1:
        wallet_version = WalletVersionEnum.v3r1
    elif msg.wallet_version == TonWalletVersion.V3R2:
        wallet_version = WalletVersionEnum.v3r2
    elif msg.wallet_version == TonWalletVersion.V4R1:
        wallet_version = WalletVersionEnum.v4r1
    elif msg.wallet_version == TonWalletVersion.V4R2:
        wallet_version = WalletVersionEnum.v4r2
    else:
        raise wire.DataError("Invalid wallet version.")
    
    is_jetton_transfer = check_jetton_transfer(msg)

    wallet = Wallets.ALL[wallet_version](public_key=public_key, wallet_id=msg.wallet_id, wc=workchain)
    address = wallet.address.to_string(
        is_user_friendly=True, is_url_safe=True, is_bounceable=msg.is_bounceable, is_test_only=msg.is_testnet_only)

    # disply
    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON
    from trezor.ui.layouts import confirm_final, confirm_ton_transfer, confirm_unknown_token_transfer, confirm_output

    if is_jetton_transfer:
        token = tokens.token_by_address(
            "TON_TOKEN", msg.jetton_master_address
        )

        recipient = Address(msg.destination).to_string(True, True)
        format_value = f"{format_amount(msg.jetton_amount, 9)} {token.symbol}"
        
        if token is tokens.UNKNOWN_TOKEN:
            # unknown token, confirm contract address
            await confirm_unknown_token_transfer(
                ctx, msg.jetton_master_address
            )
        await confirm_output(ctx, recipient, format_value)

        await confirm_ton_transfer(ctx, address, recipient, format_value, msg.comment)

        await confirm_final(ctx, token.symbol)

        body = JettonWallet().create_transfer_body(
            Address(msg.destination),
            msg.jetton_amount,
            msg.fwd_fee,
            msg.comment,
            wallet.address
        )

        digest = wallet.create_transaction_digest(
            to_addr=msg.destination, 
            amount=msg.ton_amount, 
            seqno=msg.seqno,
            expire_at=msg.expire_at, 
            payload=body,
        )

    else:
        recipient = Address(msg.destination).to_string(True, True)
        format_value = f"{format_amount(msg.ton_amount, 9)} TON"

        from trezor.ui.layouts import confirm_output
        await confirm_output(ctx, recipient, format_value)

        await confirm_ton_transfer(ctx, address, recipient, format_value, msg.comment)
        
        await confirm_final(ctx, "TON")

        digest = wallet.create_transaction_digest(
            to_addr=msg.destination, 
            amount=msg.ton_amount, 
            seqno=msg.seqno, 
            expire_at=msg.expire_at, 
            payload=msg.comment,
        )

    signature = ed25519.sign(node.private_key(), digest)

    return TonSignedMessage(signature=signature)

def check_jetton_transfer(msg: TonSignMessage):
    if msg.jetton_amount is None and msg.jetton_master_address is None:
        return False 
    elif msg.jetton_amount is not None and msg.jetton_master_address is not None:
        return True
    else:
        raise wire.DataError("Invalid jetton transfer message.")