import gc

from trezor import log, loop, messages, utils, wire, workflow
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.lvglui.scrs import lv

from apps.ur_registry.ur_py.ur.ur import UR
from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder


# QR Task
class QRTask:
    def __init__(self) -> None:
        self.ur_type = None
        self.req = None
        self.scanning = False
        self.callback_obj = None
        self.hd_key = None
        self.encoder = None
        self.ready_signal = loop.chan()

    def get_and_rest_hd_key(self) -> str | None:
        if self.hd_key is None:
            return None
        current = self.hd_key
        self.hd_key = None
        return current

    def get_hd_key(self) -> str | None:
        return self.hd_key

    def set_hd_key(self, hd_key: str):
        self.hd_key = hd_key

    def set_encoder(self, encoder: UREncoder):
        self.encoder = encoder

    def get_and_rest_encoder(self) -> UREncoder | None:
        if self.encoder is None:
            return None
        current = self.encoder
        self.encoder = None
        return current

    def get_encoder(self) -> UREncoder | None:
        if self.encoder is not None:
            self.encoder.fountain_encoder.seq_num = 0
        return self.encoder

    async def ready(self):
        return await self.ready_signal.take()

    def is_camera_scanning(self) -> bool:
        return self.scanning

    def set_camera_scanning(self, state: bool):
        self.scanning = state

    def set_callback_obj(self, callback_obj):
        self.callback_obj = callback_obj

    def set_app_obj(self, app_obj):
        self.app_obj = app_obj

    async def callback_finish(self):
        if self.app_obj is not None:
            # TODO
            # self.app_obj.apps.show_page(0)
            # self.app_obj.apps.visible = False
            # self.app_obj.apps.show()
            area = lv.area_t()
            area.x1 = 0
            area.y1 = 0
            area.x2 = 480
            area.y2 = 800
            self.app_obj.invalidate_area(area)

    async def finish(self):
        assert self.ur_type is not None, "ur should not be None."
        registry_type = self.ur_type
        if registry_type == "eth-sign-request":
            if self.req is not None:
                from trezor.ui.layouts import show_ur_response

                if self.req.qr is not None:
                    await show_ur_response(
                        wire.QR_CONTEXT,
                        _(i18n_keys.TITLE__EXPORT_SIGNED_TRANSACTION),
                        self.req.qr,
                    )
        elif registry_type == "onekey-app-call-device":
            if self.req is not None:
                from trezor.ui.layouts import show_ur_response

                if self.req.qr is not None:
                    await show_ur_response(
                        wire.QR_CONTEXT,
                        _(i18n_keys.CONTENT__EXPORT_ACCOUNT),
                        self.req.qr,
                    )
                elif self.req.encoder is not None:
                    await show_ur_response(
                        wire.QR_CONTEXT,
                        _(i18n_keys.CONTENT__EXPORT_ACCOUNT),
                        None,
                        self.req.encoder,
                    )
        elif registry_type == "crypto-psbt":
            if __debug__:
                print("sign psbt finshed...")
            if self.req is not None:
                from trezor.ui.layouts import show_ur_response

                if self.req.qr is not None:
                    await show_ur_response(
                        wire.QR_CONTEXT,
                        _(i18n_keys.TITLE__EXPORT_SIGNED_TRANSACTION),
                        self.req.qr,
                    )
                elif self.req.encoder is not None:
                    await show_ur_response(
                        wire.QR_CONTEXT,
                        _(i18n_keys.TITLE__EXPORT_SIGNED_TRANSACTION),
                        None,
                        self.req.encoder,
                    )
        self.req = None
        if self.callback_obj is not None:
            lv.event_send(self.callback_obj.nav_back.nav_btn, lv.EVENT.CLICKED, None)

    async def run(self):
        if self.req is not None:
            await self.req.run()

    async def gen_request(self, ur: UR) -> bool:
        self.ur_type = ur.registry_type
        registry_type = self.ur_type
        success = False
        if __debug__:
            print(f"receive >> request ur type: {registry_type}")

        if registry_type not in [
            "eth-sign-request",
            "onekey-app-call-device",
            "crypto-psbt",
        ]:
            if __debug__:
                print(f"unsupported type {registry_type}")
            from trezor.ui.layouts import show_error_no_interact

            await show_error_no_interact(
                title=_(i18n_keys.TITLE__DATA_FORMAT_NOT_SUPPORT),
                subtitle=_(
                    i18n_keys.CONTENT__QR_CODE_TYPE_NOT_SUPPORT_PLEASE_TRY_AGAIN
                ),
            )
        else:
            if registry_type == "eth-sign-request":
                from apps.ur_registry.chains.ethereum.eth_sign_request import (
                    EthSignRequest,
                )

                try:
                    self.req = await EthSignRequest.gen_request(ur)
                    if __debug__:
                        print("req: ", type(self.req))
                except Exception as e:
                    if __debug__:
                        import sys

                        sys.print_exception(e)  # type: ignore["print_exception" is not a known member of module]
                    from trezor.ui.layouts import show_error_no_interact

                    await show_error_no_interact(
                        title=_(i18n_keys.TITLE__INVALID_TRANSACTION),
                        subtitle=_(
                            i18n_keys.CONTENT__TX_DATA_IS_INCORRECT_PLEASE_TRY_AGAIN
                        ),
                    )
                else:
                    success = True
            elif registry_type == "onekey-app-call-device":
                from apps.ur_registry.chains.hardware_requests.hardware_call import (
                    HardwareCall,
                )

                try:
                    self.req = await HardwareCall.gen_request(ur)
                    if __debug__:
                        print("req: ", type(self.req))
                except Exception as e:
                    if __debug__:
                        import sys

                        sys.print_exception(e)  # type: ignore["print_exception" is not a known member of module]
                    from trezor.ui.layouts import show_error_no_interact

                    await show_error_no_interact(
                        title=_(i18n_keys.TITLE__DATA_FORMAT_NOT_SUPPORT),
                        subtitle=_(
                            i18n_keys.CONTENT__TX_DATA_IS_INCORRECT_PLEASE_TRY_AGAIN
                        ),
                    )
                else:
                    success = True
            elif registry_type == "crypto-psbt":
                from apps.ur_registry.chains.bitcoin.transaction import SignPsbt

                try:
                    self.req = await SignPsbt.gen_request(ur)
                    if __debug__:
                        print("req: ", type(self.req))
                except Exception as e:
                    if __debug__:
                        import sys

                        sys.print_exception(e)  # type: ignore["print_exception" is not a known member of module]
                    from trezor.ui.layouts import show_error_no_interact

                    await show_error_no_interact(
                        title=_(i18n_keys.TITLE__INVALID_TRANSACTION),
                        subtitle=_(
                            i18n_keys.CONTENT__TX_DATA_IS_INCORRECT_PLEASE_TRY_AGAIN
                        ),
                    )
                else:
                    success = True

        if success:
            self.ready_signal.publish(True)
        return success


qr_task = QRTask()


async def handle_qr(qr: UR) -> bool:
    return await qr_task.gen_request(qr)


def save_app_obj(callback_obj):
    qr_task.set_app_obj(callback_obj)


def scan_qr(callback_obj):
    async def camear_scan():
        from trezorio import camera
        from apps.ur_registry.ur_py.ur.ur_decoder import URDecoder
        from trezor import motor

        decoder = URDecoder()
        qr_task.set_camera_scanning(True)
        qr_task.set_callback_obj(callback_obj)
        while True:
            if not qr_task.is_camera_scanning():
                camera.stop()
                # await qr_task.callback_finish()
                break
            qr_data = camera.scan_qrcode(80, 180)
            if qr_data:
                if __debug__:
                    print(qr_data.decode("utf-8"))
                try:
                    decoder.receive_part(qr_data.decode("utf-8"))
                    del qr_data
                except Exception:
                    decoder.reset()
                    await callback_obj.error_feedback()
                    await loop.sleep(100)
                    continue
                else:
                    if decoder.is_complete():
                        motor.vibrate()
                        if type(decoder.result) is UR:
                            if __debug__:
                                utils.mem_trace(__name__, 4)
                            res = await handle_qr(decoder.result)
                            if __debug__:
                                utils.mem_trace(__name__, 5)
                            if res:
                                del (decoder.result, decoder)
                                camera.stop()
                                from trezor import uart

                                uart.flashled_close()
                                break
                            else:
                                decoder.reset()
                                continue
            await loop.sleep(50)

    workflow.spawn(camear_scan())


def close_camera():
    qr_task.set_camera_scanning(False)


def retrieval_hd_key():
    return qr_task.get_and_rest_hd_key()


def get_hd_key():
    return qr_task.get_hd_key()


def get_encoder():
    return qr_task.get_encoder()


def retrieval_encoder():
    return qr_task.get_and_rest_encoder()


async def gen_hd_key(callback=None):
    global qr_task
    if qr_task.hd_key is not None:
        return
    from apps.base import handle_Initialize
    from apps.ur_registry.crypto_hd_key import genCryptoHDKeyForETHStandard

    # pyright: off
    await handle_Initialize(wire.QR_CONTEXT, messages.Initialize())
    ur = await genCryptoHDKeyForETHStandard(wire.QR_CONTEXT)
    # pyright: on
    qr_task.set_hd_key(ur)
    if callback is not None:
        callback()


async def gen_multi_accounts(callback=None):
    from apps.base import handle_Initialize
    from apps.ur_registry.crypto_multi_accounts import generate_crypto_multi_accounts

    from apps.common import passphrase

    if passphrase.is_enabled():
        wire.QR_CONTEXT.passphrase = None
    # pyright: off
    await handle_Initialize(wire.QR_CONTEXT, messages.Initialize())
    ur = await generate_crypto_multi_accounts(wire.QR_CONTEXT)
    # pyright: on
    qr_task.set_encoder(ur)
    if callback is not None:
        callback()


async def handle_qr_task():
    while True:
        try:
            if await qr_task.ready():
                gc.collect()
                # pyright: off
                gc.threshold(gc.mem_free() // 4 + gc.mem_alloc())
                # pyright: on
                mods = utils.unimport_begin()
                from apps.base import handle_Initialize

                await handle_Initialize(wire.QR_CONTEXT, messages.Initialize())
                del handle_Initialize
                # if __debug__:
                #     utils.mem_trace(__name__, 6)
                await qr_task.run()
                utils.unimport_end(mods)
                # if __debug__:
                #     utils.mem_trace(__name__, 7)
                await qr_task.finish()
                utils.unimport_end(mods)
        except Exception as exec:
            if __debug__:
                log.exception(__name__, exec)
            if not isinstance(exec, wire.ActionCancelled):
                from trezor.ui.layouts import show_error_no_interact

                await show_error_no_interact(
                    title=_(i18n_keys.TITLE__INVALID_TRANSACTION),
                    subtitle=_(
                        i18n_keys.CONTENT__TX_DATA_IS_INCORRECT_PLEASE_TRY_AGAIN
                    ),
                )
            loop.clear()
            return  # pylint: disable=lost-exception
