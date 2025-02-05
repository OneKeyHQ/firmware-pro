from trezor import io, loop

import usb

from .fido2 import handle_reports


def boot() -> None:
    loop.schedule(handle_reports(usb.iface_webauthn, io.SPI(io.SPI_FIDO_FACE)))
