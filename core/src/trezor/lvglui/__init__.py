from storage import device
from trezor import io, log, loop, utils

import lvgl as lv  # type: ignore[Import "lvgl" could not be resolved]

DEFAULT_DISPLAY = None


async def lvgl_tick():
    from trezor import workflow

    inactive_time_bak = 0
    while True:
        if utils.EMULATOR:
            lv.tick_inc(10)
        await loop.sleep(5)
        lv.timer_handler()
        inactive_time = get_elapsed()
        if inactive_time < inactive_time_bak:
            workflow.idle_timer.touch()

        inactive_time_bak = inactive_time


def init_lvgl() -> None:
    import lvgldrv as lcd  # type: ignore[Import "lvgldrv" could not be resolved]

    lv.init()
    if not utils.EMULATOR:
        import stjpeg  # type: ignore[Import "stjpeg" could not be resolved]

        stjpeg.init()
    disp_buf1 = lv.disp_draw_buf_t()
    buf1_1 = lcd.framebuffer(1)
    buf2_2 = lcd.framebuffer(2)
    # disp_buf1.init(buf1_1, buf2_2, len(buf1_1))
    disp_buf1.init(buf1_1, buf2_2, len(buf1_1))
    disp_drv = lv.disp_drv_t()
    disp_drv.init()
    disp_drv.draw_buf = disp_buf1
    disp_drv.flush_cb = lcd.flush
    disp_drv.hor_res = 480
    disp_drv.ver_res = 800
    if buf2_2 is not None:
        disp_drv.full_refresh = True
    # disp_drv.direct_mode = True
    disp_drv.register()

    indev_drv = lv.indev_drv_t()
    indev_drv.init()
    indev_drv.type = lv.INDEV_TYPE.POINTER
    indev_drv.read_cb = lcd.ts_read
    indev_drv.long_press_time = 500
    indev_drv.long_press_repeat_time = 200
    indev_drv.register()


def init_theme() -> None:
    global DEFAULT_DISPLAY
    DEFAULT_DISPLAY = lv.disp_get_default()
    theme = lv.theme_default_init(
        DEFAULT_DISPLAY,
        lv.palette_main(lv.PALETTE.BLUE),
        lv.palette_main(lv.PALETTE.RED),
        True,
        lv.font_default(),
    )
    # dispp.set_bg_color(lv.color_hex(0x000000))
    DEFAULT_DISPLAY.set_theme(theme)


def init_file_system() -> None:
    if not utils.EMULATOR:

        def clean():
            for size, attrs, name in io.fatfs.listdir("1:/res/wallpapers"):
                if size > 0 and attrs[1] == "h":
                    io.fatfs.unlink(f"1:/res/wallpapers/{name}")
            for size, attrs, name in io.fatfs.listdir("1:/res/nfts/imgs"):
                if size > 0 and attrs[1] == "h":
                    io.fatfs.unlink(f"1:/res/nfts/imgs/{name}")
            for size, attrs, name in io.fatfs.listdir("1:/res/nfts/zooms"):
                if size > 0 and (attrs[1] == "h" or name[:1] == "."):
                    io.fatfs.unlink(f"1:/res/nfts/zooms/{name}")
            for size, attrs, name in io.fatfs.listdir("1:/res/nfts/desc"):
                if size > 0 and (attrs[1] == "h" or name[:1] == "."):
                    io.fatfs.unlink(f"1:/res/nfts/desc/{name}")

        io.fatfs.mount()
        io.fatfs.mkdir("/boot", True)
        io.fatfs.mkdir("1:/res", True)
        io.fatfs.mkdir("1:/res/wallpapers", True)
        io.fatfs.mkdir("1:/res/nfts", True)
        io.fatfs.mkdir("1:/res/nfts/imgs", True)
        io.fatfs.mkdir("1:/res/nfts/zooms", True)
        io.fatfs.mkdir("1:/res/nfts/desc", True)
        clean()


def get_elapsed() -> int:
    """Get elapsed time since last user activity(e.g. click).
    @return: elapsed time in milliseconds
    """
    # disp = lv.disp_get_default()
    assert DEFAULT_DISPLAY is not None
    return DEFAULT_DISPLAY.get_inactive_time()


try:
    init_file_system()
    init_lvgl()
    init_theme()
    if __debug__:
        log.info("init", "initialized successfully")
except BaseException:
    if __debug__:
        log.error("init", "failed to initialize emulator")


class StatusBar(lv.obj):
    _instance = None

    BLE_STATE_CONNECTED = 0
    BLE_STATE_DISABLED = 1
    BLE_STATE_ENABLED = 2

    @classmethod
    def get_instance(cls) -> "StatusBar":
        if cls._instance is None:
            cls._instance = StatusBar()
        return cls._instance

    def __init__(self):
        super().__init__(lv.layer_top())
        self.set_size(lv.pct(100), 44)
        from trezor.lvglui.scrs.widgets.style import StyleWrapper
        from trezor.lvglui.scrs import font_GeistRegular20

        self.add_style(
            StyleWrapper()
            .border_width(0)
            .text_font(font_GeistRegular20)
            .text_letter_space(-1)
            .text_color(lv.color_hex(0xFFFFFF))
            .bg_opa(lv.OPA.TRANSP)
            .pad_column(0)
            .pad_ver(6)
            .radius(0)
            .pad_hor(4),
            0,
        )
        self.align(lv.ALIGN.TOP_MID, 0, 0)
        self.set_flex_flow(lv.FLEX_FLOW.ROW)
        # align style of the items in the container
        self.set_flex_align(
            lv.FLEX_ALIGN.END, lv.FLEX_ALIGN.CENTER, lv.FLEX_ALIGN.CENTER
        )
        # usb status
        self.usb = lv.img(self)
        self.usb.set_src("A:/res/usb.png")
        self.usb.add_flag(lv.obj.FLAG.HIDDEN)

        # ble status
        ble_enabled = device.ble_enabled()
        self.ble = lv.img(self)
        self.ble.set_src(
            "A:/res/ble-enabled.png" if ble_enabled else "A:/res/ble-disabled.png"
        )

        # battery capacity percent
        self.percent = lv.label(self)

        self.percent.add_style(
            StyleWrapper().pad_hor(5).pad_ver(3),
            0,
        )
        self.percent.add_flag(lv.obj.FLAG.HIDDEN)

        # battery capacity icon
        self.battery = lv.img(self)
        self.battery.set_src("A:/res/battery-60-white.png")
        self.battery.add_flag(lv.obj.FLAG.HIDDEN)
        # charging status
        self.charging = lv.img(self)
        self.charging.set_src("A:/res/charging.png")
        self.charging.add_flag(lv.obj.FLAG.HIDDEN)

        # air gap mode tips
        self.air_gap_tips = lv.label(lv.layer_top())
        self.air_gap_tips.set_text("Air Gap Mode")
        self.air_gap_tips.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular20)
            .text_color(lv.color_hex(0xFFFFFF))
            .pad_hor(4)
            .pad_ver(0),
            0,
        )
        self.air_gap_tips.align_to(
            self,
            lv.ALIGN.OUT_BOTTOM_LEFT,
            0,
            -((44 - self.air_gap_tips.get_height()) // 2) - 9,
        )
        self.air_gap_tips.add_flag(lv.obj.FLAG.HIDDEN)
        if device.is_airgap_mode():
            self.air_gap_tips.clear_flag(lv.obj.FLAG.HIDDEN)
            self.ble.add_flag(lv.obj.FLAG.HIDDEN)

    def show_ble(self, status: int = 0, show: bool = True):
        if show:
            if status == StatusBar.BLE_STATE_CONNECTED:
                icon_path = "A:/res/ble-connected.png"
            elif status == StatusBar.BLE_STATE_ENABLED:
                icon_path = "A:/res/ble-enabled.png"
            else:
                icon_path = "A:/res/ble-disabled.png"
            self.ble.set_src(icon_path)
            if self.ble.has_flag(lv.obj.FLAG.HIDDEN) and not device.is_airgap_mode():
                self.ble.clear_flag(lv.obj.FLAG.HIDDEN)
        else:
            if not self.ble.has_flag(lv.obj.FLAG.HIDDEN):
                self.ble.add_flag(lv.obj.FLAG.HIDDEN)

    def show_usb(self, show: bool = False):
        if show:
            if self.usb.has_flag(lv.obj.FLAG.HIDDEN):
                self.usb.clear_flag(lv.obj.FLAG.HIDDEN)
        else:
            if not self.usb.has_flag(lv.obj.FLAG.HIDDEN):
                self.usb.add_flag(lv.obj.FLAG.HIDDEN)

    def show_charging(self, show: bool = False):
        if show:
            if self.charging.has_flag(lv.obj.FLAG.HIDDEN):
                self.charging.clear_flag(lv.obj.FLAG.HIDDEN)
        else:
            if not self.charging.has_flag(lv.obj.FLAG.HIDDEN):
                self.charging.add_flag(lv.obj.FLAG.HIDDEN)

    def set_battery_img(self, value: int, charging: bool):
        if charging:
            self.percent.clear_flag(lv.obj.FLAG.HIDDEN)
            self.percent.set_text(f"{min(value, 100)}%")
        else:
            self.percent.add_flag(lv.obj.FLAG.HIDDEN)
        icon_path = retrieve_icon_path(value, charging)
        self.battery.clear_flag(lv.obj.FLAG.HIDDEN)
        self.battery.set_src(icon_path)

    def show_air_gap_mode_tips(self, show: bool = False):
        if show:
            if self.air_gap_tips.has_flag(lv.obj.FLAG.HIDDEN):
                self.air_gap_tips.clear_flag(lv.obj.FLAG.HIDDEN)
                self.ble.add_flag(lv.obj.FLAG.HIDDEN)
        else:
            if not self.air_gap_tips.has_flag(lv.obj.FLAG.HIDDEN):
                self.air_gap_tips.add_flag(lv.obj.FLAG.HIDDEN)
                self.ble.clear_flag(lv.obj.FLAG.HIDDEN)


def retrieve_icon_path(value: int, charging: bool) -> str:
    color = "green" if charging else "white"
    for threshold in range(95, -1, -5):
        if value >= threshold:
            return f"A:/res/battery-{threshold + 5}-{color}.png"
    return f"A:/res/battery-none-{color}.png"
