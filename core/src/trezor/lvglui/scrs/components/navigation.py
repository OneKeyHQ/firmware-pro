from .. import lv
from ..widgets.style import StyleWrapper


class Navigation(lv.obj):
    def __init__(
        self,
        parent,
        btn_bg_img: str = "A:/res/nav-back.png",
        nav_btn_align: lv.ALIGN = lv.ALIGN.LEFT_MID,
        align: lv.ALIGN = lv.ALIGN.TOP_LEFT,
    ) -> None:
        super().__init__(parent)
        self.remove_style_all()
        self.set_size(lv.pct(50), 72)
        self.align(align, 0, 44)
        self.add_style(StyleWrapper().pad_all(12), 0)
        self.nav_btn = lv.imgbtn(self)
        self.nav_btn.set_size(48, 48)
        self.nav_btn.align(nav_btn_align, 0, 0)
        self.nav_btn.set_ext_click_area(100)
        self.nav_btn.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
        self.nav_btn.add_style(StyleWrapper().bg_img_src(btn_bg_img), 0)
        self.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
