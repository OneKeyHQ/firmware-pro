from .. import lv
from ..widgets.style import StyleWrapper


class Navigation(lv.obj):
    def __init__(self, parent) -> None:
        super().__init__(parent)
        self.remove_style_all()
        self.set_size(lv.pct(50), 72)
        self.align(lv.ALIGN.TOP_LEFT, 0, 44)
        self.add_style(StyleWrapper().pad_all(12), 0)
        self.nav_btn = lv.imgbtn(self)
        self.nav_btn.set_size(48, 48)
        self.nav_btn.set_align(lv.ALIGN.LEFT_MID)
        self.nav_btn.set_ext_click_area(100)
        self.nav_btn.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
        self.nav_btn.add_style(StyleWrapper().bg_img_src("A:/res/nav-back.png"), 0)
        self.add_flag(lv.obj.FLAG.EVENT_BUBBLE)


class GeneralNavigation(lv.obj):
    def __init__(self, parent, img: str = "A:/res/general.png") -> None:
        super().__init__(parent)
        self.remove_style_all()

        self.set_size(lv.pct(50), 72)
        self.align(lv.ALIGN.TOP_RIGHT, 0, 44)

        self.add_style(StyleWrapper().pad_all(12), 0)
        self.select_btn = lv.imgbtn(self)
        self.select_btn.set_size(48, 48)
        self.select_btn.set_align(lv.ALIGN.RIGHT_MID)
        self.select_btn.set_ext_click_area(100)
        self.select_btn.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
        self.select_btn.add_style(StyleWrapper().bg_img_src(img), 0)

        self.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
