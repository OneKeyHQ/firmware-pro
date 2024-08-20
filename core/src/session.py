from trezor import log, loop, utils
from trezor.lvglui import lvgl_tick
from trezor.qr import handle_qr_task
from trezor.uart import (
    fetch_all,
    handle_ble_info,
    handle_fingerprint,
    handle_uart,
    handle_usb_state,
    stop_mode,
)
from trezor.ui import display

import apps.base
import usb

apps.base.boot()

if not utils.BITCOIN_ONLY and usb.ENABLE_IFACE_WEBAUTHN:
    import apps.webauthn

    apps.webauthn.boot()

if __debug__:
    import apps.debug

    apps.debug.boot()


async def handle_stop_mode():
    while True:
        # leave enough time for usb to be detected
        await loop.sleep(200)

        if display.backlight() == 0:
            stop_mode(False)


# if the screen is off, enter low power mode after reloop
# if display.backlight() == 0:
#     stop_mode(True)

# run main event loop and specify which screen is the default
apps.base.set_homescreen()

loop.schedule(handle_fingerprint())
loop.schedule(fetch_all())
loop.schedule(handle_uart())

loop.schedule(handle_ble_info())

loop.schedule(handle_usb_state())


loop.schedule(lvgl_tick())
loop.schedule(handle_qr_task())
loop.schedule(handle_stop_mode())


utils.set_up()
if utils.show_app_guide():
    from trezor.ui.layouts import show_onekey_app_guide

    loop.schedule(show_onekey_app_guide())

loop.run()

if __debug__:
    log.debug(__name__, "Restarting main loop")
