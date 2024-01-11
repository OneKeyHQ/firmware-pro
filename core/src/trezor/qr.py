from trezor import log, loop, messages, wire, workflow
from trezor.lvglui.scrs import lv

from apps.ur_registry.ur_py.ur.ur import UR


# QR Task
class QRTask:
    def __init__(self) -> None:
        self.mulit_pkg = False
        self.ur = None
        self.req = None
        self.resp = None
        self.scanning = False
        self.callback_obj = None
        self.hd_key = ""

    def get_hd_key(self) -> str:
        return self.hd_key

    def set_hd_key(self, hd_key: str):
        self.hd_key = hd_key

    def next_task(self) -> bool:
        if self.req is not None:
            return True
        return False

    def is_mulit_pkg(self) -> bool:
        return self.mulit_pkg

    def get_camera_state(self) -> bool:
        return self.scanning

    def set_camera_state(self, state: bool):
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
        if self.req is not None:
            from trezor.ui.layouts import show_signature

            self.resp = self.req.resp
            if self.req.qr is not None:
                await show_signature(wire.DUMMY_CONTEXT, self.req.qr)
            self.req = None
            if self.callback_obj is not None:
                lv.event_send(
                    self.callback_obj.nav_back.nav_btn, lv.EVENT.CLICKED, None
                )

    async def initial_tx(self):
        if self.req is not None:
            return await self.req.initial_tx()

    async def run(self):
        if self.req is not None:
            await self.req.run()

    async def push(self, ur: UR):
        self.ur = ur
        print("push: ", self.ur.type)
        if ur.type == "eth-sign-request":
            from apps.ur_registry.chains.ethereum.eth_sign_request import (
                EthSignRequest,
                RequestType_TypedData,
            )

            eth_req = EthSignRequest.from_cbor(ur.cbor)
            if eth_req.get_data_type() == RequestType_TypedData:
                self.mulit_pkg = True
            self.req = await EthSignRequest.gen_transaction(ur)
            print("req: ", type(self.req))
        elif ur.type == "crypto-psbt":
            print("TODO crypto-psbt")
        else:
            print("TODO unsupport")


qr_task = QRTask()


async def handle_qr(qr: UR):
    await qr_task.push(qr)


def save_app_obj(callback_obj):
    qr_task.set_app_obj(callback_obj)


def scan_qr(callback_obj):
    async def camear_scan():
        from trezorio import camera
        from trezor.qr import handle_qr
        from apps.ur_registry.ur_py.ur.ur_decoder import URDecoder

        decoder = URDecoder()
        qr_task.set_camera_state(True)
        qr_task.set_callback_obj(callback_obj)
        while True:
            qr_data = camera.scan_qrcode(80, 180)
            if qr_data:
                print(qr_data.decode("utf-8"))
                decoder.receive_part(qr_data.decode("utf-8"))
                if decoder.is_complete():
                    camera.stop()
                    if type(decoder.result) is UR:
                        await handle_qr(decoder.result)
                    break
            if qr_task.get_camera_state() is False:
                camera.stop()
                await qr_task.callback_finish()
                break
            await loop.sleep(1)

    workflow.spawn(camear_scan())


def close_camera():
    qr_task.set_camera_state(False)


def get_hd_key():
    return qr_task.get_hd_key()


async def gen_hd_key():
    global qr_task
    if qr_task.get_hd_key() is not None:
        return
    from apps.base import handle_Initialize
    from apps.ur_registry.crypto_hd_key import genCryptoHDKeyForETHStandard

    await handle_Initialize(wire.QR_CONTEXT, messages.Initialize())
    ur = await genCryptoHDKeyForETHStandard()
    qr_task.set_hd_key(ur)


async def handle_qr_task():
    while True:
        try:
            await loop.sleep(10)
            if qr_task.next_task() is True:
                from apps.base import handle_Initialize

                await handle_Initialize(wire.QR_CONTEXT, messages.Initialize())
                if qr_task.is_mulit_pkg() is True:
                    print("qr_task.initial_tx()...")
                    await qr_task.initial_tx()
                else:
                    print("qr_task.run()...")
                    await qr_task.run()
                await qr_task.finish()
        except Exception as exec:
            if __debug__:
                log.exception(__name__, exec)
            loop.clear()
            return  # pylint: disable=lost-exception


# QRContext handler
async def handle_qr_ctx():
    while True:
        try:
            # qr context
            await loop.sleep(10)
            if qr_task.next_task() is True and qr_task.is_mulit_pkg() is True:
                await qr_task.run()
        except Exception as exec:
            if __debug__:
                log.exception(__name__, exec)
            loop.clear()
            return  # pylint: disable=lost-exception