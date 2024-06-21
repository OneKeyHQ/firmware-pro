
from typing import TYPE_CHECKING

from trezor.crypto.curve import ed25519

from trezor import utils
from trezor.lvglui.scrs import lv
from trezor.enums import TonWalletVersion, TonWorkChain
from trezor.messages import TonSignJettonTransfer,TonSignedJettonTransfer
from trezor.ui.layouts import show_address
from trezor.strings import format_amount

from apps.common import paths,seed
from apps.common.keychain import Keychain, auto_keychain

from .tonsdk.contract.wallet import Wallets, WalletVersionEnum
from .tonsdk.utils._address import Address
from .tonsdk.contract.token.ft import JettonWallet

from .import ICON, PRIMARY_COLOR

if TYPE_CHECKING:
    from trezor.wire import Context

@auto_keychain(__name__)
async def sign_jetton_transfer(
    ctx: Context, msg: TonSignJettonTransfer, keychain: Keychain
) -> TonSignedJettonTransfer:
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

    wallet = Wallets.ALL[wallet_version](public_key=public_key, wallet_id=msg.wallet_id, wc=workchain)
    address = wallet.address.to_string(
        is_user_friendly=True, is_url_safe=True, is_bounceable=msg.is_bounceable, is_test_only=msg.is_testnet_only)

    # disply
    ctx.primary_color, ctx.icon_path = lv.color_hex(PRIMARY_COLOR), ICON

    recipient = Address(msg.destination).to_string(True, True)
    format_value = f"{format_amount(msg.jetton_amount, 9)} TOKEN"

    from trezor.ui.layouts import confirm_ton_jetton_transfer
    await confirm_ton_jetton_transfer(ctx, address=msg.jetton_master_address)
    
    from trezor.ui.layouts import confirm_output, confirm_ton_transfer
    # await confirm_output(ctx, recipient, format_value)
    await confirm_ton_transfer(ctx, address, recipient, format_value, None)



    from trezor.ui.layouts import confirm_final
    await confirm_final(ctx, "TOKEN")

    body = JettonWallet().create_transfer_body(
        Address(msg.destination),
        msg.jetton_amount
    )

    digest = wallet.create_transaction_digest(
        to_addr=msg.destination, 
        amount=msg.ton_amount, 
        seqno=msg.seqno, 
        expire_at=msg.expire_at, 
        payload=body,
    )

    signature = ed25519.sign(node.private_key(), digest)

    return TonSignedJettonTransfer(signature=signature)