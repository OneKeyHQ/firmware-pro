import storage.device
from trezor import loop, utils
from trezor.lvglui import lvgl_tick
from trezor.qr import handle_qr_task
from trezor.uart import (
    ctrl_wireless_charge,
    disconnect_ble,
    fetch_all,
    handle_ble_info,
    handle_fingerprint,
    handle_uart,
    handle_usb_state,
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
    from trezor import log

    apps.debug.boot()


def stop_mode(reset_timer: bool = False):
    ctrl_wireless_charge(True)
    disconnect_ble()
    utils.enter_lowpower(reset_timer, storage.device.get_autoshutdown_delay_ms())


async def handle_stop_mode():
    while True:
        # leave enough time for usb to be detected
        await loop.sleep(200)

        if display.backlight():  # screen is on
            return
        stop_mode(False)


# if the screen is off, enter low power mode after reloop
if display.backlight() == 0:
    stop_mode(True)
else:
    if utils.CHARGE_WIRELESS_STATUS == utils.CHARGE_WIRELESS_START:
        utils.CHARGE_WIRELESS_STATUS = utils.CHARGE_WIRELESS_CHARGING
        apps.base.screen_off_if_possible()

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
    log.debug(__name__, "Restarting main loop")  # type: ignore["log" is possibly unbound]
