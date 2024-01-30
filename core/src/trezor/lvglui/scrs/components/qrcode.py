import lvgl as lv  # type: ignore[Import "lvgl" could not be resolved]


class QRCode(lv.qrcode):
    def __init__(
        self, parent, data: str, icon_path=None, size: int = 380, scale: bool = False
    ):
        bg_color = lv.color_hex(0xFFFFFF)
        fg_color = lv.color_hex(0x000000)
        super().__init__(parent, size, fg_color, bg_color)
        self.set_style_border_color(bg_color, 0)
        self.set_style_border_width(38, 0)
        self.set_style_bg_opa(0, 0)
        self.set_style_radius(64, 0)
        self.update(data, len(data))

        if icon_path:
            self.icon = lv.img(self)
            self.icon.set_src(icon_path)
            if scale:
                self.icon.set_zoom(512)
            self.icon.set_align(lv.ALIGN.CENTER)

    def __new__(cls, *args, **kwargs):
        if not hasattr(cls, "_instance"):
            cls._instance = super(lv.qrcode, cls).__new__(cls)
        return cls._instance
