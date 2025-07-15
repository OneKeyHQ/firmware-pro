import gc
from typing import TYPE_CHECKING

from storage import device
from trezor import loop, utils
from trezor.lvglui.scrs.components.anim import Anim

import lvgl as lv  # type: ignore[Import "lvgl" could not be resolved]

from ..lv_colors import lv_colors
from .components import slider
from .components.button import NormalButton
from .components.label import ScreenTitle, SubTitle, Title
from .components.navigation import Navigation
from .components.radio import Radio
from .widgets.style import StyleWrapper

if TYPE_CHECKING:
    from typing import Any

    pass

if __debug__:
    SETTINGS_MOVE_TIME = 120
    SETTINGS_MOVE_DELAY = 80

GO_PAGE_ANMI_TIME = 150
GO_PAGE_ANMI_DISTANCE = 60
BACK_PAGE_ANMI_TIME = 70
BACK_PAGE_ANMI_DISTANCE = 60


def apply_animations(targets, back=False, exclude_types=()):
    valid_targets = [t for t in targets if not isinstance(t, exclude_types)]

    def create_move_cb(targets_list):
        def cb(value):
            for target in targets_list:
                target.set_style_translate_x(value, 0)
                target.invalidate()

        return cb

    if valid_targets:
        move_anim = Anim(
            GO_PAGE_ANMI_DISTANCE if not back else (0 - BACK_PAGE_ANMI_DISTANCE),
            0,
            create_move_cb(valid_targets),
            time=GO_PAGE_ANMI_TIME if not back else BACK_PAGE_ANMI_TIME,
            delay=0,
            path_cb=lv.anim_t.path_ease_out,
        )
        move_anim.start()


class AnimScreen(lv.obj):
    """Singleton screen object."""

    def __init__(self, prev_scr=None, **kwargs):
        super().__init__()
        self.prev_scr = prev_scr or lv.scr_act()
        self.channel = loop.chan()
        self.add_style(StyleWrapper().bg_color(lv_colors.BLACK).bg_opa(lv.OPA.COVER), 0)
        self.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
        # panel to pin the screen size not scrolled
        self.content_area = lv.obj(self)
        self.content_area.set_size(lv.pct(100), 800)
        self.content_area.align(lv.ALIGN.TOP_MID, 0, 0)
        self.content_area.set_scrollbar_mode(lv.SCROLLBAR_MODE.ACTIVE)
        self.content_area.add_style(
            StyleWrapper()
            .bg_opa(lv.OPA.TRANSP)
            .pad_all(0)
            .border_width(0)
            .radius(0)
            .pad_bottom(24),
            0,
        )
        self.content_area.add_style(
            StyleWrapper().bg_color(lv_colors.WHITE_3),
            lv.PART.SCROLLBAR | lv.STATE.DEFAULT,
        )
        self.content_area.add_flag(lv.obj.FLAG.EVENT_BUBBLE)

        if kwargs.get("nav_back", False):
            self.nav_back = Navigation(self.content_area)
            self.add_event_cb(self.on_nav_back, lv.EVENT.GESTURE, None)
        if "title" in kwargs:
            self.title = ScreenTitle(self.content_area, None, (), kwargs["title"])
            self.title.set_width(lv.pct(78))
            self.title.set_long_mode(lv.label.LONG.DOT)
            self.title.set_style_text_align(lv.TEXT_ALIGN.CENTER, 0)
            self.title.align(lv.ALIGN.TOP_MID, 0, 63)
        if "rti_path" in kwargs:
            self.rti_btn = lv.imgbtn(self.content_area)
            self.rti_btn.set_size(48, 48)
            self.rti_btn.set_ext_click_area(100)
            self.rti_btn.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            self.rti_btn.add_style(StyleWrapper().bg_img_src(kwargs["rti_path"]), 0)
            self.rti_btn.align(lv.ALIGN.TOP_RIGHT, -12, 56)
        if "icon_path" in kwargs:
            self.icon = lv.img(self.content_area)
            self.icon.set_src(kwargs["icon_path"])
            if hasattr(self, "nav_back"):
                self.icon.align_to(self.nav_back, lv.ALIGN.OUT_BOTTOM_LEFT, 12, 8)
            else:
                self.icon.align(lv.ALIGN.TOP_LEFT, 12, 56)
        if "subtitle" in kwargs:
            self.subtitle = SubTitle(
                self.content_area, self.title, (0, 16), kwargs["subtitle"]
            )
        if "btn_text" in kwargs:
            self.btn = NormalButton(self, kwargs["btn_text"])
            self.btn.enable(lv_colors.ONEKEY_GREEN)
        self.add_event_cb(self.eventhandler, lv.EVENT.CLICKED, None)
        self.load_screen(self)

    def on_nav_back(self, event_obj):
        code = event_obj.code
        if code == lv.EVENT.GESTURE:
            _dir = lv.indev_get_act().get_gesture_dir()
            if _dir == lv.DIR.RIGHT:
                lv.event_send(self.nav_back.nav_btn, lv.EVENT.CLICKED, None)

    # event callback
    def eventhandler(self, event_obj):
        event = event_obj.code
        target = event_obj.get_target()
        if event == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if isinstance(target, lv.imgbtn):
                if hasattr(self, "nav_back") and target == self.nav_back.nav_btn:
                    if self.prev_scr is not None:  #
                        self.load_screen(self.prev_scr, destroy_self=True)
                elif hasattr(self, "rti_btn") and target == self.rti_btn:
                    self.on_click_ext(target)
            else:
                if hasattr(self, "btn") and target == self.btn:
                    self.on_click(target)

    def on_click(self, target):
        pass

    def on_click_ext(self, target):
        pass

    async def request(self) -> Any:
        return await self.channel.take()

    def refresh(self):
        area = lv.area_t()
        area.x1 = 0
        area.y1 = 0
        area.x2 = 480
        area.y2 = 800
        self.invalidate_area(area)

    def collect_animation_targets(self) -> list:
        return []

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        """Load a screen with container animation."""
        scr.invalidate()
        if device.is_animation_enabled() and isinstance(scr, AnimScreen):
            targets = scr.collect_animation_targets()
            exclude_types = (
                Title,
                SubTitle,
                Navigation,
            )
            if targets:
                apply_animations(targets, back=back, exclude_types=exclude_types)
            lv.scr_load(scr)
        else:
            scr.set_pos(0, 0)
            lv.scr_load(scr)

    # NOTE:====================Functional Code Don't Edit========================

    def __new__(cls, pre_scr=None, *args, **kwargs):
        if not hasattr(cls, "_instance"):
            cls._instance = super(lv.obj, cls).__new__(cls)
            utils.SCREENS.append(cls._instance)
        return cls._instance

    def load_screen(self, scr, destroy_self: bool = False):
        if destroy_self:
            self._load_scr(scr.__class__(), back=True)
            utils.try_remove_scr(self)
            self.del_delayed(1000)
            del self.__class__._instance
            del self
        else:
            self._load_scr(scr)

    def __del__(self):
        """Micropython doesn't support user defined __del__ now, so this not work at all."""
        try:
            self.delete()
        except BaseException:
            pass

    # NOTE:====================Functional Code Don't Edit========================


class Screen(lv.obj):
    """Singleton screen object."""

    def __init__(self, prev_scr=None, **kwargs):
        super().__init__()
        self.prev_scr = prev_scr or lv.scr_act()
        self.channel = loop.chan()
        self.add_style(StyleWrapper().bg_color(lv_colors.BLACK).bg_opa(lv.OPA.COVER), 0)
        self.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
        # panel to pin the screen size not scrolled
        self.content_area = lv.obj(self)
        self.content_area.set_size(lv.pct(100), 800)
        self.content_area.align(lv.ALIGN.TOP_MID, 0, 0)
        self.content_area.set_scrollbar_mode(lv.SCROLLBAR_MODE.ACTIVE)
        self.content_area.add_style(
            StyleWrapper()
            .bg_opa(lv.OPA.TRANSP)
            .pad_all(0)
            .border_width(0)
            .radius(0)
            .pad_bottom(24),
            0,
        )
        self.content_area.add_style(
            StyleWrapper().bg_color(lv_colors.WHITE_3),
            lv.PART.SCROLLBAR | lv.STATE.DEFAULT,
        )
        self.content_area.add_flag(lv.obj.FLAG.EVENT_BUBBLE)

        # nav_back
        if kwargs.get("nav_back", False):
            self.nav_back = Navigation(self.content_area)
            self.add_event_cb(self.on_nav_back, lv.EVENT.GESTURE, None)
        # icon
        if "icon_path" in kwargs:
            self.icon = lv.img(self.content_area)
            self.icon.set_src(kwargs["icon_path"])
            if hasattr(self, "nav_back"):
                self.icon.align_to(self.nav_back, lv.ALIGN.OUT_BOTTOM_LEFT, 12, 8)
            else:
                self.icon.align(lv.ALIGN.TOP_LEFT, 12, 56)
        # rt_icon
        if "rti_path" in kwargs:
            self.rti_btn = lv.imgbtn(self.content_area)
            self.rti_btn.set_size(48, 48)
            self.rti_btn.set_ext_click_area(100)
            self.rti_btn.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            self.rti_btn.add_style(StyleWrapper().bg_img_src(kwargs["rti_path"]), 0)
            self.rti_btn.align(lv.ALIGN.TOP_RIGHT, -12, 56)
        # title
        if "title" in kwargs:
            self.title = Title(self.content_area, None, (), kwargs["title"])
            if hasattr(self, "icon"):
                self.title.align_to(self.icon, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 16)
            elif hasattr(self, "nav_back"):
                self.title.align_to(self.nav_back, lv.ALIGN.OUT_BOTTOM_LEFT, 12, 12)
        # subtitle
        if "subtitle" in kwargs:
            self.subtitle = SubTitle(
                self.content_area, self.title, (0, 16), kwargs["subtitle"]
            )
        # btn
        if "btn_text" in kwargs:
            self.btn = NormalButton(self, kwargs["btn_text"])
            self.btn.enable(lv_colors.ONEKEY_GREEN)
        self.add_event_cb(self.eventhandler, lv.EVENT.CLICKED, None)

        self.load_screen(self)

    def on_nav_back(self, event_obj):
        code = event_obj.code
        if code == lv.EVENT.GESTURE:
            _dir = lv.indev_get_act().get_gesture_dir()
            if _dir == lv.DIR.RIGHT:
                lv.event_send(self.nav_back.nav_btn, lv.EVENT.CLICKED, None)

    # event callback
    def eventhandler(self, event_obj):
        event = event_obj.code
        target = event_obj.get_target()
        if event == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if isinstance(target, lv.imgbtn):
                if hasattr(self, "nav_back") and target == self.nav_back.nav_btn:
                    if self.prev_scr is not None:  #
                        self.load_screen(self.prev_scr, destroy_self=True)
                elif hasattr(self, "rti_btn") and target == self.rti_btn:
                    self.on_click_ext(target)
            else:
                if hasattr(self, "btn") and target == self.btn:
                    self.on_click(target)

    # click event callback
    def on_click(self, target):
        pass

    def on_click_ext(self, target):
        pass

    async def request(self) -> Any:
        return await self.channel.take()

    def refresh(self):
        area = lv.area_t()
        area.x1 = 0
        area.y1 = 0
        area.x2 = 480
        area.y2 = 800
        self.invalidate_area(area)

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        # """Load a screen with animation."""
        scr.set_pos(0, 0)
        lv.scr_load(scr)

    # NOTE:====================Functional Code Don't Edit========================

    def __new__(cls, pre_scr=None, *args, **kwargs):
        if not hasattr(cls, "_instance"):
            cls._instance = super(lv.obj, cls).__new__(cls)
            utils.SCREENS.append(cls._instance)
        return cls._instance

    def load_screen(self, scr, destroy_self: bool = False):
        if destroy_self:
            self._load_scr(scr.__class__(), back=True)
            utils.try_remove_scr(self)
            self.del_delayed(1000)
            del self.__class__._instance
            del self
            gc.collect()
            # pyright: off
            gc.threshold(gc.mem_free() // 4 + gc.mem_alloc())
            # pyright: on
        else:
            self._load_scr(scr)

    def __del__(self):
        """Micropython doesn't support user defined __del__ now, so this not work at all."""
        try:
            self.delete()
        except BaseException:
            pass

    # NOTE:====================Functional Code Don't Edit========================


class ANIM_DIRS:

    NONE = 0
    HOR = 1
    VER = 2


class FullSizeWindow(lv.obj):
    """Generic screen contains a title, a subtitle, and one or two button and a optional icon."""

    def __init__(
        self,
        title: str | None,
        subtitle: str | None,
        confirm_text: str = "",
        cancel_text: str = "",
        icon_path: str | None = None,
        options: str | None = None,
        hold_confirm: bool = False,
        auto_close_ms: int = 0,
        anim_dir: int = ANIM_DIRS.HOR,
        primary_color=lv_colors.ONEKEY_GREEN,
        sub_icon_path: str | None = None,
    ):
        super().__init__(lv.scr_act())

        if __debug__:
            self.layout_title = title
            self.layout_subtitle = subtitle

        self.channel = loop.chan()
        self.anim_dir = anim_dir
        self.set_size(lv.pct(100), lv.pct(100))
        self.align(lv.ALIGN.TOP_MID, 0, 0)
        self.show_load_anim()
        self.add_style(
            StyleWrapper()
            .bg_color(lv_colors.BLACK)
            .pad_all(0)
            .border_width(0)
            .radius(0),
            0,
        )
        self.hold_confirm = hold_confirm
        self.content_area = lv.obj(self)
        self.content_area.set_size(lv.pct(100), lv.SIZE.CONTENT)
        self.content_area.align(lv.ALIGN.TOP_MID, 0, 44)
        self.content_area.set_scrollbar_mode(lv.SCROLLBAR_MODE.ACTIVE)
        self.content_area.add_style(
            StyleWrapper()
            .bg_color(lv_colors.BLACK)
            .pad_all(0)
            .pad_bottom(24)
            .border_width(0)
            .radius(0),
            0,
        )
        self.content_area.add_style(
            StyleWrapper().bg_color(lv_colors.WHITE_3),
            lv.PART.SCROLLBAR | lv.STATE.DEFAULT,
        )
        self.content_area.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
        self.icon_path = icon_path
        if icon_path:
            self.icon = lv.img(self.content_area)
            self.icon.remove_style_all()
            self.icon.set_size(lv.SIZE.CONTENT, lv.SIZE.CONTENT)
            self.icon.set_src(icon_path)
            self.icon.align(lv.ALIGN.TOP_LEFT, 12, 12)
            if sub_icon_path:
                self.sub_icon = lv.img(self.content_area)
                self.sub_icon.remove_style_all()
                self.sub_icon.set_src(sub_icon_path)
                self.sub_icon.set_zoom(107)
                self.sub_icon.align_to(self.icon, lv.ALIGN.BOTTOM_RIGHT, 28, 28)

        if title:
            self.title = Title(self.content_area, None, (), title, pos_y=12)
            if icon_path:
                self.title.align_to(self.icon, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 16)
            if subtitle is not None:
                self.subtitle = SubTitle(
                    self.content_area, self.title, (0, 16), subtitle
                )
        else:
            if hasattr(self, "icon") and self.icon:
                self.icon.align(lv.ALIGN.TOP_MID, 0, 0)
            if subtitle is not None:
                self.subtitle = SubTitle(self.content_area, None, (0, 12), subtitle)

        if options:
            self.content_area.set_height(646)
            self.selector = Radio(self.content_area, options)
        else:
            self.content_area.set_style_max_height(646, 0)
            self.content_area.set_style_min_height(500, 0)
        if cancel_text:
            self.btn_no = NormalButton(self, cancel_text)
            if confirm_text:
                if not self.hold_confirm:
                    self.btn_no.set_size(224, 98)
                else:
                    self.btn_no.add_style(StyleWrapper().bg_color(lv_colors.BLACK), 0)
                    self.btn_no.add_style(
                        StyleWrapper().bg_color(lv_colors.ONEKEY_BLACK).bg_opa(),
                        lv.PART.MAIN | lv.STATE.PRESSED,
                    )
                    self.btn_no.clear_flag(lv.obj.FLAG.CLICKABLE)
                    self.btn_no.click_mask.add_flag(lv.obj.FLAG.CLICKABLE)
                self.btn_no.align(lv.ALIGN.BOTTOM_LEFT, 12, -8)
                # else:
                #     self.btn_no.align(lv.ALIGN.BOTTOM_LEFT, 8, -8)
        if confirm_text:
            if cancel_text:
                if self.hold_confirm:
                    self.content_area.set_style_max_height(520, 0)
                    self.slider = slider.Slider(self, confirm_text)
                else:
                    self.btn_yes = NormalButton(self, confirm_text)
                    self.btn_yes.set_size(224, 98)
                    self.btn_yes.align_to(self, lv.ALIGN.BOTTOM_RIGHT, -12, -8)
            else:
                self.btn_yes = NormalButton(self, confirm_text)
            if self.hold_confirm:
                self.slider.add_event_cb(self.eventhandler, lv.EVENT.READY, None)
            else:
                self.btn_yes.enable(primary_color, text_color=lv_colors.BLACK)
        if self.vibrate_necessary():
            self.vibrated = False
            self.add_event_cb(self.on_win_visible, lv.EVENT.DRAW_POST_END, None)
        self.add_event_cb(self.eventhandler, lv.EVENT.CLICKED, None)
        self.clear_flag(lv.obj.FLAG.GESTURE_BUBBLE)
        if auto_close_ms:
            self.destroy(delay_ms=auto_close_ms)
        if __debug__:
            self.notify_change()

    def on_win_visible(self, _event_obj):
        from trezor import motor

        if __debug__:
            print("on_draw_post_end called.")
        if self.vibrated:
            self.remove_event_cb(None)
            return
        self.vibrated = True
        self.remove_event_cb(None)
        if __debug__:
            print("vibrate start...")
        if self.icon_path == "A:/res/success.png":
            motor.vibrate(motor.SUCCESS)
        elif self.icon_path == "A:/res/warning.png":
            motor.vibrate(motor.WARNING)
        elif self.icon_path == "A:/res/danger.png":
            motor.vibrate(motor.ERROR)

    def vibrate_necessary(self) -> bool:
        if not hasattr(self, "icon"):
            return False
        if __debug__:
            print(f"vibrate necessary ? {self.icon_path}")
        if self.icon_path in [
            "A:/res/success.png",
            "A:/res/warning.png",
            "A:/res/danger.png",
        ]:
            if __debug__:
                print("vibrate necessary: true")
            return True
        return False

    def btn_layout_ver(self):
        if not all([hasattr(self, "btn_no"), hasattr(self, "btn_yes")]):
            return
        self.content_area.set_style_max_height(520, 0)
        self.btn_no.set_size(456, 98)
        self.btn_no.align(lv.ALIGN.BOTTOM_MID, 0, -8)
        self.btn_yes.set_size(456, 98)
        self.btn_yes.align_to(self.btn_no, lv.ALIGN.OUT_TOP_MID, 0, -8)

    def add_nav_back(self):
        """
        Add a navigation back button to the screen. The nav_back button is aligned to the left of the screen.
        If added, add a event handler called <on_nav_back> or override the method <eventhandler>
        when you need to handle the back event specifically.
        """
        self.nav_back = Navigation(self)
        self.content_area.set_style_max_height(574, 0)
        self.content_area.align_to(self.nav_back, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 0)

    def add_nav_back_right(self):
        """
        Add a navigation back button to the screen. The nav_back button is aligned to the right of the screen.
        If added, add a event handler called <on_nav_back> or override the method <eventhandler>
        when you need to handle the back event specifically.
        """
        self.nav_back = Navigation(
            self,
            btn_bg_img="A:/res/cancel.png",
            nav_btn_align=lv.ALIGN.RIGHT_MID,
            align=lv.ALIGN.TOP_RIGHT,
        )
        self.content_area.set_style_max_height(574, 0)
        self.content_area.align_to(self.nav_back, lv.ALIGN.OUT_BOTTOM_RIGHT, 0, 0)

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if hasattr(self, "btn_no") and (
                (target == self.btn_no.click_mask)
                if self.hold_confirm
                else (target == self.btn_no)
            ):
                self.show_dismiss_anim()
                self.channel.publish(0)
            elif hasattr(self, "btn_yes") and target == self.btn_yes:
                self.show_unload_anim()
                if hasattr(self, "selector"):
                    self.channel.publish(self.selector.get_selected_str())
                else:
                    if not self.hold_confirm:
                        self.channel.publish(1)
            elif hasattr(self, "nav_back") and target == self.nav_back.nav_btn:
                if not hasattr(self, "on_nav_back"):
                    self.show_dismiss_anim()
                    self.channel.publish(0)
        elif code == lv.EVENT.READY and self.hold_confirm:
            if target == self.slider:
                self.show_dismiss_anim()
                self.channel.publish(1)

    async def request(self) -> Any:
        if __debug__:
            from apps.debug import confirm_signal, input_signal
            from trezor.ui import Result

            value = None
            # return await loop.race(confirm_signal(),self.channel.take())
            try:
                value = await loop.race(
                    confirm_signal(), input_signal(), self.channel.take()
                )
            except Result as result:
                # Result exception was raised, this means this layout is complete.

                value = result.value
            return value

        else:
            return await self.channel.take()

    if __debug__:

        def notify_change(self):
            from apps.debug import notify_layout_change

            notify_layout_change(self)

        def read_content(self) -> list[str]:
            return [self.layout_title or ""] + [self.layout_subtitle or ""]

    def destroy(self, delay_ms=400):
        try:
            self.del_delayed(delay_ms)
        except Exception:
            pass

    def _delete_cb(self, _anim):
        try:
            self.del_delayed(100)
        except Exception:
            pass

    def _load_anim_hor(self):
        Anim(
            480,
            0,
            self._set_x,
            time=120 if not __debug__ else SETTINGS_MOVE_TIME,
            delay=80 if not __debug__ else SETTINGS_MOVE_DELAY,
        ).start_anim()

    def _load_anim_ver(self):
        self.set_y(800)
        Anim(
            800,
            0,
            self._set_y,
            time=120 if not __debug__ else SETTINGS_MOVE_TIME,
            delay=80 if not __debug__ else SETTINGS_MOVE_DELAY,
        ).start_anim()

    def _set_y(self, y):
        try:
            self.set_y(y)
        except Exception:
            pass

    def _set_x(self, x):
        try:
            self.set_x(x)
        except Exception:
            pass

    def _dismiss_anim_hor(self):
        Anim(
            0,
            480,
            self._set_x,
            time=120 if not __debug__ else SETTINGS_MOVE_TIME,
            delay=80 if not __debug__ else SETTINGS_MOVE_DELAY,
            del_cb=self._delete_cb,
        ).start_anim()

    def _dismiss_anim_ver(self):
        Anim(
            0,
            800,
            self._set_y,
            time=120 if not __debug__ else SETTINGS_MOVE_TIME,
            delay=80 if not __debug__ else SETTINGS_MOVE_DELAY,
            del_cb=self._delete_cb,
        ).start_anim()

    def show_load_anim(self):
        self.set_pos(0, 0)
        return

    def show_dismiss_anim(self):
        self.destroy()
        return

    def show_unload_anim(self):
        # if self.anim_dir == ANIM_DIRS.HOR:
        #     Anim(0, -480, self.set_pos, time=200, y_axis=False, delay=200, del_cb=self._delete).start()
        # else:
        #     self.show_dismiss_anim()
        self.destroy(1100)

    def refresh(self):
        area = lv.area_t()
        area.x1 = 0
        area.y1 = 0
        area.x2 = 480
        area.y2 = 800
        self.invalidate_area(area)
