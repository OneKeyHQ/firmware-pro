import gc
import math
import utime
from micropython import const

import storage.cache
import storage.device as storage_device
from trezor import io, loop, uart, utils, wire, workflow
from trezor.enums import SafetyCheckLevel
from trezor.langs import langs, langs_keys
from trezor.lvglui.i18n import gettext as _, i18n_refresh, keys as i18n_keys
from trezor.lvglui.lv_colors import lv_colors
from trezor.lvglui.lv_symbols import LV_SYMBOLS
from trezor.lvglui.scrs.components.pageable import Indicator
from trezor.qr import (
    close_camera,
    get_hd_key,
    retrieval_encoder,
    retrieval_hd_key,
    save_app_obj,
    scan_qr,
)
from trezor.ui import display, style

from apps.common import passphrase, safety_checks

from . import (
    font_GeistRegular20,
    font_GeistRegular26,
    font_GeistRegular30,
    font_GeistSemiBold26,
    font_GeistSemiBold30,
    font_GeistSemiBold38,
)
from .address import AddressManager, chains_brief_info
from .common import AnimScreen, FullSizeWindow, Screen, lv  # noqa: F401, F403, F405
from .components.anim import Anim
from .components.banner import LEVEL, Banner
from .components.button import ListItemBtn, ListItemBtnWithSwitch, NormalButton
from .components.container import ContainerFlexCol, ContainerFlexRow, ContainerGrid
from .components.label import SubTitle, Title
from .components.listitem import (
    DisplayItemWithFont_30,
    DisplayItemWithFont_TextPairs,
    ImgGridItem,
    ListItemWithLeadingCheckbox,
)
from .deviceinfo import DeviceInfoManager
from .nftmanager import NftGallery
from .preview_utils import (
    create_preview_container,
    create_preview_image,
    create_top_mask,
)
from .widgets.style import StyleWrapper

_attach_to_pin_task_running = False
_cached_styles = {}
_animation_in_progress = False
_last_jpeg_loaded = None
_active_timers = []


def _clear_preview_cache() -> None:
    lv.img.cache_invalidate_src(None)
    gc.collect()


def _normalize_wallpaper_src(raw, *, allow_default: bool = True, default=None) -> str:
    if isinstance(raw, bytes):
        if hasattr(raw, "decode"):
            raw = raw.decode("utf-8", "ignore")
        else:
            raw = ""
    if not raw and allow_default:
        raw = default if default is not None else utils.get_default_wallpaper()
    return raw or ""


def _wallpaper_display_path() -> str:
    raw = _normalize_wallpaper_src(storage_device.get_homescreen())
    if raw.startswith("A:/"):
        if "/res/nfts/" in raw:
            return "1:" + raw[2:]
        return raw[3:]
    if raw.startswith("A:1:"):
        return raw[2:]
    return raw


class Layer2Manager:
    """Encapsulate Layer2 background state and helpers to avoid scattered globals."""

    @classmethod
    def is_animating(cls) -> bool:
        global _animation_in_progress
        return _animation_in_progress

    @classmethod
    def set_animating(cls, value: bool) -> None:
        global _animation_in_progress
        _animation_in_progress = value

    @classmethod
    def ensure_background(cls, display) -> bool:
        global _last_jpeg_loaded
        loader = getattr(display, "cover_background_load_jpeg", None)
        if not loader:
            return False
        path = _wallpaper_display_path()
        if _last_jpeg_loaded == path:
            return True
        loader(path)
        _last_jpeg_loaded = path
        return True

    @classmethod
    def reset_background_cache(cls) -> None:
        global _last_jpeg_loaded
        _last_jpeg_loaded = None

    @classmethod
    def preload_background(cls, display) -> None:
        """Warm up the Layer2 JPEG background if the loader is available."""
        cls.ensure_background(display)

    @classmethod
    def set_visibility(
        cls, display, visible: bool, position: int | None = None
    ) -> None:
        setter = getattr(display, "cover_background_set_visible", None)
        mover = getattr(display, "cover_background_move_to_y", None)
        shower = getattr(display, "cover_background_show", None)
        hider = getattr(display, "cover_background_hide", None)
        if visible:
            if setter:
                setter(True)
            if shower:
                shower()
            if mover and position is not None:
                mover(position)
        else:
            if setter:
                setter(False)
            if hider:
                hider()

    @classmethod
    def cleanup_timers(cls):
        global _active_timers
        for timer in _active_timers:
            if timer and hasattr(timer, "delete"):
                timer.delete()
        _active_timers.clear()

    @classmethod
    def schedule_once(cls, delay_ms: int, callback):
        global _active_timers
        timer = lv.timer_create(lambda _t: callback(), delay_ms, None)
        timer.set_repeat_count(1)
        _active_timers.append(timer)
        return timer

    @staticmethod
    def with_lvgl_timer_pause(func, *args, **kwargs):
        pause_handler = getattr(lv, "timer_handler_pause", None)
        resume_handler = getattr(lv, "timer_handler_resume", None)
        if pause_handler:
            pause_handler()
        try:
            return func(*args, **kwargs)
        finally:
            if resume_handler:
                resume_handler()


def get_cached_style(image_src):
    safe_src = _normalize_wallpaper_src(image_src)
    if not safe_src:
        safe_src = utils.get_default_wallpaper()
    return StyleWrapper().bg_img_src(safe_src).border_width(0)


def _get_main_screen_instance():
    MainScreenCls = globals().get("MainScreen")
    if not MainScreenCls:
        return None
    return getattr(MainScreenCls, "_instance", None)


def _invalidate_image_cache(*paths):
    if not hasattr(lv.img, "cache_invalidate_src"):
        return
    for path in paths:
        if path:
            lv.img.cache_invalidate_src(path)
    lv.img.cache_invalidate_src(None)


def apply_home_wallpaper(new_wallpaper: str | None) -> None:
    if not new_wallpaper:
        return

    old_wallpaper = storage_device.get_appdrawer_background()
    storage_device.set_appdrawer_background(new_wallpaper)

    Layer2Manager.reset_background_cache()
    _invalidate_image_cache(old_wallpaper, new_wallpaper)

    main_screen = _get_main_screen_instance()
    if main_screen and hasattr(main_screen, "apps") and main_screen.apps:
        main_screen.apps.refresh_background()
        if main_screen.apps.has_flag(lv.obj.FLAG.HIDDEN):
            main_screen.apps.invalidate()


def apply_lock_wallpaper(new_wallpaper: str | None) -> None:
    if not new_wallpaper:
        return

    old_wallpaper = storage_device.get_homescreen()
    storage_device.set_homescreen(new_wallpaper)

    Layer2Manager.reset_background_cache()
    _invalidate_image_cache(old_wallpaper, new_wallpaper)

    main_screen = _get_main_screen_instance()
    if main_screen:
        sanitized = _normalize_wallpaper_src(new_wallpaper)
        main_screen.set_background_image(sanitized)
        lv.refr_now(None)

    from .lockscreen import LockScreen

    LockScreen.invalidate("wallpaper change")


def replace_wallpaper_if_in_use(
    deleted_path: str | None, replacement_path: str | None = None
) -> None:
    if not deleted_path:
        return

    replacement = replacement_path or utils.get_default_wallpaper()

    base_name = deleted_path.split("/")[-1]
    blur_name = base_name
    lower_name = base_name.lower()
    for ext in (".jpg", ".jpeg", ".png"):
        if lower_name.endswith(ext):
            blur_name = base_name[: -len(ext)] + "-blur" + ext
            break

    current_home = storage_device.get_appdrawer_background()
    current_lock = storage_device.get_homescreen()

    def _matches(candidate: str | None) -> bool:
        if not candidate:
            return False
        return (
            deleted_path in candidate  # type: ignore[not supported]
            or candidate.endswith("/" + base_name)
            or candidate.endswith("/" + blur_name)
        )

    if _matches(current_home):
        apply_home_wallpaper(replacement)
    if _matches(current_lock):
        apply_lock_wallpaper(replacement)


def brightness2_percent_str(brightness: int) -> str:
    return f"{int(brightness / style.BACKLIGHT_MAX * 100)}%"


GRID_CELL_SIZE_ROWS = const(240)
GRID_CELL_SIZE_COLS = const(144)

APP_DRAWER_UP_TIME = 300
APP_DRAWER_DOWN_TIME = 300
APP_DRAWER_UP_DELAY = 0
APP_DRAWER_DOWN_DELAY = 0
PATH_EASE_OUT = lv.anim_t.path_ease_out
APP_DRAWER_UP_PATH_CB = PATH_EASE_OUT
APP_DRAWER_DOWN_PATH_CB = PATH_EASE_OUT


def change_state(is_busy: bool = False):
    if hasattr(MainScreen, "_instance"):
        if MainScreen._instance:
            MainScreen._instance.change_state(is_busy)


class MainScreen(Screen):
    # When True, the next time MainScreen appears it should reopen AppDrawer
    _reopen_drawer_on_next_show = False

    def _ensure_background_style(self):
        """Ensure the singleton screen has a reusable background style."""
        if not hasattr(self, "_background_style"):
            self._background_style = StyleWrapper().border_width(0)
            self.add_style(self._background_style, 0)
        return self._background_style

    def set_background_image(self, image_src: str) -> None:
        """Update background image without accumulating style objects."""
        style = self._ensure_background_style()
        style.bg_img_src(image_src)
        self.invalidate()

    def __init__(self, device_name=None, ble_name=None, dev_state=None):
        self._is_busy = False
        self._communication_hold = False

        lockscreen = _normalize_wallpaper_src(storage_device.get_homescreen())
        if not hasattr(self, "_init"):
            self._init = True

            # Check if device name display is enabled
            show_device_names = storage_device.is_device_name_display_enabled()

            # Get real device names
            real_device_name = storage_device.get_label()  # User custom label
            real_ble_name = storage_device.get_ble_name() or uart.get_ble_name()

            # Initialize Screen with proper kwargs
            if show_device_names:
                super().__init__(title=real_device_name, subtitle=real_ble_name)
                self.title.add_style(StyleWrapper().text_align_center(), 0)
                self.subtitle.add_style(
                    StyleWrapper().text_align_center().text_color(lv_colors.WHITE), 0
                )
            else:
                super().__init__()

            # Set background for first-time initialization
            self.set_background_image(lockscreen)
        else:
            # Check if device name display setting has changed
            show_device_names = storage_device.is_device_name_display_enabled()

            # Get real device names
            real_device_name = storage_device.get_label()  # User custom label
            real_ble_name = storage_device.get_ble_name() or uart.get_ble_name()

            # Update title and subtitle based on current setting

            if show_device_names:
                # Ensure title and subtitle exist; lazily create if missing
                if not (hasattr(self, "title") and self.title):
                    self.title = Title(self.content_area, None, (), real_device_name)
                    # Center align like first-time init and place at 76px from top
                    self.title.align_to(self.content_area, lv.ALIGN.TOP_MID, 0, 76)
                    self.title.add_style(StyleWrapper().text_align_center(), 0)
                else:
                    self.title.set_text(real_device_name)
                    self.title.clear_flag(lv.obj.FLAG.HIDDEN)

                if not (hasattr(self, "subtitle") and self.subtitle):
                    self.subtitle = SubTitle(
                        self.content_area, self.title, (0, 16), real_ble_name
                    )
                    # Center align + white color like first-time init
                    self.subtitle.add_style(
                        StyleWrapper().text_align_center().text_color(lv_colors.WHITE),
                        0,
                    )
                else:
                    self.subtitle.set_text(real_ble_name)
                    self.subtitle.clear_flag(lv.obj.FLAG.HIDDEN)
            else:
                # Hide device names if they exist
                if hasattr(self, "title") and self.title:
                    self.title.add_flag(lv.obj.FLAG.HIDDEN)
                    self.title.set_text("")
                if hasattr(self, "subtitle") and self.subtitle:
                    self.subtitle.add_flag(lv.obj.FLAG.HIDDEN)
                    self.subtitle.set_text("")

            lockscreen = _normalize_wallpaper_src(storage_device.get_homescreen())
            self.set_background_image(lockscreen)
            if hasattr(self, "dev_state"):
                from apps.base import get_state

                state = get_state()
                if state:
                    self.dev_state.show(state)
                else:
                    self.dev_state.delete()
                    del self.dev_state
            if self.bottom_tips:
                self.bottom_tips.set_text(_(i18n_keys.BUTTON__SWIPE_TO_SHOW_APPS))
                self.up_arrow.align_to(self.bottom_tips, lv.ALIGN.OUT_TOP_MID, 0, -8)
            if self.apps:
                self.apps.refresh_text()
                # Refresh AppDrawer background to ensure wallpaper updates sync background
                self.refresh_appdrawer_background()
                # If returning from an app opened via AppDrawer, restore the drawer
                if getattr(self.__class__, "_reopen_drawer_on_next_show", False):
                    # Show AppDrawer contents directly (no Layer2 animation)
                    self._show_appdrawer_contents()
                    # Reset the flag so it only happens once
                    self.__class__._reopen_drawer_on_next_show = False
            return
        if hasattr(self, "title") and self.title:
            self.title.align_to(self.content_area, lv.ALIGN.TOP_MID, 0, 76)
        if hasattr(self, "subtitle") and self.subtitle:
            if hasattr(self, "title") and self.title:
                self.subtitle.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, 16)
            else:
                self.subtitle.align_to(self.content_area, lv.ALIGN.TOP_MID, 0, 76)
        if dev_state:
            self.dev_state = MainScreen.DevStateTipsBar(self)
            # Align to subtitle if it exists, otherwise to title, otherwise to content area
            if hasattr(self, "subtitle") and self.subtitle:
                self.dev_state.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 48)
            elif hasattr(self, "title") and self.title:
                self.dev_state.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, 48)
            else:
                self.dev_state.align_to(self.content_area, lv.ALIGN.TOP_MID, 0, 124)
            self.dev_state.show(dev_state)
        post_lock = _normalize_wallpaper_src(storage_device.get_homescreen())
        self.set_background_image(post_lock)
        self.clear_flag(lv.obj.FLAG.SCROLLABLE)
        self.bottom_tips = lv.label(self.content_area)
        self.bottom_tips.set_long_mode(lv.label.LONG.WRAP)
        self.bottom_tips.set_size(456, lv.SIZE.CONTENT)
        self.bottom_tips.set_text(_(i18n_keys.BUTTON__SWIPE_TO_SHOW_APPS))
        self.bottom_tips.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular26)
            .text_color(lv_colors.WHITE)
            .text_align_center(),
            0,
        )
        self.bottom_tips.align(lv.ALIGN.BOTTOM_MID, 0, 0)
        self.up_arrow = lv.img(self.content_area)
        self.up_arrow.set_src("A:/res/up-home.png")
        self.up_arrow.align_to(self.bottom_tips, lv.ALIGN.OUT_TOP_MID, 0, -8)

        self.apps = self.AppDrawer(self)
        self.set_size(480, 800)
        # Align with upstream: show MainScreen by default, hide AppDrawer
        self.apps.add_flag(lv.obj.FLAG.HIDDEN)
        # Gesture handling for MainScreen
        self.add_event_cb(self.on_main_gesture, lv.EVENT.GESTURE, None)

        # Set singleton instance (important for later access)
        MainScreen._instance = self

        save_app_obj(self)

    def on_main_gesture(self, event_obj):
        code = event_obj.code
        if code == lv.EVENT.GESTURE:
            if getattr(self, "_communication_hold", False):
                return
            if Layer2Manager.is_animating():
                return

            if hasattr(self, "apps") and self.apps:
                is_app_drawer_hidden = self.apps.has_flag(lv.obj.FLAG.HIDDEN)
                if not is_app_drawer_hidden:
                    return

            indev = lv.indev_get_act()
            _dir = indev.get_gesture_dir()

            if _dir == lv.DIR.TOP:
                self.refresh_appdrawer_background()
                self.show_appdrawer_simple()

    def show_appdrawer_simple(self):
        if getattr(self, "_communication_hold", False):
            return
        if Layer2Manager.is_animating() or not (hasattr(self, "apps") and self.apps):
            return

        from trezorui import Display

        display = Display()
        background_ready = Layer2Manager.ensure_background(display)
        can_animate = (
            background_ready
            and hasattr(display, "cover_background_animate_to_y")
            and hasattr(display, "cover_background_move_to_y")
        )

        if can_animate:
            Layer2Manager.set_visibility(display, True, position=0)

        self._show_appdrawer_contents()

        if not can_animate:
            return

        Layer2Manager.set_animating(True)

        def start_layer2_animation():
            try:
                Layer2Manager.with_lvgl_timer_pause(
                    display.cover_background_animate_to_y, -800, 200  # type: ignore[is unknown]
                )
            except Exception:
                Layer2Manager.set_visibility(display, False)
                Layer2Manager.set_animating(False)
                Layer2Manager.cleanup_timers()
                return

            def on_slide_complete():
                Layer2Manager.set_visibility(display, False)
                Layer2Manager.set_animating(False)
                Layer2Manager.cleanup_timers()

            Layer2Manager.schedule_once(200, on_slide_complete)

        Layer2Manager.schedule_once(20, start_layer2_animation)

    def _toggle_main_content(self, visible: bool) -> None:
        self.hidden_others(not visible)
        toggle_targets = ("up_arrow", "bottom_tips", "content_area")
        for attr in toggle_targets:
            obj = getattr(self, attr, None)
            if not obj:
                continue
            if visible:
                obj.clear_flag(lv.obj.FLAG.HIDDEN)
            else:
                obj.add_flag(lv.obj.FLAG.HIDDEN)
        dev_state = getattr(self, "dev_state", None)
        if dev_state:
            if visible and hasattr(dev_state, "show"):
                dev_state.show()
            elif not visible and hasattr(dev_state, "hidden"):
                dev_state.hidden()

    def _show_appdrawer_contents(self):
        self._toggle_main_content(False)
        lv.refr_now(None)
        self.apps.clear_flag(lv.obj.FLAG.HIDDEN)
        self.apps.clear_flag(lv.obj.FLAG.GESTURE_BUBBLE)
        self.apps.visible = True
        if hasattr(self.apps, "current_page"):
            self.apps.show_page(self.apps.current_page)

    def restore_main_content(self):
        self._toggle_main_content(True)

    def _get_title_labels(self):
        labels = []
        if hasattr(self, "title") and self.title:
            labels.append(self.title)
        if hasattr(self, "subtitle") and self.subtitle:
            labels.append(self.subtitle)
        return labels

    def _cancel_title_fade(self):
        labels = self._get_title_labels()
        for label in labels:
            lv.anim_del(label, None)
            label.set_style_text_opa(lv.OPA.COVER, 0)
            label.invalidate()
        if hasattr(self, "_title_fade_anims"):
            self._title_fade_anims.clear()
        self._title_fade_prepared = False

    def prepare_title_fade_in(self):
        labels = self._get_title_labels()
        if not labels:
            self._title_fade_prepared = False
            return

        self._cancel_title_fade()
        if not hasattr(self, "_title_fade_anims"):
            self._title_fade_anims = []

        for label in labels:
            label.clear_flag(lv.obj.FLAG.HIDDEN)
            label.set_style_text_opa(0, 0)
            label.invalidate()

        self._title_fade_prepared = True

    def start_title_fade_in(self, duration=150):
        labels = self._get_title_labels()
        if not labels:
            self._title_fade_prepared = False
            return

        if not getattr(self, "_title_fade_prepared", False):
            # Ensure labels start from 0 opacity when fade is triggered without preparation
            for label in labels:
                label.clear_flag(lv.obj.FLAG.HIDDEN)
                label.set_style_text_opa(0, 0)
                label.invalidate()

        if not hasattr(self, "_title_fade_anims"):
            self._title_fade_anims = []
        else:
            self._title_fade_anims.clear()

        def _make_exec_cb(target_label):
            def _exec_cb(_anim, value):
                target_label.set_style_text_opa(value, 0)
                target_label.invalidate()

            return _exec_cb

        def _make_ready_cb(target_label):
            def _ready_cb(_anim):
                target_label.set_style_text_opa(lv.OPA.COVER, 0)
                target_label.invalidate()
                if (
                    hasattr(self, "_title_fade_anims")
                    and _anim in self._title_fade_anims
                ):
                    self._title_fade_anims.remove(_anim)
                if not getattr(self, "_title_fade_anims", []):
                    self._title_fade_prepared = False

            return _ready_cb

        for label in labels:
            lv.anim_del(label, None)
            fade_anim = lv.anim_t()
            fade_anim.init()
            fade_anim.set_var(label)
            fade_anim.set_time(duration)
            fade_anim.set_values(0, lv.OPA.COVER)
            fade_anim.set_path_cb(lv.anim_t.path_linear)
            fade_anim.set_custom_exec_cb(_make_exec_cb(label))
            fade_anim.set_ready_cb(_make_ready_cb(label))
            self._title_fade_anims.append(fade_anim)
            lv.anim_t.start(fade_anim)

    def hidden_others(self, hidden: bool = True):
        labels = self._get_title_labels()
        if hidden:
            self._cancel_title_fade()
            for label in labels:
                label.add_flag(lv.obj.FLAG.HIDDEN)
                label.set_style_text_opa(lv.OPA.COVER, 0)
        else:
            for label in labels:
                label.clear_flag(lv.obj.FLAG.HIDDEN)
                if not getattr(self, "_title_fade_prepared", False):
                    label.set_style_text_opa(lv.OPA.COVER, 0)
                label.invalidate()

    def refresh_appdrawer_background(self):
        if hasattr(self, "apps") and self.apps:
            self.apps.refresh_background()

    def change_state(self, busy: bool):
        if getattr(self, "_is_busy", False) == busy:
            return
        self._is_busy = busy
        apps = getattr(self, "apps", None)
        if busy:
            self._communication_hold = True
            if apps:
                lock_comm = getattr(apps, "lock_for_communication", None)
                if callable(lock_comm):
                    lock_comm()
                hide_fallback = getattr(apps, "hide_to_mainscreen_fallback", None)
                if callable(hide_fallback):
                    hide_fallback()
            self.clear_flag(lv.obj.FLAG.CLICKABLE)
            self.up_arrow.add_flag(lv.obj.FLAG.HIDDEN)
            self.bottom_tips.set_text(_(i18n_keys.BUTTON__PROCESSING))
        else:
            self._communication_hold = False
            if apps:
                unlock_comm = getattr(apps, "unlock_after_communication", None)
                if callable(unlock_comm):
                    unlock_comm()
            self.add_flag(lv.obj.FLAG.CLICKABLE)
            self.up_arrow.clear_flag(lv.obj.FLAG.HIDDEN)
            self.bottom_tips.set_text(_(i18n_keys.BUTTON__SWIPE_TO_SHOW_APPS))

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)

    class DevStateTipsBar(lv.obj):
        def __init__(self, parent) -> None:
            super().__init__(parent)
            self.remove_style_all()
            self.set_size(432, 64)
            self.add_style(
                StyleWrapper()
                .bg_color(lv.color_hex(0x332C00))
                .bg_opa(lv.OPA._50)
                .border_width(1)
                .border_color(lv.color_hex(0xC1A400))
                .pad_ver(16)
                .pad_hor(24)
                .radius(40)
                .text_color(lv.color_hex(0xE0BC00))
                .text_font(font_GeistRegular26)
                .text_align_left(),
                0,
            )
            self.icon = lv.img(self)
            self.icon.set_align(lv.ALIGN.LEFT_MID)
            self.icon.set_src("A:/res/alert-warning-yellow-solid.png")
            self.icon.set_antialias(True)  # Enable anti-aliasing for smooth edges
            self.warnings = lv.label(self)
            self.warnings.align_to(self.icon, lv.ALIGN.OUT_RIGHT_MID, 8, 0)

        def show(self, text=None):
            self.clear_flag(lv.obj.FLAG.HIDDEN)
            if text:
                self.warnings.set_text(text)

        def hidden(self):
            self.add_flag(lv.obj.FLAG.HIDDEN)

    class AppDrawer(lv.obj):
        PAGE_SIZE = 2
        PAGE_SLIDE_TIME = 300  # Animation time with ease_out for smooth feel

        def __init__(self, parent):
            super().__init__(parent)
            self.parent = parent
            self.visible = False
            self.text_label = {}
            self._icon_sources = set()
            self._page_anim_refs = []
            self._page_anim_handles = []
            self.communication_locked = False

            # Remove style and lazy loading related code to fix system freeze

            self.init_ui()
            self.init_items()  # Restore original immediate creation of all items
            self._configure_image_cache()
            self.init_indicators()
            self.init_anim()

        def _is_on_hold(self) -> bool:
            return bool(getattr(self.parent, "_communication_hold", False))

        # Removed styles property to fix system freeze

        def _ensure_background_style(self):
            if getattr(self, "_background_style", None) is None:
                self._background_style = StyleWrapper().border_width(0)
                self.add_style(self._background_style, 0)
            return self._background_style

        def _set_background_image(self, image_src: str | None):
            if image_src:
                style = self._ensure_background_style()
                style.bg_img_src(image_src)  # type: ignore[is not a known member of "None"]
                self.invalidate()
            elif getattr(self, "_background_style", None):
                self.remove_style(self._background_style, 0)
                self._background_style = None

        def init_ui(self):
            self.remove_style_all()
            self._background_style = None
            self.set_pos(0, 0)
            self.set_size(lv.pct(100), lv.pct(100))
            self.add_style(
                StyleWrapper()
                .bg_opa(lv.OPA.COVER)
                .bg_color(lv_colors.BLACK)
                .border_width(0),
                0,
            )
            homescreen = _normalize_wallpaper_src(
                storage_device.get_appdrawer_background(), allow_default=False
            )
            self._set_background_image(homescreen or None)
            # If homescreen is empty, keep the existing black background

            self.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

            self.add_flag(lv.obj.FLAG.GESTURE_BUBBLE)
            self.add_event_cb(self.on_gesture, lv.EVENT.GESTURE, None)

            self.main_cont = lv.obj(self)
            self.main_cont.set_size(480, 750)
            # Align content container to screen-left to avoid early clip on slide-out
            self.main_cont.set_pos(0, 75)
            self.main_cont.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            self.main_cont.add_flag(
                lv.obj.FLAG.GESTURE_BUBBLE
            )  # Allow gesture events to bubble up
            self.main_cont.set_style_pad_all(0, 0)
            self.main_cont.set_style_border_width(0, 0)
            self.main_cont.set_style_bg_opa(lv.OPA.TRANSP, 0)
            # Ensure neither AppDrawer nor its content container scrolls to prevent bounce
            self.clear_flag(lv.obj.FLAG.SCROLLABLE)
            self.main_cont.clear_flag(lv.obj.FLAG.SCROLLABLE)

            self.current_page = 0
            # Page containers allow us to slide whole pages instead of item-by-item toggling.
            self.page_conts = []
            self.page_wraps = []
            self.page_items = [[] for _ in range(self.PAGE_SIZE)]
            self._page_wrap_origin_x = 64
            self._page_wrap_origin_y = 89
            self._page_wrap_width = 352
            self._page_wrap_height = 492
            self._page_wrap_slide = self._page_wrap_origin_x + self._page_wrap_width
            self.page_width = self.main_cont.get_width()
            if not self.page_width:
                self.page_width = 480
            self.page_height = self.main_cont.get_height()
            if not self.page_height:
                self.page_height = 750
            for idx in range(self.PAGE_SIZE):
                page_cont = lv.obj(self.main_cont)
                page_cont.remove_style_all()
                page_cont.set_size(self.page_width, self.page_height)
                page_cont.set_pos(0, 0)
                page_cont.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
                page_cont.add_flag(
                    lv.obj.FLAG.GESTURE_BUBBLE
                )  # Allow gesture events to bubble up
                page_cont.clear_flag(lv.obj.FLAG.SCROLLABLE)
                # Use inline styles
                page_cont.add_style(
                    StyleWrapper().bg_opa(lv.OPA.TRANSP).border_width(0).pad_all(0), 0
                )
                wrap = lv.obj(page_cont)
                wrap.remove_style_all()
                wrap.set_pos(self._page_wrap_origin_x, self._page_wrap_origin_y)
                wrap.set_size(self._page_wrap_width, self._page_wrap_height)
                wrap.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
                wrap.add_flag(lv.obj.FLAG.GESTURE_BUBBLE)
                wrap.clear_flag(lv.obj.FLAG.SCROLLABLE)
                wrap.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
                wrap.set_scroll_dir(lv.DIR.NONE)
                wrap.add_style(
                    StyleWrapper()
                    .bg_opa(lv.OPA.TRANSP)
                    .border_width(0)
                    .radius(0)
                    .clip_corner(True),
                    0,
                )
                self.page_wraps.append(wrap)
                if idx != 0:
                    page_cont.add_flag(lv.obj.FLAG.HIDDEN)
                self.page_conts.append(page_cont)
            self.page_animating = False
            self.show_page(0)

        def init_items(self):
            if utils.BITCOIN_ONLY:
                items = [
                    ("connect", "app-connect", i18n_keys.APP__CONNECT_WALLET),
                    ("scan", "app-scan", i18n_keys.APP__SCAN),
                    ("my_address", "app-address", i18n_keys.APP__ADDRESS),
                    ("settings", "app-settings", i18n_keys.APP__SETTINGS),
                    ("backup", "app-backup", i18n_keys.APP__BACK_UP),
                    ("nft", "app-nft", i18n_keys.APP__NFT_GALLERY),
                    ("guide", "app-tips", i18n_keys.APP__TIPS),
                ]
            else:
                items = [
                    ("connect", "app-connect", i18n_keys.APP__CONNECT_WALLET),
                    ("scan", "app-scan", i18n_keys.APP__SCAN),
                    ("my_address", "app-address", i18n_keys.APP__ADDRESS),
                    ("settings", "app-settings", i18n_keys.APP__SETTINGS),
                    ("passkey", "app-keys", i18n_keys.FIDO_FIDO_KEYS_LABEL),
                    ("backup", "app-backup", i18n_keys.APP__BACK_UP),
                    ("nft", "app-nft", i18n_keys.APP__NFT_GALLERY),
                    ("guide", "app-tips", i18n_keys.APP__TIPS),
                ]

            items_per_page = 4
            cols = 2
            item_width = 144
            item_height = 214
            col_gap = 64
            row_gap = 64

            grid_offset_x = 64

            for idx, (name, img, text) in enumerate(items):
                page = idx // items_per_page
                page_idx = idx % items_per_page
                # Fix grid calculation: For a 2×3 grid, divide by cols to get row
                row = page_idx // cols  # Fixed: was page_idx // rows
                col = page_idx % cols
                x = grid_offset_x + col * (item_width + col_gap)
                y = (
                    row * (item_height + row_gap) + 89
                )  # Main container already has 75px offset, no extra offset needed

                item = self.create_item(
                    self.page_wraps[page],
                    name,
                    img,
                    text,
                    x - self._page_wrap_origin_x,
                    y - self._page_wrap_origin_y,
                )
                self.page_items[page].append(item)

        def create_item(self, parent, name, img_src, text_key, x, y):
            cont = lv.obj(parent)
            # Use inline styles - fix system freeze issue
            cont.add_style(
                StyleWrapper()
                .bg_color(lv_colors.BLACK)
                .bg_opa(lv.OPA.TRANSP)
                .radius(0)
                .border_width(0)
                .pad_all(0),
                0,
            )
            cont.set_size(144, 214)  # Updated to match main branch
            cont.set_pos(x, y)
            cont.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            cont.add_flag(
                lv.obj.FLAG.GESTURE_BUBBLE
            )  # Allow gesture events to bubble up
            # Disable container scrolling to reduce unnecessary calculations
            cont.clear_flag(lv.obj.FLAG.SCROLLABLE)

            btn = lv.imgbtn(cont)
            btn.set_size(144, 144)  # Updated to match main branch
            icon_path = f"A:/res/{img_src}.png"
            btn.set_style_bg_img_src(icon_path, 0)
            self._icon_sources.add(icon_path)
            # Use inline button styles - remove border radius settings
            btn.add_style(StyleWrapper().bg_opa(lv.OPA.TRANSP).shadow_width(0), 0)
            # Add press effect: darken the icon by 30% opacity black overlay
            btn.add_style(
                StyleWrapper()
                .bg_img_recolor_opa(lv.OPA._30)
                .bg_img_recolor(lv_colors.BLACK),
                lv.PART.MAIN | lv.STATE.PRESSED,
            )
            btn.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            # Use absolute positioning instead of align to avoid layout calculation issues
            btn.set_pos(0, 0)  # Center horizontally: (144 - 144) / 2 = 0
            btn.set_style_border_width(0, 0)
            btn.clear_flag(lv.obj.FLAG.SCROLLABLE)
            btn.clear_flag(lv.obj.FLAG.GESTURE_BUBBLE)

            label = lv.label(cont)
            label.set_text(_(text_key))
            label.add_style(
                StyleWrapper()
                .width(144)
                .text_font(font_GeistSemiBold26)
                .text_color(lv_colors.WHITE)
                .text_align_center(),
                0,
            )
            label.add_style(
                StyleWrapper().text_opa(lv.OPA._70), lv.PART.MAIN | lv.STATE.PRESSED
            )
            label.set_style_text_letter_space(-1, 0)
            label.set_long_mode(lv.label.LONG.WRAP)
            label.set_style_max_height(52, 0)

            label.set_pos(0, 144 + 8)

            self.text_label[text_key] = label

            btn.add_event_cb(
                lambda e: self.on_pressed(text_key), lv.EVENT.PRESSED, None
            )
            btn.add_event_cb(
                lambda e: self.on_released(text_key), lv.EVENT.RELEASED, None
            )
            btn.add_event_cb(lambda e: self.on_item_click(name), lv.EVENT.CLICKED, None)
            return cont

        def _configure_image_cache(self):
            """Configure LVGL image cache."""
            icon_count = len(self._icon_sources)
            if not icon_count:
                return

            cache_set_size = getattr(getattr(lv, "img", None), "cache_set_size", None)
            if cache_set_size:
                cache_set_size(max(icon_count + 2, 20))

        def init_indicators(self):
            self.container = ContainerFlexRow(self, None, padding_col=0)
            self.container.align(lv.ALIGN.BOTTOM_MID, 0, -32)
            self.indicators = [
                Indicator(self.container, i) for i in range(self.PAGE_SIZE)
            ]

        def set_position(self, val):
            pass

        def init_anim(self):
            self.show_anim = Anim(
                130,
                75,
                self.set_position,
                start_cb=self.show_anim_start_cb,
                delay=APP_DRAWER_UP_DELAY,
                time=APP_DRAWER_UP_TIME,
                path_cb=APP_DRAWER_UP_PATH_CB,
            )
            self.dismiss_anim = Anim(
                75,
                130,
                self.set_position,
                path_cb=APP_DRAWER_DOWN_PATH_CB,
                time=APP_DRAWER_DOWN_TIME,
                start_cb=self.dismiss_anim_start_cb,
                del_cb=self.dismiss_anim_del_cb,
                delay=APP_DRAWER_DOWN_DELAY,
            )

        def is_animating(self) -> bool:
            return bool(getattr(self, "page_animating", False))

        async def wait_for_animation(self, timeout_ms: int = 500):
            if not self.is_animating():
                return
            deadline = utime.ticks_add(utime.ticks_ms(), timeout_ms)
            while self.is_animating():
                if utime.ticks_diff(deadline, utime.ticks_ms()) <= 0:
                    self.finish_page_animation(force=True)
                    break
                try:
                    await loop.sleep(10)
                except Exception:
                    break

        def lock_for_communication(self):
            if self.communication_locked:
                return
            self.communication_locked = True
            if self.is_animating():
                self.finish_page_animation(force=True)

        def unlock_after_communication(self):
            self.communication_locked = False

        def on_gesture(self, event_obj):
            if self._is_on_hold():
                return
            code = event_obj.code
            is_hidden = self.has_flag(lv.obj.FLAG.HIDDEN)

            if Layer2Manager.is_animating():
                return

            if code == lv.EVENT.GESTURE:
                if is_hidden:
                    return

                indev = lv.indev_get_act()
                _dir = indev.get_gesture_dir()

                if _dir == lv.DIR.BOTTOM:
                    self.hide_to_mainscreen()
                elif _dir == lv.DIR.TOP:
                    return
                else:
                    self.handle_page_gesture(_dir)

        def hide_to_mainscreen(self):
            if Layer2Manager.is_animating():
                return

            from trezorui import Display

            display = Display()
            animate_cb = getattr(display, "cover_background_animate_to_y", None)
            move_cb = getattr(display, "cover_background_move_to_y", None)
            can_animate = bool(
                animate_cb and move_cb and Layer2Manager.ensure_background(display)
            )
            if not can_animate:
                self.hide_to_mainscreen_fallback()
                return

            Layer2Manager.set_animating(True)

            # Keep AppDrawer visible while layer2 slides down over it.
            move_cb(-800)  # type: ignore[cannot be called]
            if hasattr(display, "cover_background_set_visible"):
                display.cover_background_set_visible(True)

            def on_layer2_covers_screen():
                self.add_flag(lv.obj.FLAG.HIDDEN)
                self.visible = False
                self.add_flag(lv.obj.FLAG.GESTURE_BUBBLE)

                if hasattr(self.parent, "prepare_title_fade_in"):
                    self.parent.prepare_title_fade_in()
                self.parent.restore_main_content()

                lv.refr_now(None)

                if hasattr(display, "cover_background_hide"):
                    display.cover_background_hide()
                if hasattr(self.parent, "start_title_fade_in"):
                    self.parent.start_title_fade_in(duration=100)

                Layer2Manager.set_animating(False)
                Layer2Manager.cleanup_timers()

            try:
                Layer2Manager.with_lvgl_timer_pause(animate_cb, 0, 200)
            except Exception:
                self.hide_to_mainscreen_fallback()
                return

            Layer2Manager.schedule_once(200, on_layer2_covers_screen)

        def hide_to_mainscreen_fallback(self):
            self.add_flag(lv.obj.FLAG.HIDDEN)
            self.add_flag(lv.obj.FLAG.GESTURE_BUBBLE)
            self.visible = False

            try:
                from trezorui import Display

                display = Display()
            except Exception:
                display = None

            if display:
                Layer2Manager.set_visibility(display, False)

            # Ensure animation flag is properly reset
            Layer2Manager.set_animating(False)
            Layer2Manager.cleanup_timers()

            if hasattr(self.parent, "restore_main_content"):
                self.parent.restore_main_content()

        def handle_page_gesture(self, _dir):
            if _dir not in [lv.DIR.RIGHT, lv.DIR.LEFT]:
                return
            if self._is_on_hold():
                return
            if self.page_animating:
                return
            if self.communication_locked:
                return

            if not hasattr(self, "indicators") or not self.indicators:
                return

            if _dir == lv.DIR.LEFT:
                target_page = (self.current_page + 1) % self.PAGE_SIZE
            else:
                target_page = (self.current_page - 1 + self.PAGE_SIZE) % self.PAGE_SIZE

            if target_page == self.current_page:
                return

            self.indicators[self.current_page].set_active(False)
            self.indicators[target_page].set_active(True)
            self._page_anim_target = target_page
            self.animate_page_transition(target_page, _dir)

        def show_page(self, index: int):
            if index < 0 or index >= self.PAGE_SIZE:
                return

            for idx, page_cont in enumerate(self.page_conts):
                if idx == index:
                    wrap = self.page_wraps[idx]
                    page_cont.clear_flag(lv.obj.FLAG.HIDDEN)
                    page_cont.set_x(0)
                    wrap.set_pos(self._page_wrap_origin_x, self._page_wrap_origin_y)
                else:
                    wrap = self.page_wraps[idx]
                    page_cont.set_x(0)
                    page_cont.add_flag(lv.obj.FLAG.HIDDEN)
                    wrap.set_pos(self._page_wrap_origin_x, self._page_wrap_origin_y)

            if hasattr(self, "indicators") and self.indicators:
                for idx, indicator in enumerate(self.indicators):
                    indicator.set_active(idx == index)

            self.current_page = index

        def hidden_page(self, index: int):
            if index < 0 or index >= self.PAGE_SIZE:
                return
            page_cont = self.page_conts[index]
            wrap = self.page_wraps[index]
            page_cont.set_x(0)
            page_cont.add_flag(lv.obj.FLAG.HIDDEN)
            wrap.set_pos(self._page_wrap_origin_x, self._page_wrap_origin_y)

        def animate_page_transition(self, target_index: int, direction: int):
            if target_index < 0 or target_index >= self.PAGE_SIZE:
                return

            if self.communication_locked:
                return

            old_index = self.current_page
            old_cont = self.page_conts[old_index]
            new_cont = self.page_conts[target_index]
            old_wrap = self.page_wraps[old_index]
            new_wrap = self.page_wraps[target_index]

            if not old_cont or not new_cont:
                return

            # Clean up memory before starting animation to prevent GC during animation
            gc.collect()

            self.page_animating = True
            self._page_anim_target = target_index

            # Cancel any running animations on these containers to prevent jitter/bounce
            lv.anim_del(old_wrap, None)
            lv.anim_del(new_wrap, None)

            # Ensure the incoming page starts off-screen in the intended direction.
            slide_distance = self._page_wrap_slide
            offset = slide_distance if direction == lv.DIR.LEFT else -slide_distance
            # Set position BEFORE making it visible to avoid a 1-frame flash at x=0
            new_wrap.set_x(self._page_wrap_origin_x + offset)
            new_wrap.set_y(self._page_wrap_origin_y)
            # Optimization: set position and properties first, then display
            new_cont.clear_flag(lv.obj.FLAG.HIDDEN)
            new_wrap.add_flag(lv.obj.FLAG.IGNORE_LAYOUT)
            old_wrap.add_flag(lv.obj.FLAG.IGNORE_LAYOUT)
            new_wrap.clear_flag(lv.obj.FLAG.HIDDEN)
            # Disable layout calculations to improve performance

            anim_time = self.PAGE_SLIDE_TIME

            def animate_x(target_obj, start_x, end_x, easing_cb):
                anim = lv.anim_t()
                anim.init()
                anim.set_var(target_obj)
                anim.set_values(start_x, end_x)
                anim.set_time(anim_time)
                anim.set_path_cb(easing_cb)

                def exec_cb(_anim, val, *, target=target_obj):
                    target.set_x(int(val))

                anim.set_custom_exec_cb(exec_cb)
                anim.set_repeat_count(1)
                return anim

            # Use ease_out for smooth feel
            easing_cb = lv.anim_t.path_ease_out
            anim_out = animate_x(
                old_wrap,
                old_wrap.get_x(),
                self._page_wrap_origin_x - offset,
                easing_cb,
            )
            anim_in = animate_x(
                new_wrap,
                self._page_wrap_origin_x + offset,
                self._page_wrap_origin_x,
                easing_cb,
            )
            anim_in.set_ready_cb(
                lambda _a, self=self, old_index=old_index, target_index=target_index: self._on_page_anim_ready(
                    old_index, target_index
                )
            )

            self._page_anim_refs = [anim_out, anim_in]
            self._page_anim_handles = [
                lv.anim_t.start(anim_out),
                lv.anim_t.start(anim_in),
            ]

        def finish_page_animation(self, force: bool = False):
            if not getattr(self, "page_animating", False):
                return

            target_index = getattr(self, "_page_anim_target", None)
            if (
                target_index is None
                or target_index < 0
                or target_index >= self.PAGE_SIZE
            ):
                # No valid target, just clean up the animation bookkeeping.
                self.page_animating = False
                self._page_anim_refs = []
                self._page_anim_handles = []
                return

            old_index = self.current_page

            old_wrap = self.page_wraps[old_index]
            lv.anim_del(old_wrap, None)

            new_wrap = self.page_wraps[target_index]
            lv.anim_del(new_wrap, None)
            self.page_conts[target_index].clear_flag(lv.obj.FLAG.HIDDEN)

            self._on_page_anim_ready(old_index, target_index, force=force)

        def _on_page_anim_ready(
            self, old_index: int, target_index: int, *, force: bool = False
        ):
            # Reset both pages to their resting positions and visibility.
            old_cont = self.page_conts[old_index]
            new_cont = self.page_conts[target_index]
            old_wrap = self.page_wraps[old_index]
            new_wrap = self.page_wraps[target_index]

            old_cont.set_x(0)
            new_cont.set_x(0)
            old_wrap.set_pos(self._page_wrap_origin_x, self._page_wrap_origin_y)
            new_wrap.set_pos(self._page_wrap_origin_x, self._page_wrap_origin_y)

            # Restore layout flags
            old_wrap.clear_flag(lv.obj.FLAG.IGNORE_LAYOUT)
            new_wrap.clear_flag(lv.obj.FLAG.IGNORE_LAYOUT)

            self.show_page(target_index)
            self.page_animating = False
            self._page_anim_refs = []
            self._page_anim_handles = []

            # Clean up immediately if forced, otherwise let normal GC handle it
            if force:
                gc.collect()

        def force_cleanup(self):
            self.finish_page_animation(force=True)

            self.communication_locked = False

            try:
                is_hidden = self.has_flag(lv.obj.FLAG.HIDDEN)
            except Exception:
                is_hidden = False

            if not is_hidden:
                try:
                    self.hide_to_mainscreen_fallback()
                except Exception:
                    self.add_flag(lv.obj.FLAG.HIDDEN)
                    self.visible = False

            try:
                from trezorui import Display

                display = Display()
            except Exception:
                display = None

            if display:
                Layer2Manager.set_visibility(display, False)

            Layer2Manager.set_animating(False)
            Layer2Manager.cleanup_timers()

            lv.refr_now(None)

            gc.collect()

        def show_anim_start_cb(self, _anim):
            self.parent.hidden_others()
            self.hidden_page(self.current_page)
            self.parent.clear_state(lv.STATE.USER_1)

        def show_anim_del_cb(self, _anim):
            self.show_page(self.current_page)
            self.visible = True

        def dismiss_anim_start_cb(self, _anim):
            self.hidden_page(self.current_page)

        def dismiss_anim_del_cb(self, _anim):
            self.parent.hidden_others(False)
            self.add_flag(lv.obj.FLAG.HIDDEN)
            self.visible = False
            self.add_flag(lv.obj.FLAG.GESTURE_BUBBLE)

        def show(self):
            return

        def dismiss(self):
            return

        def refresh_background(self):
            homescreen = _normalize_wallpaper_src(
                storage_device.get_appdrawer_background(), allow_default=False
            )
            self._set_background_image(homescreen or None)

        def on_pressed(self, text_key):
            label = self.text_label[text_key]
            label.add_state(lv.STATE.PRESSED)

        def on_released(self, text_key):
            label = self.text_label[text_key]
            label.clear_state(lv.STATE.PRESSED)

        def on_item_click(self, name):
            handlers = {
                "settings": lambda: SettingsScreen(self.parent),
                "guide": lambda: UserGuide(self.parent),
                "nft": lambda: NftGallery(self.parent),
                "backup": lambda: BackupWallet(self.parent),
                "scan": lambda: ScanScreen(self.parent),
                "connect": lambda: ConnectWalletWays(self.parent),
                "my_address": lambda: ShowAddress(self.parent),
                "passkey": lambda: PasskeysManager(self.parent),
            }
            if name in handlers:
                # Mark that we should reopen the drawer when navigating back
                self.parent.__class__._reopen_drawer_on_next_show = True
                # Ensure the drawer closes and the main screen restores visibility
                self.hide_to_mainscreen_fallback()
                handlers[name]()

        def on_click(self, event_obj):
            code = event_obj.code
            if code == lv.EVENT.CLICKED:
                if utils.lcd_resume():
                    return
                # Simplified: remove slide check since we've simplified state management

        def refresh_text(self):
            for text_key, label in self.text_label.items():
                label.set_text(_(text_key))


class PasskeysManager(AnimScreen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            if not self.is_visible():
                if hasattr(self, "banner") and self.banner:
                    self.banner.delete()
                    del self.banner
                if hasattr(self, "learn_more") and self.learn_more:
                    self.learn_more.delete()
                    del self.learn_more
                if hasattr(self, "empty_tips") and self.empty_tips:
                    self.empty_tips.delete()
                    del self.empty_tips
                if hasattr(self, "container") and self.container:
                    self.container.delete()
                    del self.container
                self.fresh_show()
                lv.scr_load(self)
            return
        super().__init__(
            prev_scr=prev_scr,
            title=_(i18n_keys.FIDO_FIDO_KEYS_LABEL),
            nav_back=True,
            rti_path="A:/res/go2settings.png",
        )

        self.fresh_show()
        self.add_event_cb(self.on_click_event, lv.EVENT.CLICKED, None)

    async def list_credential(self):
        from .app_passkeys import PasskeysListItemBtn

        BATCH_SIZE = 5
        # pyright: off
        stored_credentials = [None] * self.count
        for i, credential in enumerate(self.credentials):
            stored_credentials[i] = (
                credential.app_name(),
                credential.account_name(),
                credential.index,
                credential.creation_time,
            )
            self.overlay.set_value(i + 1)
            if (i < BATCH_SIZE) or ((i + 1) % BATCH_SIZE == 0):
                gc.collect()
                await loop.sleep(10)
        stored_credentials.sort(key=lambda x: x[3])
        for i, credential in enumerate(stored_credentials):
            self.listed_credentials[i] = PasskeysListItemBtn(
                self.container,
                credential[0],
                credential[1] or "",
                credential[2],
            )
            if (i < BATCH_SIZE) or ((i + 1) % BATCH_SIZE == 0):
                gc.collect()
        # pyright: on
        self.container.refresh_self_size()
        del stored_credentials
        self.overlay.del_delayed(10)

    def fresh_show(self):
        from .app_passkeys import (
            get_registered_credentials,
            get_registered_credentials_count,
        )

        if hasattr(self, "container"):
            self.container.refresh_self_size()
            self.count = len(self.listed_credentials)
        else:
            self.count = get_registered_credentials_count()
            self.credentials = get_registered_credentials()
            self.listed_credentials = [None] * self.count

        fido_enabled = storage_device.is_fido_enabled()
        if not hasattr(self, "banner") and not fido_enabled:
            self.banner = Banner(
                self.content_area,
                LEVEL.HIGHLIGHT,
                _(i18n_keys.FIDO_DISABLED_INFO_TEXT),
            )
            self.banner.align(lv.ALIGN.TOP_MID, 0, 116)
        if self.count == 0:
            self.empty_tips = lv.label(self.content_area)
            self.empty_tips.set_text(_(i18n_keys.FIDO_LIST_EMPTY_TEXT))
            self.empty_tips.add_style(
                StyleWrapper()
                .text_font(font_GeistRegular30)
                .text_color(lv_colors.WHITE_2)
                .text_letter_space(-1),
                0,
            )
            self.empty_tips.align(lv.ALIGN.TOP_MID, 0, 432)
            if fido_enabled:
                self.learn_more = NormalButton(
                    self, text=_(i18n_keys.ACTION__LEARN_MORE)
                )
        else:
            if not hasattr(self, "container"):
                algin_base = self.title if fido_enabled else self.banner
                self.container = ContainerFlexCol(
                    self.content_area, algin_base, padding_row=2
                )
                workflow.spawn(self.list_credential())

                from .components.overlay import OverlayWithProcessBar

                self.overlay = OverlayWithProcessBar(self, self.count)

    def auto_adjust_scroll(self, item_height):
        scroll_value = self.content_area.get_scroll_y()
        if scroll_value > 0:
            auto_adjust = scroll_value - item_height
            self.content_area.scroll_to(
                0, auto_adjust if auto_adjust > 0 else 0, lv.ANIM.OFF
            )

    async def on_remove(self, i):
        # pyright: off
        credential = self.listed_credentials.pop(i)
        from .app_passkeys import delete_credential

        delete_credential(credential.credential_index)
        item_height = credential.get_height()
        credential.delete()
        # pyright: on
        self.fresh_show()
        self.auto_adjust_scroll(item_height)

    def on_click_event(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            from trezor.lvglui.scrs import app_passkeys

            if hasattr(self, "learn_more") and target == self.learn_more:
                from .app_passkeys import PasskeysRegister

                PasskeysRegister()
            elif target in self.listed_credentials:
                for i, credential in enumerate(self.listed_credentials):
                    if target == credential:
                        # pyright: off
                        workflow.spawn(
                            app_passkeys.request_credential_details(
                                credential.app_name,
                                credential.account_name,
                                on_remove=lambda index=i: self.on_remove(index),
                            )
                        )
                        # pyright: on
            elif hasattr(self, "rti_btn") and target == self.rti_btn:
                FidoKeysSetting(self)

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)


class ShowAddress(AnimScreen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self.prev_session_id = storage.cache.get_session_id()
            if not self.prev_session_id:
                self.curr_session_id = storage.cache.start_session()
                self.prev_session_id = self.curr_session_id
            else:
                self.curr_session_id = storage.cache.start_session()
            self._init = True
            self.current_index = 0
            kwargs = {
                "prev_scr": prev_scr,
                "title": _(i18n_keys.TITLE__SELECT_NETWORK),
                "nav_back": True,
            }
            super().__init__(**kwargs)

            self.addr_manager = AddressManager()

            self.init_ui()

        else:
            if not self.is_visible():
                self._load_scr(self)
            gc.collect()

    async def _get_passphrase_from_user(self, init=False, prev_scr=None):
        try:
            from apps.bitcoin.get_address import get_address as btc_get_address
            from trezor import messages
            from trezor.enums import InputScriptType

            msg = messages.GetAddress(
                address_n=[0x80000000 + 44, 0x80000000 + 0, 0x80000000 + 0, 0, 0],
                show_display=False,
                script_type=InputScriptType.SPENDADDRESS,
            )
            # pyright: off
            await btc_get_address(wire.QRContext(), msg)
            # pyright: on

        except Exception:
            pass

        if init:
            self._init = True
            self.current_index = 0
            kwargs = {
                "prev_scr": prev_scr,
                "title": _(i18n_keys.TITLE__SELECT_NETWORK),
                "nav_back": True,
            }
            super().__init__(**kwargs)

            self.addr_manager = AddressManager()

            self.init_ui()

        self.invalidate()

    def animate_list_items(self):
        def create_move_cb_container(obj, item_index):
            def cb(value):
                obj.set_style_translate_x(value, 0)
                obj.invalidate()

            return cb

        container_move_anim = Anim(
            50,
            0,
            create_move_cb_container(self.container, 0),
            time=150,
            delay=0,
            path_cb=lv.anim_t.path_ease_out,
        )

        container_move_back_anim = Anim(
            -50,
            0,
            create_move_cb_container(self.container, 0),
            time=150,
            delay=0,
            path_cb=lv.anim_t.path_ease_out,
        )

        self.animations_next.append(container_move_anim)
        self.animations_prev.append(container_move_back_anim)

    def init_ui(self):

        if passphrase.is_enabled() and not passphrase.is_passphrase_pin_enabled():
            from .components.navigation import Navigation

            self.nav_passphrase = Navigation(
                self.content_area,
                btn_bg_img="A:/res/repeat.png",
                nav_btn_align=lv.ALIGN.RIGHT_MID,
                align=lv.ALIGN.TOP_RIGHT,
            )

        # Account button
        self.index_btn = ListItemBtn(
            self.content_area,
            f" Account #{self.current_index + 1}",
            left_img_src="A:/res/wallet.png",
            has_next=False,
            min_height=87,
            pad_ver=5,
        )
        self.index_btn.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, 40)
        self.index_btn.set_style_radius(40, 0)
        self.index_btn.add_event_cb(self.on_index_click, lv.EVENT.CLICKED, None)

        self.container = ContainerFlexCol(
            self.content_area, self.title, padding_row=2, pos=(0, 157)
        )
        self.container.align_to(self.index_btn, lv.ALIGN.OUT_BOTTOM_MID, 0, 12)
        self.container.set_style_bg_color(lv_colors.BLACK, 0)
        self.container.set_style_bg_opa(255, 0)

        # Initialize variables
        self.chains = chains_brief_info()
        self.visible_chains_count = 8

        self.is_expanded = False
        self.chain_buttons = []
        self.created_count = 0
        self.current_page = 0
        self.items_per_page = 5
        self.max_pages = 0

        self.chain_buttons = []
        for _i in range(self.items_per_page):
            btn = ListItemBtn(
                self.container,
                "",
                left_img_src="A:/res/btc-btc-48.png",
                min_height=87,
                pad_ver=5,
            )
            self.chain_buttons.append(btn)
            btn.add_flag(lv.obj.FLAG.HIDDEN)

        self._create_visible_chain_buttons()
        self.max_pages = (len(self.chains) - 1) // self.items_per_page

        if self.max_pages > 0:
            self.next_btn = NormalButton(self, "")
            self.next_btn.set_size(224, 98)
            self.next_btn.align(lv.ALIGN.BOTTOM_RIGHT, -12, -8)
            self.next_btn.set_style_bg_img_src("A:/res/arrow-right-2.png", 0)

            self.back_btn = NormalButton(self, "")
            self.back_btn.set_size(224, 98)
            self.back_btn.align(lv.ALIGN.BOTTOM_LEFT, 12, -8)
            self.back_btn.set_style_bg_img_src("A:/res/arrow-left-2.png", 0)

        self.disable_style = (
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_BLACK_5)
            .bg_img_recolor(lv_colors.ONEKEY_GRAY_1)
            .bg_img_recolor_opa(lv.OPA.COVER)
        )
        self.enable_style = (
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_BLACK_3)
            .bg_opa(lv.OPA.COVER)
            .radius(98)
        )

        self._create_visible_chain_buttons()

        if self.max_pages > 0:
            self.update_page_buttons()
            self.next_btn.add_style(self.enable_style, 0)

        self.animations_next = []
        self.animations_prev = []
        self.list_items = self.chain_buttons

        if storage_device.is_animation_enabled():
            self.animate_list_items()

    def _create_visible_chain_buttons(self):
        start_idx = self.current_page * self.items_per_page
        end_idx = min(start_idx + self.items_per_page, len(self.chains))

        for i, btn in enumerate(self.chain_buttons):
            btn.remove_event_cb(None)
            if i < (end_idx - start_idx):
                chain = self.chains[start_idx + i]
                chain_name, chain_icon = chain
                btn.label_left.set_text(chain_name)
                btn.img_left.set_src(chain_icon)
                btn.add_event_cb(
                    lambda e, name=chain_name: self.on_chain_click(e, name),
                    lv.EVENT.CLICKED,
                    None,
                )
                btn.clear_flag(lv.obj.FLAG.HIDDEN)

                if chain_name == "Ethereum" and not hasattr(btn, "img_right"):
                    btn.img_right = lv.img(btn)
                    btn.img_right.set_src("A:/res/stacked-chains.png")
                    btn.img_right.set_align(lv.ALIGN.RIGHT_MID)
                elif chain_name == "Ethereum" and hasattr(btn, "img_right"):
                    btn.img_right.set_style_img_opa(255, 0)
                elif i == 1 and hasattr(btn, "img_right"):
                    btn.img_right.set_style_img_opa(0, 0)
            else:
                btn.add_flag(lv.obj.FLAG.HIDDEN)

    def enable_page_buttons(self, btn):
        btn.add_flag(lv.btn.FLAG.CLICKABLE)
        btn.remove_style(self.disable_style, 0)
        btn.add_style(self.enable_style, 0)

    def disable_page_buttons(self, btn):
        btn.clear_flag(lv.btn.FLAG.CLICKABLE)
        btn.remove_style(self.enable_style, 0)
        btn.add_style(self.disable_style, 0)

    def update_page_buttons(self):
        if self.current_page == 0:
            self.disable_page_buttons(self.back_btn)
            if not self.next_btn.has_flag(lv.btn.FLAG.CLICKABLE):
                self.enable_page_buttons(self.next_btn)

        elif self.current_page == self.max_pages:
            self.disable_page_buttons(self.next_btn)
            if not self.back_btn.has_flag(lv.btn.FLAG.CLICKABLE):
                self.enable_page_buttons(self.back_btn)

        else:
            if not self.next_btn.has_flag(lv.btn.FLAG.CLICKABLE):
                self.enable_page_buttons(self.next_btn)
            if not self.back_btn.has_flag(lv.btn.FLAG.CLICKABLE):
                self.enable_page_buttons(self.back_btn)

    def next_page(self):
        if self.current_page < self.max_pages:
            self.current_page += 1
            self._create_visible_chain_buttons()
            for anim in self.animations_next:
                anim.start()
            self.update_page_buttons()

    def prev_page(self):
        if self.current_page > 0:
            self.current_page -= 1
            self._create_visible_chain_buttons()
            for anim in self.animations_prev:
                anim.start()
            self.update_page_buttons()

    def on_index_click(self, event):
        IndexSelectionScreen(self)

    def on_chain_click(self, event, name):
        if utils.lcd_resume():
            return

        workflow.spawn(self.addr_manager.generate_address(name, self.current_index))

    def update_index_btn_text(self):
        self.index_btn.label_left.set_text(f"Account #{self.current_index + 1}")

    def eventhandler(self, event_obj):
        event = event_obj.code
        target = event_obj.get_target()
        if event == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if isinstance(target, lv.imgbtn):
                if target == self.nav_back.nav_btn:
                    storage.cache.end_current_session()
                    storage.cache.start_session(self.prev_session_id)
                    if self.prev_scr is not None:
                        self.load_screen(self.prev_scr, destroy_self=True)

                elif passphrase.is_enabled() and target == self.nav_passphrase.nav_btn:
                    storage.cache.end_current_session()
                    self.curr_session_id = storage.cache.start_session()
                    workflow.spawn(self._get_passphrase_from_user(init=False))

            else:
                gc.collect()
                if hasattr(self, "back_btn") and target == self.back_btn:
                    self.prev_page()
                elif hasattr(self, "next_btn") and target == self.next_btn:
                    self.next_page()

    async def _handle_passphrase_change(self, coro):
        await coro
        self.init_ui()


class IndexSelectionScreen(AnimScreen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        super().__init__(
            prev_scr, title=_(i18n_keys.TITLE__SELECT_ACCOUNT), nav_back=True
        )

        from .components.navigation import Navigation

        # # navi
        self.nav_opt = Navigation(
            self.content_area,
            nav_btn_align=lv.ALIGN.RIGHT_MID,
            btn_bg_img="A:/res/general.png",
            align=lv.ALIGN.TOP_RIGHT,
        )

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)

        self.max_account = 1000000000
        self.max_page = (self.max_account - 1) // 5

        self.current_account = self.prev_scr.current_index + 1
        self.current_page = (self.current_account - 1) // 5

        # account select btn
        self.account_btns = []
        for _i in range(5):
            btn = ListItemBtn(
                self.container,
                "",
                has_next=False,
                use_transition=False,
            )
            btn.add_check_img()
            self.account_btns.append(btn)
        self.update_account_buttons()

        self.next_btn = NormalButton(self, "")
        self.next_btn.set_size(224, 98)
        self.next_btn.align(lv.ALIGN.BOTTOM_RIGHT, -12, -8)
        self.next_btn.set_style_bg_img_src("A:/res/arrow-right-2.png", 0)

        self.back_btn = NormalButton(self, "")
        self.back_btn.set_size(224, 98)
        self.back_btn.align(lv.ALIGN.BOTTOM_LEFT, 12, -8)
        self.back_btn.set_style_bg_img_src("A:/res/arrow-left-2.png", 0)

        self.disable_style = (
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_BLACK_5)
            .bg_img_recolor(lv_colors.ONEKEY_GRAY_1)
            .bg_img_recolor_opa(lv.OPA.COVER)
        )
        self.enable_style = (
            StyleWrapper().bg_color(lv_colors.ONEKEY_BLACK_3).bg_opa(lv.OPA.COVER)
        )

        self.update_page_buttons()
        self.next_btn.add_style(self.enable_style, 0)
        self.back_btn.add_style(self.enable_style, 0)

        self.animations_next = []
        self.animations_prev = []
        if storage_device.is_animation_enabled():
            self.animate_list_items()

    def animate_list_items(self):
        def create_move_cb_container(obj, item_index):
            def cb(value):
                obj.set_style_translate_x(value, 0)
                obj.invalidate()

            return cb

        container_move_anim = Anim(
            50,
            0,
            create_move_cb_container(self.container, 0),
            time=150,
            delay=0,
            path_cb=lv.anim_t.path_ease_out,
        )

        container_move_back_anim = Anim(
            -50,
            0,
            create_move_cb_container(self.container, 0),
            time=150,
            delay=0,
            path_cb=lv.anim_t.path_ease_out,
        )

        self.animations_next.append(container_move_anim)
        container_move_anim.start()

        self.animations_prev.append(container_move_back_anim)

    def get_page_start(self):
        return (self.current_page * 5) + 1

    def update_account_buttons(self):
        page_start = self.get_page_start()
        for i, btn in enumerate(self.account_btns):
            account_num = page_start + i
            btn.label_left.set_text(f"Account #{account_num}")

            if account_num == self.current_account:
                btn.set_checked()
            else:
                btn.set_uncheck()

    def enable_page_buttons(self, btn):
        btn.add_flag(lv.btn.FLAG.CLICKABLE)
        btn.remove_style(self.disable_style, 0)
        btn.add_style(self.enable_style, 0)

    def disable_page_buttons(self, btn):
        btn.clear_flag(lv.btn.FLAG.CLICKABLE)
        btn.remove_style(self.enable_style, 0)
        btn.add_style(self.disable_style, 0)

    def update_page_buttons(self):
        if self.current_page == 0:
            self.disable_page_buttons(self.back_btn)
            if not self.next_btn.has_flag(lv.btn.FLAG.CLICKABLE):
                self.enable_page_buttons(self.next_btn)

        elif self.current_page == self.max_page:
            self.disable_page_buttons(self.next_btn)
            if not self.back_btn.has_flag(lv.btn.FLAG.CLICKABLE):
                self.enable_page_buttons(self.back_btn)

        else:
            if not self.next_btn.has_flag(lv.btn.FLAG.CLICKABLE):
                self.enable_page_buttons(self.next_btn)
            if not self.back_btn.has_flag(lv.btn.FLAG.CLICKABLE):
                self.enable_page_buttons(self.back_btn)
        gc.collect()

    def next_page(self):
        self.current_page += 1
        self.update_account_buttons()
        self.update_page_buttons()
        for anim in self.animations_next:
            anim.start()

    def prev_page(self):
        if self.current_page > 0:
            self.current_page -= 1
            self.update_account_buttons()
            self.update_page_buttons()
            for anim in self.animations_prev:
                anim.start()

    def eventhandler(self, event_obj):
        event = event_obj.code
        target = event_obj.get_target()
        if event == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return

            if isinstance(target, lv.imgbtn):
                if target == self.nav_back.nav_btn:
                    if self.prev_scr is not None:
                        self.load_screen(self.prev_scr, destroy_self=True)
                elif target == self.nav_opt.nav_btn:
                    workflow.spawn(self.type_account_index())
            else:
                if target == self.back_btn:
                    self.prev_page()
                elif target == self.next_btn:
                    self.next_page()
                else:
                    for i, btn in enumerate(self.account_btns):
                        if target == btn:
                            for other_btn in self.account_btns:
                                other_btn.set_uncheck()

                            btn.set_checked()

                            self.current_account = self.get_page_start() + i
                            self.prev_scr.current_index = self.current_account - 1
                            self.prev_scr.update_index_btn_text()
                            break

    async def type_account_index(self):
        from trezor.lvglui.scrs.pinscreen import InputNum

        result = None
        while True:
            numscreen = InputNum(
                title=_(i18n_keys.TITLE__SET_INITIAL_ACCOUNT),
                subtitle=_(i18n_keys.TITLE__SET_INITIAL_ACCOUNT_ERROR)
                if result is not None
                else "",
                is_pin=False,
            )
            result = await numscreen.request()

            if not result:  # user cancelled
                return

            account_num = int(result)
            if 1 <= account_num <= self.max_account:
                break

        self.current_account = account_num
        self.current_page = (account_num - 1) // 5
        self.prev_scr.current_index = account_num - 1
        self.prev_scr.update_index_btn_text()

        self.update_account_buttons()
        self.update_page_buttons()


class SettingsScreen(AnimScreen):
    def collect_animation_targets(self) -> list:
        if lv.scr_act() == MainScreen._instance:
            return []
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
            kwargs = {
                "prev_scr": prev_scr,
                "title": _(i18n_keys.TITLE__SETTINGS),
                "nav_back": True,
            }
            super().__init__(**kwargs)
        else:
            self.refresh_text()
            if not self.is_visible():
                self._load_scr(self, lv.scr_act() != self)
            return
        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.general = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__GENERAL),
            left_img_src="A:/res/general.png",
        )
        self.security = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__SECURITY),
            left_img_src="A:/res/security.png",
        )
        self.wallet = ListItemBtn(
            self.container, _(i18n_keys.ITEM__WALLET), left_img_src="A:/res/wallet.png"
        )
        if not utils.BITCOIN_ONLY:
            self.fido_keys = ListItemBtn(
                self.container,
                _(i18n_keys.FIDO_FIDO_KEYS_LABEL),
                left_img_src="A:/res/fido-keys.png",
            )
        self.about = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__ABOUT_DEVICE),
            left_img_src="A:/res/about.png",
        )
        if not utils.PRODUCTION:
            self.fp_test = ListItemBtn(
                self.container,
                "指纹测试",
            )
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def refresh_text(self):
        self.title.set_text(_(i18n_keys.TITLE__SETTINGS))
        self.general.label_left.set_text(_(i18n_keys.ITEM__GENERAL))
        self.security.label_left.set_text(_(i18n_keys.ITEM__SECURITY))
        self.wallet.label_left.set_text(_(i18n_keys.ITEM__WALLET))
        if not utils.BITCOIN_ONLY:
            self.fido_keys.label_left.set_text(_(i18n_keys.FIDO_FIDO_KEYS_LABEL))
        self.about.label_left.set_text(_(i18n_keys.ITEM__ABOUT_DEVICE))

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.general:
                GeneralScreen(self)
            elif target == self.security:
                SecurityScreen(self)
            elif target == self.wallet:
                WalletScreen(self)
            elif target == self.about:
                AboutSetting(self)
            elif not utils.BITCOIN_ONLY and target == self.fido_keys:
                FidoKeysSetting(self)
            elif not utils.PRODUCTION and target == self.fp_test:
                FingerprintTest(self)


class ConnectWalletWays(Screen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
            kwargs = {
                "prev_scr": prev_scr,
                "title": _(i18n_keys.TITLE__CONNECT_APP_WALLET),
                "subtitle": _(i18n_keys.CONTENT__CONNECT_APP_WALLET_DESC),
                "nav_back": True,
                "rti_path": "A:/res/nav-options-icon.png",
            }
            super().__init__(**kwargs)
        else:
            if not self.is_visible():
                self._load_scr(self)
            return
        airgap_enabled = storage_device.is_airgap_mode()
        if airgap_enabled:
            self.waring_bar = Banner(
                self.content_area,
                LEVEL.WARNING,
                _(i18n_keys.MSG__BLUETOOTH_AND_USB_HAS_DISABLED_IN_AIR_GAP_MODE),
            )
            self.waring_bar.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 40)
        self.container = ContainerFlexCol(
            self.content_area, self.subtitle, padding_row=2
        )
        if airgap_enabled:
            self.container.align_to(self.waring_bar, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)

        self.by_ble = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__BLUETOOTH),
            left_img_src="A:/res/connect-way-ble-on.png",
        )
        self.by_usb = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__USB),
            left_img_src="A:/res/connect-way-usb-on.png",
        )
        if airgap_enabled:
            self.by_ble.disable()
            self.by_ble.img_left.set_src("A:/res/connect-way-ble-off.png")
            self.by_usb.disable()
            self.by_usb.img_left.set_src("A:/res/connect-way-usb-off.png")

        # self.by_qrcode = ListItemBtn(
        #     self.container,
        #     _(i18n_keys.BUTTON__QRCODE),
        #     left_img_src="A:/res/connect-way-qrcode.png",
        # )
        self.add_event_cb(self.on_event, lv.EVENT.CLICKED, None)

    def on_event(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.by_ble:
                ConnectWalletGuide("ble", self)
            elif target == self.by_usb:
                ConnectWalletGuide("usb", self)
            # elif target == self.by_qrcode:
            #     gc.collect()
            #     WalletList(self)
            else:
                return

    def on_click_ext(self, target):
        if target == self.rti_btn:
            QRWalletTips(self)

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)


class QRWalletTips(FullSizeWindow):
    def __init__(self, parent):
        super().__init__(
            _(i18n_keys.TITLE__QR_CODE_CONNECT),
            _(i18n_keys.TITLE__QR_CODE_CONNECT_DESC),
            confirm_text=_(i18n_keys.BUTTON__CONTINUE_WITH_QR_CODE),
            anim_dir=0,
        )
        self.parent = parent
        self.add_nav_back()

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.btn_yes:
                WalletList(self.parent, callback_obj=self)
            elif target == self.nav_back.nav_btn:
                self.destroy(100)


class ConnectWalletGuide(Screen):
    def __init__(self, c_type, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
            assert c_type in ["ble", "usb"], "Invalid connection type"
            self.connect_type = c_type
            kwargs = {
                "prev_scr": prev_scr,
                "title": _(i18n_keys.TITLE__BLUETOOTH_CONNECT)
                if c_type == "ble"
                else _(i18n_keys.TITLE__USB_CONNECT),
                "subtitle": _(i18n_keys.CONTENT__SELECT_THE_WALLET_YOU_WANT_TO_CONNECT),
                "nav_back": True,
            }
            super().__init__(**kwargs)
        else:
            return

        self.container = ContainerFlexCol(
            self.content_area, self.subtitle, padding_row=2
        )

        self.onekey = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__ONEKEY_WALLET),
            _(i18n_keys.CONTENT__BTC_TRON_SOL_ETH_BNB),
            left_img_src="A:/res/ok-logo-48.png",
            right_img_src="A:/res/support-chains-ok-normal.png",
        )
        self.onekey.text_layout_vertical(pad_top=17, pad_ver=20)

        self.mm = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__METAMASK_WALLET),
            _(i18n_keys.CONTENT__ETH_AND_EVM_POWERED_NETWORK),
            left_img_src="A:/res/mm-logo-48.png",
            right_img_src="A:/res/chains-evm.png",
        )
        self.mm.text_layout_vertical()
        if self.connect_type == "ble":
            self.mm.add_flag(lv.obj.FLAG.HIDDEN)

        self.okx = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__OKX_WALLET),
            _(i18n_keys.CONTENT__BTC_AND_EVM_COMPATIBLE_NETWORKS),
            left_img_src="A:/res/okx-logo-48.png",
            right_img_src="A:/res/support-chains-okx-normal.png",
        )
        self.okx.text_layout_vertical(pad_top=17, pad_ver=20)

        self.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target not in [self.onekey, self.mm, self.okx]:
                return
            from trezor.lvglui.scrs.template import ConnectWalletTutorial

            if target == self.onekey:
                title = _(i18n_keys.ITEM__ONEKEY_WALLET)
                subtitle = (
                    _(i18n_keys.CONTENT__IOS_ANDROID)
                    if self.connect_type == "ble"
                    else _(i18n_keys.CONTENT__DESKTOP_BROWSER_EXTENSION)
                )
                steps = [
                    (
                        _(i18n_keys.FORM__DOWNLOAD_ONEKEY_APP),
                        _(i18n_keys.FORM__DOWNLOAD_ONEKEY_APP_MOBILE)
                        if self.connect_type == "ble"
                        else _(i18n_keys.FORM__DOWNLOAD_ONEKEY_APP_DESKTOP),
                    ),
                    (
                        _(i18n_keys.FORM__CONNECT_VIA_BLUETOOTH)
                        if self.connect_type == "ble"
                        else _(i18n_keys.FORM__CONNECT_YOUR_DEVICE),
                        _(i18n_keys.FORM__CONNECT_VIA_BLUETOOTH_DESC)
                        if self.connect_type == "ble"
                        else _(i18n_keys.FORM__CONNECT_YOUR_DEVICE_DESC),
                    ),
                    (
                        _(i18n_keys.FORM__PAIR_DEVICES)
                        if self.connect_type == "ble"
                        else _(i18n_keys.FORM__START_THE_CONNECTION),
                        _(i18n_keys.FORM__PAIR_DEVICES_DESC)
                        if self.connect_type == "ble"
                        else _(i18n_keys.FORM__START_THE_CONNECTION_DESC),
                    ),
                ]
                logo = "A:/res/ok-logo-96.png"
                url = (
                    "https://help.onekey.so/articles/11461081"
                    if self.connect_type == "ble"
                    else "https://help.onekey.so/articles/11461081#h_01HMWVPP85HWYTZGPQQTB300VX"
                )
            elif target == self.mm:
                title = _(i18n_keys.ITEM__METAMASK_WALLET)
                subtitle = _(i18n_keys.CONTENT__BROWSER_EXTENSION)
                steps = [
                    (
                        _(i18n_keys.FORM__ACCESS_WALLET),
                        _(i18n_keys.FORM__OPEN_METAMASK_IN_YOUR_BROWSER),
                    ),
                    (
                        _(i18n_keys.FORM__CONNECT_HARDWARE_WALLET),
                        _(i18n_keys.FORM__CONNECT_HARDWARE_WALLET_DESC),
                    ),
                    (
                        _(i18n_keys.FORM__UNLOCK_ACCOUNT),
                        _(i18n_keys.FORM__UNLOCK_ACCOUNT_DESC),
                    ),
                ]
                logo = "A:/res/mm-logo-96.png"
                url = "https://help.onekey.so/articles/11461106"
            else:
                title = _(i18n_keys.ITEM__OKX_WALLET)
                subtitle = (
                    _(i18n_keys.CONTENT__IOS_ANDROID)
                    if self.connect_type == "ble"
                    else _(i18n_keys.CONTENT__BROWSER_EXTENSION)
                )
                steps = [
                    (
                        _(i18n_keys.FORM__ACCESS_WALLET),
                        _(i18n_keys.FORM__ACCESS_WALLET_DESC)
                        if self.connect_type == "ble"
                        else _(i18n_keys.FORM__OPEN_THE_OKX_WALLET_EXTENSION),
                    ),
                    (
                        _(i18n_keys.FORM__CONNECT_VIA_BLUETOOTH)
                        if self.connect_type == "ble"
                        else _(i18n_keys.FORM__INSTALL_ONEKEY_BRIDGE),
                        _(i18n_keys.FORM__CONNECT_VIA_BLUETOOTH_DESC)
                        if self.connect_type == "ble"
                        else _(i18n_keys.FORM__INSTALL_ONEKEY_BRIDGE_DESC),
                    ),
                    (
                        _(i18n_keys.FORM__IMPORT_WALLET_ACCOUNTS),
                        _(i18n_keys.FORM__IMPORT_WALLET_ACCOUNTS_DESC)
                        if self.connect_type == "ble"
                        else _(
                            i18n_keys.FORM__OKX_EXTENSION_IMPORT_WALLET_ACCOUNTS_DESC
                        ),
                    ),
                ]
                logo = "A:/res/okx-logo-96.png"
                url = (
                    " https://help.onekey.so/articles/11461103"
                    if self.connect_type == "ble"
                    else "https://help.onekey.so/articles/11461103"
                )
            ConnectWalletTutorial(title, subtitle, steps, url, logo)

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)


class WalletList(Screen):
    def __init__(self, prev_scr, callback_obj):
        if not hasattr(self, "_init"):
            self._init = True
            self.callback_obj = callback_obj
            kwargs = {
                "prev_scr": prev_scr,
                "title": _(i18n_keys.TITLE__QR_CODE_CONNECT),
                "subtitle": _(i18n_keys.CONTENT__SELECT_THE_WALLET_YOU_WANT_TO_CONNECT),
                "nav_back": True,
            }
            super().__init__(**kwargs)
        else:
            return

        self.container = ContainerFlexCol(
            self.content_area, self.subtitle, padding_row=2
        )

        self.onekey = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__ONEKEY_WALLET),
            _(i18n_keys.CONTENT__BTC_SOL_ETH_N_EVM_NETWORKS),
            left_img_src="A:/res/ok-logo-48.png",
            right_img_src="A:/res/support-chains-ok-qr.png",
        )
        self.onekey.text_layout_vertical()

        self.mm = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__METAMASK_WALLET),
            _(i18n_keys.CONTENT__ETH_AND_EVM_POWERED_NETWORK),
            left_img_src="A:/res/mm-logo-48.png",
            right_img_src="A:/res/chains-evm.png",
        )
        self.mm.text_layout_vertical(pad_top=17, pad_ver=20)

        self.okx = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__OKX_WALLET),
            # "BTC·ETH·TRON·SOL·NEAR ...",
            _(i18n_keys.CONTENT__COMING_SOON),
            left_img_src="A:/res/okx-logo-48.png",
        )
        self.okx.add_style(
            StyleWrapper().bg_color(lv_colors.ONEKEY_BLACK_5),
            0,
        )
        self.okx.text_layout_vertical(pad_top=17, pad_ver=20)

        self.okx.label_left.set_style_text_color(lv_colors.WHITE_2, 0)
        self.okx.label_right.set_style_text_color(lv_colors.ONEKEY_GRAY_1, 0)
        self.warning_bar = Banner(
            self.content_area,
            LEVEL.DEFAULT,
            _(i18n_keys.FORM__CAN_NOT_FIND_YOUR_CRYPTO_NETWORK),
        )
        self.warning_bar.align_to(self.container, lv.ALIGN.OUT_BOTTOM_MID, 0, 32)
        self.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.okx.clear_flag(lv.obj.FLAG.CLICKABLE)

        if (
            not storage_device.is_passphrase_enabled()
            and not passphrase.is_passphrase_pin_enabled()
        ):
            from trezor.qr import gen_hd_key

            if not get_hd_key():
                workflow.spawn(gen_hd_key(self.refresh))
        else:
            retrieval_hd_key()
            retrieval_encoder()

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.warning_bar:
                self.callback_obj.delete()
                lv.event_send(self.nav_back.nav_btn, lv.EVENT.CLICKED, None)
                return
            if target not in [self.onekey, self.mm, self.okx]:
                return
            gc.collect()
            if target == self.onekey:
                self.connect_onekey(target)
            elif target == self.mm:
                self.connect_mm(target)
            elif target == self.okx:
                qr_data = b""
                wallet_name = _(i18n_keys.ITEM__OKX_WALLET)
                ConnectWallet(
                    _(i18n_keys.TITLE__CONNECT_STR_WALLET).format(wallet_name),
                    _(
                        i18n_keys.CONTENT__OPEN_STR_WALLET_AND_SCAN_THE_QR_CODE_BELOW
                    ).format(wallet_name),
                    "Ethereum, Bitcoin, Polygon, Solana, OKT Chain, TRON and other networks.",
                    qr_data,
                    "A:/res/okx-logo-96.png",
                )

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)

    def connect_onekey(self, target):
        from trezor.qr import get_encoder

        if passphrase.is_enabled():
            encoder = retrieval_encoder()
        else:
            encoder = get_encoder()
        if encoder is None:
            from trezor.qr import gen_multi_accounts

            workflow.spawn(
                gen_multi_accounts(
                    lambda: lv.event_send(target, lv.EVENT.CLICKED, None)
                )
            )
            return
        ConnectWallet(
            _(i18n_keys.TITLE__CONNECT_STR_WALLET).format(
                _(i18n_keys.ITEM__ONEKEY_WALLET)
            ),
            _(i18n_keys.CONTENT__OPEN_ONEKEY_SCAN_THE_QRCODE),
            _(i18n_keys.CONTENT__BTC_SOL_ETH_N_EVM_NETWORKS),
            "A:/res/support-chains-ok-qr.png",
            None,
            encoder=encoder,
        )

    def connect_mm(self, target):
        qr_data = (
            retrieval_hd_key()
            if storage_device.is_passphrase_enabled()
            else get_hd_key()
        )
        if qr_data is None:
            from trezor.qr import gen_hd_key

            workflow.spawn(
                gen_hd_key(lambda: lv.event_send(target, lv.EVENT.CLICKED, None))
            )
            return
        wallet_name = _(i18n_keys.ITEM__METAMASK_WALLET)
        ConnectWallet(
            _(i18n_keys.TITLE__CONNECT_STR_WALLET).format(wallet_name),
            _(i18n_keys.CONTENT__OPEN_STR_WALLET_AND_SCAN_THE_QR_CODE_BELOW).format(
                wallet_name
            ),
            _(i18n_keys.CONTENT__ETH_AND_EVM_POWERED_NETWORK),
            "A:/res/chains-evm.png",
            qr_data,
            "A:/res/mm-logo-96.png",
        )


class BackupWallet(Screen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
            kwargs = {
                "prev_scr": prev_scr,
                "title": _(i18n_keys.APP__BACK_UP),
                "subtitle": _(i18n_keys.CONTENT__SELECT_THE_WAY_YOU_WANT_TO_BACK_UP),
                "nav_back": True,
            }
            super().__init__(**kwargs)
        else:
            if not self.is_visible():
                self._load_scr(self)
            return

        self.container = ContainerFlexCol(
            self.content_area, self.subtitle, padding_row=2
        )
        from trezor.enums import BackupType

        is_bip39 = storage_device.get_backup_type() == BackupType.Bip39
        self.lite = ListItemBtn(
            self.container,
            "OneKey Lite",
            left_img_src="A:/res/icon-lite-48.png",
        )

        self.keytag = ListItemBtn(
            self.container,
            "OneKey Keytag",
            left_img_src="A:/res/icon-dot-48.png",
        )

        if not is_bip39:
            self.lite.disable()
            self.keytag.disable()

        self.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target in [self.lite, self.keytag]:
                from trezor.wire import DUMMY_CONTEXT
                from apps.management.recovery_device import recovery_device
                from trezor.messages import RecoveryDevice

                if target == self.lite:
                    airgap_enabled = storage_device.is_airgap_mode()
                    if airgap_enabled:
                        screen = FullSizeWindow(
                            _(i18n_keys.TITLE__BACKUP_LIMITED),
                            _(i18n_keys.TITLE__BACKUP_LIMITED_DESC),
                            confirm_text=_(i18n_keys.BUTTON__GO_SETTINGS),
                            cancel_text=_(i18n_keys.BUTTON__BACK),
                            anim_dir=0,
                        )
                        screen.btn_layout_ver()
                        if hasattr(screen, "subtitle"):
                            screen.subtitle.set_recolor(True)
                        workflow.spawn(self.handle_airgap_response(screen))
                        return
                    utils.set_backup_lite()
                elif target == self.keytag:
                    utils.set_backup_keytag()
                workflow.spawn(
                    recovery_device(
                        DUMMY_CONTEXT,
                        RecoveryDevice(dry_run=True, enforce_wordlist=True),
                    )
                )

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)

    async def handle_airgap_response(self, screen):
        from trezor.wire import DUMMY_CONTEXT

        if await DUMMY_CONTEXT.wait(screen.request()):
            screen.destroy()
            AirGapSetting(self)
        else:
            screen.destroy()


class ConnectWallet(FullSizeWindow):
    def __init__(
        self,
        title,
        subtitle,
        support_chains,
        support_chains_icon,
        qr_data,
        icon_path=None,
        encoder=None,
    ):
        super().__init__(
            title,
            subtitle,
            confirm_text=_(i18n_keys.BUTTON__SHOW_DYNAMIC_QR_CODE)
            if encoder is not None
            else "",
            anim_dir=0,
        )
        self.content_area.set_style_max_height(684, 0)
        self.add_nav_back()

        from trezor.lvglui.scrs.components.qrcode import QRCode

        self.encoder = encoder
        if self.encoder is None:
            self.qr = QRCode(
                self.content_area,
                qr_data,
                icon_path=icon_path,
                size=440,
            )
            self.qr.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 40)

        if support_chains:
            self.panel = lv.obj(self.content_area)
            self.panel.set_size(456, lv.SIZE.CONTENT)
            self.panel.add_style(
                StyleWrapper()
                .bg_color(lv_colors.ONEKEY_BLACK_3)
                .bg_opa()
                .radius(40)
                .border_width(0)
                .pad_hor(24)
                .pad_ver(12)
                .text_color(lv_colors.WHITE),
                0,
            )
            self.label_top = lv.label(self.panel)
            self.label_top.set_text(_(i18n_keys.LIST_KEY__SUPPORTED_CHAINS))
            self.label_top.add_style(
                StyleWrapper().text_font(font_GeistSemiBold26).pad_ver(4).pad_hor(0), 0
            )
            self.label_top.align(lv.ALIGN.TOP_LEFT, 0, 0)
            self.line = lv.line(self.panel)
            self.line.set_size(408, 1)
            self.line.add_style(
                StyleWrapper().bg_color(lv_colors.ONEKEY_GRAY_2).bg_opa(),
                0,
            )
            self.line.align_to(self.label_top, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 9)
            self.label_bottom = lv.label(self.panel)
            self.label_bottom.set_width(316)
            self.label_bottom.add_style(
                StyleWrapper()
                .text_font(font_GeistRegular20)
                .text_color(lv_colors.LIGHT_GRAY)
                .pad_ver(12)
                .pad_hor(0),
                0,
            )
            self.content_area.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
            self.label_bottom.set_long_mode(lv.label.LONG.WRAP)
            self.label_bottom.set_text(support_chains)
            self.img_right = lv.img(self.panel)
            self.img_right.set_src(support_chains_icon)
            self.label_bottom.align_to(self.line, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 0)
            self.img_right.align_to(self.line, lv.ALIGN.OUT_BOTTOM_RIGHT, 0, 12)
            if hasattr(self, "qr"):
                self.panel.align_to(self.qr, lv.ALIGN.OUT_BOTTOM_MID, 0, 32)
            else:
                self.panel.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 40)
        self.add_event_cb(self.on_nav_back, lv.EVENT.GESTURE, None)

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
        if target == self.nav_back.nav_btn:
            self.destroy()
        elif hasattr(self, "btn_yes") and target == self.btn_yes:
            DynQr(self.encoder)
            self.destroy(100)

    def on_nav_back(self, event_obj):
        code = event_obj.code
        if code == lv.EVENT.GESTURE:
            _dir = lv.indev_get_act().get_gesture_dir()
            if _dir == lv.DIR.RIGHT:
                lv.event_send(self.nav_back.nav_btn, lv.EVENT.CLICKED, None)


class DynQr(FullSizeWindow):
    def __init__(self, encoder):
        super().__init__(
            None,
            None,
            confirm_text=_(i18n_keys.BUTTON__CLOSE),
            anim_dir=0,
        )
        assert encoder is not None, "missing required encoder"
        self.btn_yes.enable(lv_colors.ONEKEY_GRAY_3, text_color=lv_colors.WHITE)
        self.encoder = encoder
        qr_data = encoder.next_part()

        from trezor.lvglui.scrs.components.qrcode import QRCode

        self.qr = QRCode(
            self.content_area,
            qr_data,
            size=440,
        )
        self.qr.align(lv.ALIGN.TOP_MID, 0, 126)
        workflow.spawn(self.update_qr())

    async def update_qr(self):
        while True:
            stop_single = self.request()
            racer = loop.race(stop_single, loop.sleep(100))
            await racer
            if stop_single in racer.finished:
                self.destroy()
                return
            qr_data = self.encoder.next_part()
            self.qr.update(qr_data, len(qr_data))

    def destroy(self, delay_ms=10):
        self.del_delayed(delay_ms)


class ScanScreen(Screen):
    SCAN_STATE_IDLE = 0
    SCAN_STATE_SCANNING = 1
    SCAN_STATE_SUCCESS = 2
    SCAN_STATE_ERROR = 3
    VALID_TRANSITIONS = {
        SCAN_STATE_IDLE: [SCAN_STATE_SCANNING, SCAN_STATE_ERROR],
        SCAN_STATE_SCANNING: [SCAN_STATE_SUCCESS, SCAN_STATE_ERROR],
        SCAN_STATE_SUCCESS: [SCAN_STATE_IDLE],
        SCAN_STATE_ERROR: [SCAN_STATE_IDLE],
    }

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
            kwargs = {
                "prev_scr": prev_scr,
                "nav_back": True,
            }
            super().__init__(**kwargs)
        else:
            if not self.is_visible():
                self._load_scr(self)
            return

        self.nav_back.align(lv.ALIGN.TOP_RIGHT, 0, 44)
        self.nav_back.nav_btn.add_style(
            StyleWrapper().bg_img_src("A:/res/nav-close.png"), 0
        )
        self.nav_back.nav_btn.align(lv.ALIGN.RIGHT_MID, 0, 0)

        self.camera_bg = lv.img(self.content_area)
        self.camera_bg.set_src("A:/res/camera-bg.png")
        self.camera_bg.align(lv.ALIGN.TOP_MID, 0, 148)

        self.btn = NormalButton(self, f"{LV_SYMBOLS.LV_SYMBOL_LIGHTBULB}")
        self.btn.set_size(64, 64)
        self.btn.add_style(StyleWrapper().radius(lv.RADIUS.CIRCLE), 0)
        self.btn.align(lv.ALIGN.TOP_LEFT, 12, 48)
        self.btn.add_state(lv.STATE.CHECKED)
        self.add_event_cb(self.on_event, lv.EVENT.CLICKED, None)
        self.desc = lv.label(self.content_area)
        self.desc.set_size(456, lv.SIZE.CONTENT)
        self.desc.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular30)
            .text_color(lv_colors.LIGHT_GRAY)
            .pad_hor(12)
            .pad_ver(16)
            .text_letter_space(-1)
            .text_align_center(),
            0,
        )
        self.desc.align_to(self.camera_bg, lv.ALIGN.OUT_BOTTOM_MID, 0, 14)
        self.process_bar = lv.bar(self.content_area)
        self.process_bar.set_size(368, 8)
        self.process_bar.add_style(
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_GRAY_2)
            .bg_opa(lv.OPA.COVER)
            .radius(22),
            0,
        )
        self.process_bar.add_style(
            StyleWrapper().bg_color(lv_colors.ONEKEY_GREEN_2),
            lv.PART.INDICATOR | lv.STATE.DEFAULT,
        )
        self.process_bar.align_to(self.desc, lv.ALIGN.OUT_BOTTOM_MID, 0, 32)
        self.process_bar.set_range(0, 100)
        self.process_bar.set_value(0, lv.ANIM.OFF)
        self.process_bar.add_flag(lv.obj.FLAG.HIDDEN)

        self.state = ScanScreen.SCAN_STATE_IDLE
        self._fsm_show()

        scan_qr(self)

    @classmethod
    def notify_close(cls):
        if hasattr(cls, "_instance") and cls._instance._init:
            lv.event_send(cls._instance.nav_back.nav_btn, lv.EVENT.CLICKED, None)

    async def transition_to(self, new_state: int):
        self._can_transition_to(new_state)
        if new_state == ScanScreen.SCAN_STATE_ERROR:
            await self._error_feedback()
            new_state = ScanScreen.SCAN_STATE_IDLE

        self._fsm_show(new_state)
        self.state = new_state

    async def on_process_update(self, process: int):
        if self.state == ScanScreen.SCAN_STATE_IDLE:
            await self.transition_to(ScanScreen.SCAN_STATE_SCANNING)
        workflow.idle_timer.touch()
        self.process_bar.set_value(process, lv.ANIM.OFF)
        if process >= 100:
            await self.transition_to(ScanScreen.SCAN_STATE_SUCCESS)

    def on_event(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.btn:
                if self.btn.has_state(lv.STATE.CHECKED):
                    self.btn.label.set_text(f"{LV_SYMBOLS.LV_SYMBOL_TRFFIC_LIGHT}")
                    self.btn.enable(bg_color=lv_colors.ONEKEY_BLACK)
                    self.btn.clear_state(lv.STATE.CHECKED)
                    uart.flashled_open()
                else:
                    uart.flashled_close()
                    self.btn.label.set_text(f"{LV_SYMBOLS.LV_SYMBOL_LIGHTBULB}")
                    self.btn.enable()
                    self.btn.add_state(lv.STATE.CHECKED)
            elif target == self.nav_back.nav_btn:
                uart.flashled_close()
                close_camera()

    async def _error_feedback(self):
        from trezor.ui.layouts import show_error_no_interact

        await show_error_no_interact(
            _(i18n_keys.TITLE__DATA_FORMAT_NOT_SUPPORT),
            _(i18n_keys.CONTENT__QR_CODE_TYPE_NOT_SUPPORT_PLEASE_TRY_AGAIN),
        )

    def _can_transition_to(self, new_state: int):
        if new_state not in ScanScreen.VALID_TRANSITIONS[self.state]:
            self.notify_close()

    def _fsm_show(self, state: int = SCAN_STATE_IDLE):
        if state == ScanScreen.SCAN_STATE_IDLE:
            if self.state == ScanScreen.SCAN_STATE_SCANNING:
                self.process_bar.add_flag(lv.obj.FLAG.HIDDEN)
            elif self.state == ScanScreen.SCAN_STATE_SUCCESS:
                if hasattr(self, "wait_tips"):
                    self.wait_tips.add_flag(lv.obj.FLAG.HIDDEN)

            self.desc.set_text(
                _(i18n_keys.CONTENT__SCAN_THE_QR_CODE_DISPLAYED_ON_THE_APP)
            )
            self.desc.clear_flag(lv.obj.FLAG.HIDDEN)
            self.desc.align_to(self.camera_bg, lv.ALIGN.OUT_BOTTOM_MID, 0, 14)

        elif state == ScanScreen.SCAN_STATE_SCANNING:
            self.desc.set_text(_(i18n_keys.CONTENT__SCANNING_HOLD_STILL))
            self.desc.align_to(self.camera_bg, lv.ALIGN.OUT_BOTTOM_MID, 0, 14)
            if self.process_bar.has_flag(lv.obj.FLAG.HIDDEN):
                self.process_bar.clear_flag(lv.obj.FLAG.HIDDEN)
            self.process_bar.set_value(0, lv.ANIM.OFF)
        elif state == ScanScreen.SCAN_STATE_SUCCESS:
            self.process_bar.add_flag(lv.obj.FLAG.HIDDEN)
            self.desc.add_flag(lv.obj.FLAG.HIDDEN)
            if not hasattr(self, "wait_tips"):
                self.refresh()
                self.wait_tips = lv.label(self.camera_bg)
                self.wait_tips.set_text(_(i18n_keys.TITLE__PLEASE_WAIT))
                self.wait_tips.add_style(
                    StyleWrapper()
                    .text_font(font_GeistRegular30)
                    .text_color(lv_colors.LIGHT_GRAY),
                    0,
                )
                self.wait_tips.align(lv.ALIGN.CENTER, 0, 0)
            else:
                if self.wait_tips.has_flag(lv.obj.FLAG.HIDDEN):
                    self.refresh()
                    self.wait_tips.clear_flag(lv.obj.FLAG.HIDDEN)
        else:
            raise ValueError(f"Invalid state: {state}")

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)


class FingerprintTest(Screen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
            kwargs = {
                "prev_scr": prev_scr,
                "nav_back": True,
            }
            super().__init__(**kwargs)
        else:
            return

        from trezorio import fingerprint

        sensitivity, area = fingerprint.get_sensitivity_and_area()

        self.sensitivity = sensitivity
        self.area = area

        # region
        self.app_drawer_up = lv.label(self.content_area)
        self.app_drawer_up.set_size(456, lv.SIZE.CONTENT)
        self.app_drawer_up.add_style(
            StyleWrapper()
            .pad_all(12)
            .text_font(font_GeistRegular30)
            .text_color(lv_colors.WHITE),
            0,
        )
        self.app_drawer_up.set_text("按压阈值:")
        self.app_drawer_up.align_to(self.nav_back, lv.ALIGN.OUT_BOTTOM_LEFT, 12, 20)

        self.slider = lv.slider(self.content_area)
        self.slider.set_size(456, 80)
        self.slider.set_ext_click_area(20)
        self.slider.set_range(20, 250)
        self.slider.set_value(self.sensitivity, lv.ANIM.OFF)
        self.slider.align_to(self.app_drawer_up, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 20)

        self.slider.add_style(
            StyleWrapper().border_width(0).radius(40).bg_color(lv_colors.GRAY_1), 0
        )
        self.slider.add_style(
            StyleWrapper().bg_color(lv_colors.WHITE).pad_all(-50), lv.PART.KNOB
        )
        self.slider.add_style(
            StyleWrapper().radius(0).bg_color(lv_colors.WHITE), lv.PART.INDICATOR
        )
        self.percent = lv.label(self.slider)
        self.percent.align(lv.ALIGN.CENTER, 0, 0)
        self.percent.add_style(
            StyleWrapper().text_font(font_GeistRegular30).text_color(lv_colors.RED),
            0,
        )
        self.percent.set_text(f"{self.sensitivity}")
        self.slider.clear_flag(lv.obj.FLAG.GESTURE_BUBBLE)
        self.slider.add_flag(lv.obj.FLAG.EVENT_BUBBLE)

        self.app_drawer_up_delay = lv.label(self.content_area)
        self.app_drawer_up_delay.set_size(456, lv.SIZE.CONTENT)
        self.app_drawer_up_delay.add_style(
            StyleWrapper()
            .pad_all(12)
            .text_font(font_GeistRegular30)
            .text_color(lv_colors.WHITE),
            0,
        )
        self.app_drawer_up_delay.set_text("面积:")
        self.app_drawer_up_delay.align_to(self.slider, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 20)

        self.slider1 = lv.slider(self.content_area)
        self.slider1.set_size(456, 80)
        self.slider1.set_ext_click_area(20)
        self.slider1.set_range(1, 12)
        self.slider1.set_value(self.area, lv.ANIM.OFF)
        self.slider1.align_to(self.app_drawer_up_delay, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 20)

        self.slider1.add_style(
            StyleWrapper().border_width(0).radius(40).bg_color(lv_colors.GRAY_1), 0
        )
        self.slider1.add_style(
            StyleWrapper().bg_color(lv_colors.WHITE).pad_all(-50), lv.PART.KNOB
        )
        self.slider1.add_style(
            StyleWrapper().radius(0).bg_color(lv_colors.WHITE), lv.PART.INDICATOR
        )
        self.percent1 = lv.label(self.slider1)
        self.percent1.align(lv.ALIGN.CENTER, 0, 0)
        self.percent1.add_style(
            StyleWrapper().text_font(font_GeistRegular30).text_color(lv_colors.RED),
            0,
        )
        self.percent1.set_text(f"{self.area}")
        self.slider1.clear_flag(lv.obj.FLAG.GESTURE_BUBBLE)
        self.slider1.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
        # endregion

        self.add_event_cb(self.on_value_changed, lv.EVENT.VALUE_CHANGED, None)
        # self.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def on_nav_back(self, event_obj):
        pass

    def on_value_changed(self, event_obj):
        from trezorio import fingerprint

        target = event_obj.get_target()
        if target == self.slider:
            value = target.get_value()
            self.sensitivity = value
            self.percent.set_text(f"{value}")
            fingerprint.set_sensitivity_and_area(value, self.area)
        elif target == self.slider1:
            value = target.get_value()
            self.area = value
            self.percent1.set_text(f"{value}")
            fingerprint.set_sensitivity_and_area(self.sensitivity, value)


class GeneralScreen(AnimScreen):
    cur_language = ""

    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        list_btns = ["power"]
        for btn_name in list_btns:
            if hasattr(self, btn_name) and getattr(self, btn_name):
                targets.append(getattr(self, btn_name))
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            if self.cur_language:
                self.language.label_right.set_text(self.cur_language)
            self.refresh_text()
            return

        super().__init__(
            prev_scr=prev_scr,
            title=_(i18n_keys.TITLE__GENERAL),
            nav_back=True,
            rti_path="A:/res/poweroff-white.png",
        )

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        current_lang = storage_device.get_language()
        if current_lang not in langs_keys:
            current_lang = "en"
            storage_device.set_language(current_lang)
        GeneralScreen.cur_language = langs[langs_keys.index(current_lang)][1]
        self.language = ListItemBtn(
            self.container, _(i18n_keys.ITEM__LANGUAGE), GeneralScreen.cur_language
        )

        self.wallpaper = ListItemBtn(self.container, _(i18n_keys.BUTTON__WALLPAPER))

        self.animation = ListItemBtn(self.container, _(i18n_keys.ITEM__ANIMATIONS))

        self.touch = ListItemBtn(self.container, _(i18n_keys.BUTTON__TOUCH))

        self.display = ListItemBtn(self.container, _(i18n_keys.BUTTON__DISPLAY))

        self.content_area.add_event_cb(self.on_click_event, lv.EVENT.CLICKED, None)
        # AnimScreen.__init__ already loads the screen; avoid double-loading here

    def refresh_text(self):
        self.title.set_text(_(i18n_keys.TITLE__GENERAL))
        self.language.label_left.set_text(_(i18n_keys.ITEM__LANGUAGE))
        self.display.label_left.set_text(_(i18n_keys.TITLE__DISPLAY))
        self.wallpaper.label_left.set_text(_(i18n_keys.BUTTON__WALLPAPER))
        self.animation.label_left.set_text(_(i18n_keys.ITEM__ANIMATIONS))
        self.touch.label_left.set_text(_(i18n_keys.BUTTON__TOUCH))
        self.container.update_layout()

    def on_click_event(self, event_obj):
        target = event_obj.get_target()
        if target == self.language:
            LanguageSetting(self)
        elif target == self.display:
            DisplayScreen(self)
        elif target == self.animation:
            AnimationSetting(self)
        elif target == self.touch:
            TouchSetting(self)
        elif target == self.wallpaper:
            WallpaperScreen(self)

        elif target == self.rti_btn:
            PowerOff()
        else:
            pass


class DisplayScreen(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "auto_container") and self.auto_container:
            targets.append(self.auto_container)
        if hasattr(self, "device_info_container") and self.device_info_container:
            targets.append(self.device_info_container)
        if hasattr(self, "device_name_description") and self.device_name_description:
            targets.append(self.device_name_description)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            self.backlight.label_right.set_text(
                brightness2_percent_str(storage_device.get_brightness())
            )
            self.autolock.label_right.set_text(get_autolock_delay_str())
            self.shutdown.label_right.set_text(get_autoshutdown_delay_str())
            # Update switch state based on current storage setting
            current_setting = storage_device.is_device_name_display_enabled()
            if current_setting:
                self.model_name_bt_id.add_state()
            else:
                self.model_name_bt_id.clear_state()
            self.refresh_text()
            return

        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__DISPLAY), nav_back=True
        )

        # Main container
        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        # Enable event bubbling for gesture detection
        self.container.add_flag(lv.obj.FLAG.EVENT_BUBBLE)

        # Brightness control
        self.backlight = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__BRIGHTNESS),
            brightness2_percent_str(storage_device.get_brightness()),
        )

        # Auto lock and shutdown container
        self.auto_container = ContainerFlexCol(self.content_area, None, padding_row=2)
        self.auto_container.align_to(self.container, lv.ALIGN.OUT_BOTTOM_MID, 0, 12)
        # Enable event bubbling for gesture detection
        self.auto_container.add_flag(lv.obj.FLAG.EVENT_BUBBLE)

        self.autolock = ListItemBtn(
            self.auto_container,
            # TITLE__AUTO_LOCK,
            _(i18n_keys.TITLE__AUTO_LOCK),
            get_autolock_delay_str(),
        )
        self.shutdown = ListItemBtn(
            self.auto_container,
            _(i18n_keys.ITEM__SHUTDOWN),
            get_autoshutdown_delay_str(),
        )

        # Device name and Bluetooth ID container
        self.device_info_container = ContainerFlexCol(
            self.content_area, None, padding_row=2
        )
        self.device_info_container.align_to(
            self.auto_container, lv.ALIGN.OUT_BOTTOM_MID, 0, 12
        )
        # Enable event bubbling for gesture detection
        self.device_info_container.add_flag(lv.obj.FLAG.EVENT_BUBBLE)

        self.model_name_bt_id = ListItemBtnWithSwitch(
            self.device_info_container,
            _(i18n_keys.BUTTON__MODEL_NAME_BLUETOOTH_ID),
        )
        # Allow the switch row to grow when long locale strings wrap
        self.model_name_bt_id.set_size(456, lv.SIZE.CONTENT)
        # Fix background color to match other ListItemBtn containers
        self.model_name_bt_id.add_style(
            StyleWrapper().bg_color(lv_colors.ONEKEY_BLACK_3).min_height(94),
            0,
        )

        current_setting = storage_device.is_device_name_display_enabled()
        if current_setting is None or current_setting:
            pass
        else:
            # Clear the default checked state
            self.model_name_bt_id.clear_state()

        # Description below the toggle - keep width identical so multi-line text aligns
        self.device_name_description = lv.label(self.content_area)
        desc_width = self.device_info_container.get_width()
        self.device_name_description.set_size(desc_width, lv.SIZE.CONTENT)
        self.device_name_description.set_long_mode(lv.label.LONG.WRAP)
        self.device_name_description.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular26)
            .text_color(lv_colors.GRAY_2)
            .text_align_left()
            .pad_hor(24)
            .pad_ver(8),
            0,
        )
        self.device_name_description.set_text(
            _(i18n_keys.BUTTON__MODEL_NAME_BLUETOOTH_ID_DESC),
        )
        self.device_name_description.align_to(
            self.device_info_container, lv.ALIGN.OUT_BOTTOM_MID, 0, 8
        )

        # Disable elastic scrolling and scrollbar to match other pages
        self.content_area.clear_flag(lv.obj.FLAG.SCROLL_ELASTIC)
        self.content_area.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)

        # Keep vertical scrolling so gestures bubble up to nav-back handler
        self.content_area.set_scroll_dir(lv.DIR.VER)
        self.content_area.add_flag(lv.obj.FLAG.GESTURE_BUBBLE)

        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.auto_container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.device_info_container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.model_name_bt_id.switch.add_event_cb(
            self.on_switch_change, lv.EVENT.VALUE_CHANGED, None
        )
        gc.collect()

    def refresh_text(self):
        self.title.set_text(_(i18n_keys.TITLE__DISPLAY))
        if hasattr(self, "backlight"):
            self.backlight.label_left.set_text(_(i18n_keys.ITEM__BRIGHTNESS))
        if hasattr(self, "autolock"):
            self.autolock.label_left.set_text(_(i18n_keys.TITLE__AUTO_LOCK))
        if hasattr(self, "shutdown"):
            self.shutdown.label_left.set_text(_(i18n_keys.ITEM__SHUTDOWN))
        if hasattr(self, "device_name_description"):
            self.device_name_description.set_text(
                _(i18n_keys.BUTTON__MODEL_NAME_BLUETOOTH_ID_DESC)
            )

    def on_switch_change(self, event_obj):
        code = event_obj.code
        if code == lv.EVENT.VALUE_CHANGED:
            # Get the new switch state
            new_switch_checked = (
                self.model_name_bt_id.switch.get_state() & lv.STATE.CHECKED
            ) != 0

            storage_device.set_device_name_display_enabled(new_switch_checked)

            if hasattr(MainScreen, "_instance") and MainScreen._instance:
                main_screen = MainScreen._instance
                real_device_name = storage_device.get_label()
                real_ble_name = storage_device.get_ble_name() or uart.get_ble_name()

                if new_switch_checked:
                    # Show device names - create labels if they don't exist
                    if not (hasattr(main_screen, "title") and main_screen.title):
                        main_screen.title = Title(
                            main_screen.content_area, None, (), real_device_name
                        )
                        main_screen.title.align_to(
                            main_screen.content_area, lv.ALIGN.TOP_MID, 0, 76
                        )
                    else:
                        main_screen.title.set_text(real_device_name)
                        main_screen.title.clear_flag(lv.obj.FLAG.HIDDEN)
                    # Ensure centered alignment
                    main_screen.title.add_style(StyleWrapper().text_align_center(), 0)

                    if not (hasattr(main_screen, "subtitle") and main_screen.subtitle):
                        main_screen.subtitle = SubTitle(
                            main_screen.content_area,
                            main_screen.title,
                            (0, 16),
                            real_ble_name,
                        )
                    else:
                        main_screen.subtitle.set_text(real_ble_name)
                        main_screen.subtitle.clear_flag(lv.obj.FLAG.HIDDEN)
                    main_screen.subtitle.add_style(
                        StyleWrapper().text_align_center().text_color(lv_colors.WHITE),
                        0,
                    )
                else:

                    # Hide device names - only if title/subtitle exist
                    if hasattr(main_screen, "title") and main_screen.title:
                        main_screen.title.add_flag(lv.obj.FLAG.HIDDEN)
                        main_screen.title.set_text("")
                    if hasattr(main_screen, "subtitle") and main_screen.subtitle:
                        main_screen.subtitle.add_flag(lv.obj.FLAG.HIDDEN)
                        main_screen.subtitle.set_text("")

            # Update LockScreen display if it exists
            from .lockscreen import LockScreen

            _visible, lock_screen = LockScreen.retrieval()
            if lock_screen:
                real_device_name = storage_device.get_label()
                real_ble_name = storage_device.get_ble_name() or uart.get_ble_name()
                if new_switch_checked:
                    # Show device names - ensure title/subtitle exist first
                    if hasattr(lock_screen, "title") and lock_screen.title:
                        lock_screen.title.set_text(real_device_name)
                        lock_screen.title.clear_flag(lv.obj.FLAG.HIDDEN)
                    if hasattr(lock_screen, "subtitle") and lock_screen.subtitle:
                        lock_screen.subtitle.set_text(real_ble_name)
                        lock_screen.subtitle.clear_flag(lv.obj.FLAG.HIDDEN)
                        # Ensure centered alignment with opacity
                        lock_screen.title.add_style(
                            StyleWrapper()
                            .text_align_center()
                            .text_opa(int(lv.OPA.COVER * 0.85)),
                            0,
                        )
                        lock_screen.subtitle.add_style(
                            StyleWrapper()
                            .text_align_center()
                            .text_color(lv_colors.WHITE)
                            .text_opa(int(lv.OPA.COVER * 0.85)),
                            0,
                        )
                else:
                    # Hide device names - only if title/subtitle exist
                    if hasattr(lock_screen, "title") and lock_screen.title:
                        lock_screen.title.add_flag(lv.obj.FLAG.HIDDEN)
                        lock_screen.title.set_text("")
                    if hasattr(lock_screen, "subtitle") and lock_screen.subtitle:
                        lock_screen.subtitle.add_flag(lv.obj.FLAG.HIDDEN)
                        lock_screen.subtitle.set_text("")

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.backlight:
                BacklightSetting(self)
            elif target == self.autolock:
                # Go directly to auto-lock specific settings
                AutoLockSetting(self)
            elif target == self.shutdown:
                # Go directly to shutdown time selection
                AutoShutDownSetting(self)
            # Note: model_name_bt_id switch changes are handled by on_switch_change method


class AutolockSetting(AnimScreen):
    cur_auto_lock = ""
    cur_auto_lock_ms = 0

    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        return targets

    def __init__(self, prev_scr=None):
        AutolockSetting.cur_auto_lock_ms = storage_device.get_autolock_delay_ms()
        AutolockSetting.cur_auto_lock = self.get_str_from_ms(
            AutolockSetting.cur_auto_lock_ms
        )
        if not hasattr(self, "_init"):
            self._init = True
        else:
            if self.cur_auto_lock:
                self.auto_lock.label_right.set_text(AutolockSetting.cur_auto_lock)
            self.refresh_text()
            return

        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__AUTO_LOCK), nav_back=True
        )

        self.container = ContainerFlexCol(
            self.content_area, self.title, padding_row=2, pos=(0, 40)
        )

        self.auto_lock = ListItemBtn(
            self.container,
            _(i18n_keys.TITLE__AUTO_LOCK),
            AutolockSetting.cur_auto_lock,
        )

        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        # AnimScreen.__init__ already loads the screen; avoid double-loading here
        gc.collect()

    def get_str_from_ms(self, delay_ms: int) -> str:
        if delay_ms in (0, storage_device.AUTOLOCK_DELAY_MAXIMUM):
            return "从不"
        elif delay_ms < 60000:
            return f"{delay_ms // 1000}秒"
        elif delay_ms < 3600000:
            return f"{delay_ms // 60000}分钟"
        else:
            return f"{delay_ms // 3600000}小时"

    def refresh_text(self):
        self.title.set_text(_(i18n_keys.TITLE__AUTO_LOCK))
        self.auto_lock.label_left.set_text(_(i18n_keys.TITLE__AUTO_LOCK))

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.auto_lock:
                # Use the existing AutoLockSetting class
                AutoLockSetting(self)


class AppdrawerBackgroundSetting(AnimScreen):
    @classmethod
    def _dispose_existing(cls, reason=""):
        if hasattr(cls, "_instance"):
            instance = cls._instance
            if hasattr(utils, "SCREENS") and instance in utils.SCREENS:
                utils.SCREENS.remove(instance)
            instance.delete()
            if hasattr(instance, "_init"):
                delattr(instance, "_init")
            del cls._instance
            gc.collect()

    def collect_animation_targets(self) -> list:
        return []

    def __init__(
        self, prev_scr=None, selected_wallpaper=None, return_from_wallpaper=False
    ):
        if not hasattr(self, "_init"):
            self._init = True
            AppdrawerBackgroundSetting._instance = self
        else:
            self._return_to_prev_instance = return_from_wallpaper
            if selected_wallpaper:
                self.selected_wallpaper = selected_wallpaper
                self.current_wallpaper_path = selected_wallpaper
                if hasattr(self, "lockscreen_preview"):
                    self.lockscreen_preview.set_src(selected_wallpaper)
            else:
                lockscreen_path = storage_device.get_homescreen()
                if lockscreen_path:
                    try:
                        if lockscreen_path.startswith("A:/res/wallpapers/"):
                            file_path = lockscreen_path.replace(
                                "A:/res/wallpapers/", "1:/res/wallpapers/"
                            )
                        elif lockscreen_path.startswith("A:/res/"):
                            file_path = lockscreen_path[2:]
                        elif lockscreen_path.startswith("A:1:/"):
                            file_path = lockscreen_path[2:]
                        else:
                            file_path = lockscreen_path

                        io.fatfs.stat(file_path)
                        self.current_wallpaper_path = lockscreen_path
                        if hasattr(self, "lockscreen_preview"):
                            self.lockscreen_preview.set_src(lockscreen_path)
                    except Exception:
                        self.current_wallpaper_path = "A:/res/wallpaper-7.jpg"
                        if hasattr(self, "lockscreen_preview"):
                            self.lockscreen_preview.set_src("A:/res/wallpaper-7.jpg")
                        storage_device.set_homescreen("A:/res/wallpaper-7.jpg")
            self.refresh_text()
            return

        self.selected_wallpaper = selected_wallpaper
        self._return_to_prev_instance = return_from_wallpaper

        super().__init__(
            prev_scr=prev_scr, nav_back=True, rti_path="A:/res/checkmark.png"
        )

        self.content_area.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
        self.content_area.clear_flag(lv.obj.FLAG.SCROLLABLE)
        self.content_area.set_style_pad_bottom(0, 0)

        self.container = lv.obj(self.content_area)
        self.container.set_size(lv.pct(100), lv.pct(100))
        self.container.align(lv.ALIGN.TOP_MID, 0, 0)
        self.container.add_style(
            StyleWrapper().bg_opa(lv.OPA.TRANSP).pad_all(0).border_width(0), 0
        )
        self.container.clear_flag(lv.obj.FLAG.CLICKABLE)
        self.container.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
        self.container.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
        self.container.clear_flag(lv.obj.FLAG.SCROLLABLE)

        if "appdrawer_preview_container" not in _cached_styles:
            _cached_styles["appdrawer_preview_container"] = (
                StyleWrapper()
                .bg_color(lv_colors.BLACK)
                .bg_opa(lv.OPA.COVER)
                .pad_all(0)
                .border_width(0)
            )
        container_style = _cached_styles["appdrawer_preview_container"]

        self.preview_container = create_preview_container(
            self.container,
            width=344,
            height=572,
            top_offset=125,
            style=container_style,
        )

        self.lockscreen_preview = create_preview_image(
            self.preview_container,
            target_size=(344, 572),
        )
        self.preview_mask = create_top_mask(self.preview_container, height=5)

        if self.selected_wallpaper:
            self.current_wallpaper_path = self.selected_wallpaper
            self.lockscreen_preview.set_src(self.selected_wallpaper)
        else:
            lockscreen_path = storage_device.get_homescreen()
            if lockscreen_path:
                try:
                    if lockscreen_path.startswith("A:/res/wallpapers/"):
                        file_path = lockscreen_path.replace(
                            "A:/res/wallpapers/", "1:/res/wallpapers/"
                        )
                    elif lockscreen_path.startswith("A:/res/"):
                        file_path = lockscreen_path[2:]
                    elif lockscreen_path.startswith("A:1:/"):
                        file_path = lockscreen_path[2:]
                    else:
                        file_path = lockscreen_path

                    io.fatfs.stat(file_path)
                    self.current_wallpaper_path = lockscreen_path
                    self.lockscreen_preview.set_src(lockscreen_path)
                except Exception:
                    self.current_wallpaper_path = "A:/res/wallpaper-7.jpg"
                    self.lockscreen_preview.set_src("A:/res/wallpaper-7.jpg")
                    storage_device.set_homescreen("A:/res/wallpaper-7.jpg")
            else:
                self.current_wallpaper_path = "A:/res/wallpaper-7.jpg"
                self.lockscreen_preview.set_src("A:/res/wallpaper-7.jpg")

        device_name = storage_device.get_label() or "OneKey Pro"
        ble_name = storage_device.get_ble_name() or uart.get_ble_name()

        self.device_name_label = lv.label(self.preview_container)
        self.device_name_label.set_text(device_name)

        self.device_name_label.add_style(
            StyleWrapper()
            .text_font(font_GeistSemiBold38)
            .text_color(lv_colors.WHITE)
            .text_align(lv.TEXT_ALIGN.CENTER),
            0,
        )

        self.device_name_label.align_to(self.preview_container, lv.ALIGN.TOP_MID, 0, 49)

        self.bluetooth_label = lv.label(self.preview_container)
        if ble_name and len(ble_name) >= 4:
            self.bluetooth_label.set_text("Pro " + ble_name[-4:])
        else:
            self.bluetooth_label.set_text("Pro")

        self.bluetooth_label.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular26)
            .text_color(lv_colors.WHITE)
            .text_align(lv.TEXT_ALIGN.CENTER),
            0,
        )

        self.bluetooth_label.align_to(
            self.device_name_label, lv.ALIGN.OUT_BOTTOM_MID, 0, 8
        )

        self.change_button_container = lv.obj(self.container)
        self.change_button_container.set_size(120, 100)
        self.change_button_container.align_to(
            self.preview_container, lv.ALIGN.OUT_BOTTOM_MID, 0, 10
        )
        self.change_button_container.add_style(
            StyleWrapper().bg_opa(lv.OPA.TRANSP).border_width(0).pad_all(0),
            0,
        )
        self.change_button_container.add_flag(lv.obj.FLAG.CLICKABLE)
        self.change_button_container.clear_flag(lv.obj.FLAG.SCROLLABLE)

        self.change_button = lv.btn(self.change_button_container)
        self.change_button.set_size(64, 64)
        self.change_button.align(lv.ALIGN.TOP_MID, 0, 0)
        self.change_button.add_style(
            StyleWrapper().border_width(0).radius(40).bg_opa(lv.OPA.TRANSP),
            0,
        )

        self.button_icon = lv.img(self.change_button)
        self.button_icon.set_src("A:/res/change-wallper.png")
        self.button_icon.set_antialias(True)
        self.button_icon.align(lv.ALIGN.CENTER, 0, 0)

        self.change_label = lv.label(self.change_button_container)
        self.change_label.set_text(_(i18n_keys.BUTTON__CHANGE))
        self.change_label.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular20)
            .text_color(lv_colors.WHITE)
            .text_align(lv.TEXT_ALIGN.CENTER),
            0,
        )
        self.change_label.align_to(self.change_button, lv.ALIGN.OUT_BOTTOM_MID, 0, 4)
        self.change_label.add_flag(lv.obj.FLAG.CLICKABLE)
        self.change_label.add_event_cb(self.on_select_clicked, lv.EVENT.CLICKED, None)

        self.change_button.add_event_cb(self.on_select_clicked, lv.EVENT.CLICKED, None)
        self.change_button_container.add_event_cb(
            self.on_select_clicked, lv.EVENT.CLICKED, None
        )

        def _on_button_icon_clicked(e):
            WallperChange(self)

        self.button_icon.add_flag(lv.obj.FLAG.CLICKABLE)
        self.button_icon.add_event_cb(_on_button_icon_clicked, lv.EVENT.CLICKED, None)

        loop.schedule(self._first_frame_fix())
        gc.collect()

    def on_select_clicked(self, event_obj):
        target = event_obj.get_target()
        if target in [
            self.change_button_container,
            self.change_button,
            self.change_label,
        ]:
            WallperChange(self)

    def on_wallpaper_clicked(self, event_obj):
        self.cycle_wallpaper()

    def cycle_wallpaper(self):
        wallpapers = [
            "A:/res/wallpaper-1.jpg",
            "A:/res/wallpaper-2.jpg",
            "A:/res/wallpaper-3.jpg",
            "A:/res/wallpaper-4.jpg",
        ]

        current_src = self.lockscreen_preview.get_src()
        try:
            current_index = wallpapers.index(current_src)
            next_index = (current_index + 1) % len(wallpapers)
        except ValueError:
            next_index = 0

        self.lockscreen_preview.set_src(wallpapers[next_index])

    def refresh_text(self):

        if hasattr(self, "lockscreen_preview") and hasattr(
            self, "current_wallpaper_path"
        ):
            if self.current_wallpaper_path:
                self.lockscreen_preview.set_src(self.current_wallpaper_path)

        if hasattr(self, "container"):
            self.container.invalidate()

        self.invalidate()

    async def _first_frame_fix(self):
        utime.sleep_ms(100)

        self.refresh()
        if hasattr(self, "container") and self.container:
            self.container.invalidate()
        if hasattr(self, "preview_container") and self.preview_container:
            self.preview_container.invalidate()
        if hasattr(self, "lockscreen_preview") and self.lockscreen_preview:
            self.lockscreen_preview.invalidate()

    def __del__(self):

        if hasattr(utils, "SCREENS") and self in utils.SCREENS:
            utils.SCREENS.remove(self)

        if hasattr(self, "container") and self.container:
            self.container.delete()

        _clear_preview_cache()

    def eventhandler(self, event_obj):
        event = event_obj.code
        target = event_obj.get_target()

        if event == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return

            if isinstance(target, lv.imgbtn):
                if hasattr(self, "nav_back") and target == self.nav_back.nav_btn:
                    if self.prev_scr is not None:
                        self._return_to_previous_screen()
                    return
                elif hasattr(self, "rti_btn") and target == self.rti_btn:
                    self.on_click_ext(target)
                    return

            if hasattr(self, "nav_back") and target == self.nav_back:
                if self.prev_scr is not None:
                    self._return_to_previous_screen()
                return

            if isinstance(target, (lv.btn, lv.imgbtn)) and not hasattr(
                target, "_processed"
            ):
                super().eventhandler(event_obj)

    def on_click_ext(self, target):
        if hasattr(self, "rti_btn") and target == self.rti_btn:
            current_wallpaper = getattr(self, "current_wallpaper_path", None)
            if current_wallpaper:
                apply_lock_wallpaper(current_wallpaper)

            if self.prev_scr is not None:
                self._return_to_previous_screen()

    def _return_to_previous_screen(self):
        if not self.prev_scr:
            return
        _clear_preview_cache()
        if getattr(self, "_return_to_prev_instance", False):
            try:
                self._load_scr(self.prev_scr, back=True)
            except Exception:
                self.load_screen(self.prev_scr, destroy_self=True)
                return
            utils.try_remove_scr(self)
            if (
                hasattr(self.__class__, "_instance")
                and self.__class__._instance is self
            ):
                del self.__class__._instance
            self.del_delayed(100)
            gc.collect()
        else:
            self.load_screen(self.prev_scr, destroy_self=True)


class WallperChange(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
            super().__init__(
                prev_scr=prev_scr,
                title=_(i18n_keys.TITLE__CHANGE_WALLPAPER),
                nav_back=True,
            )
        else:
            if hasattr(self, "container"):
                self.container.delete()
            if hasattr(self, "custom_header_container"):
                self.custom_header_container.delete()
            if prev_scr is not None:
                self.prev_scr = prev_scr

        self.edit_mode = False
        self.marked_for_deletion = set()
        self.selected_wallpapers = set()

        file_name_list = []
        if not utils.EMULATOR:
            for size, _attrs, name in io.fatfs.listdir("1:/res/wallpapers"):
                if (
                    size > 0
                    and name.startswith("wp-")
                    and not name.endswith("-blur.jpeg")
                    and not name.endswith("-blur.jpg")
                ):
                    file_name_list.append(name)

        if len(file_name_list) > 0 and storage_device.get_wp_cnts() == 0:
            storage_device.increase_wp_cnts()

        if file_name_list:

            def extract_timestamp(filename):
                try:
                    parts = filename.rsplit("-", 1)
                    if len(parts) == 2:
                        timestamp_str = parts[1].split(".")[0]
                        return int(timestamp_str)
                except (ValueError, IndexError):
                    return 0
                return 0

            file_name_list.sort(key=extract_timestamp, reverse=True)
            file_name_list = file_name_list[:5]

        internal_wp_nums = 7
        custom_wp_nums = len(file_name_list)

        custom_rows = math.ceil(custom_wp_nums / 3) if custom_wp_nums > 0 else 0
        pro_rows = math.ceil(internal_wp_nums / 3)

        row_dsc = []
        if custom_rows > 0:
            row_dsc.extend([GRID_CELL_SIZE_ROWS] * custom_rows)
            row_dsc.append(60)
        else:
            row_dsc.append(180)
            row_dsc.append(60)
        row_dsc.extend([GRID_CELL_SIZE_ROWS] * pro_rows)
        row_dsc.append(lv.GRID_TEMPLATE.LAST)

        col_dsc = [
            GRID_CELL_SIZE_COLS,
            GRID_CELL_SIZE_COLS,
            GRID_CELL_SIZE_COLS,
            lv.GRID_TEMPLATE.LAST,
        ]

        self.container = ContainerGrid(
            self.content_area,
            row_dsc=row_dsc,
            col_dsc=col_dsc,
            pad_gap=12,
            pad_hor=0,
        )
        self.container.set_width(lv.pct(100))
        self.container.align(lv.ALIGN.TOP_MID, 0, 196)

        self.container.add_flag(lv.obj.FLAG.EVENT_BUBBLE)

        current_row = 0

        self.custom_header_container = lv.obj(self.content_area)
        self.custom_header_container.set_size(lv.pct(100), 60)
        self.custom_header_container.clear_flag(lv.obj.FLAG.SCROLLABLE)
        self.custom_header_container.set_style_bg_opa(lv.OPA.TRANSP, 0)
        self.custom_header_container.set_style_border_opa(lv.OPA.TRANSP, 0)
        self.custom_header_container.set_style_pad_hor(0, 0)
        self.custom_header_container.set_style_pad_ver(0, 0)
        self.custom_header_container.set_style_clip_corner(False, 0)
        self.custom_header_container.align(lv.ALIGN.TOP_MID, 0, 136)

        self.custom_header = lv.label(self.custom_header_container)
        self.custom_header.set_text(_(i18n_keys.OPTION__CUSTOM__INSERT))
        self.custom_header.add_style(
            StyleWrapper()
            .text_font(font_GeistSemiBold30)
            .text_color(lv_colors.WHITE)
            .text_align(lv.TEXT_ALIGN.LEFT),
            0,
        )
        self.custom_header.align(lv.ALIGN.LEFT_MID, 7, 0)

        if file_name_list:
            btn_style = (
                StyleWrapper()
                .bg_opa(lv.OPA.TRANSP)
                .border_opa(lv.OPA.TRANSP)
                .pad_ver(5)
            )

            self.edit_button = lv.btn(self.custom_header_container)
            self.edit_button.set_size(lv.SIZE.CONTENT, 60)
            self.edit_button.add_style(btn_style.pad_left(12).pad_right(0), 0)
            self.edit_button.align(lv.ALIGN.RIGHT_MID, -12, 0)
            self.edit_button_icon = lv.img(self.edit_button)
            self.edit_button_icon.set_src("A:/res/edit.png")
            self.edit_button_icon.center()
            self.edit_button.add_event_cb(
                self.on_edit_button_clicked, lv.EVENT.CLICKED, None
            )

            self.done_button = lv.btn(self.custom_header_container)
            self.done_button.set_size(lv.SIZE.CONTENT, 60)
            self.done_button.add_style(btn_style.pad_left(12).pad_right(0), 0)
            self.done_button.align(lv.ALIGN.RIGHT_MID, -12, 0)
            self.done_button.add_flag(lv.obj.FLAG.HIDDEN)
            self.done_button_icon = lv.img(self.done_button)
            self.done_button_icon.set_src("A:/res/checkmark.png")
            self.done_button_icon.center()
            self.done_button.add_event_cb(
                self.on_done_button_clicked, lv.EVENT.CLICKED, None
            )

            self.delete_button = lv.btn(self.custom_header_container)
            self.delete_button.set_size(lv.SIZE.CONTENT, 60)
            self.delete_button.add_style(btn_style.pad_hor(8), 0)
            self.delete_button.add_flag(lv.obj.FLAG.HIDDEN)
            self.delete_button_icon = lv.img(self.delete_button)
            self.delete_button_icon.set_src("A:/res/delete.png")
            self.delete_button_icon.center()
            self.delete_button.add_event_cb(
                self.on_delete_button_clicked, lv.EVENT.CLICKED, None
            )

        self.wps = []
        self.custom_wps = []
        if file_name_list:
            for i, file_name in enumerate(file_name_list):
                path_dir = "A:1:/res/wallpapers/"
                zoom_file_name = f"zoom-{file_name}"
                current_wp = ImgGridItem(
                    self.container,
                    i % 3,
                    current_row + (i // 3),
                    zoom_file_name,
                    path_dir,
                    img_path_unselected=None,
                    is_internal=True,
                    style_type="wallpaper",
                )
                self.wps.append(current_wp)
                self.custom_wps.append(current_wp)

                selection_checkbox = lv.btn(current_wp)
                selection_checkbox.set_size(32, 32)
                selection_checkbox.clear_flag(lv.obj.FLAG.SCROLLABLE)
                selection_checkbox.add_flag(lv.obj.FLAG.HIDDEN)
                selection_checkbox.add_flag(lv.obj.FLAG.CLICKABLE)

                selection_checkbox.set_style_bg_opa(lv.OPA.TRANSP, 0)
                selection_checkbox.set_style_border_opa(lv.OPA.TRANSP, 0)
                selection_checkbox.set_style_shadow_opa(lv.OPA.TRANSP, 0)
                selection_checkbox.set_style_clip_corner(True, 0)
                selection_checkbox.set_style_pad_all(0, 0)

                selection_checkbox_img = lv.img(selection_checkbox)
                selection_checkbox_img.set_src("A:/res/unselect.png")
                selection_checkbox_img.set_size(32, 32)
                selection_checkbox_img.center()
                selection_checkbox_img.clear_flag(lv.obj.FLAG.CLICKABLE)

                selection_checkbox.align(lv.ALIGN.TOP_RIGHT, -12, 12)
                selection_checkbox.move_foreground()

                selection_checkbox.add_event_cb(
                    lambda e, wp=current_wp: self.on_selection_checkbox_clicked(e, wp),
                    lv.EVENT.CLICKED,
                    None,
                )

                current_wp.selection_checkbox = selection_checkbox
                current_wp.selection_checkbox_img = selection_checkbox_img
                current_wp.is_selected = False

                remove_icon = lv.btn(self.container)
                remove_icon.set_size(44, 44)
                remove_icon.clear_flag(lv.obj.FLAG.SCROLLABLE)
                remove_icon.add_flag(lv.obj.FLAG.HIDDEN)
                remove_icon.add_flag(lv.obj.FLAG.CLICKABLE)

                remove_icon.set_style_bg_opa(lv.OPA.TRANSP, 0)
                remove_icon.set_style_border_opa(lv.OPA.TRANSP, 0)
                remove_icon.set_style_shadow_opa(lv.OPA.TRANSP, 0)

                remove_icon_img = lv.img(remove_icon)
                remove_icon_img.set_src("A:/res/remove_icon.png")
                remove_icon_img.set_size(40, 40)
                remove_icon_img.set_antialias(True)
                remove_icon_img.center()
                remove_icon_img.clear_flag(lv.obj.FLAG.CLICKABLE)

                remove_icon.align_to(current_wp, lv.ALIGN.TOP_RIGHT, 14, -14)
                remove_icon.move_foreground()

                remove_icon.add_event_cb(
                    lambda e, wp=current_wp: self.on_remove_icon_clicked(e, wp),
                    lv.EVENT.CLICKED,
                    None,
                )

                current_wp.remove_icon = remove_icon

                current_wp.add_event_cb(
                    lambda e, wp=current_wp: self.on_custom_wallpaper_clicked(e, wp),
                    lv.EVENT.CLICKED,
                    None,
                )

            current_row += custom_rows
        else:
            self.empty_state_container = lv.obj(self.container)
            self.empty_state_container.set_size(lv.pct(100), lv.SIZE.CONTENT)
            self.empty_state_container.clear_flag(lv.obj.FLAG.SCROLLABLE)
            self.empty_state_container.set_style_bg_opa(lv.OPA.TRANSP, 0)
            self.empty_state_container.set_style_border_opa(lv.OPA.TRANSP, 0)
            self.empty_state_container.set_style_pad_all(0, 0)
            self.empty_state_container.set_grid_cell(
                lv.GRID_ALIGN.STRETCH, 0, 3, lv.GRID_ALIGN.START, current_row, 1
            )

            self.empty_title = lv.label(self.empty_state_container)
            self.empty_title.set_text(_(i18n_keys.TITLE__ADD_WALLPAPER_FROM_ONEKEY_APP))
            self.empty_title.set_long_mode(lv.label.LONG.WRAP)
            self.empty_title.set_size(lv.pct(100), lv.SIZE.CONTENT)
            self.empty_title.add_style(
                StyleWrapper()
                .text_font(font_GeistSemiBold26)
                .text_color(lv_colors.WHITE)
                .text_align(lv.TEXT_ALIGN.LEFT)
                .text_letter_space(-1),
                0,
            )
            self.empty_title.align(lv.ALIGN.TOP_LEFT, 3, 10)
            self.empty_desc = lv.label(self.empty_state_container)
            self.empty_desc.set_text(
                _(i18n_keys.TITLE__ADD_WALLPAPER_FROM_ONEKEY_APP_DESC)
            )
            self.empty_desc.set_long_mode(lv.label.LONG.WRAP)
            self.empty_desc.set_size(lv.pct(100), lv.SIZE.CONTENT)
            self.empty_desc.add_style(
                StyleWrapper()
                .text_font(font_GeistRegular26)
                .text_color(lv_colors.ONEKEY_GRAY_1)
                .text_align(lv.TEXT_ALIGN.LEFT)
                .text_letter_space(-2)
                .text_line_space(4),
                0,
            )
            self.empty_desc.align_to(self.empty_title, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 16)

            current_row += 1

        self.pro_header = lv.label(self.container)
        self.pro_header.set_text(_(i18n_keys.TITLE__COLLECTION))
        self.pro_header.add_style(
            StyleWrapper()
            .text_font(font_GeistSemiBold30)
            .text_color(lv_colors.WHITE)
            .text_align(lv.TEXT_ALIGN.LEFT),
            0,
        )
        self.pro_header.set_grid_cell(
            lv.GRID_ALIGN.START, 0, 3, lv.GRID_ALIGN.CENTER, current_row, 1
        )
        self.pro_header.set_x(8)
        current_row += 1

        for i in range(internal_wp_nums):
            path_dir = "A:/res/"
            file_name = f"zoom-wallpaper-{i+1}.jpg"

            current_wp = ImgGridItem(
                self.container,
                i % 3,
                current_row + (i // 3),
                file_name,
                path_dir,
                img_path_unselected=None,
                is_internal=True,
                style_type="wallpaper",
            )
            self.wps.append(current_wp)

        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

        self.remove_event_cb(None)
        self.add_event_cb(self.eventhandler, lv.EVENT.CLICKED, None)

        try:
            loop.schedule(self._refresh_previews_after_load())
        except (ImportError, AttributeError):
            self._refresh_previews_immediate()

        gc.collect()

    async def _refresh_previews_after_load(self):
        utime.sleep_ms(10)
        self._refresh_previews_immediate()

    def _refresh_previews_immediate(self):
        if hasattr(self, "wps"):
            for wp in self.wps:
                wp.invalidate()

        if hasattr(self, "container"):
            self.container.invalidate()
            self.container.clear_flag(lv.obj.FLAG.HIDDEN)

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if getattr(self, "_pending_custom_wallpaper_click", None):
                self._pending_custom_wallpaper_click = None
                return
            if utils.lcd_resume():
                return

            if self.edit_mode and hasattr(self, "custom_wps"):
                for _i, wp in enumerate(self.custom_wps):
                    if hasattr(wp, "remove_icon"):
                        if target == wp.remove_icon or (
                            hasattr(wp.remove_icon, "get_child")
                            and target == wp.remove_icon.get_child(0)
                        ):
                            self.on_remove_icon_clicked(event_obj, wp)
                            return

            if self.edit_mode:
                return

            if not hasattr(self, "wps") or target not in self.wps:
                return
            for wp in self.wps:
                if target == wp:
                    if hasattr(self.prev_scr, "__class__"):
                        if self.prev_scr.__class__.__name__ == "HomeScreenSetting":
                            current_blur_state = getattr(
                                self.prev_scr, "is_blur_active", False
                            )
                            current_wallpaper = getattr(
                                self.prev_scr, "current_wallpaper_path", ""
                            )
                            selected_wallpaper_base = (
                                wp.img_path.replace("zoom-", "")
                                .replace("A:/res/", "")
                                .replace("A:1:/res/", "")
                            )
                            current_wallpaper_base = (
                                current_wallpaper.replace("A:/res/", "")
                                .replace("A:1:/res/", "")
                                .replace("-blur", "")
                            )

                            if selected_wallpaper_base == current_wallpaper_base:

                                try:
                                    self._load_scr(self.prev_scr, back=True)
                                    utils.try_remove_scr(self)
                                    if hasattr(self.__class__, "_instance"):
                                        del self.__class__._instance
                                    self.del_delayed(100)
                                except Exception:
                                    fallback_screen = WallpaperScreen()
                                    self._load_scr(fallback_screen, back=True)
                                    utils.try_remove_scr(self)
                                    if hasattr(self.__class__, "_instance"):
                                        del self.__class__._instance
                                    self.del_delayed(100)
                            else:
                                try:
                                    if hasattr(HomeScreenSetting, "_instance"):
                                        old_instance = HomeScreenSetting._instance
                                        if (
                                            hasattr(utils, "SCREENS")
                                            and old_instance in utils.SCREENS
                                        ):
                                            utils.SCREENS.remove(old_instance)

                                        old_instance.delete()

                                        if hasattr(old_instance, "_init"):
                                            delattr(old_instance, "_init")

                                        del HomeScreenSetting._instance
                                        gc.collect()

                                    new_screen = HomeScreenSetting(
                                        self.prev_scr.prev_scr,
                                        selected_wallpaper=wp.img_path,
                                        preserve_blur_state=current_blur_state,
                                        return_from_wallpaper=True,
                                    )
                                    self._load_scr(new_screen, back=True)

                                    utils.try_remove_scr(self)
                                    if hasattr(self.__class__, "_instance"):
                                        del self.__class__._instance
                                    self.del_delayed(100)
                                except Exception:
                                    fallback_screen = WallpaperScreen()
                                    self._load_scr(fallback_screen, back=True)
                                    utils.try_remove_scr(self)
                                    if hasattr(self.__class__, "_instance"):
                                        del self.__class__._instance
                                    self.del_delayed(100)
                        elif (
                            self.prev_scr.__class__.__name__
                            == "AppdrawerBackgroundSetting"
                        ):
                            try:
                                self.prev_scr.selected_wallpaper = wp.img_path
                                self.prev_scr.current_wallpaper_path = wp.img_path

                                if hasattr(self.prev_scr, "lockscreen_preview"):
                                    self.prev_scr.lockscreen_preview.set_src(
                                        wp.img_path
                                    )

                                if hasattr(self.prev_scr, "refresh_text"):
                                    self.prev_scr.refresh_text()
                                if hasattr(self.prev_scr, "invalidate"):
                                    self.prev_scr.invalidate()

                                self._load_scr(self.prev_scr, back=True)

                                utils.try_remove_scr(self)
                                if hasattr(self.__class__, "_instance"):
                                    del self.__class__._instance
                                self.del_delayed(100)
                            except Exception:
                                try:
                                    AppdrawerBackgroundSetting._dispose_existing(
                                        "recreate after wallpaper selection fallback"
                                    )
                                    new_screen = AppdrawerBackgroundSetting(
                                        self.prev_scr.prev_scr,
                                        selected_wallpaper=wp.img_path,
                                        return_from_wallpaper=True,
                                    )
                                    self._load_scr(new_screen, back=True)
                                    utils.try_remove_scr(self)
                                    if hasattr(self.__class__, "_instance"):
                                        del self.__class__._instance
                                    self.del_delayed(100)
                                except Exception:
                                    fallback_screen = WallpaperScreen()
                                    self._load_scr(fallback_screen, back=True)
                                    utils.try_remove_scr(self)
                                    if hasattr(self.__class__, "_instance"):
                                        del self.__class__._instance
                                    self.del_delayed(100)
                        else:
                            try:
                                self._load_scr(self.prev_scr, back=True)
                                utils.try_remove_scr(self)
                                if hasattr(self.__class__, "_instance"):
                                    del self.__class__._instance
                                self.del_delayed(100)
                            except Exception:
                                fallback_screen = WallpaperScreen()
                                self._load_scr(fallback_screen, back=True)
                                utils.try_remove_scr(self)
                                if hasattr(self.__class__, "_instance"):
                                    del self.__class__._instance
                                self.del_delayed(100)

    def on_select_clicked(self, event_obj):
        target = event_obj.get_target()
        if target in [
            self.change_button_container,
            self.change_button,
            self.change_label,
        ]:
            WallperChange(self)

    def on_wallpaper_clicked(self, event_obj):
        self.cycle_wallpaper()

    def cycle_wallpaper(self):
        wallpapers = [
            "A:/res/wallpaper-1.jpg",
            "A:/res/wallpaper-2.jpg",
            "A:/res/wallpaper-3.jpg",
            "A:/res/wallpaper-4.jpg",
        ]

        current_src = self.lockscreen_preview.get_src()
        try:
            current_index = wallpapers.index(current_src)
            next_index = (current_index + 1) % len(wallpapers)
        except ValueError:
            next_index = 0

        self.lockscreen_preview.set_src(wallpapers[next_index])

    def refresh_text(self):
        pass

    def on_edit_button_clicked(self, event_obj):
        if self.edit_mode or not getattr(self, "custom_wps", None):
            return

        self._enter_edit_mode()

    def on_delete_button_clicked(self, event_obj):
        if not self.edit_mode or not self.selected_wallpapers:
            return

        self.marked_for_deletion = self.selected_wallpapers.copy()
        self.delete_marked_files()

    def on_done_button_clicked(self, event_obj):
        if not self.edit_mode:
            return

        self._exit_edit_mode(commit=False)

    def _enter_edit_mode(self):
        if self.edit_mode:
            return

        self.edit_mode = True
        self._pending_custom_wallpaper_click = None

        if hasattr(self, "edit_button"):
            self.edit_button.add_flag(lv.obj.FLAG.HIDDEN)
        if hasattr(self, "done_button"):
            self.done_button.clear_flag(lv.obj.FLAG.HIDDEN)
        if hasattr(self, "delete_button"):
            self.delete_button.clear_flag(lv.obj.FLAG.HIDDEN)
            self.delete_button.align_to(self.done_button, lv.ALIGN.OUT_LEFT_MID, -8, 0)

        for _i, wp in enumerate(self.custom_wps):
            if hasattr(wp, "selection_checkbox"):
                wp.selection_checkbox.clear_flag(lv.obj.FLAG.HIDDEN)
                wp.selection_checkbox.move_foreground()
                wp.is_selected = False
                wp.selection_checkbox_img.set_src("A:/res/unselect.png")

        self.selected_wallpapers.clear()

    def _exit_edit_mode(self, *, commit: bool):
        if not self.edit_mode:
            return

        self.edit_mode = False
        self._pending_custom_wallpaper_click = None

        if hasattr(self, "edit_button"):
            self.edit_button.clear_flag(lv.obj.FLAG.HIDDEN)
        if hasattr(self, "delete_button"):
            self.delete_button.add_flag(lv.obj.FLAG.HIDDEN)
        if hasattr(self, "done_button"):
            self.done_button.add_flag(lv.obj.FLAG.HIDDEN)

        for wp in self.custom_wps:
            if hasattr(wp, "selection_checkbox"):
                wp.selection_checkbox.add_flag(lv.obj.FLAG.HIDDEN)

        if not commit:
            self.selected_wallpapers.clear()
            return

        if self.selected_wallpapers:
            self.marked_for_deletion = self.selected_wallpapers.copy()
            self.delete_marked_files()
            self.selected_wallpapers.clear()

    def _get_active_wallpaper_path(self):
        if hasattr(self.prev_scr, "current_wallpaper_path"):
            current = getattr(self.prev_scr, "current_wallpaper_path", None)
            if current:
                return current

        if hasattr(self.prev_scr, "selected_wallpaper"):
            selected = getattr(self.prev_scr, "selected_wallpaper", None)
            if selected:
                return selected

        try:
            return storage_device.get_homescreen()
        except Exception:
            return None

    def _normalize_wallpaper_path(self, path):
        if not path:
            return None

        normalized = path
        if normalized.startswith("A:1:/"):
            normalized = normalized[5:]
        elif normalized.startswith("A:/"):
            normalized = normalized[3:]
        elif normalized.startswith("1:/"):
            normalized = normalized[3:]
        normalized = normalized.lstrip("/")
        normalized = normalized.replace("zoom-", "")
        normalized = normalized.replace("-blur", "")
        return normalized

    def on_custom_wallpaper_clicked(self, event_obj, wallpaper):
        if event_obj.code != lv.EVENT.CLICKED or not self.edit_mode:
            return

        if utils.lcd_resume():
            return

        if wallpaper not in getattr(self, "custom_wps", []):
            return

        self._pending_custom_wallpaper_click = wallpaper
        self.on_selection_checkbox_clicked(event_obj, wallpaper)

    def on_selection_checkbox_clicked(self, event_obj, wallpaper):
        if not self.edit_mode:
            return

        wallpaper.is_selected = not wallpaper.is_selected

        if wallpaper.is_selected:
            self.selected_wallpapers.add(wallpaper)
            wallpaper.selection_checkbox_img.set_src("A:/res/selected.png")
        else:
            self.selected_wallpapers.discard(wallpaper)
            wallpaper.selection_checkbox_img.set_src("A:/res/unselect.png")

    def on_remove_icon_clicked(self, event_obj, wallpaper):

        self.marked_for_deletion.add(wallpaper)

        wallpaper.add_flag(lv.obj.FLAG.HIDDEN)

        if hasattr(wallpaper, "remove_icon"):
            wallpaper.remove_icon.add_flag(lv.obj.FLAG.HIDDEN)

    def delete_marked_files(self):
        marked_count = len(self.marked_for_deletion)

        for wallpaper in self.marked_for_deletion:
            img_path = wallpaper.img_path
            if "A:1:/res/wallpapers/" in img_path:
                filename = img_path.replace("A:1:/res/wallpapers/", "")

                original_path = f"1:/res/wallpapers/{filename}"
                io.fatfs.unlink(original_path)

                zoom_filename = f"zoom-{filename}"
                zoom_path = f"1:/res/wallpapers/{zoom_filename}"

                io.fatfs.unlink(zoom_path)

                if "." in filename:
                    name_part, ext_part = filename.rsplit(".", 1)
                    blur_filename = f"{name_part}-blur.{ext_part}"
                else:
                    blur_filename = f"{filename}-blur"
                blur_path = f"1:/res/wallpapers/{blur_filename}"
                io.fatfs.unlink(blur_path)

                self.replace_if_in_use(img_path)

        self.marked_for_deletion.clear()

        for _i in range(marked_count):
            storage_device.decrease_wp_cnts()

        self.__init__(self.prev_scr)

    def replace_if_in_use(self, deleted_path):
        replace_wallpaper_if_in_use(deleted_path, "A:/res/wallpaper-7.jpg")

    def eventhandler(self, event_obj):
        event = event_obj.code
        target = event_obj.get_target()
        if event == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if isinstance(target, lv.imgbtn):
                if hasattr(self, "nav_back") and target == self.nav_back.nav_btn:
                    if self.prev_scr is not None:
                        try:
                            if (
                                hasattr(self.prev_scr, "__class__")
                                and self.prev_scr.__class__.__name__
                                == "AppdrawerBackgroundSetting"
                            ):
                                self.load_screen(self.prev_scr, destroy_self=True)
                            elif (
                                hasattr(self.prev_scr, "__class__")
                                and self.prev_scr.__class__.__name__
                                == "HomeScreenSetting"
                            ):
                                self.load_screen(self.prev_scr, destroy_self=True)
                            else:
                                self.load_screen(self.prev_scr, destroy_self=True)
                        except Exception:
                            try:
                                self._load_scr(self.prev_scr, back=True)
                                self.del_delayed(100)
                            except Exception:
                                from .homescreen import SettingsScreen

                                settings_screen = SettingsScreen()
                                self._load_scr(settings_screen, back=True)
                                self.del_delayed(100)

                    return
                elif hasattr(self, "rti_btn") and target == self.rti_btn:
                    self.on_click_ext(target)
                    return

        if event == lv.EVENT.CLICKED:
            self.on_click(event_obj)


class ShutdownSetting(AnimScreen):
    cur_auto_shutdown = ""
    cur_auto_shutdown_ms = 0

    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        return targets

    def __init__(self, prev_scr=None):
        ShutdownSetting.cur_auto_shutdown_ms = (
            storage_device.get_autoshutdown_delay_ms()
        )
        ShutdownSetting.cur_auto_shutdown = self.get_str_from_ms(
            ShutdownSetting.cur_auto_shutdown_ms
        )
        if not hasattr(self, "_init"):
            self._init = True
        else:
            if self.cur_auto_shutdown:
                self.auto_shutdown.label_right.set_text(
                    ShutdownSetting.cur_auto_shutdown
                )
            self.refresh_text()
            return

        super().__init__(prev_scr=prev_scr, title="自动关机", nav_back=True)

        self.container = ContainerFlexCol(
            self.content_area, self.title, padding_row=2, pos=(0, 40)
        )

        self.auto_shutdown = ListItemBtn(
            self.container,
            "自动关机",
            ShutdownSetting.cur_auto_shutdown,
        )

        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.load_screen(self)
        gc.collect()

    def get_str_from_ms(self, delay_ms: int) -> str:
        if delay_ms in (0, storage_device.AUTOSHUTDOWN_DELAY_MAXIMUM):
            return "从不"
        elif delay_ms < 60000:
            return f"{delay_ms // 1000}秒"
        elif delay_ms < 3600000:
            return f"{delay_ms // 60000}分钟"
        else:
            return f"{delay_ms // 3600000}小时"

    def refresh_text(self):
        self.title.set_text("自动关机")
        self.auto_shutdown.label_left.set_text("自动关机")

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.auto_shutdown:
                # Use the direct auto-shutdown setting screen
                AutoShutDownSetting(self)


def get_autolock_delay_str() -> str:
    delay_ms = storage_device.get_autolock_delay_ms()

    if delay_ms in (0, storage_device.AUTOLOCK_DELAY_MAXIMUM):
        result = _(i18n_keys.OPTION__NEVER)
    elif delay_ms < 60000:
        seconds = delay_ms // 1000
        result = _(i18n_keys.OPTION__STR_SECONDS).format(seconds)
    elif delay_ms < 3600000:
        minutes = delay_ms // 60000
        result = _(
            i18n_keys.OPTION__STR_MINUTE
            if minutes == 1
            else i18n_keys.OPTION__STR_MINUTES
        ).format(minutes)
    else:
        hours = delay_ms // 3600000
        result = _(
            i18n_keys.OPTION__STR_HOUR if hours == 1 else i18n_keys.OPTION__STR_HOURS
        ).format(hours)

    return result


def get_autoshutdown_delay_str() -> str:
    delay_ms = storage_device.get_autoshutdown_delay_ms()
    if delay_ms in (0, storage_device.AUTOSHUTDOWN_DELAY_MAXIMUM):
        return _(i18n_keys.OPTION__NEVER)
    elif delay_ms < 60000:
        seconds = delay_ms // 1000
        return _(i18n_keys.OPTION__STR_SECONDS).format(seconds)
    elif delay_ms < 3600000:
        minutes = delay_ms // 60000
        return _(
            i18n_keys.OPTION__STR_MINUTE
            if minutes == 1
            else i18n_keys.OPTION__STR_MINUTES
        ).format(minutes)
    else:
        hours = delay_ms // 3600000
        return _(
            i18n_keys.OPTION__STR_HOUR if hours == 1 else i18n_keys.OPTION__STR_HOURS
        ).format(hours)


class Autolock_and_ShutingDown(AnimScreen):
    cur_auto_lock = ""
    cur_auto_lock_ms = 0
    cur_auto_shutdown = ""
    cur_auto_shutdown_ms = 0

    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        return targets

    def __init__(self, prev_scr=None):
        Autolock_and_ShutingDown.cur_auto_lock_ms = (
            storage_device.get_autolock_delay_ms()
        )
        Autolock_and_ShutingDown.cur_auto_shutdown_ms = (
            storage_device.get_autoshutdown_delay_ms()
        )
        Autolock_and_ShutingDown.cur_auto_lock = self.get_str_from_ms(
            Autolock_and_ShutingDown.cur_auto_lock_ms
        )
        Autolock_and_ShutingDown.cur_auto_shutdown = self.get_str_from_ms(
            Autolock_and_ShutingDown.cur_auto_shutdown_ms
        )

        if not hasattr(self, "_init"):
            self._init = True
        else:
            if self.cur_auto_lock:
                self.auto_lock.label_right.set_text(
                    Autolock_and_ShutingDown.cur_auto_lock
                )
            if self.cur_auto_shutdown:
                self.auto_shutdown.label_right.set_text(
                    Autolock_and_ShutingDown.cur_auto_shutdown
                )
            self.refresh_text()
            return

        super().__init__(
            prev_scr=prev_scr,
            title=_(i18n_keys.ITEM__AUTO_LOCK_AND_SHUTDOWN),
            nav_back=True,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.auto_lock = ListItemBtn(
            self.container, _(i18n_keys.ITEM__AUTO_LOCK), self.cur_auto_lock
        )
        self.auto_shutdown = ListItemBtn(
            self.container, _(i18n_keys.ITEM__SHUTDOWN), self.cur_auto_shutdown
        )
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.load_screen(self)
        gc.collect()

    def refresh_text(self):
        self.title.set_text(_(i18n_keys.ITEM__AUTO_LOCK_AND_SHUTDOWN))
        self.auto_lock.label_left.set_text(_(i18n_keys.ITEM__AUTO_LOCK))
        self.auto_shutdown.label_left.set_text(_(i18n_keys.ITEM__SHUTDOWN))

    def get_str_from_ms(self, time_ms) -> str:

        if time_ms == storage_device.AUTOLOCK_DELAY_MAXIMUM:
            text = _(i18n_keys.ITEM__STATUS__NEVER)
        else:
            auto_lock_time = time_ms / 1000 // 60
            if auto_lock_time > 60:
                value = str(auto_lock_time // 60).split(".")[0]
                text = _(
                    i18n_keys.OPTION__STR_HOUR
                    if value == "1"
                    else i18n_keys.OPTION__STR_HOURS
                ).format(value)
            elif auto_lock_time < 1:
                value = str(time_ms // 1000).split(".")[0]
                text = _(i18n_keys.OPTION__STR_SECONDS).format(value)
            else:
                value = str(auto_lock_time).split(".")[0]
                text = _(
                    i18n_keys.OPTION__STR_MINUTE
                    if value == "1"
                    else i18n_keys.OPTION__STR_MINUTES
                ).format(value)
        return text

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.auto_lock:
                AutoLockSetting(self)
            elif target == self.auto_shutdown:
                AutoShutDownSetting(self)
            else:
                pass


# pyright: off
class AutoLockSetting(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "tips") and self.tips:
            targets.append(self.tips)
        return targets

    # TODO: i18n
    def __init__(self, prev_scr=None):
        # Initialize the class variables first
        Autolock_and_ShutingDown.cur_auto_lock_ms = (
            storage_device.get_autolock_delay_ms()
        )
        # Create a temporary instance to use get_str_from_ms method
        temp_instance = Autolock_and_ShutingDown.__new__(Autolock_and_ShutingDown)
        Autolock_and_ShutingDown.cur_auto_lock = temp_instance.get_str_from_ms(
            Autolock_and_ShutingDown.cur_auto_lock_ms
        )

        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__AUTO_LOCK), nav_back=True
        )

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.setting_items = [0.5, 1, 2, 5, 10, 30, "Never", None]
        has_custom = True
        self.checked_index = 0
        self.btns: [ListItemBtn] = [None] * (len(self.setting_items))

        for index, item in enumerate(self.setting_items):
            if item is None:
                break
            if not item == "Never":  # last item
                if item == 0.5:
                    item = _(i18n_keys.OPTION__STR_SECONDS).format(int(item * 60))
                else:
                    item = _(
                        i18n_keys.OPTION__STR_MINUTE
                        if item == 1
                        else i18n_keys.OPTION__STR_MINUTES
                    ).format(int(item))
            else:
                item = _(i18n_keys.ITEM__STATUS__NEVER)

            self.btns[index] = ListItemBtn(
                self.container, item, has_next=False, use_transition=False
            )
            self.btns[index].add_check_img()

            if item == Autolock_and_ShutingDown.cur_auto_lock:
                has_custom = False
                self.btns[index].set_checked()
                self.checked_index = index

        if has_custom:
            self.custom = storage_device.get_autolock_delay_ms()
            self.btns[-1] = ListItemBtn(
                self.container,
                f"{Autolock_and_ShutingDown.cur_auto_lock}({_(i18n_keys.OPTION__CUSTOM__INSERT)})",
                has_next=False,
                use_transition=False,
            )
            self.btns[-1].add_check_img()
            self.btns[-1].set_checked()
            self.btns[-1].label_left.add_style(
                StyleWrapper().text_font(font_GeistRegular30), 0
            )
            self.checked_index = -1
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.tips = lv.label(self.content_area)
        self.tips.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 8, 0)
        self.tips.set_long_mode(lv.label.LONG.WRAP)
        self.fresh_tips()
        self.tips.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular26)
            .width(448)
            .text_color(lv_colors.WHITE_2)
            .text_align_left()
            .text_letter_space(-1)
            .pad_ver(16),
            0,
        )
        self.load_screen(self)
        gc.collect()

    def fresh_tips(self):
        item_text = self.btns[self.checked_index].label_left.get_text()
        if self.setting_items[self.checked_index] is None:
            item_text = item_text.split("(")[0]
        if self.setting_items[self.checked_index] == "Never":
            self.tips.set_text(
                _(i18n_keys.CONTENT__SETTINGS_GENERAL_AUTO_LOCK_OFF_HINT)
            )
        else:
            self.tips.set_text(
                _(i18n_keys.CONTENT__SETTINGS_GENERAL_AUTO_LOCK_ON_HINT).format(
                    item_text or Autolock_and_ShutingDown.cur_auto_lock[:1]
                )
            )

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target in self.btns:
                for index, item in enumerate(self.btns):
                    if item == target and self.checked_index != index:
                        item.set_checked()
                        self.btns[self.checked_index].set_uncheck()
                        self.checked_index = index
                        if index == 6:
                            auto_lock_time = storage_device.AUTOLOCK_DELAY_MAXIMUM
                        elif index == 7:
                            auto_lock_time = self.custom
                        else:
                            auto_lock_time = self.setting_items[index] * 60 * 1000
                        storage_device.set_autolock_delay_ms(int(auto_lock_time))
                        Autolock_and_ShutingDown.cur_auto_lock_ms = auto_lock_time
                        self.fresh_tips()
                        from apps.base import reload_settings_from_storage

                        reload_settings_from_storage()


# pyright: on
class LanguageSetting(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__LANGUAGE), nav_back=True
        )

        self.check_index = 0
        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.lang_buttons = []
        for idx, lang in enumerate(langs):
            lang_button = ListItemBtn(
                self.container, lang[1], has_next=False, use_transition=False
            )
            lang_button.add_check_img()
            self.lang_buttons.append(lang_button)
            if GeneralScreen.cur_language == lang[1]:
                lang_button.set_checked()
                self.check_index = idx
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

        self.load_screen(self)
        gc.collect()

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            last_checked = self.check_index
            for idx, button in enumerate(self.lang_buttons):
                if target != button and idx == last_checked:
                    button.set_uncheck()
                if target == button and idx != last_checked:
                    storage_device.set_language(langs_keys[idx])
                    GeneralScreen.cur_language = langs[idx][1]
                    i18n_refresh()
                    self.title.set_text(_(i18n_keys.TITLE__LANGUAGE))
                    self.check_index = idx
                    button.set_checked()

                    # Refresh previous screen texts and language label immediately
                    if hasattr(self, "prev_scr") and self.prev_scr:
                        if hasattr(self.prev_scr, "language") and hasattr(
                            self.prev_scr.language, "label_right"
                        ):
                            self.prev_scr.language.label_right.set_text(
                                GeneralScreen.cur_language
                            )
                        if hasattr(self.prev_scr, "refresh_text"):
                            self.prev_scr.refresh_text()


class BacklightSetting(AnimScreen):
    @classmethod
    def page_is_visible(cls) -> bool:
        try:
            if cls._instance is not None and cls._instance.is_visible():
                return True
        except Exception:
            pass
        return False

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__BRIGHTNESS), nav_back=True
        )

        self.current_brightness = storage_device.get_brightness()
        self.temp_brightness = self.current_brightness
        self.container = ContainerFlexCol(self.content_area, self.title)
        self.slider = lv.slider(self.container)
        self.slider.set_size(456, 94)
        self.slider.set_ext_click_area(100)
        self.slider.set_range(style.BACKLIGHT_MIN, style.BACKLIGHT_MAX)
        self.slider.set_value(self.current_brightness, lv.ANIM.OFF)
        self.slider.add_style(
            StyleWrapper().border_width(0).radius(40).bg_color(lv_colors.GRAY_1), 0
        )
        self.slider.add_style(
            StyleWrapper().bg_color(lv_colors.WHITE).pad_all(-50), lv.PART.KNOB
        )
        self.slider.add_style(
            StyleWrapper().radius(0).bg_color(lv_colors.WHITE), lv.PART.INDICATOR
        )
        self.percent = lv.label(self.container)
        self.percent.add_style(
            StyleWrapper().text_font(font_GeistRegular30).text_color(lv_colors.BLACK), 0
        )
        self.percent.set_text(brightness2_percent_str(self.current_brightness))
        self.slider.add_event_cb(self.on_value_changed, lv.EVENT.VALUE_CHANGED, None)
        self.slider.clear_flag(lv.obj.FLAG.GESTURE_BUBBLE)
        self.load_screen(self)
        gc.collect()

    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        return targets

    def on_value_changed(self, event_obj):
        target = event_obj.get_target()
        if target == self.slider:
            value = target.get_value()
            self.temp_brightness = value
            display.backlight(value)
            self.percent.set_text(brightness2_percent_str(value))

    def eventhandler(self, event_obj):
        event = event_obj.code
        target = event_obj.get_target()
        if event == lv.EVENT.CLICKED:
            if isinstance(target, lv.imgbtn):
                if target == self.nav_back.nav_btn:
                    if self.temp_brightness != self.current_brightness:
                        storage_device.set_brightness(self.temp_brightness)
            super().eventhandler(event_obj)


class AnimationSetting(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "tips") and self.tips:
            targets.append(self.tips)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        super().__init__(
            prev_scr=prev_scr,
            title=_(i18n_keys.TITLE__ANIMATIONS),
            nav_back=True,
        )

        self.container = ContainerFlexCol(self.content_area, self.title)
        self.item = ListItemBtnWithSwitch(self.container, _(i18n_keys.ITEM__ANIMATIONS))

        self.tips = lv.label(self.content_area)
        desc_width = self.container.get_width()
        self.tips.set_size(desc_width, lv.SIZE.CONTENT)
        self.tips.set_long_mode(lv.label.LONG.WRAP)
        self.tips.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular26)
            .text_color(lv_colors.WHITE_2)
            .text_align_left()
            .pad_hor(24)
            .pad_ver(8),
            0,
        )
        self.tips.align_to(self.container, lv.ALIGN.OUT_BOTTOM_MID, 0, 12)
        if storage_device.is_animation_enabled():
            self.item.add_state()
            self.tips.set_text(_(i18n_keys.CONTENT__ANIMATIONS__ENABLED_HINT))
        else:
            self.item.clear_state()
            self.tips.set_text(_(i18n_keys.CONTENT__ANIMATIONS__DISABLED_HINT))

        self.container.add_event_cb(self.on_value_changed, lv.EVENT.VALUE_CHANGED, None)
        self.load_screen(self)
        gc.collect()

    def on_value_changed(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.item.switch:
                if target.has_state(lv.STATE.CHECKED):
                    storage_device.set_animation_enable(True)
                    self.tips.set_text(_(i18n_keys.CONTENT__ANIMATIONS__ENABLED_HINT))
                else:
                    storage_device.set_animation_enable(False)
                    self.tips.set_text(_(i18n_keys.CONTENT__ANIMATIONS__DISABLED_HINT))


class TouchSetting(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "keyboard_tips") and self.keyboard_tips:
            targets.append(self.keyboard_tips)
        if hasattr(self, "container2") and self.container2:
            targets.append(self.container2)
        if hasattr(self, "description") and self.description:
            targets.append(self.description)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__TOUCH), nav_back=True
        )

        # First container for keyboard haptic
        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.keyboard = ListItemBtnWithSwitch(
            self.container, _(i18n_keys.ITEM__KEYBOARD_HAPTIC), is_haptic_feedback=True
        )

        # Keyboard haptic description (left aligned with list items)
        self.keyboard_tips = lv.label(self.content_area)
        self.keyboard_tips.set_size(456, lv.SIZE.CONTENT)
        self.keyboard_tips.set_long_mode(lv.label.LONG.WRAP)
        self.keyboard_tips.set_style_text_color(lv_colors.ONEKEY_GRAY, lv.STATE.DEFAULT)
        self.keyboard_tips.set_style_text_font(font_GeistRegular26, lv.STATE.DEFAULT)
        self.keyboard_tips.set_style_text_line_space(3, 0)
        # Add left text alignment and horizontal padding to match list item padding (24)
        self.keyboard_tips.add_style(
            StyleWrapper().text_align_left().pad_hor(24).pad_ver(0), 0
        )
        self.keyboard_tips.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 16)
        self.keyboard_tips.set_text(_(i18n_keys.CONTENT__VIBRATION_HAPTIC__HINT))

        # Second container for tap awake
        self.container2 = ContainerFlexCol(
            self.content_area, self.keyboard_tips, padding_row=2
        )
        # Align directly under keyboard tips without horizontal offset
        self.container2.align_to(self.keyboard_tips, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 24)
        self.tap_awake = ListItemBtnWithSwitch(
            self.container2, _(i18n_keys.ITEM__TAP_TO_WAKE)
        )

        # Tap awake description (left aligned with list items)
        self.description = lv.label(self.content_area)
        self.description.set_size(456, lv.SIZE.CONTENT)
        self.description.set_long_mode(lv.label.LONG.WRAP)
        self.description.set_style_text_color(lv_colors.ONEKEY_GRAY, lv.STATE.DEFAULT)
        self.description.set_style_text_font(font_GeistRegular26, lv.STATE.DEFAULT)
        self.description.set_style_text_line_space(3, 0)
        self.description.add_style(
            StyleWrapper().text_align_left().pad_hor(24).pad_ver(0), 0
        )
        self.description.align_to(self.tap_awake, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 16)

        # Set keyboard haptic state
        if storage_device.keyboard_haptic_enabled():
            self.keyboard.add_state()
        else:
            self.keyboard.clear_state()

        # Set tap awake state
        if storage_device.is_tap_awake_enabled():
            self.tap_awake.add_state()
            self.description.set_text(_(i18n_keys.CONTENT__TAP_TO_WAKE_ENABLED__HINT))
        else:
            self.tap_awake.clear_state()
            self.description.set_text(_(i18n_keys.CONTENT__TAP_TO_WAKE_DISABLED__HINT))
        self.container.add_event_cb(self.on_value_changed, lv.EVENT.VALUE_CHANGED, None)
        self.container2.add_event_cb(
            self.on_value_changed, lv.EVENT.VALUE_CHANGED, None
        )
        self.load_screen(self)
        gc.collect()

    def on_value_changed(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.keyboard.switch:
                if target.has_state(lv.STATE.CHECKED):
                    storage_device.toggle_keyboard_haptic(True)
                else:
                    storage_device.toggle_keyboard_haptic(False)
            elif target == self.tap_awake.switch:
                if target.has_state(lv.STATE.CHECKED):
                    self.description.set_text(
                        _(i18n_keys.CONTENT__TAP_TO_WAKE_ENABLED__HINT)
                    )
                    storage_device.set_tap_awake_enable(True)
                else:
                    self.description.set_text(
                        _(i18n_keys.CONTENT__TAP_TO_WAKE_DISABLED__HINT)
                    )
                    storage_device.set_tap_awake_enable(False)


class AutoShutDownSetting(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "tips") and self.tips:
            targets.append(self.tips)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__SHUTDOWN), nav_back=True
        )

        # Get current shutdown delay
        cur_delay_ms = storage_device.get_autoshutdown_delay_ms()

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.setting_items = [1, 2, 5, 10, "Never", None]
        has_custom = True
        self.checked_index = 0
        # pyright: off
        self.btns: [ListItemBtn] = [None] * (len(self.setting_items))
        for index, item in enumerate(self.setting_items):
            if item is None:
                break
            original_item = item  # Keep original for comparison
            if not item == "Never":  # last item
                item = _(
                    i18n_keys.OPTION__STR_MINUTE
                    if item == 1
                    else i18n_keys.OPTION__STR_MINUTES
                ).format(int(item))
            else:
                item = _(i18n_keys.ITEM__STATUS__NEVER)

            self.btns[index] = ListItemBtn(
                self.container, item, has_next=False, use_transition=False
            )
            self.btns[index].add_check_img()

            # Compare based on delay_ms instead of string formatting
            expected_delay_ms = 0
            if original_item == "Never":
                expected_delay_ms = storage_device.AUTOSHUTDOWN_DELAY_MAXIMUM
            else:
                expected_delay_ms = (
                    original_item * 60 * 1000
                )  # Convert minutes to milliseconds

            if cur_delay_ms == expected_delay_ms:
                has_custom = False
                self.btns[index].set_checked()
                self.checked_index = index
        if has_custom:
            self.custom = storage_device.get_autoshutdown_delay_ms()
            # Use get_autoshutdown_delay_str() for display
            cur_shutdown_str = get_autoshutdown_delay_str()
            self.btns[-1] = ListItemBtn(
                self.container,
                f"{cur_shutdown_str}({_(i18n_keys.OPTION__CUSTOM__INSERT)})",
                has_next=False,
                has_bgcolor=False,
            )
            self.btns[-1].add_check_img()
            self.btns[-1].set_checked()
            self.checked_index = -1
        # pyright: on
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.tips = lv.label(self.content_area)
        self.tips.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 8, 0)
        self.tips.set_long_mode(lv.label.LONG.WRAP)
        self.fresh_tips()
        self.tips.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular26)
            .width(448)
            .text_color(lv_colors.WHITE_2)
            .text_align_left()
            .text_letter_space(-1)
            .pad_ver(16),
            0,
        )
        self.load_screen(self)
        gc.collect()

    def fresh_tips(self):
        item_text = self.btns[self.checked_index].label_left.get_text()
        if self.setting_items[self.checked_index] is None:
            item_text = item_text.split("(")[0]

        if self.setting_items[self.checked_index] == "Never":
            self.tips.set_text(_(i18n_keys.CONTENT__SETTINGS_GENERAL_SHUTDOWN_OFF_HINT))
        else:
            self.tips.set_text(
                _(i18n_keys.CONTENT__SETTINGS_GENERAL_SHUTDOWN_ON_HINT).format(
                    item_text
                )
            )

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target in self.btns:
                for index, item in enumerate(self.btns):
                    if item == target and self.checked_index != index:
                        item.set_checked()
                        self.btns[self.checked_index].set_uncheck()
                        self.checked_index = index
                        if index == 4:
                            auto_shutdown_time = (
                                storage_device.AUTOSHUTDOWN_DELAY_MAXIMUM
                            )
                        elif index == 5:
                            auto_shutdown_time = self.custom
                        else:
                            auto_shutdown_time = self.setting_items[index] * 60 * 1000
                        storage_device.set_autoshutdown_delay_ms(auto_shutdown_time)
                        GeneralScreen.cur_auto_shutdown_ms = auto_shutdown_time
                        self.fresh_tips()
                        from apps.base import reload_settings_from_storage

                        reload_settings_from_storage()


class PinMapSetting(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "tips") and self.tips:
            targets.append(self.tips)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__PIN_KEYPAD), nav_back=True
        )

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.order = ListItemBtn(
            self.container,
            _(i18n_keys.OPTION__DEFAULT),
            has_next=False,
            use_transition=False,
        )
        self.order.add_check_img()
        self.random = ListItemBtn(
            self.container,
            _(i18n_keys.OPTION__RANDOMIZED),
            has_next=False,
            use_transition=False,
        )
        self.random.add_check_img()
        self.tips = lv.label(self.content_area)
        self.tips.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 12, 0)
        self.tips.set_long_mode(lv.label.LONG.WRAP)
        self.fresh_tips()
        self.tips.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular26)
            .width(448)
            .text_color(lv_colors.WHITE_2)
            .text_letter_space(-1)
            .text_align_left()
            .pad_ver(16),
            0,
        )

        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.load_screen(self)
        gc.collect()

    def fresh_tips(self):
        if storage_device.is_random_pin_map_enabled():
            self.random.set_checked()
            self.tips.set_text(
                _(i18n_keys.CONTENT__SECURITY_PIN_KEYPAD_LAYOUT_RANDOMIZED__HINT)
            )
        else:
            self.order.set_checked()
            self.tips.set_text(
                _(i18n_keys.CONTENT__SECURITY_PIN_KEYPAD_LAYOUT_DEFAULT__HINT)
            )

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.random:
                self.random.set_checked()
                self.order.set_uncheck()
                if not storage_device.is_random_pin_map_enabled():
                    storage_device.set_random_pin_map_enable(True)
            elif target == self.order:
                self.random.set_uncheck()
                self.order.set_checked()
                if storage_device.is_random_pin_map_enabled():
                    storage_device.set_random_pin_map_enable(False)
            else:
                return
            self.fresh_tips()


class AirGapSetting(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "description") and self.description:
            targets.append(self.description)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            air_gap_enabled = storage_device.is_airgap_mode()
            if air_gap_enabled:
                self.air_gap.add_state()
                self.description.set_text(
                    _(
                        i18n_keys.CONTENT__BLUETOOTH_USB_AND_NFT_TRANSFER_FUNCTIONS_HAVE_BEEN_DISABLED
                    )
                )
            else:
                self.air_gap.clear_state()
                self.description.set_text(
                    _(
                        i18n_keys.CONTENT__AFTER_ENABLING_THE_AIRGAP_BLUETOOTH_USB_AND_NFC_TRANSFER_WILL_BE_DISABLED_SIMULTANEOUSLY
                    )
                )
            return
        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__AIR_GAP_MODE), nav_back=True
        )

        self.container = ContainerFlexCol(self.content_area, self.title)
        self.air_gap = ListItemBtnWithSwitch(
            self.container, _(i18n_keys.ITEM__AIR_GAP_MODE)
        )

        self.description = lv.label(self.content_area)
        self.description.set_size(456, lv.SIZE.CONTENT)
        self.description.set_long_mode(lv.label.LONG.WRAP)
        self.description.set_style_text_color(lv_colors.ONEKEY_GRAY, lv.STATE.DEFAULT)
        self.description.set_style_text_font(font_GeistRegular26, lv.STATE.DEFAULT)
        self.description.set_style_text_line_space(3, 0)
        self.description.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 8, 16)
        air_gap_enabled = storage_device.is_airgap_mode()
        if air_gap_enabled:
            self.air_gap.add_state()
            self.description.set_text(
                _(
                    i18n_keys.CONTENT__BLUETOOTH_USB_AND_NFT_TRANSFER_FUNCTIONS_HAVE_BEEN_DISABLED
                )
            )
        else:
            self.air_gap.clear_state()
            self.description.set_text(
                _(
                    i18n_keys.CONTENT__AFTER_ENABLING_THE_AIRGAP_BLUETOOTH_USB_AND_NFC_TRANSFER_WILL_BE_DISABLED_SIMULTANEOUSLY
                )
            )
        # self.usb = ListItemBtnWithSwitch(self.container, _(i18n_keys.ITEM__USB))
        self.add_event_cb(self.on_event, lv.EVENT.VALUE_CHANGED, None)
        self.add_event_cb(self.on_event, lv.EVENT.READY, None)
        self.add_event_cb(self.on_event, lv.EVENT.CANCEL, None)
        self.load_screen(self)
        gc.collect()

    def on_event(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.air_gap.switch:
                from trezor.lvglui.scrs.template import AirGapToggleTips

                if target.has_state(lv.STATE.CHECKED):
                    AirGapToggleTips(
                        enable=True,
                        callback_obj=self,
                    )
                else:
                    AirGapToggleTips(
                        enable=False,
                        callback_obj=self,
                    )
        elif code == lv.EVENT.READY:
            if not storage_device.is_airgap_mode():
                self.description.set_text(
                    _(
                        i18n_keys.CONTENT__BLUETOOTH_USB_AND_NFT_TRANSFER_FUNCTIONS_HAVE_BEEN_DISABLED
                    )
                )
                utils.enable_airgap_mode()
            else:
                self.description.set_text(
                    _(
                        i18n_keys.CONTENT__AFTER_ENABLING_THE_AIRGAP_BLUETOOTH_USB_AND_NFC_TRANSFER_WILL_BE_DISABLED_SIMULTANEOUSLY
                    )
                )
                utils.disable_airgap_mode()
        elif code == lv.EVENT.CANCEL:
            if storage_device.is_airgap_mode():
                self.air_gap.add_state()
            else:
                self.air_gap.clear_state()


class AboutSetting(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "firmware_update") and self.firmware_update:
            targets.append(self.firmware_update)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        preloaded_info = DeviceInfoManager.instance().get_info()
        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__ABOUT_DEVICE), nav_back=True
        )
        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=0)
        self.container.add_dummy()
        self.model = DisplayItemWithFont_30(
            self.container, _(i18n_keys.ITEM__MODEL), preloaded_info["model"]
        )
        self.ble_mac = DisplayItemWithFont_30(
            self.container,
            _(i18n_keys.ITEM__BLUETOOTH_NAME),
            preloaded_info["ble_name"],
        )
        self.version = DisplayItemWithFont_30(
            self.container, _(i18n_keys.ITEM__SYSTEM_VERSION), preloaded_info["version"]
        )
        self.ble_version = DisplayItemWithFont_30(
            self.container,
            _(i18n_keys.ITEM__BLUETOOTH_VERSION),
            preloaded_info["ble_version"],
        )
        self.boot_version = DisplayItemWithFont_30(
            self.container,
            _(i18n_keys.ITEM__BOOTLOADER_VERSION),
            preloaded_info["boot_version"],
        )
        self.board_version = DisplayItemWithFont_30(
            self.container,
            _(i18n_keys.ITEM__BOARDLOADER_VERSION),
            preloaded_info["board_version"],
        )
        se_firmware_content_pairs = [
            ("01:", preloaded_info["onekey_se01_version"]),
            ("02:", preloaded_info["onekey_se02_version"]),
            ("03:", preloaded_info["onekey_se03_version"]),
            ("04:", preloaded_info["onekey_se04_version"]),
        ]
        self.se_firmware = DisplayItemWithFont_TextPairs(
            self.container,
            _(i18n_keys.ITEM__SE_FIRMWARE),
            se_firmware_content_pairs,
        )
        se_boot_content_pairs = [
            ("01:", preloaded_info["onekey_se01_boot_version"]),
            ("02:", preloaded_info["onekey_se02_boot_version"]),
            ("03:", preloaded_info["onekey_se03_boot_version"]),
            ("04:", preloaded_info["onekey_se04_boot_version"]),
        ]
        self.se_bootloader = DisplayItemWithFont_TextPairs(
            self.container,
            "SE Bootloader",
            se_boot_content_pairs,
        )

        self.serial = DisplayItemWithFont_30(
            self.container, _(i18n_keys.ITEM__SERIAL_NUMBER), preloaded_info["serial"]
        )
        self.serial.add_flag(lv.obj.FLAG.EVENT_BUBBLE)

        self.container.add_dummy()

        self.certification = NormalButton(
            self.content_area,
            _(i18n_keys.CONTENT__CERTIFICATIONS),
            label_align=lv.ALIGN.LEFT_MID,
        )
        self.certification.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 8)
        if __debug__:
            self.firmware_update = NormalButton(
                self.content_area, _(i18n_keys.BUTTON__SYSTEM_UPDATE)
            )
            self.firmware_update.align_to(
                self.certification, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 8
            )
            self.firmware_update.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

        self.serial.add_event_cb(self.on_long_pressed, lv.EVENT.LONG_PRESSED, None)
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.certification.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.load_screen(self)
        gc.collect()

    def on_click(self, event_obj):
        target = event_obj.get_target()
        if target == self.certification:
            from .template import CertificationInfo

            CertificationInfo()
        elif __debug__ and target == self.firmware_update:
            Go2UpdateMode(self)

    def on_long_pressed(self, event_obj):
        target = event_obj.get_target()
        if target == self.serial:
            GO2BoardLoader()


class TrezorModeToggle(FullSizeWindow):
    def __init__(self, callback_obj, enable=False):
        super().__init__(
            title=_(
                i18n_keys.TITLE__RESTORE_TREZOR_COMPATIBILITY
                if enable
                else i18n_keys.TITLE__DISABLE_TREZOR_COMPATIBILITY
            ),
            subtitle=_(
                i18n_keys.SUBTITLE__RESTORE_TREZOR_COMPATIBILITY
                if enable
                else i18n_keys.SUBTITLE__DISABLE_TREZOR_COMPATIBILITY
            ),
            confirm_text=_(i18n_keys.BUTTON__RESTART),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
        )
        self.enable = enable
        self.callback_obj = callback_obj
        if not enable:
            self.btn_yes.enable(
                bg_color=lv_colors.ONEKEY_YELLOW, text_color=lv_colors.BLACK
            )
            self.tips_bar = Banner(
                self.content_area,
                LEVEL.WARNING,
                _(i18n_keys.MSG__DO_NOT_CHANGE_THIS_SETTING),
            )
            self.tips_bar.align(lv.ALIGN.TOP_LEFT, 8, 8)
            self.title.align_to(self.tips_bar, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 16)
            self.subtitle.align_to(self.title, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 16)

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.btn_no:
                self.callback_obj.reset_switch()
                self.destroy(200)
            elif target == self.btn_yes:

                async def restart_delay():
                    await loop.sleep(1000)
                    utils.reset()

                storage_device.enable_trezor_compatible(self.enable)
                workflow.spawn(restart_delay())


class GO2BoardLoader(FullSizeWindow):
    def __init__(self):
        super().__init__(
            title=_(i18n_keys.TITLE__ENTERING_BOARDLOADER),
            subtitle=_(i18n_keys.SUBTITLE__SWITCH_TO_BOARDLOADER_RECONFIRM),
            confirm_text=_(i18n_keys.BUTTON__RESTART),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            # icon_path="A:/res/warning.png",
        )

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.btn_yes:
                utils.reboot2boardloader()
            elif target == self.btn_no:
                self.destroy(100)


class Go2UpdateMode(Screen):
    def __init__(self, prev_scr):
        super().__init__(
            prev_scr=prev_scr,
            title=_(i18n_keys.TITLE__SYSTEM_UPDATE),
            subtitle=_(i18n_keys.SUBTITLE__SWITCH_TO_UPDATE_MODE_RECONFIRM),
            # icon_path="A:/res/update-green.png",
        )
        self.btn_yes = NormalButton(self.content_area, _(i18n_keys.BUTTON__RESTART))
        self.btn_yes.set_size(231, 98)
        self.btn_yes.align(lv.ALIGN.BOTTOM_RIGHT, -8, -8)
        self.btn_yes.enable(lv_colors.ONEKEY_GREEN, lv_colors.BLACK)
        self.btn_no = NormalButton(self.content_area, _(i18n_keys.BUTTON__CANCEL))
        self.btn_no.set_size(231, 98)
        self.btn_no.align(lv.ALIGN.BOTTOM_LEFT, 8, -8)

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.btn_yes:
                utils.reboot_to_bootloader()
            elif target == self.btn_no:
                self.load_screen(self.prev_scr, destroy_self=True)


class PowerOff(FullSizeWindow):
    IS_ACTIVE = False

    def __init__(self, re_loop: bool = False):
        if PowerOff.IS_ACTIVE:
            return
        PowerOff.IS_ACTIVE = True
        super().__init__(
            title=_(i18n_keys.TITLE__POWER_OFF),
            confirm_text=_(i18n_keys.BUTTON__POWER_OFF),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            subtitle=_(i18n_keys.CONTENT__POWER_OFF_LOW_BATTERY_DESC)
            if utils.is_low_battery()
            else None,
        )
        self.btn_yes.enable(lv_colors.ONEKEY_RED_1, text_color=lv_colors.BLACK)
        self.re_loop = re_loop
        from trezor import config

        self.has_pin = config.has_pin()
        if self.has_pin and storage_device.is_initialized():
            config.lock()

            if passphrase.is_passphrase_pin_enabled():
                storage.cache.end_current_session()

    def back(self):
        PowerOff.IS_ACTIVE = False
        self.destroy(100)

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.btn_yes:
                ShutingDown()
            elif target == self.btn_no:
                if (
                    not utils.is_initialization_processing()
                    and self.has_pin
                    and storage_device.is_initialized()
                ):
                    from apps.common.request_pin import verify_user_pin

                    workflow.spawn(
                        verify_user_pin(
                            re_loop=self.re_loop,
                            allow_cancel=False,
                            callback=self.back,
                            allow_fingerprint=False,
                            pin_use_type=2,
                        )
                    )
                else:
                    self.back()


class ShutingDown(FullSizeWindow):
    def __init__(self):
        super().__init__(
            title=_(i18n_keys.TITLE__SHUTTING_DOWN), subtitle=None, anim_dir=0
        )

        async def shutdown_delay():
            await loop.sleep(3000)
            uart.ctrl_power_off()

        workflow.spawn(shutdown_delay())


class WallpaperScreen(AnimScreen):
    cur_language = ""

    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            if hasattr(self, "container"):
                self.container.delete()
            if prev_scr is not None:
                self.prev_scr = prev_scr

        if not hasattr(self, "content_area"):
            super().__init__(
                prev_scr=prev_scr,
                title=_(i18n_keys.TITLE__WALLPAPER),
                nav_back=True,
            )

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)

        self.lock_screen = ListItemBtn(
            self.container, _(i18n_keys.ITEM__LOCK_SCREEN), use_transition=False
        )

        self.home_screen = ListItemBtn(
            self.container, _(i18n_keys.BUTTON__HOME_SCREEN), use_transition=False
        )

        if not hasattr(self, "_event_added"):
            self.content_area.add_event_cb(self.on_click_event, lv.EVENT.CLICKED, None)
            self._event_added = True

    def refresh_text(self):
        if hasattr(self, "lock_screen") and self.lock_screen:
            self.lock_screen.label_left.set_text(_(i18n_keys.ITEM__LOCK_SCREEN))
        if hasattr(self, "home_screen") and self.home_screen:
            self.home_screen.label_left.set_text(_(i18n_keys.BUTTON__HOME_SCREEN))

    def __del__(self):
        if hasattr(self, "container") and self.container:
            self.container.delete()
        _clear_preview_cache()

    def on_click_event(self, event_obj):
        target = event_obj.get_target()
        if hasattr(self, "lock_screen") and target == self.lock_screen:
            if hasattr(AppdrawerBackgroundSetting, "_instance"):
                AppdrawerBackgroundSetting._dispose_existing(
                    "navigation from WallpaperScreen"
                )
            AppdrawerBackgroundSetting(self)
        elif hasattr(self, "home_screen") and target == self.home_screen:
            HomeScreenSetting(self)


class HomeScreenSetting(AnimScreen):
    def collect_animation_targets(self) -> list:
        return []

    def __init__(
        self,
        prev_scr=None,
        selected_wallpaper=None,
        preserve_blur_state=None,
        return_from_wallpaper=False,
    ):

        if not hasattr(self, "_init"):
            self._init = True
        else:
            self._return_to_prev_instance = return_from_wallpaper
            if selected_wallpaper:

                self.selected_wallpaper = selected_wallpaper
                self.original_wallpaper_path = selected_wallpaper

                display_path = selected_wallpaper

                self.is_blur_active = False
                final_display_path = display_path

                if preserve_blur_state and preserve_blur_state is True:
                    blur_path = self._get_blur_wallpaper_path(display_path)
                    if blur_path and self._blur_wallpaper_exists(blur_path):
                        final_display_path = blur_path
                        self.is_blur_active = True

                self.current_wallpaper_path = final_display_path

                if hasattr(self, "homescreen_preview"):
                    self.homescreen_preview.set_src(final_display_path)

                if hasattr(self, "blur_button"):
                    self._update_blur_button_state()
            else:
                self._load_blur_state()
                if hasattr(self, "homescreen_preview"):
                    self.homescreen_preview.set_src(self.current_wallpaper_path)
                if hasattr(self, "blur_button"):
                    self._update_blur_button_state()

            return

        self.selected_wallpaper = selected_wallpaper
        self._return_to_prev_instance = return_from_wallpaper

        super().__init__(
            prev_scr=prev_scr, nav_back=True, rti_path="A:/res/checkmark.png"
        )

        self.content_area.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
        self.content_area.clear_flag(lv.obj.FLAG.SCROLLABLE)
        self.content_area.set_style_pad_bottom(0, 0)

        self.container = lv.obj(self.content_area)
        self.container.set_size(lv.pct(100), lv.pct(100))
        self.container.align(lv.ALIGN.TOP_MID, 0, 0)
        self.container.add_style(
            StyleWrapper().bg_opa(lv.OPA.TRANSP).pad_all(0).border_width(0), 0
        )
        self.container.clear_flag(lv.obj.FLAG.CLICKABLE)
        self.container.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
        self.container.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
        self.container.clear_flag(lv.obj.FLAG.SCROLLABLE)

        self.preview_container = create_preview_container(
            self.container,
            width=344,
            height=572,
            top_offset=125,
            style=StyleWrapper()
            .bg_color(lv_colors.BLACK)
            .bg_opa(lv.OPA.COVER)
            .pad_all(0)
            .border_width(0),
        )

        self.homescreen_preview = create_preview_image(
            self.preview_container,
            target_size=(344, 572),
        )
        self.preview_mask = create_top_mask(self.preview_container, height=5)
        self._blur_cache = {}

        if self.selected_wallpaper:

            self.original_wallpaper_path = self.selected_wallpaper

            display_path = self.selected_wallpaper

            self.is_blur_active = False
            final_display_path = display_path

            if preserve_blur_state and preserve_blur_state is True:
                blur_path = self._get_blur_wallpaper_path(display_path)
                if blur_path and self._blur_wallpaper_exists(blur_path):
                    final_display_path = blur_path
                    self.is_blur_active = True

            self.current_wallpaper_path = final_display_path
            self.homescreen_preview.set_src(final_display_path)
        else:
            self._load_blur_state()
            self.homescreen_preview.set_src(self.current_wallpaper_path)

        self.homescreen_preview.invalidate()
        self.preview_container.invalidate()

        self.app_icons = []

        scale_x = 343.0 / 480.0
        scale_y = 572.0 / 800.0
        offset_x = 0.5
        offset_y = 1.0

        desktop_positions = [
            (64, 164),
            (272, 164),
            (64, 442),
            (272, 442),
        ]

        for i in range(4):
            screen_x, screen_y = desktop_positions[i]

            x_pos = int(screen_x * scale_x + offset_x)
            y_pos = int(screen_y * scale_y + offset_y)

            icon_img = lv.img(self.preview_container)
            icon_img.set_src("A:/res/icon_example.png")
            icon_img.set_antialias(True)
            icon_img.set_size(lv.SIZE.CONTENT, lv.SIZE.CONTENT)
            icon_img.set_pos(x_pos, y_pos)

            self.app_icons.append(icon_img)
        self._create_buttons()
        loop.schedule(self._first_frame_fix())
        self.change_button_container.align_to(
            self.preview_container, lv.ALIGN.OUT_BOTTOM_LEFT, 50, 10
        )
        self.blur_button_container.align_to(
            self.preview_container, lv.ALIGN.OUT_BOTTOM_RIGHT, -50, 10
        )
        gc.collect()

    def _create_button_with_label(self, icon_path, text, callback):
        button_container = lv.obj(self.container)
        button_container.set_size(120, 100)
        button_container.align_to(self.preview_container, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)
        button_container.add_style(
            StyleWrapper().bg_opa(lv.OPA.TRANSP).border_width(0).pad_all(0),
            0,
        )
        button_container.add_flag(lv.obj.FLAG.CLICKABLE)
        button_container.clear_flag(lv.obj.FLAG.SCROLLABLE)

        button = lv.btn(button_container)
        button.set_size(64, 64)
        button.align(lv.ALIGN.TOP_MID, 0, 0)
        button.add_style(
            StyleWrapper().border_width(0).radius(40).bg_opa(lv.OPA.TRANSP),
            0,
        )
        button.add_flag(lv.obj.FLAG.CLICKABLE)
        button.clear_flag(lv.obj.FLAG.EVENT_BUBBLE)

        icon = lv.img(button)
        if icon_path:
            icon.set_src(icon_path)
        icon.set_antialias(True)
        icon.align(lv.ALIGN.CENTER, 0, 0)

        label = lv.label(button_container)
        label.set_text(text)
        label.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular20)
            .text_color(lv_colors.WHITE)
            .text_align(lv.TEXT_ALIGN.CENTER),
            0,
        )
        label.align_to(button, lv.ALIGN.OUT_BOTTOM_MID, 0, 4)
        label.add_flag(lv.obj.FLAG.CLICKABLE)
        label.add_event_cb(callback, lv.EVENT.CLICKED, None)

        button.add_event_cb(callback, lv.EVENT.CLICKED, None)
        button_container.add_event_cb(callback, lv.EVENT.CLICKED, None)

        return button_container, button, icon, label

    def _create_buttons(self):
        (
            self.change_button_container,
            self.change_button,
            self.button_icon,
            self.change_label,
        ) = self._create_button_with_label(
            "A:/res/change-wallper.png",
            _(i18n_keys.BUTTON__CHANGE),
            self.on_select_clicked,
        )

        (
            self.blur_button_container,
            self.blur_button,
            self.blur_button_icon,
            self.blur_label,
        ) = self._create_button_with_label(
            "", _(i18n_keys.BUTTON__BLUR), self.on_blur_clicked
        )

        self._update_blur_button_state()

    async def _first_frame_fix(self):
        utime.sleep_ms(100)

        self.refresh()
        if hasattr(self, "container") and self.container:
            self.container.invalidate()
        if hasattr(self, "preview_container") and self.preview_container:
            self.preview_container.invalidate()
        if hasattr(self, "homescreen_preview") and self.homescreen_preview:
            self.homescreen_preview.invalidate()

    def on_select_clicked(self, event_obj):
        WallperChange(prev_scr=self)

    def eventhandler(self, event_obj):
        event = event_obj.code
        target = event_obj.get_target()

        if event == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return

            if isinstance(target, lv.imgbtn):
                if hasattr(self, "nav_back") and target == self.nav_back.nav_btn:
                    if self.prev_scr is not None:
                        self._return_to_previous_screen()
                    return
                elif hasattr(self, "rti_btn") and target == self.rti_btn:
                    self.on_click_ext(target)
                    return

        super().eventhandler(event_obj)

    def refresh_text(self):
        if hasattr(self, "change_label"):
            self.change_label.set_text(_(i18n_keys.BUTTON__CHANGE))

    def __del__(self):
        if hasattr(utils, "SCREENS") and self in utils.SCREENS:
            utils.SCREENS.remove(self)
        _clear_preview_cache()

    def _rebuild_image_object(self):
        if hasattr(self, "homescreen_preview") and hasattr(self, "preview_container"):
            target_path = getattr(
                self, "current_wallpaper_path", "A:/res/wallpaper-2.jpg"
            )

            if self.homescreen_preview:
                self.homescreen_preview.delete()
            gc.collect()

            self.preview_button = lv.btn(self.preview_container)
            self.preview_button.set_size(344, 567)
            self.preview_button.align(lv.ALIGN.CENTER, 0, 0)
            self.preview_button.remove_style_all()
            self.preview_button.add_style(
                StyleWrapper().bg_opa(lv.OPA.TRANSP).border_width(0).pad_all(0), 0
            )
            self.preview_button.clear_flag(lv.obj.FLAG.CLICKABLE)

            self.homescreen_preview = create_preview_image(
                self.preview_button,
                target_size=(344, 572),
            )

            self.homescreen_preview.set_src(target_path)
        if hasattr(self, "blur_label"):
            self.blur_label.set_text("Blur")

    def on_click_ext(self, target):
        if hasattr(self, "rti_btn") and target == self.rti_btn:
            current_wallpaper = getattr(self, "current_wallpaper_path", None)
            if current_wallpaper:
                apply_home_wallpaper(current_wallpaper)

            if self.prev_scr is not None:
                self._return_to_previous_screen()

    def _return_to_previous_screen(self):
        if not self.prev_scr:
            return
        _clear_preview_cache()
        if getattr(self, "_return_to_prev_instance", False):
            self._load_scr(self.prev_scr, back=True)
            utils.try_remove_scr(self)
            if (
                hasattr(self.__class__, "_instance")
                and self.__class__._instance is self
            ):
                del self.__class__._instance
            self.del_delayed(100)
            gc.collect()
        else:
            self.load_screen(self.prev_scr, destroy_self=True)

    def _get_blur_wallpaper_path(self, original_path):
        if not original_path:
            return None

        if "." in original_path:
            path_without_ext, ext = original_path.rsplit(".", 1)
            blur_path = f"{path_without_ext}-blur.{ext}"
        else:
            blur_path = f"{original_path}-blur"

        return blur_path

    def _blur_wallpaper_exists(self, blur_path):
        if not blur_path:
            return False

        try:
            if blur_path.startswith("A:/res/wallpapers/"):
                file_path = blur_path.replace(
                    "A:/res/wallpapers/", "1:/res/wallpapers/"
                )
            elif blur_path.startswith("A:/res/"):
                file_path = blur_path[2:]
            elif blur_path.startswith("A:1:/"):
                file_path = blur_path[2:]
            else:
                file_path = blur_path

            io.fatfs.stat(file_path)
            return True

        except Exception:
            return False

    def _update_blur_button_state(self):
        if not hasattr(self, "original_wallpaper_path"):
            return

        blur_path = self._get_blur_wallpaper_path(self.original_wallpaper_path)
        blur_exists = self._blur_wallpaper_exists(blur_path) if blur_path else False

        if not blur_exists:
            icon_path = "A:/res/blur_not_available.png"
            self.blur_button.clear_flag(lv.obj.FLAG.CLICKABLE)
            self.blur_button.set_style_bg_opa(lv.OPA.TRANSP, 0)
            self.blur_button.set_style_border_width(0, 0)
            self.blur_label.set_style_text_color(lv_colors.WHITE_2, 0)
        else:
            self.blur_button.add_flag(lv.obj.FLAG.CLICKABLE)
            self.blur_button.set_style_bg_opa(lv.OPA.COVER, 0)
            self.blur_button.set_style_border_width(1, 0)
            self.blur_label.set_style_text_color(lv_colors.WHITE, 0)

            if getattr(self, "is_blur_active", False):
                icon_path = "A:/res/blur_selected.png"
            else:
                icon_path = "A:/res/blur_no_selected.png"

        self.blur_button_icon.set_src(icon_path)

    def on_blur_clicked(self, event_obj):
        blur_path = self._get_blur_wallpaper_path(self.original_wallpaper_path)

        if not blur_path or not self._blur_wallpaper_exists(blur_path):
            return

        self.is_blur_active = not getattr(self, "is_blur_active", False)

        test_path = blur_path if self.is_blur_active else self.original_wallpaper_path

        if test_path and "/res/wallpapers/" in test_path:
            if "-blur." not in test_path:
                if test_path.startswith("A:/res/wallpapers/"):
                    test_path = test_path.replace(
                        "A:/res/wallpapers/", "A:1:/res/wallpapers/"
                    )
                elif test_path.startswith("1:/res/wallpapers/"):
                    test_path = test_path.replace(
                        "1:/res/wallpapers/", "A:1:/res/wallpapers/"
                    )
            else:
                if test_path.startswith("A:/res/wallpapers/"):
                    test_path = test_path.replace(
                        "A:/res/wallpapers/", "A:1:/res/wallpapers/"
                    )
                elif test_path.startswith("1:/res/wallpapers/"):
                    test_path = test_path.replace(
                        "1:/res/wallpapers/", "A:1:/res/wallpapers/"
                    )

        self.current_wallpaper_path = test_path

        if hasattr(self.homescreen_preview, "set_src"):
            self.homescreen_preview.set_src(self.current_wallpaper_path)
        else:
            filename = (
                self.current_wallpaper_path.split("/")[-1]
                if "/" in self.current_wallpaper_path
                else self.current_wallpaper_path
            )
            display_text = f"Wallpaper\n{filename}\n\n(LVGL filesystem\ndamaged after\nAppDrawer animation)"
            self.homescreen_preview.set_text(display_text)
        self.invalidate()

        self._update_blur_button_state()

    def _load_blur_state(self):
        try:
            current_homescreen = (
                storage_device.get_appdrawer_background()
                or storage_device.get_homescreen()
                or "A:/res/wallpaper-2.jpg"
            )

            if not self._blur_wallpaper_exists(current_homescreen):
                current_homescreen = "A:/res/wallpaper-7.jpg"
                storage_device.set_appdrawer_background(current_homescreen)
                storage_device.set_homescreen(current_homescreen)

            if "-blur." in current_homescreen:
                self.is_blur_active = True
                self.current_wallpaper_path = current_homescreen
                path_parts = current_homescreen.rsplit(".", 1)
                if len(path_parts) == 2:
                    name_part, ext = path_parts
                    if name_part.endswith("-blur"):
                        self.original_wallpaper_path = name_part[:-5] + "." + ext
                    else:
                        self.original_wallpaper_path = current_homescreen
                else:
                    self.original_wallpaper_path = current_homescreen
            else:
                self.is_blur_active = False
                self.original_wallpaper_path = current_homescreen
                self.current_wallpaper_path = current_homescreen

        except Exception:
            current_homescreen = "A:/res/wallpaper-7.jpg"
            self.original_wallpaper_path = current_homescreen
            self.current_wallpaper_path = current_homescreen
            self.is_blur_active = False


class SecurityScreen(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            utils.mark_collecting_fingerprint_done()
            return
        super().__init__(prev_scr, title=_(i18n_keys.TITLE__SECURITY), nav_back=True)

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)

        self.container.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
        self.container.set_scroll_dir(lv.DIR.NONE)

        self.device_auth = ListItemBtn(
            self.container,
            _(i18n_keys.TITLE__SECURITY_CHECK),
        )
        self.pin_map_type = ListItemBtn(self.container, _(i18n_keys.ITEM__PIN_KEYPAD))
        self.fingerprint = ListItemBtn(self.container, _(i18n_keys.TITLE__FINGERPRINT))
        self.usb_lock = ListItemBtn(self.container, _(i18n_keys.ITEM__USB_LOCK))
        self.change_pin = ListItemBtn(self.container, _(i18n_keys.ITEM__CHANGE_PIN))
        self.safety_check = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__SAFETY_CHECKS),
        )
        self.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.load_screen(self)
        gc.collect()

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        # pyright: off
        if code == lv.EVENT.CLICKED:
            from trezor.wire import DUMMY_CONTEXT

            if utils.lcd_resume():
                return
            if target == self.change_pin:
                from apps.management.change_pin import change_pin
                from trezor.messages import ChangePin

                workflow.spawn(change_pin(DUMMY_CONTEXT, ChangePin(remove=False)))
            elif target == self.pin_map_type:
                PinMapSetting(self)
            elif target == self.usb_lock:
                UsbLockSetting(self)
            elif target == self.fingerprint:
                from apps.common.request_pin import verify_user_pin

                workflow.spawn(
                    verify_user_pin(
                        re_loop=False,
                        allow_cancel=True,
                        callback=lambda: FingerprintSetting(self),
                        allow_fingerprint=False,
                        standy_wall_only=True,
                        pin_use_type=1,
                    )
                )
            elif target == self.device_auth:
                DeviceAuthScreen(self)
            elif target == self.safety_check:
                SafetyCheckSetting(self)
            else:
                pass
        # pyright: on


class DeviceAuthScreen(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "btn") and self.btn:
            targets.append(self.btn)
        return targets

    def __init__(self, prev_scr=None) -> None:
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        from binascii import hexlify

        super().__init__(
            prev_scr,
            title=_(i18n_keys.TITLE__SECURITY_CHECK),
            nav_back=True,
        )
        firmware_version = storage_device.get_firmware_version()
        firmware_build_id = utils.BUILD_ID[-7:].decode()
        firmware_hash_str = hexlify(utils.onekey_firmware_hash()).decode()[:7]
        version_str = f"{firmware_version} ({firmware_build_id}-{firmware_hash_str})"

        ble_version = uart.get_ble_version()
        ble_build_id = uart.get_ble_build_id()
        ble_hash_str = hexlify(uart.get_ble_hash()).decode()[:7]
        ble_version_str = f"{ble_version} ({ble_build_id}-{ble_hash_str})"

        boot_version = utils.boot_version()
        boot_build_id = utils.boot_build_id()
        boot_hash_str = hexlify(utils.boot_hash()).decode()[:7]
        boot_version_str = f"{boot_version} ({boot_build_id}-{boot_hash_str})"
        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=0)
        self.container.add_dummy()

        self.ser_num = DisplayItemWithFont_30(
            self.container,
            _(i18n_keys.ITEM__SERIAL_NUMBER),
            storage_device.get_serial(),
        )
        self.version = DisplayItemWithFont_30(
            self.container,
            _(i18n_keys.ITEM__SYSTEM_VERSION),
            version_str,
            url=f"https://github.com/OneKeyHQ/firmware-pro/releases/tag/v{firmware_version}",
        )
        self.ble_version = DisplayItemWithFont_30(
            self.container,
            _(i18n_keys.ITEM__BLUETOOTH_VERSION),
            ble_version_str,
            url=f"https://github.com/OneKeyHQ/bluetooth-firmware-pro/releases/tag/v{ble_version}",
        )
        self.boot_version = DisplayItemWithFont_30(
            self.container,
            _(i18n_keys.ITEM__BOOTLOADER_VERSION),
            boot_version_str,
            url=f"https://github.com/OneKeyHQ/firmware-pro/releases/tag/bootloader-v{boot_version}",
        )
        self.container.add_dummy()
        self.btn = NormalButton(self, _(i18n_keys.ACTION_VERIFY_NOW))
        self.btn.enable(lv_colors.ONEKEY_GREEN, text_color=lv_colors.BLACK)
        self.load_screen(self)
        gc.collect()

    def on_click(self, target):
        if target == self.btn:
            DeviceAuthTutorial(self)


class DeviceAuthTutorial(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "warning_banner") and self.warning_banner:
            targets.append(self.warning_banner)
        return targets

    def __init__(self, prev_scr=None) -> None:
        super().__init__(
            prev_scr,
            title=_(i18n_keys.TITLE__VEIRIFY_DEVICE),
            nav_back=True,
        )
        from trezor.lvglui.scrs.components.listitem import CardHeader, DisplayItem

        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        steps = [
            (
                _(i18n_keys.FORM__DOWNLOAD_ONEKEY_APP),
                _(i18n_keys.FORM__DOWNLOAD_APP_FROM_DOWNLOAD_CENTER),
            ),
            (
                _(i18n_keys.TITLE__VEIRIFY_DEVICE),
                _(i18n_keys.VERIFY_DEVICE_CONNECT_DEVICE_DESC),
            ),
        ]
        for i, step in enumerate(steps):
            self.group = ContainerFlexCol(
                self.container,
                None,
                padding_row=0,
                no_align=True,
            )
            self.item_group_header = CardHeader(
                self.group,
                step[0],
                f"A:/res/group-icon-num-{i+1}.png",
            )
            self.item_group_body = DisplayItem(
                self.group,
                None,
                step[1],
            )
            self.item_group_body.label.add_style(
                StyleWrapper().text_color(lv_colors.ONEKEY_GRAY_4),
                0,
            )
            self.group.add_dummy()

        self.warning_banner = Banner(
            self.content_area,
            LEVEL.HIGHLIGHT,
            _(i18n_keys.VERIFY_DEVICE_HELP_CENTER_TEXT),
            title=_(i18n_keys.ACTION__LEARN_MORE),
        )
        self.warning_banner.set_style_text_color(lv_colors.LIGHT_GRAY, 0)
        self.warning_banner.align_to(self.container, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)
        self.load_screen(self)
        gc.collect()


class UsbLockSetting(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "description") and self.description:
            targets.append(self.description)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__USB_LOCK), nav_back=True
        )

        self.container = ContainerFlexCol(self.content_area, self.title)
        self.usb_lock = ListItemBtnWithSwitch(
            self.container, _(i18n_keys.ITEM__USB_LOCK)
        )

        self.description = lv.label(self.content_area)
        self.description.set_size(456, lv.SIZE.CONTENT)
        self.description.set_long_mode(lv.label.LONG.WRAP)
        self.description.set_style_text_color(lv_colors.ONEKEY_GRAY, lv.STATE.DEFAULT)
        self.description.set_style_text_font(font_GeistRegular26, lv.STATE.DEFAULT)
        self.description.set_style_text_line_space(3, 0)
        self.description.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 8, 16)

        if storage_device.is_usb_lock_enabled():
            self.usb_lock.add_state()
            self.description.set_text(_(i18n_keys.CONTENT__USB_LOCK_ENABLED__HINT))
        else:
            self.usb_lock.clear_state()
            self.description.set_text(_(i18n_keys.CONTENT__USB_LOCK_DISABLED__HINT))
        self.container.add_event_cb(self.on_value_changed, lv.EVENT.VALUE_CHANGED, None)
        self.load_screen(self)
        gc.collect()

    def on_value_changed(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.usb_lock.switch:
                if target.has_state(lv.STATE.CHECKED):
                    self.description.set_text(
                        _(i18n_keys.CONTENT__USB_LOCK_ENABLED__HINT)
                    )
                    storage_device.set_usb_lock_enable(True)
                else:
                    self.description.set_text(
                        _(i18n_keys.CONTENT__USB_LOCK_DISABLED__HINT)
                    )
                    storage_device.set_usb_lock_enable(False)


class FingerprintSetting(AnimScreen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        from trezor import config

        config.fingerprint_data_read_remaining()

        from trezorio import fingerprint

        fingerprint.clear_template_cache(True)

        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__FINGERPRINT), nav_back=True
        )

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.fresh_show()
        self.add_event_cb(self.on_value_changed, lv.EVENT.VALUE_CHANGED, None)
        self.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def fresh_show(self):
        self.container.clean()
        if hasattr(self, "container_fun"):
            self.container_fun.delete()

        from . import fingerprints

        self.fingerprint_list = fingerprints.get_fingerprint_list()
        counter = fingerprints.get_fingerprint_count()
        group_data = fingerprints.get_fingerprint_group()
        self.data_new_version = fingerprints.data_version_is_new()

        valid_fps = [fp for fp in self.fingerprint_list if fp is not None]

        def _filter_and_pad_group(grp, valid_fps):
            if not grp:
                return None
            filtered = [idx for idx in grp["indexes"] if idx in valid_fps]
            if not filtered:
                return None
            while len(filtered) < 3:
                filtered.append(0xFF)
            return {"group_id": grp["group_id"], "indexes": filtered}

        groups = []
        if self.data_new_version:
            g1 = self._parse_group_data(group_data[0:4])
            g2 = self._parse_group_data(group_data[4:8])

            g1 = _filter_and_pad_group(g1, valid_fps)
            g2 = _filter_and_pad_group(g2, valid_fps)

            for g in (g1, g2):
                if g is not None:
                    groups.append(g)

            in_group = {idx for g in groups for idx in g["indexes"] if idx != 0xFF}
            not_in_group = [idx for idx in valid_fps if idx not in in_group]

            if len(groups) < 2 and not_in_group:
                new_idx = not_in_group[:]
                while len(new_idx) < 3:
                    new_idx.append(0xFF)
                groups.append({"group_id": not_in_group[0], "indexes": new_idx})

            counter = len(groups)
            self.groups = groups

        else:
            if len(valid_fps) == 1:
                idx = valid_fps[0]
                indexes = [idx]
                while len(indexes) < 3:
                    indexes.append(0xFF)
                self.groups = [{"group_id": idx, "indexes": indexes}]
            else:
                self.groups = []

        self.added_fingerprints = []
        if not self.data_new_version:
            for idx in self.fingerprint_list:
                self.added_fingerprints.append(
                    ListItemBtn(
                        self.container,
                        _(i18n_keys.FORM__FINGER_STR).format(idx + 1),
                        left_img_src="A:/res/settings-fingerprint.png",
                        has_next=False,
                    )
                    if idx is not None
                    else None
                )
        else:
            for group in self.groups:
                self.added_fingerprints.append(
                    ListItemBtn(
                        self.container,
                        _(i18n_keys.FORM__FINGER_STR).format(group["group_id"] + 1),
                        left_img_src="A:/res/settings-fingerprint.png",
                        has_next=False,
                    )
                )

        self.add_fingerprint = None

        if counter < 2:
            self.add_fingerprint = ListItemBtn(
                self.container,
                _(i18n_keys.BUTTON__ADD_FINGERPRINT),
                left_img_src="A:/res/settings-plus.png",
                has_next=False,
            )

        self.container_fun = ContainerFlexCol(
            self.content_area, self.container, pos=(0, 12), padding_row=1
        )
        self.unlock = ListItemBtnWithSwitch(
            self.container_fun, _(i18n_keys.FORM__UNLOCK_DEVICE)
        )

        if not storage_device.is_fingerprint_unlock_enabled():
            self.unlock.clear_state()

    def _parse_group_data(self, data):
        if data and data[0] != 0xFF:
            return {"group_id": data[0], "indexes": data[1:4]}
        return None

    async def on_remove(self, fp_id):
        from trezorio import fingerprint

        fingerprint.remove(fp_id)

        self.fresh_show()

    async def on_remove_group(self, group):
        from trezorio import fingerprint

        group_bytes = bytes([group["group_id"]]) + bytes(group["indexes"])
        fingerprint.remove_group(group_bytes)
        self.fresh_show()

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            from trezor.lvglui.scrs import fingerprints

            if target == self.add_fingerprint:
                if not self.groups:
                    group_id = 0
                else:
                    current_group_id = self.groups[0]["group_id"]
                    if current_group_id not in (0, 1):
                        group_id = 0
                    else:
                        group_id = 1 - current_group_id
                workflow.spawn(
                    fingerprints.add_fingerprint(
                        group_id=group_id,
                        callback=lambda: self.fresh_show(),
                    )
                )
            elif target in self.added_fingerprints:
                for i, item in enumerate(self.added_fingerprints):
                    if target == item:
                        if self.data_new_version:
                            group = self.groups[i]
                            prompt = _(i18n_keys.FORM__FINGER_STR).format(
                                group["group_id"] + 1
                            )
                            workflow.spawn(
                                fingerprints.request_delete_fingerprint(
                                    prompt,
                                    on_remove=lambda: self.on_remove_group(group),
                                )
                            )
                        else:
                            fp_id = self.fingerprint_list[i]
                            prompt = _(i18n_keys.FORM__FINGER_STR).format(i + 1)
                            workflow.spawn(
                                fingerprints.request_delete_fingerprint(
                                    prompt, on_remove=lambda: self.on_remove(fp_id)
                                )
                            )

    def on_value_changed(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.unlock.switch:

                if target.has_state(lv.STATE.CHECKED):
                    storage_device.enable_fingerprint_unlock(True)
                else:
                    storage_device.enable_fingerprint_unlock(False)


class SafetyCheckSetting(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "description") and self.description:
            targets.append(self.description)
        if hasattr(self, "warning_desc") and self.warning_desc:
            targets.append(self.warning_desc)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        super().__init__(
            prev_scr=prev_scr,
            title=_(i18n_keys.TITLE__SAFETY_CHECKS),
            nav_back=True,
        )

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.safety_check = ListItemBtnWithSwitch(
            self.container, _(i18n_keys.ITEM__SAFETY_CHECKS)
        )
        self.description = lv.label(self.content_area)
        self.description.set_size(456, lv.SIZE.CONTENT)
        self.description.set_long_mode(lv.label.LONG.WRAP)
        self.description.set_style_text_font(font_GeistRegular26, lv.STATE.DEFAULT)
        self.description.set_style_text_line_space(3, 0)
        self.description.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 8, 16)
        self.description.set_recolor(True)
        self.retrieval_state()

        self.container.add_event_cb(self.on_click, lv.EVENT.VALUE_CHANGED, None)
        self.add_event_cb(self.on_click, lv.EVENT.READY, None)
        self.load_screen(self)
        gc.collect()

    def retrieval_state(self):
        if safety_checks.is_strict():
            self.safety_check.add_state()
            self.description.set_text(_(i18n_keys.CONTENT__SAFETY_CHECKS_STRICT__HINT))
            self.description.set_style_text_color(
                lv_colors.ONEKEY_GRAY, lv.STATE.DEFAULT
            )
            self.clear_warning_desc()
        else:
            self.safety_check.clear_state()
            if safety_checks.is_prompt_always():
                self.description.set_text(
                    _(i18n_keys.CONTENT__SAFETY_CHECKS_PERMANENTLY_PROMPT__HINT)
                )
                self.add_warning_desc(LEVEL.DANGER)
            else:
                self.description.set_text(
                    _(i18n_keys.CONTENT__SAFETY_CHECKS_TEMPORARILY_PROMPT__HINT)
                )
                self.add_warning_desc(LEVEL.WARNING)

    def add_warning_desc(self, level):
        if not hasattr(self, "warning_desc"):
            self.warning_desc = Banner(
                self.content_area, level, _(i18n_keys.MSG__SAFETY_CHECKS_PROMPT_WARNING)
            )

    def clear_warning_desc(self):
        if hasattr(self, "warning_desc"):
            self.warning_desc.delete()
            del self.warning_desc

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.safety_check.switch:
                if target.has_state(lv.STATE.CHECKED):
                    SafetyCheckStrictConfirm(self)
                else:
                    SafetyCheckPromptConfirm(self)
        elif code == lv.EVENT.READY:
            self.retrieval_state()


class SafetyCheckStrictConfirm(FullSizeWindow):
    def __init__(self, callback_obj):
        super().__init__(
            _(i18n_keys.TITLE__ENABLE_SAFETY_CHECKS),
            _(i18n_keys.SUBTITLE__ENABLE_SAFETY_CHECKS),
            confirm_text=_(i18n_keys.BUTTON__CONFIRM),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            # icon_path="A:/res/warning.png",
        )
        self.callback = callback_obj

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.btn_yes:
                safety_checks.apply_setting(SafetyCheckLevel.Strict)
            elif target != self.btn_no:
                return
            lv.event_send(self.callback, lv.EVENT.READY, None)
            self.destroy(0)


class SafetyCheckPromptConfirm(FullSizeWindow):
    def __init__(self, callback_obj):
        super().__init__(
            _(i18n_keys.TITLE__DISABLE_SAFETY_CHECKS),
            _(i18n_keys.SUBTITLE__SET_SAFETY_CHECKS_TO_PROMPT),
            confirm_text=_(i18n_keys.BUTTON__SLIDE_TO_DISABLE),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            # icon_path="A:/res/warning.png",
            hold_confirm=True,
            anim_dir=0,
        )
        self.slider.change_knob_style(1)
        self.callback = callback_obj

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.btn_no.click_mask:
                self.destroy(100)
        elif code == lv.EVENT.READY:
            if target == self.slider:
                safety_checks.apply_setting(SafetyCheckLevel.PromptTemporarily)
                self.destroy(0)
        lv.event_send(self.callback, lv.EVENT.READY, None)


class WalletScreen(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "rest_device") and self.rest_device:
            targets.append(self.rest_device)
        if hasattr(self, "advanced_zone") and self.advanced_zone:
            targets.append(self.advanced_zone)
        if hasattr(self, "air_gap") and self.air_gap:
            targets.append(self.air_gap)
        if hasattr(self, "description") and self.description:
            targets.append(self.description)
        if hasattr(self, "danger_zone") and self.danger_zone:
            targets.append(self.danger_zone)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        super().__init__(prev_scr, title=_(i18n_keys.TITLE__WALLET), nav_back=True)

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.check_mnemonic = ListItemBtn(
            self.container, _(i18n_keys.ITEM__CHECK_RECOVERY_PHRASE)
        )
        from apps.common import backup_types

        if backup_types.is_extendable_backup_type(storage_device.get_backup_type()):
            self.mul_share_bk = ListItemBtn(
                self.container, _(i18n_keys.BUTTON__CREATE_MULTI_SHARE_BACKUP)
            )
        self.passphrase = ListItemBtn(self.container, _(i18n_keys.ITEM__PASSPHRASE))
        self.turbo_mode = ListItemBtn(self.container, _(i18n_keys.TITLE__TURBO_MODE))
        self.trezor_mode = ListItemBtnWithSwitch(
            self.container, _(i18n_keys.ITEM__COMPATIBLE_WITH_TREZOR)
        )
        self.trezor_mode.add_style(
            StyleWrapper().bg_color(lv_colors.ONEKEY_BLACK_3).bg_opa(lv.OPA.COVER), 0
        )
        if not storage_device.is_trezor_compatible():
            self.trezor_mode.clear_state()
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.trezor_mode.add_event_cb(
            self.on_value_changed, lv.EVENT.VALUE_CHANGED, None
        )

        # Advanced Settings
        self.advanced_zone = lv.label(self.content_area)
        self.advanced_zone.set_size(456, lv.SIZE.CONTENT)
        self.advanced_zone.set_long_mode(lv.label.LONG.WRAP)
        self.advanced_zone.set_style_text_color(lv_colors.WHITE_2, lv.STATE.DEFAULT)
        self.advanced_zone.set_style_text_font(font_GeistSemiBold30, lv.STATE.DEFAULT)
        self.advanced_zone.set_text(_(i18n_keys.TITLE__ADVANCED))
        self.advanced_zone.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 12, 28)

        self.air_gap = ListItemBtnWithSwitch(
            self.content_area, _(i18n_keys.ITEM__AIR_GAP_MODE)
        )
        self.air_gap.add_style(
            StyleWrapper().bg_color(lv_colors.ONEKEY_BLACK_3).bg_opa(lv.OPA.COVER), 0
        )
        self.air_gap.set_style_radius(40, 0)
        self.air_gap.align_to(self.advanced_zone, lv.ALIGN.OUT_BOTTOM_LEFT, -12, 16)

        self.description = lv.label(self.content_area)
        self.description.set_size(456, lv.SIZE.CONTENT)
        self.description.set_long_mode(lv.label.LONG.WRAP)
        self.description.set_style_text_color(lv_colors.ONEKEY_GRAY, lv.STATE.DEFAULT)
        self.description.set_style_text_font(font_GeistRegular26, lv.STATE.DEFAULT)
        self.description.set_style_text_line_space(3, 0)
        self.description.align_to(self.air_gap, lv.ALIGN.OUT_BOTTOM_LEFT, 12, 16)
        air_gap_enabled = storage_device.is_airgap_mode()
        if air_gap_enabled:
            self.air_gap.add_state()
            self.description.set_text(
                _(
                    i18n_keys.CONTENT__BLUETOOTH_USB_AND_NFT_TRANSFER_FUNCTIONS_HAVE_BEEN_DISABLED
                )
            )
        else:
            self.air_gap.clear_state()
            self.description.set_text(
                _(
                    i18n_keys.CONTENT__AFTER_ENABLING_THE_AIRGAP_BLUETOOTH_USB_AND_NFC_TRANSFER_WILL_BE_DISABLED_SIMULTANEOUSLY
                )
            )
        self.air_gap.add_event_cb(self.on_event, lv.EVENT.VALUE_CHANGED, None)
        self.air_gap.add_event_cb(self.on_event, lv.EVENT.READY, None)
        self.air_gap.add_event_cb(self.on_event, lv.EVENT.CANCEL, None)
        # Danger Zone: Reset Device
        self.danger_zone = lv.label(self.content_area)
        self.danger_zone.set_size(456, lv.SIZE.CONTENT)
        self.danger_zone.set_long_mode(lv.label.LONG.WRAP)
        self.danger_zone.set_style_text_color(lv_colors.WHITE_2, lv.STATE.DEFAULT)
        self.danger_zone.set_style_text_font(font_GeistSemiBold30, lv.STATE.DEFAULT)
        self.danger_zone.set_text(_(i18n_keys.TITLE__DANGER_ZONE))
        self.danger_zone.align_to(self.description, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 40)
        self.rest_device = ListItemBtn(
            self.content_area,
            _(i18n_keys.ITEM__RESET_DEVICE),
            has_next=False,
        )
        self.rest_device.label_left.set_style_text_color(lv_colors.ONEKEY_RED_1, 0)
        self.rest_device.align_to(self.danger_zone, lv.ALIGN.OUT_BOTTOM_MID, -12, 16)
        self.rest_device.set_style_radius(40, 0)
        self.rest_device.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.load_screen(self)
        gc.collect()

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            from trezor.wire import DUMMY_CONTEXT

            if target == self.check_mnemonic:
                from apps.management.recovery_device import recovery_device
                from trezor.messages import RecoveryDevice

                utils.set_backup_none()
                workflow.spawn(
                    recovery_device(
                        DUMMY_CONTEXT,
                        RecoveryDevice(dry_run=True, enforce_wordlist=True),
                    )
                )
            elif hasattr(self, "mul_share_bk") and target == self.mul_share_bk:
                from apps.management.recovery_device.create_mul_shares import (
                    create_multi_share_backup,
                )

                workflow.spawn(create_multi_share_backup())
            elif target == self.passphrase:
                PassphraseScreen(self)
            elif target == self.turbo_mode:
                TurboModeScreen(self)
            elif target == self.rest_device:
                from apps.management.wipe_device import wipe_device
                from trezor.messages import WipeDevice

                workflow.spawn(wipe_device(DUMMY_CONTEXT, WipeDevice()))

    def on_value_changed(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.trezor_mode.switch:
                TrezorModeToggle(self, not storage_device.is_trezor_compatible())

    def reset_switch(self):
        if storage_device.is_trezor_compatible():
            self.trezor_mode.add_state()
        else:
            self.trezor_mode.clear_state()

    def on_event(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.air_gap.switch:
                from trezor.lvglui.scrs.template import AirGapToggleTips

                if target.has_state(lv.STATE.CHECKED):
                    AirGapToggleTips(
                        enable=True,
                        callback_obj=self.air_gap,
                    )
                else:
                    AirGapToggleTips(
                        enable=False,
                        callback_obj=self.air_gap,
                    )
        elif code == lv.EVENT.READY:
            if not storage_device.is_airgap_mode():
                self.description.set_text(
                    _(
                        i18n_keys.CONTENT__BLUETOOTH_USB_AND_NFT_TRANSFER_FUNCTIONS_HAVE_BEEN_DISABLED
                    )
                )
                self.danger_zone.align_to(
                    self.description, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 40
                )
                self.rest_device.align_to(
                    self.danger_zone, lv.ALIGN.OUT_BOTTOM_MID, -12, 16
                )
                utils.enable_airgap_mode()
            else:
                self.description.set_text(
                    _(
                        i18n_keys.CONTENT__AFTER_ENABLING_THE_AIRGAP_BLUETOOTH_USB_AND_NFC_TRANSFER_WILL_BE_DISABLED_SIMULTANEOUSLY
                    )
                )
                self.danger_zone.align_to(
                    self.description, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 40
                )
                self.rest_device.align_to(
                    self.danger_zone, lv.ALIGN.OUT_BOTTOM_MID, -12, 16
                )
                utils.disable_airgap_mode()
        elif code == lv.EVENT.CANCEL:
            if storage_device.is_airgap_mode():
                self.air_gap.add_state()
            else:
                self.air_gap.clear_state()


class FidoKeysSetting(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "description") and self.description:
            targets.append(self.description)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            if not self.is_visible():
                self._load_scr(self)
            return
        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.FIDO_FIDO_KEYS_LABEL), nav_back=True
        )

        self.container = ContainerFlexCol(self.content_area, self.title)
        self.fido = ListItemBtnWithSwitch(
            self.container, _(i18n_keys.SECURITY__ENABLE_FIDO_KEYS)
        )
        self.description = lv.label(self.content_area)
        self.description.set_size(456, lv.SIZE.CONTENT)
        self.description.set_long_mode(lv.label.LONG.WRAP)
        self.description.set_style_text_color(lv_colors.ONEKEY_GRAY, lv.STATE.DEFAULT)
        self.description.set_style_text_font(font_GeistRegular26, lv.STATE.DEFAULT)
        self.description.set_style_text_line_space(3, 0)
        self.description.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 8, 16)

        self.reset_state()
        self.container.add_event_cb(self.on_value_changed, lv.EVENT.VALUE_CHANGED, None)
        self.load_screen(self)

    def reset_state(self):
        if storage_device.is_fido_enabled():
            self.fido.add_state()
            self.description.set_text(_(i18n_keys.SECURITY__ENABLE_FIDO_KEYS_DESC))
        else:
            self.fido.clear_state()
            self.description.set_text(_(i18n_keys.FIDO_DISABLED_INFO_TEXT))

    def on_value_changed(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.fido.switch:
                FidoKeysToggle(self, not storage_device.is_fido_enabled())


class FidoKeysToggle(FullSizeWindow):
    def __init__(self, callback_obj, enable=False):
        super().__init__(
            title=_(
                i18n_keys.SECURITY__ENABLE_FIDO_KEYS
                if enable
                else i18n_keys.SECURITY__DISABLE_FIDO_KEYS
            ),
            subtitle=_(i18n_keys.SUBTITLE__RESTORE_TREZOR_COMPATIBILITY),
            confirm_text=_(i18n_keys.BUTTON__RESTART),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
        )
        self.enable = enable
        self.callback_obj = callback_obj

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.btn_no:
                self.callback_obj.reset_state()
                self.destroy(200)
            elif target == self.btn_yes:

                async def restart_delay():
                    await loop.sleep(1000)
                    utils.reset()

                loop.pop_tasks_on_iface(io.UART | io.POLL_READ)
                storage_device.set_fido_enable(self.enable)
                workflow.spawn(restart_delay())


class PassphraseScreen(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "description") and self.description:
            targets.append(self.description)
        if hasattr(self, "advance_label") and self.advance_label:
            targets.append(self.advance_label)
        if hasattr(self, "attach_to_pin") and self.attach_to_pin:
            targets.append(self.attach_to_pin)
        if hasattr(self, "pin_description") and self.pin_description:
            targets.append(self.pin_description)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            if not self.is_visible():
                self._load_scr(self, lv.scr_act() != self)
            return
        super().__init__(
            prev_scr=prev_scr,
            title=_(i18n_keys.TITLE__PASSPHRASE),
            nav_back=True,
        )

        self.container = ContainerFlexCol(self.content_area, self.title)
        self.passphrase = ListItemBtnWithSwitch(
            self.container, _(i18n_keys.ITEM__PASSPHRASE)
        )

        self.description = lv.label(self.content_area)
        self.description.set_size(456, lv.SIZE.CONTENT)
        self.description.set_long_mode(lv.label.LONG.WRAP)
        self.description.set_style_text_color(lv_colors.ONEKEY_GRAY, lv.STATE.DEFAULT)
        self.description.set_style_text_font(font_GeistRegular26, lv.STATE.DEFAULT)
        self.description.set_style_text_line_space(3, 0)
        self.description.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 8, 16)

        self.advance_label = lv.label(self.content_area)
        self.advance_label.set_text(_(i18n_keys.PASSPHRASE__ADVANCE))
        self.advance_label.set_style_text_color(lv_colors.WHITE, lv.STATE.DEFAULT)
        self.advance_label.set_style_text_font(font_GeistRegular26, lv.STATE.DEFAULT)

        self.attach_to_pin = ListItemBtn(
            self.content_area,
            _(i18n_keys.PASSPHRASE__ATTACH_TO_PIN),
            left_img_src="A:/res/icon-attach-to-pin.png",
        )
        self.attach_to_pin.add_style(
            StyleWrapper().bg_color(lv_colors.ONEKEY_GRAY_3).bg_opa(lv.OPA.COVER), 0
        )
        self.attach_to_pin.set_style_radius(40, 0)

        self.pin_description = lv.label(self.content_area)
        self.pin_description.set_text(_(i18n_keys.PASSPHRASE__ATTACH_TO_PIN_DESC))
        self.pin_description.set_size(456, lv.SIZE.CONTENT)
        self.pin_description.set_long_mode(lv.label.LONG.WRAP)
        self.pin_description.set_style_text_color(
            lv_colors.ONEKEY_GRAY, lv.STATE.DEFAULT
        )
        self.pin_description.set_style_text_font(font_GeistRegular26, lv.STATE.DEFAULT)

        passphrase_enable = storage_device.is_passphrase_enabled()
        if passphrase_enable:
            self.passphrase.add_state()
            self.description.set_text(_(i18n_keys.PASSPHRASE__ENABLE_DESC))
            self.advance_label.clear_flag(lv.obj.FLAG.HIDDEN)
            self.attach_to_pin.clear_flag(lv.obj.FLAG.HIDDEN)
            self.pin_description.clear_flag(lv.obj.FLAG.HIDDEN)
        else:
            self.passphrase.clear_state()
            self.description.set_text(_(i18n_keys.CONTENT__PASSPHRASE_DISABLED__HINT))
            self.advance_label.add_flag(lv.obj.FLAG.HIDDEN)
            self.attach_to_pin.add_flag(lv.obj.FLAG.HIDDEN)
            self.pin_description.add_flag(lv.obj.FLAG.HIDDEN)

        self._update_layout()

        self.container.add_event_cb(self.on_value_changed, lv.EVENT.VALUE_CHANGED, None)
        self.attach_to_pin.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.add_event_cb(self.on_value_changed, lv.EVENT.READY, None)
        self.add_event_cb(self.on_value_changed, lv.EVENT.CANCEL, None)
        self.load_screen(self)
        gc.collect()

    def _update_layout(self):
        self.description.refresh_self_size()

        lv.timer_handler()

        advance_y_offset = 40

        self.advance_label.align_to(
            self.description, lv.ALIGN.OUT_BOTTOM_LEFT, 0, advance_y_offset
        )

        self.attach_to_pin.align_to(
            self.advance_label, lv.ALIGN.OUT_BOTTOM_LEFT, -8, 12
        )

        self.pin_description.align_to(
            self.attach_to_pin, lv.ALIGN.OUT_BOTTOM_LEFT, 8, 8
        )

    def on_value_changed(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.passphrase.switch:
                if target.has_state(lv.STATE.CHECKED):
                    screen = PassphraseTipsConfirm(
                        _(i18n_keys.TITLE__ENABLE_PASSPHRASE),
                        _(i18n_keys.SUBTITLE__ENABLE_PASSPHRASE),
                        _(i18n_keys.BUTTON__ENABLE),
                        self,
                        primary_color=lv_colors.ONEKEY_YELLOW,
                    )
                    screen.btn_yes.enable(lv_colors.ONEKEY_YELLOW, lv_colors.BLACK)
                else:
                    subtitle = None
                    from trezor.crypto import se_thd89
                    from apps.common.pin_constants import AttachCommon

                    current_space = se_thd89.get_pin_passphrase_space()
                    if current_space < AttachCommon.MAX_PASSPHRASE_PIN_NUM:
                        subtitle = _(i18n_keys.TITLE__DISABLE_PASSPHRASE_DESC)
                    else:
                        subtitle = _(i18n_keys.SUBTITLE__DISABLE_PASSPHRASE)

                    PassphraseTipsConfirm(
                        _(i18n_keys.TITLE__DISABLE_PASSPHRASE),
                        subtitle,
                        _(i18n_keys.BUTTON__DISABLE),
                        self,
                        icon_path="",
                    )
            elif target == self.attach_to_pin.switch:

                storage_device.set_passphrase_always_on_device(
                    target.has_state(lv.STATE.CHECKED)
                )

        elif code == lv.EVENT.READY:
            if self.passphrase.switch.has_state(lv.STATE.CHECKED):
                self.description.set_text(_(i18n_keys.PASSPHRASE__ENABLE_DESC))
                storage_device.set_passphrase_enabled(True)
                storage_device.set_passphrase_always_on_device(False)
                self.advance_label.clear_flag(lv.obj.FLAG.HIDDEN)
                self.attach_to_pin.clear_flag(lv.obj.FLAG.HIDDEN)
                self.pin_description.clear_flag(lv.obj.FLAG.HIDDEN)
                self._update_layout()
            else:
                self.description.set_text(
                    _(i18n_keys.CONTENT__PASSPHRASE_DISABLED__HINT)
                )
                storage_device.set_passphrase_enabled(False)
                if storage_device.is_passphrase_pin_enabled():
                    from apps.base import lock_device_if_unlocked

                    storage_device.set_passphrase_pin_enabled(False)
                    lock_device_if_unlocked()
                    return

                self.advance_label.add_flag(lv.obj.FLAG.HIDDEN)
                self.attach_to_pin.add_flag(lv.obj.FLAG.HIDDEN)
                self.pin_description.add_flag(lv.obj.FLAG.HIDDEN)

                self._update_layout()

        elif code == lv.EVENT.CANCEL:
            if self.passphrase.switch.has_state(lv.STATE.CHECKED):
                self.passphrase.clear_state()
                self.advance_label.add_flag(lv.obj.FLAG.HIDDEN)
                self.attach_to_pin.add_flag(lv.obj.FLAG.HIDDEN)
                self.pin_description.add_flag(lv.obj.FLAG.HIDDEN)
                self._update_layout()
            else:
                self.passphrase.add_state()
                self.attach_to_pin.clear_flag(lv.obj.FLAG.HIDDEN)
                self.advance_label.clear_flag(lv.obj.FLAG.HIDDEN)
                self.attach_to_pin.clear_flag(lv.obj.FLAG.HIDDEN)
                self.pin_description.clear_flag(lv.obj.FLAG.HIDDEN)
                self._update_layout()

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.attach_to_pin:
                global _attach_to_pin_task_running

                _attach_to_pin_task_running = True

                async def handle_attach_to_pin():
                    try:
                        from trezor.ui.layouts.lvgl.attach_to_pin import (
                            show_attach_to_pin_window,
                        )

                        ctx = wire.DUMMY_CONTEXT
                        result = await show_attach_to_pin_window(ctx)

                        if result:
                            self.load_screen(self)

                        return result
                    except Exception:
                        self.load_screen(self)
                        return False
                    finally:
                        global _attach_to_pin_task_running
                        _attach_to_pin_task_running = False

                workflow.spawn(handle_attach_to_pin())


class PassphraseTipsConfirm(FullSizeWindow):
    def __init__(
        self,
        title: str,
        subtitle: str,
        confirm_text: str,
        callback_obj,
        icon_path="A:/res/warning.png",
        primary_color=lv_colors.ONEKEY_GREEN,
    ):
        super().__init__(
            title,
            subtitle,
            confirm_text,
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            icon_path=icon_path,
            anim_dir=2,
            primary_color=primary_color,
        )
        self.callback_obj = callback_obj

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            elif target == self.btn_no:
                lv.event_send(self.callback_obj, lv.EVENT.CANCEL, None)
            elif target == self.btn_yes:
                lv.event_send(self.callback_obj, lv.EVENT.READY, None)
            else:
                return
            self.show_dismiss_anim()


class TurboModeScreen(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "tips") and self.tips:
            targets.append(self.tips)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        super().__init__(prev_scr, title=_(i18n_keys.TITLE__TURBO_MODE), nav_back=True)

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)

        self.turbo_mode = ListItemBtnWithSwitch(
            self.container, _(i18n_keys.TITLE__TURBO_MODE)
        )
        self.turbo_mode.add_style(
            StyleWrapper().bg_color(lv_colors.ONEKEY_BLACK_3).bg_opa(lv.OPA.COVER), 0
        )
        if not storage_device.is_turbomode_enabled():
            self.turbo_mode.clear_state()

        self.tips = lv.label(self.content_area)
        self.tips.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 8, 16)
        self.tips.set_long_mode(lv.label.LONG.WRAP)
        self.tips.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular26)
            .width(448)
            .text_color(lv_colors.WHITE_2)
            .text_align_left(),
            0,
        )
        self.tips.set_text(
            _(
                i18n_keys.CONTENT__SIGN_TRANSACTIONS_WITH_ONE_CLICK_ONLY_EVM_NETWORK_AND_SOLANA
            )
        )

        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.turbo_mode.add_event_cb(
            self.on_value_changed, lv.EVENT.VALUE_CHANGED, None
        )

        self.load_screen(self)
        gc.collect()

    def on_value_changed(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.turbo_mode.switch:
                if not storage_device.is_turbomode_enabled():
                    TurboModeConfirm(self, True)
                    self.turbo_mode.add_state()
                else:
                    self.turbo_mode.clear_state()
                    storage_device.set_turbomode_enable(False)

    def reset_switch(self):
        self.turbo_mode.clear_state()


class TurboModeConfirm(FullSizeWindow):
    def __init__(self, callback_obj, enable=False):
        if enable:
            super().__init__(
                title=_(i18n_keys.TITLE__ENABLE_TURBO_MODE),
                subtitle=_(i18n_keys.CONTENT__SIGN_TRANSACTIONS_WITH_ONE_CLICK),
                confirm_text=_(i18n_keys.ACTION__SLIDE_TO_ENABLE),
                cancel_text=_(i18n_keys.BUTTON__CANCEL),
                hold_confirm=True,
            )
            self.container = ContainerFlexCol(
                self.content_area, self.subtitle, padding_row=2
            )
            self.item1 = ListItemWithLeadingCheckbox(
                self.container,
                _(
                    i18n_keys.ACTION__ONCE_ENABLED_THE_DEVICE_WILL_OMIT_DETAILS_WHEN_REVIEWING_TRANSACTIONS_I_KNOW_THE_RISKS
                ),
                radius=40,
            )

            self.enable = enable
            self.callback_obj = callback_obj

        self.slider_enable(False)
        self.container.add_event_cb(self.on_value_changed, lv.EVENT.VALUE_CHANGED, None)

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.btn_no.click_mask:
                self.callback_obj.reset_switch()
                self.destroy(200)
        elif code == lv.EVENT.READY and self.hold_confirm:
            if target == self.slider:
                storage_device.set_turbomode_enable(self.enable)
                self.destroy(200)

    def on_value_changed(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.item1.checkbox:
                if target.get_state() & lv.STATE.CHECKED:
                    self.item1.enable_bg_color()
                    self.slider_enable()
                else:
                    self.item1.enable_bg_color(False)
                    self.slider_enable(False)

    def slider_enable(self, enable: bool = True):
        if enable:
            self.slider.add_flag(lv.obj.FLAG.CLICKABLE)
            self.slider.enable()
            self.slider.set_style_bg_color(
                lv_colors.WHITE, lv.PART.KNOB | lv.STATE.DEFAULT
            )
        else:
            self.slider.clear_flag(lv.obj.FLAG.CLICKABLE)
            self.slider.enable(False)


class EthereumSetting(Screen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        super().__init__(prev_scr, title=_(i18n_keys.TITLE__ETHEREUM), nav_back=True)

        self.container = ContainerFlexCol(self, self.title, padding_row=2)
        self.blind_sign = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__BLIND_SIGNING),
            right_text=_(i18n_keys.ITEM__STATUS__OFF),
        )
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.blind_sign:
                BlindSign(self, coin_type=_(i18n_keys.TITLE__ETHEREUM))


class SolanaSetting(Screen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        super().__init__(prev_scr, title=_(i18n_keys.TITLE__SOLANA), nav_back=True)

        self.container = ContainerFlexCol(self, self.title, padding_row=2)
        self.blind_sign = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__BLIND_SIGNING),
            right_text=_(i18n_keys.ITEM__STATUS__OFF),
        )
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.blind_sign:
                BlindSign(self, coin_type=_(i18n_keys.TITLE__SOLANA))


class BlindSign(Screen):
    def __init__(self, prev_scr=None, coin_type: str = _(i18n_keys.TITLE__ETHEREUM)):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            self.coin_type = coin_type
            return
        super().__init__(
            prev_scr, title=_(i18n_keys.TITLE__BLIND_SIGNING), nav_back=True
        )

        self.coin_type = coin_type
        self.container = ContainerFlexCol(self, self.title, padding_row=2)
        self.blind_sign = ListItemBtnWithSwitch(
            self.container, f"{coin_type} Blind Signing"
        )
        self.blind_sign.clear_state()
        self.container.add_event_cb(self.on_value_changed, lv.EVENT.VALUE_CHANGED, None)
        self.popup = None

    def on_value_changed(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.blind_sign.switch:
                if target.has_state(lv.STATE.CHECKED):
                    from .components.popup import Popup

                    self.popup = Popup(
                        self,
                        _(i18n_keys.TITLE__ENABLE_STR_BLIND_SIGNING).format(
                            self.coin_type
                        ),
                        _(i18n_keys.SUBTITLE_SETTING_CRYPTO_BLIND_SIGN_ENABLED),
                        icon_path="A:/res/warning.png",
                        btn_text=_(i18n_keys.BUTTON__ENABLE),
                    )
                else:
                    pass


class UserGuide(AnimScreen):
    def collect_animation_targets(self) -> list:
        if lv.scr_act() == MainScreen._instance:
            return []
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
            self.from_appdrawer = True
            kwargs = {
                "prev_scr": prev_scr,
                "title": _(i18n_keys.APP__USER_GUIDE),
                "nav_back": True,
            }
            super().__init__(**kwargs)
        else:
            if not self.is_visible():
                self._load_scr(self, lv.scr_act() != self)
            self.from_appdrawer = False
            self.refresh_text()
            return

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.base_tutorial = ListItemBtn(
            self.container, _(i18n_keys.ITEM__BASIC_TUTORIAL)
        )
        self.security_protection = ListItemBtn(
            self.container, _(i18n_keys.ITEM__SECURITY_PROTECTION)
        )
        self.need_help = ListItemBtn(self.container, _(i18n_keys.ITEM__NEED_HELP))
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.load_screen(self)

    def refresh_text(self):
        self.title.set_text(_(i18n_keys.APP__USER_GUIDE))
        self.base_tutorial.label_left.set_text(_(i18n_keys.ITEM__BASIC_TUTORIAL))
        self.security_protection.label_left.set_text(
            _(i18n_keys.ITEM__SECURITY_PROTECTION)
        )
        self.need_help.label_left.set_text(_(i18n_keys.ITEM__NEED_HELP))

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.base_tutorial:
                BaseTutorial(self)
            elif target == self.security_protection:
                SecurityProtection(self)
            elif target == self.need_help:
                HelpDetails()
            else:
                pass

    def _load_scr(self, scr: "AnimScreen", back: bool = False) -> None:
        if self.from_appdrawer:
            scr.set_pos(0, 0)
            lv.scr_load(scr)
        else:
            super()._load_scr(scr, back)


class BaseTutorial(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
            self.from_appdrawer = True
            kwargs = {
                "prev_scr": prev_scr,
                "title": _(i18n_keys.APP__USER_GUIDE),
                "nav_back": True,
            }
            super().__init__(**kwargs)
        else:
            self.from_appdrawer = False
            self.refresh_text()
            return

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.app_tutorial = ListItemBtn(
            self.container, _(i18n_keys.ITEM__ONEKEY_APP_TUTORIAL)
        )
        self.power_off = ListItemBtn(
            self.container,
            _(i18n_keys.TITLE__POWER_ON_OFF__GUIDE),
        )
        self.recovery_phrase = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__WHAT_IS_RECOVERY_PHRASE),
        )
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.load_screen(self)
        gc.collect()

    def refresh_text(self):
        self.title.set_text(_(i18n_keys.APP__USER_GUIDE))
        self.app_tutorial.label_left.set_text(_(i18n_keys.ITEM__ONEKEY_APP_TUTORIAL))
        self.power_off.label_left.set_text(_(i18n_keys.TITLE__POWER_ON_OFF__GUIDE))
        self.recovery_phrase.label_left.set_text(
            _(i18n_keys.ITEM__WHAT_IS_RECOVERY_PHRASE)
        )

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.app_tutorial:
                from trezor.lvglui.scrs import app_guide

                app_guide.GuideAppDownload()
            elif target == self.power_off:
                PowerOnOffDetails()
            elif target == self.recovery_phrase:
                RecoveryPhraseDetails()
            else:
                pass


class SecurityProtection(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
            self.from_appdrawer = True
            kwargs = {
                "prev_scr": prev_scr,
                "title": _(i18n_keys.APP__USER_GUIDE),
                "nav_back": True,
            }
            super().__init__(**kwargs)
        else:
            self.from_appdrawer = False
            self.refresh_text()
            return

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.pin_protection = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__ENABLE_PIN_PROTECTION),
        )
        self.fingerprint = ListItemBtn(
            self.container,
            _(i18n_keys.TITLE__FINGERPRINT),
        )
        self.hardware_wallet = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__HOW_HARDWARE_WALLET_WORKS),
        )
        self.passphrase = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__PASSPHRASE_ACCESS_HIDDEN_WALLETS),
        )
        self.attach_to_pin = ListItemBtn(
            self.container, _(i18n_keys.PASSPHRASE__ATTACH_TO_PIN)
        )
        self.passkeys = ListItemBtn(
            self.container,
            _(i18n_keys.FIDO_FIDO_KEYS_LABEL),
        )
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.load_screen(self)

    def refresh_text(self):
        self.title.set_text(_(i18n_keys.APP__USER_GUIDE))
        self.pin_protection.label_left.set_text(
            _(i18n_keys.ITEM__ENABLE_PIN_PROTECTION)
        )
        self.fingerprint.label_left.set_text(_(i18n_keys.TITLE__FINGERPRINT))
        self.hardware_wallet.label_left.set_text(
            _(i18n_keys.ITEM__HOW_HARDWARE_WALLET_WORKS)
        )
        self.passphrase.label_left.set_text(
            _(i18n_keys.ITEM__PASSPHRASE_ACCESS_HIDDEN_WALLETS)
        )
        self.attach_to_pin.label_left.set_text(_(i18n_keys.PASSPHRASE__ATTACH_TO_PIN))
        self.passkeys.label_left.set_text(_(i18n_keys.FIDO_FIDO_KEYS_LABEL))

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.pin_protection:
                PinProtectionDetails()
            elif target == self.hardware_wallet:
                HardwareWalletDetails()
            elif target == self.passphrase:
                PassphraseDetails()
            elif target == self.fingerprint:
                FingerprintDetails()
            elif target == self.attach_to_pin:
                AttachToPinDetails()
            elif target == self.passkeys:
                from .app_passkeys import PasskeysRegister

                PasskeysRegister()
            else:
                pass


class AttachToPinDetails(FullSizeWindow):
    def __init__(self):
        super().__init__(
            None,
            None,
            cancel_text=_(i18n_keys.BUTTON__CLOSE),
            icon_path="A:/res/attach-to-pin-guide.png",
        )
        self.container = ContainerFlexCol(self.content_area, self.icon, pos=(0, 24))
        self.item = DisplayItemWithFont_30(
            self.container,
            _(i18n_keys.PASSPHRASE__ATTACH_TO_PIN),
            _(i18n_keys.ITEM__ATTACH_TO_PIN_DESC),
        )
        self.item.label_top.set_style_text_color(lv_colors.WHITE, 0)
        self.item.label.set_style_text_color(lv_colors.WHITE_2, 0)
        self.item.label.align_to(self.item.label_top, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 16)
        self.item.label.set_long_mode(lv.label.LONG.WRAP)


class PowerOnOffDetails(FullSizeWindow):
    def __init__(self):
        super().__init__(
            None,
            None,
            cancel_text=_(i18n_keys.BUTTON__CLOSE),
            icon_path="A:/res/power-on-off.png",
        )
        self.container = ContainerFlexCol(self.content_area, self.icon, pos=(0, 24))
        self.item = DisplayItemWithFont_30(
            self.container,
            _(i18n_keys.TITLE__POWER_ON_OFF__GUIDE),
            _(i18n_keys.SUBTITLE__POWER_ON_OFF__GUIDE),
        )
        self.item.label_top.set_style_text_color(lv_colors.WHITE, 0)
        self.item.label.set_style_text_color(lv_colors.WHITE_2, 0)
        self.item.label.set_long_mode(lv.label.LONG.WRAP)


class RecoveryPhraseDetails(FullSizeWindow):
    def __init__(self):
        super().__init__(
            None,
            None,
            cancel_text=_(i18n_keys.BUTTON__CLOSE),
            icon_path="A:/res/recovery-phrase.png",
        )
        self.container = ContainerFlexCol(self.content_area, self.icon, pos=(0, 24))
        self.item = DisplayItemWithFont_30(
            self.container,
            _(i18n_keys.TITLE__WHAT_IS_RECOVERY_PHRASE__GUIDE),
            _(i18n_keys.SUBTITLE__WHAT_IS_RECOVERY_PHRASE__GUIDE),
        )
        self.item.label_top.set_style_text_color(lv_colors.WHITE, 0)
        self.item.label.set_style_text_color(lv_colors.WHITE_2, 0)
        self.item.label.align_to(self.item.label_top, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 16)
        self.item.label.set_long_mode(lv.label.LONG.WRAP)


class PinProtectionDetails(FullSizeWindow):
    def __init__(self):
        super().__init__(
            None,
            None,
            cancel_text=_(i18n_keys.BUTTON__CLOSE),
            icon_path="A:/res/pin-protection.png",
        )
        self.container = ContainerFlexCol(self.content_area, self.icon, pos=(0, 24))
        self.item = DisplayItemWithFont_30(
            self.container,
            _(i18n_keys.TITLE__ENABLE_PIN_PROTECTION__GUIDE),
            _(i18n_keys.SUBTITLE__ENABLE_PIN_PROTECTION__GUIDE),
        )
        self.item.label_top.set_style_text_color(lv_colors.WHITE, 0)
        self.item.label.set_style_text_color(lv_colors.WHITE_2, 0)
        self.item.label.align_to(self.item.label_top, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 16)
        self.item.label.set_long_mode(lv.label.LONG.WRAP)


class FingerprintDetails(FullSizeWindow):
    def __init__(self):
        super().__init__(
            None,
            None,
            cancel_text=_(i18n_keys.BUTTON__CLOSE),
            icon_path="A:/res/power-on-off.png",
        )
        self.container = ContainerFlexCol(self.content_area, self.icon, pos=(0, 24))
        self.item = DisplayItemWithFont_30(
            self.container,
            _(i18n_keys.TITLE__FINGERPRINT),
            _(
                i18n_keys.CONTENT__AFTER_SETTING_UP_FINGERPRINT_YOU_CAN_USE_IT_TO_UNLOCK_THE_DEVICE
            ),
        )
        self.item.label_top.set_style_text_color(lv_colors.WHITE, 0)
        self.item.label.set_style_text_color(lv_colors.WHITE_2, 0)
        self.item.label.set_long_mode(lv.label.LONG.WRAP)


class HardwareWalletDetails(FullSizeWindow):
    def __init__(self):
        super().__init__(
            None,
            None,
            cancel_text=_(i18n_keys.BUTTON__CLOSE),
            icon_path="A:/res/hardware-wallet-works-way.png",
        )
        self.container = ContainerFlexCol(self.content_area, self.icon, pos=(0, 24))
        self.item = DisplayItemWithFont_30(
            self.container,
            _(i18n_keys.TITLE__HOW_HARDWARE_WALLET_WORKS__GUIDE),
            _(i18n_keys.SUBTITLE__HOW_HARDWARE_WALLET_WORKS__GUIDE),
        )
        self.item.label_top.set_style_text_color(lv_colors.WHITE, 0)
        self.item.label.set_style_text_color(lv_colors.WHITE_2, 0)
        self.item.label.align_to(self.item.label_top, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 16)
        self.item.label.set_long_mode(lv.label.LONG.WRAP)


class PassphraseDetails(FullSizeWindow):
    def __init__(self):
        super().__init__(
            None,
            None,
            cancel_text=_(i18n_keys.BUTTON__CLOSE),
            icon_path="A:/res/hidden-wallet.png",
        )
        self.container = ContainerFlexCol(self.content_area, self.icon, pos=(0, 24))
        self.item = DisplayItemWithFont_30(
            self.container,
            _(i18n_keys.TITLE__ACCESS_HIDDEN_WALLET),
            _(i18n_keys.SUBTITLE__PASSPHRASE_ACCESS_HIDDEN_WALLETS__GUIDE),
        )
        self.item.label_top.set_style_text_color(lv_colors.WHITE, 0)
        self.item.label.set_style_text_color(lv_colors.WHITE_2, 0)
        self.item.label.align_to(self.item.label_top, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 16)
        self.item.label.set_long_mode(lv.label.LONG.WRAP)


class HelpDetails(FullSizeWindow):
    def __init__(self):
        super().__init__(
            None,
            None,
            cancel_text=_(i18n_keys.BUTTON__CLOSE),
            icon_path="A:/res/onekey-help.png",
        )
        self.container = ContainerFlexCol(self.content_area, self.icon, pos=(0, 24))
        self.item = DisplayItemWithFont_30(
            self.container,
            _(i18n_keys.TITLE__NEED_HELP__GUIDE),
            _(i18n_keys.SUBTITLE__NEED_HELP__GUIDE),
        )
        self.item.label_top.set_style_text_color(lv_colors.WHITE, 0)
        self.item.label.set_style_text_color(lv_colors.WHITE_2, 0)
        self.item.label.set_long_mode(lv.label.LONG.WRAP)
        self.item.label.align_to(self.item.label_top, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 16)

        self.website = lv.label(self.item)
        self.website.set_style_text_font(font_GeistRegular30, 0)
        self.website.set_style_text_color(lv_colors.WHITE_2, 0)
        self.website.set_style_text_line_space(3, 0)
        self.website.set_style_text_letter_space(-1, 0)
        self.website.set_text("https://help.onekey.so/")
        self.website.align_to(self.item.label, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 0)
        self.underline = lv.line(self.item)
        self.underline.set_points(
            [
                {"x": 0, "y": 2},
                {"x": 305, "y": 2},
            ],
            2,
        )
        self.underline.set_style_line_color(lv_colors.WHITE_2, 0)
        self.underline.align_to(self.website, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 0)
