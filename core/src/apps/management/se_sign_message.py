from typing import TYPE_CHECKING

from trezor import utils, wire
from trezor.crypto import se_thd89
from trezor.messages import SEMessageSignature

if TYPE_CHECKING:
    from trezor.messages import SESignMessage


async def se_sign_message(ctx: wire.Context, msg: SESignMessage) -> SEMessageSignature:
    if utils.EMULATOR:
        raise wire.ProcessError("Not support by emulator.")

    from trezor.ui.layouts.lvgl import confirm_security_check

    await confirm_security_check(ctx)

    signature = se_thd89.sign_message(msg.message)
    return SEMessageSignature(signature=signature)
