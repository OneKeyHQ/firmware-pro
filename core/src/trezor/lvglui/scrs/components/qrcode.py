import lvgl as lv  # type: ignore[Import "lvgl" could not be resolved]

QRCODE_STYLE = {
    440: {
        "radius": 16,
        "border_width": 8,
    },
    380: {
        "radius": 64,
        "border_width": 38,
    },
}


class QRCode(lv.qrcode):
    def __init__(
        self, parent, data: str, icon_path=None, size: int = 380, scale: bool = False
    ):
        if size > 440:
            raise ValueError("QR code size must be less than 440")
        bg_color = lv.color_hex(0xFFFFFF)
        fg_color = lv.color_hex(0x000000)
        super().__init__(parent, size, fg_color, bg_color)
        self.set_style_border_color(bg_color, 0)
        self.set_style_border_width(QRCODE_STYLE[size]["border_width"], 0)
        self.set_style_bg_opa(0, 0)
        self.set_style_radius(QRCODE_STYLE[size]["radius"], 0)
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
