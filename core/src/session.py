from trezorio import nfc

import storage.device
from trezor import log, loop, utils
from trezor.lvglui import lvgl_tick
from trezor.qr import handle_qr_ctx, handle_qr_task
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

    apps.debug.boot()


async def nfc_test():
    nfc.pwr_ctrl(True)
    while True:
        await loop.sleep(100)
        if nfc.poll_card():
            print("Card detected")
            # select mf
            print("Select MF")
            resp, sw1sw2 = nfc.send_recv(b"\x00\xa4\x04\x00")
            print("Response: ", resp)
            print("SW1SW2: ", sw1sw2)
            # reset card
            print("Reset card")
            resp, sw1sw2 = nfc.send_recv(
                b"\x80\xcb\x80\x00\x05\xdf\xfe\x02\x82\x05", True
            )
            print("Response: ", resp)
            print("SW1SW2: ", sw1sw2)
            # set pin
            print("Set pin")
            resp, sw1sw2 = nfc.send_recv(
                b"\x80\xcb\x80\x00\x0e\xdf\xfe\x0b\x82\x04\x08\x00\x06\x31\x32\x33\x34\x35\x36",
                True,
            )
            print("Response: ", resp)
            print("SW1SW2: ", sw1sw2)

            print("Select applet")
            resp, sw1sw2 = nfc.send_recv(
                b"\x00\xa4\x04\x00\x08\xD1\x56\x00\x01\x32\x83\x40\x01"
            )
            print("Response: ", resp)
            print("SW1SW2: ", sw1sw2)

            print("Verify pin(correct)")
            resp, sw1sw2 = nfc.send_recv(
                b"\x80\x20\x00\x00\x07\x06\x31\x32\x33\x34\x35\x36", True
            )
            print("Response: ", resp)
            print("SW1SW2: ", sw1sw2)

            print("Verify pin(incorrect)")
            resp, sw1sw2 = nfc.send_recv(
                b"\x80\x20\x00\x00\x07\x06\x31\x32\x33\x34\x35\x37", True
            )
            print("Response: ", resp)
            print("SW1SW2: ", sw1sw2)

            return

        else:
            print("No card detected")


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

loop.schedule(handle_qr_ctx())
loop.schedule(handle_qr_task())

loop.schedule(lvgl_tick())
loop.schedule(handle_stop_mode())

loop.schedule(nfc_test())

utils.set_up()
if utils.show_app_guide():
    from trezor.ui.layouts import show_onekey_app_guide

    loop.schedule(show_onekey_app_guide())

loop.run()

if __debug__:
    log.debug(__name__, "Restarting main loop")
