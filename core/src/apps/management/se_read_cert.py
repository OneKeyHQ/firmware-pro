from typing import TYPE_CHECKING

from trezor import utils, wire
from trezor.crypto import se_thd89
from trezor.messages import SEPublicCert

if TYPE_CHECKING:
    from trezor.messages import ReadSEPublicCert


async def se_read_cert(ctx: wire.Context, msg: ReadSEPublicCert) -> SEPublicCert:
    if utils.EMULATOR:
        raise wire.ProcessError("Not support by emulator.")

    cert = se_thd89.read_certificate()
    return SEPublicCert(public_cert=cert)
