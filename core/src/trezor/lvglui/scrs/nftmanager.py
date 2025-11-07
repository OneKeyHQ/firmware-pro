import ujson as json
import math
from typing import Callable, TYPE_CHECKING

import storage.device as storage_device
from trezor import io as trezor_io, uart, utils, workflow
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.lvglui.lv_colors import lv_colors

from . import (
    font_GeistRegular20,
    font_GeistRegular26,
    font_GeistRegular30,
    font_GeistSemiBold38,
    font_GeistSemiBold48,
)
from .common import AnimScreen, FullSizeWindow, Screen, lv
from .components.banner import LEVEL, Banner
from .components.button import NormalButton
from .components.container import ContainerGrid
from .components.listitem import ImgGridItem
from .widgets.style import StyleWrapper
from .preview_utils import (
    create_preview_container,
    create_preview_image,
    create_top_mask,
)

if TYPE_CHECKING:
    from .homescreen import MainScreen

# Path constants to reduce qstr usage
_P1 = "1:/res/nfts/zooms"
_P2 = "1:/res/nfts/imgs/"
_P3 = "1:/res/nfts/desc/"
_P4 = "A:1:/res/nfts/zooms/"
_P5 = "A:1:/res/nfts/imgs/"
_P6 = "A:/res/"
_P7 = "A:/res/wallpaper-7.jpg"
_P8 = "A:/res/checkmark.png"
_P9 = "A:/res/btn-del-white.png"
_P10 = "A:/res/icon_example.png"
_P11 = "A:/res/blur_no_selected.png"
_P12 = "A:/res/blur_not_available.png"
_P13 = "A:/res/blur_selected.png"
_S1 = "NftLockScreenPreview.MainScreen"
_S2 = "NftLockScreenPreview.LockScreen"
_S3 = "NftHomeScreenPreview.MainScreen"
_K1 = "nft_preview_container"
_K2 = "nft_device_name"
_K3 = "nft_bluetooth_name"


def _cached_style(_name: str, factory: Callable[[], StyleWrapper]) -> StyleWrapper:
    return factory()


def _safe_wallpaper_src(path: str | None, context: str = "") -> str:
    if not path:
        return ""
    # LVGL expects paths prefixed with drive designator. Convert raw FAT path if needed.
    if path.startswith("1:/"):
        return "A:1:/" + path[len("1:/") :]
    return path


def _fatfs_file_exists(path: str) -> bool:
    if not path:
        return False
    normalized = path.replace("A:1:", "1:")
    if normalized.startswith("A:/"):
        normalized = "1:/" + normalized[len("A:/") :]
    if "/" not in normalized:
        return False
    dir_path, name = normalized.rsplit("/", 1)
    for _size, _attrs, entry in trezor_io.fatfs.listdir(dir_path):
        if entry == name:
            return True
    return False


def _get_main_screen_cls():
    from .homescreen import MainScreen

    return MainScreen


class WallpaperPreviewBase(AnimScreen):
    """Base class for wallpaper preview screens with common functionality."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def _create_preview_container(self, top_offset=118):
        self.content_area.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
        self.content_area.clear_flag(lv.obj.FLAG.SCROLLABLE)
        self.content_area.set_style_pad_bottom(0, 0)

        # Main container
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

        style = _cached_style(
            _K1,
            lambda: StyleWrapper()
            .bg_opa(lv.OPA.TRANSP)
            .pad_all(0)
            .border_width(0),
        )
        self.preview_container = create_preview_container(
            self.container,
            width=344,
            height=574,
            top_offset=top_offset,
            style=style,
            bg_color=lv.color_hex(0x000000),
            bg_opa=lv.OPA.COVER,
        )

    def _create_preview_image(self, image_path):
        src = _safe_wallpaper_src(image_path, "NftPreview.Image")

        # Reuse the generic preview helper so scaling matches the system wallpaper preview.
        self.preview_image = create_preview_image(
            self.preview_container,
            base_size=(480, 800),
            target_size=(344, 574),
        )
        self.preview_image.set_src(src)

        self.preview_mask = create_top_mask(self.preview_container, height=5)
        return self.preview_image

    def _create_app_icons(self):
        self.app_icons = []
        scale_x = 343.0 / 480.0
        scale_y = 572.0 / 800.0
        offset_x = 0.5
        offset_y = 1.0

        desktop_positions = [
            (64, 164),   # Left-Top
            (272, 164),  # Right-Top
            (64, 442),   # Left-Bottom
            (272, 442),  # Right-Bottom
        ]

        for i in range(4):
            screen_x, screen_y = desktop_positions[i]
            x_pos = int(screen_x * scale_x + offset_x)
            y_pos = int(screen_y * scale_y + offset_y)

            icon_img = lv.img(self.preview_container)
            icon_img.set_src(_P10)
            icon_img.set_size(lv.SIZE.CONTENT, lv.SIZE.CONTENT)
            icon_img.set_pos(x_pos, y_pos)
            self.app_icons.append(icon_img)

    def _create_button_with_label(self, icon_path, text, callback):
        """Create a button with icon and label."""
        button = lv.btn(self.container)
        button.set_size(64, 64)
        button.align_to(self.preview_container, lv.ALIGN.OUT_BOTTOM_MID, 0, 10)
        button.add_style(StyleWrapper().border_width(0).radius(40), 0)
        button.add_flag(lv.obj.FLAG.CLICKABLE)
        button.clear_flag(lv.obj.FLAG.EVENT_BUBBLE)

        icon = lv.img(button)
        if icon_path:
            icon.set_src(icon_path)
        icon.align(lv.ALIGN.CENTER, 0, 0)

        label = lv.label(self.container)
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

        return button, icon, label

    def _check_blur_exists(self, blur_path):
        """Check if blur version exists."""
        return _fatfs_file_exists(blur_path)

    def _update_blur_button_state(self):
        """Update blur button state and appearance."""
        if not hasattr(self, "blur_exists"):
            return

        if not self.blur_exists:
            icon_path = _P12
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
                icon_path = _P13
            else:
                icon_path = _P11

        self.blur_button_icon.set_src(icon_path)

class NftGallery(Screen):
    def __init__(self, prev_scr=None):
        is_reinit = hasattr(self, "_init")

        if not is_reinit:
            self._init = True
            kwargs = {
                "prev_scr": prev_scr,
                "title": _(i18n_keys.TITLE__NFT_GALLERY),
                "nav_back": True,
            }
            super().__init__(**kwargs)

            # Keep right scrollbar but remove bottom scrollbar
            self.content_area.set_scroll_dir(lv.DIR.VER)
            self.content_area.set_width(480)
            self.content_area.clear_flag(lv.obj.FLAG.SCROLL_CHAIN)
            self.content_area.clear_flag(lv.obj.FLAG.SCROLL_MOMENTUM)
            self.content_area.clear_flag(lv.obj.FLAG.SCROLL_ELASTIC)
        else:
            # Re-entering: clean up old UI elements and reload screen
            if hasattr(self, "overview") and self.overview:
                self.overview.delete()
            if hasattr(self, "container") and self.container:
                self.container.delete()
            if hasattr(self, "empty_tips") and self.empty_tips:
                self.empty_tips.delete()
            if hasattr(self, "tips_bar") and self.tips_bar:
                self.tips_bar.delete()
            # Don't update prev_scr on re-entry to maintain correct navigation chain

        nft_counts = 0
        file_name_list = []
        if not utils.EMULATOR:
            for size, _attrs, name in trezor_io.fatfs.listdir(_P1):
                if nft_counts >= 24:
                    break
                if size > 0:
                    nft_counts += 1
                    file_name_list.append(name)
        if nft_counts == 0:
            self.empty()
        else:
            rows_num = math.ceil(nft_counts / 2)
            row_dsc = [226] * rows_num
            row_dsc.append(lv.GRID_TEMPLATE.LAST)
            # 2 columns
            col_dsc = [
                226,
                226,
                lv.GRID_TEMPLATE.LAST,
            ]

            self.overview = lv.label(self.content_area)
            self.overview.set_size(456, lv.SIZE.CONTENT)
            self.overview.add_style(
                StyleWrapper()
                .text_font(font_GeistRegular30)
                .text_color(lv_colors.WHITE_2)
                .text_align_left()
                .text_letter_space(-1),
                0,
            )
            self.overview.align_to(self.title, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 32)
            self.overview.set_text(
                _(i18n_keys.CONTENT__STR_ITEMS).format(nft_counts)
                if nft_counts > 1
                else _(i18n_keys.CONTENT__STR_ITEM).format(nft_counts)
            )
            # Use zero horizontal padding to keep the gallery within the viewport and avoid horizontal scrollbar.
            self.container = ContainerGrid(
                self.content_area,
                row_dsc=row_dsc,
                col_dsc=col_dsc,
                align_base=self.title,
                pos=(-12, 74),
                pad_gap=4,
            )
            self.nfts = []
            if not utils.EMULATOR:
                file_name_list.sort(
                    key=lambda name: int(
                        name[5:].split("-")[-1][: -(len(name.split(".")[1]) + 1)]
                    )
                )
                for i, file_name in enumerate(file_name_list):
                    path_dir = _P4
                    current_nft = ImgGridItem(
                        self.container,
                        (i) % 2,
                        (i) // 2,
                        file_name,
                        path_dir,
                        is_internal=False,
                        style_type="nft",  # Use NFT style - no clipping, full image display
                    )
                    self.nfts.append(current_nft)

            self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

        # Load screen if this is a re-initialization (second+ time)
        # First initialization calls load_screen automatically via super().__init__()
        if is_reinit:
            self.load_screen(self)

    def empty(self):

        self.empty_tips = lv.label(self.content_area)
        self.empty_tips.set_text(_(i18n_keys.CONTENT__NO_ITEMS))
        self.empty_tips.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular30)
            .text_color(lv_colors.WHITE_2)
            .text_letter_space(-1),
            0,
        )
        self.empty_tips.align(lv.ALIGN.TOP_MID, 0, 372)

        self.tips_bar = Banner(
            self.content_area,
            LEVEL.HIGHLIGHT,
            _(i18n_keys.CONTENT__HOW_TO_COLLECT_NFT__HINT),
        )

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target not in self.nfts:
                return
            for nft in self.nfts:
                if target == nft:
                    file_name_without_ext = nft.file_name.split(".")[0][5:]
                    desc_file_path = f"1:/res/nfts/desc/{file_name_without_ext}.json"
                    metadata = {
                        "header": "",
                        "subheader": "",
                        "network": "",
                        "owner": "",
                    }
                    with trezor_io.fatfs.open(desc_file_path, "r") as f:
                        description = bytearray(2048)
                        n = f.read(description)
                        if 0 < n < 2048:
                            metadata_load = json.loads(
                                (description[:n]).decode("utf-8")
                            )
                            if metadata_load and all(
                                key in metadata_load for key in metadata.keys()
                            ):
                                metadata = metadata_load
                    NftManager(self, metadata, nft.file_name)


class NftManager(AnimScreen):
    def __init__(self, prev_scr, nft_config, file_name):
        self.zoom_path = f"A:1:/res/nfts/zooms/{file_name}"
        self.file_name = file_name.replace("zoom-", "")
        self.img_path = f"A:1:/res/nfts/imgs/{self.file_name}"

        super().__init__(
            prev_scr=prev_scr,
            nav_back=True,
        )
        self.nft_config = nft_config

        # Disable horizontal scrolling and keep only the vertical scrollbar.
        self.content_area.set_scroll_dir(lv.DIR.VER)
        self.content_area.set_width(480)
        self.content_area.clear_flag(lv.obj.FLAG.SCROLL_CHAIN)
        self.content_area.clear_flag(lv.obj.FLAG.SCROLL_MOMENTUM)
        self.content_area.clear_flag(lv.obj.FLAG.SCROLL_ELASTIC)
        # Ensure detail page uses solid background instead of underlying wallpaper.
        self.content_area.add_style(
            StyleWrapper().bg_color(lv_colors.BLACK).bg_opa(lv.OPA.COVER),
            0,
        )

        # Add trash icon to title bar (right side)
        self.trash_icon = lv.imgbtn(self.content_area)
        self.trash_icon.set_src(
            lv.imgbtn.STATE.RELEASED, _P9, None, None
        )
        self.trash_icon.set_size(40, 40)
        self.trash_icon.align(lv.ALIGN.TOP_RIGHT, -16, 60)
        self.trash_icon.add_style(
            StyleWrapper().bg_opa(lv.OPA.TRANSP).border_width(0), 0
        )
        self.trash_icon.add_flag(lv.obj.FLAG.EVENT_BUBBLE)  # Enable event bubbling

        # Main NFT image (456x456 as requested)
        self.nft_image = lv.img(self.content_area)
        self.nft_image.set_src(self.img_path)
        self.nft_image.set_size(456, 456)
        self.nft_image.align(lv.ALIGN.TOP_MID, 0, 128)  # Position image 128px from top
        self.nft_image.add_style(StyleWrapper().radius(20).clip_corner(True), 0)

        # Title text below image
        self.nft_title = lv.label(self.content_area)
        self.nft_title.set_text(nft_config["header"] or "Title")
        self.nft_title.set_long_mode(lv.label.LONG.WRAP)
        self.nft_title.set_size(456, lv.SIZE.CONTENT)
        self.nft_title.add_style(
            StyleWrapper()
            .text_font(font_GeistSemiBold48)
            .text_color(lv_colors.WHITE)
            .text_align(lv.TEXT_ALIGN.LEFT),
            0,
        )
        self.nft_title.align_to(self.nft_image, lv.ALIGN.OUT_BOTTOM_LEFT, 12, 12)

        # Description text below title
        self.nft_description = lv.label(self.content_area)
        self.nft_description.set_text(
            nft_config["subheader"] or "Type description here."
        )
        self.nft_description.set_long_mode(lv.label.LONG.WRAP)  # Enable text wrapping
        self.nft_description.set_size(
            456, lv.SIZE.CONTENT
        )  # Max width 456px, auto height
        self.nft_description.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular30)
            .text_color(lv_colors.WHITE_2)
            .text_align(lv.TEXT_ALIGN.LEFT),
            0,
        )
        self.nft_description.align_to(self.nft_title, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 8)

        # Set as Lock Screen button (purple) - height 98px as requested
        self.btn_lock_screen = NormalButton(self.content_area)
        self.btn_lock_screen.set_size(456, 98)
        self.btn_lock_screen.enable(lv_colors.ONEKEY_PURPLE, lv_colors.BLACK)
        self.btn_lock_screen.label.set_text(
            _(i18n_keys.BUTTON__SET_AS_LOCK_SCREEN)
        )
        self.btn_lock_screen.align_to(
            self.nft_description, lv.ALIGN.OUT_BOTTOM_LEFT, -8, 32
        )

        # Set as Home Screen button (gray) - height 98px as requested
        self.btn_home_screen = NormalButton(self.content_area)
        self.btn_home_screen.set_size(456, 98)
        self.btn_home_screen.enable(lv_colors.GRAY_1, lv_colors.WHITE)
        self.btn_home_screen.label.set_text(
            _(i18n_keys.BUTTON__SET_AS_HOME_SCREEN)
        )
        self.btn_home_screen.align_to(
            self.btn_lock_screen, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 8
        )

    def del_callback(self):
        trezor_io.fatfs.unlink(self.zoom_path[2:])
        trezor_io.fatfs.unlink(self.img_path[2:])
        # Remove blur variant if present (A: prefix -> 1:/ for filesystem)
        img_file = self.img_path.split("/")[-1]
        if "." in img_file:
            base_name, ext = img_file.rsplit(".", 1)
            blur_file = f"{base_name}-blur.{ext}"
        else:
            blur_file = f"{img_file}-blur"
        blur_path = self.img_path.replace(img_file, blur_file)
        if _fatfs_file_exists(blur_path):
            trezor_io.fatfs.unlink(blur_path[2:])
        trezor_io.fatfs.unlink(
            _P3 + self.file_name.split(".")[0] + ".json"
        )

        replacement_path = _P7
        deleted_name = self.img_path.split("/")[-1]

        current_home = storage_device.get_appdrawer_background()
        current_lock = storage_device.get_homescreen()

        home_uses_image = current_home and (
            current_home == self.img_path or current_home.endswith("/" + deleted_name)
        )
        lock_uses_image = current_lock and (
            current_lock == self.img_path or current_lock.endswith("/" + deleted_name)
        )

        if home_uses_image:
            storage_device.set_appdrawer_background(replacement_path)
        if lock_uses_image:
            storage_device.set_homescreen(replacement_path)

        if home_uses_image or lock_uses_image:
            if hasattr(lv.img, "cache_invalidate_src") and _fatfs_file_exists(
                self.img_path
            ):
                lv.img.cache_invalidate_src(self.img_path)
            import trezor.lvglui.scrs.homescreen as hs_mod

            hs_mod._last_jpeg_loaded = None

        self.load_screen(self.prev_scr, destroy_self=True)

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if isinstance(target, lv.imgbtn):
                if target == self.nav_back.nav_btn:
                    if self.prev_scr is not None:
                        self.load_screen(self.prev_scr, destroy_self=True)
                elif target == self.trash_icon:
                    # Handle trash icon click - delete NFT
                    from trezor.ui.layouts import confirm_remove_nft
                    from trezor.wire import DUMMY_CONTEXT

                    workflow.spawn(
                        confirm_remove_nft(
                            DUMMY_CONTEXT,
                            self.del_callback,
                            self.zoom_path,
                        )
                    )
            else:
                if target == self.btn_lock_screen:
                    # Navigate to lock screen preview
                    NftLockScreenPreview(self, self.img_path, self.nft_config)
                elif target == self.btn_home_screen:
                    # Navigate to home screen preview
                    NftHomeScreenPreview(self, self.img_path, self.nft_config)


class NftLockScreenPreview(WallpaperPreviewBase):
    def __init__(self, prev_scr, nft_path, nft_config):
        super().__init__(
            prev_scr=prev_scr,
            title=_(i18n_keys.TITLE__PREVIEW),
            nav_back=True,
            rti_path=_P8,
        )
        self.nft_path = nft_path
        self.nft_config = nft_config

        # Use base class methods to create UI
        self._create_preview_container(top_offset=118)
        self.lockscreen_preview = self._create_preview_image(nft_path)

        # Device name and bluetooth name overlaid on the image
        device_name = storage_device.get_label() or "OneKey Pro"
        ble_name = storage_device.get_ble_name() or uart.get_ble_name()

        # Device name label (overlaid on image)
        self.device_name_label = lv.label(self.preview_container)
        self.device_name_label.set_text(device_name)
        self.device_name_label.add_style(
            _cached_style(
                _K2,
                lambda: StyleWrapper()
                .text_font(font_GeistSemiBold38)
                .text_color(lv_colors.WHITE)
                .text_align(lv.TEXT_ALIGN.CENTER),
            ),
            0,
        )
        self.device_name_label.align_to(self.preview_container, lv.ALIGN.TOP_MID, 0, 49)

        # Bluetooth name label (overlaid on image)
        self.bluetooth_label = lv.label(self.preview_container)
        if ble_name and len(ble_name) >= 4:
            self.bluetooth_label.set_text("Pro " + ble_name[-4:])
        else:
            self.bluetooth_label.set_text("Pro")
        self.bluetooth_label.add_style(
            _cached_style(
                _K3,
                lambda: StyleWrapper()
                .text_font(font_GeistRegular26)
                .text_color(lv_colors.WHITE)
                .text_align(lv.TEXT_ALIGN.CENTER),
            ),
            0,
        )
        self.bluetooth_label.align_to(
            self.device_name_label, lv.ALIGN.OUT_BOTTOM_MID, 0, 8
        )

    def eventhandler(self, event_obj):
        event = event_obj.code
        target = event_obj.get_target()
        if event == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if isinstance(target, lv.imgbtn):
                if hasattr(self, "nav_back") and target == self.nav_back.nav_btn:
                    if self.prev_scr is not None:
                        lv.scr_load(self.prev_scr)
                    return
                elif hasattr(self, "rti_btn") and target == self.rti_btn:
                    lockscreen_path = self.nft_path
                    MainScreen = _get_main_screen_cls()

                    storage_device.set_homescreen(lockscreen_path)

                    main_screen = getattr(MainScreen, "_instance", None)
                    safe_unlock_path = _safe_wallpaper_src(lockscreen_path, _S1)
                    if main_screen:
                        main_screen.set_background_image(safe_unlock_path)
                        if hasattr(main_screen, "apps") and main_screen.apps:
                            main_screen.apps.refresh_background()

                    if hasattr(lv.img, "cache_invalidate_src"):
                        lv.img.cache_invalidate_src(lockscreen_path)

                    import trezor.lvglui.scrs.homescreen as hs_mod
                    if hs_mod is not None:
                        hs_mod._last_jpeg_loaded = None

                    from .lockscreen import LockScreen as LockScreenCls  # type: ignore
                    LockScreenCls.invalidate("NFT wallpaper applied")
                    main_screen = (
                        MainScreen._instance
                        if hasattr(MainScreen, "_instance") and MainScreen._instance
                        else MainScreen()
                    )
                    self.load_screen(main_screen, destroy_self=True)
                    return


class NftHomeScreenPreview(WallpaperPreviewBase):
    def __init__(self, prev_scr, nft_path, nft_config):
        super().__init__(
            prev_scr=prev_scr,
            title=_(i18n_keys.TITLE__PREVIEW),
            nav_back=True,
            rti_path=_P8,
        )
        self.nft_path = nft_path
        self.nft_config = nft_config
        self.original_wallpaper_path = nft_path
        self.is_blur_active = False
        self.current_wallpaper_path = nft_path

        # Check if blur file exists
        file_name = nft_path.split("/")[-1]
        if "." in file_name:
            base_name, ext = file_name.rsplit(".", 1)
            blur_file = f"{base_name}-blur.{ext}"
        else:
            base_name, ext = file_name, ""
            blur_file = f"{base_name}-blur"
        self.blur_path = nft_path.replace(file_name, blur_file)
        self.blur_exists = self._check_blur_exists(self.blur_path)

        # Use base class methods to create UI
        self._create_preview_container(top_offset=118)
        self.homescreen_preview = self._create_preview_image(nft_path)
        self._create_app_icons()

        # Create Blur button
        (
            self.blur_button,
            self.blur_button_icon,
            self.blur_label,
        ) = self._create_button_with_label(_P11, _(i18n_keys.BUTTON__BLUR), self.on_blur_clicked)

        # Position blur button centered at bottom
        self.blur_button.align_to(
            self.preview_container, lv.ALIGN.OUT_BOTTOM_MID, 0, 10
        )
        self.blur_label.align_to(self.blur_button, lv.ALIGN.OUT_BOTTOM_MID, 0, 4)

        # Initialize blur button state
        self._update_blur_button_state()

    def on_blur_clicked(self, _event_obj):
        if self.blur_exists:
            self._toggle_blur()

    def _toggle_blur(self):
        if not self.blur_exists:
            return

        # Get original file name and construct blur path
        file_name = self.nft_path.split("/")[-1]
        if "." in file_name:
            base_name, ext = file_name.rsplit(".", 1)
            blur_file = f"{base_name}-blur.{ext}"
        else:
            blur_file = f"{file_name}-blur"

        # Construct blur path using the same directory structure (A: prefix preserved)
        blur_path = self.nft_path.replace(file_name, blur_file)

        if self.is_blur_active:
            # Switch to original
            self.current_wallpaper_path = self.original_wallpaper_path
            self.is_blur_active = False
        else:
            # Switch to blur
            self.current_wallpaper_path = blur_path
            self.is_blur_active = True

        self.homescreen_preview.set_src(self.current_wallpaper_path)
        self.homescreen_preview.align(lv.ALIGN.CENTER, 0, 0)
        self._update_blur_button_state()

    def eventhandler(self, event_obj):
        event = event_obj.code
        target = event_obj.get_target()
        if event == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if isinstance(target, lv.imgbtn):
                if hasattr(self, "nav_back") and target == self.nav_back.nav_btn:
                    if self.prev_scr is not None:
                        lv.scr_load(self.prev_scr)
                    return
                elif hasattr(self, "rti_btn") and target == self.rti_btn:
                    # Set as home screen - convert A:1: to A: format for storage
                    wallpaper_path = self.current_wallpaper_path
                    MainScreen = _get_main_screen_cls()

                    # Save old wallpaper path BEFORE setting new one
                    old_wallpaper = storage_device.get_appdrawer_background()

                    # Set the new AppDrawer background
                    storage_device.set_appdrawer_background(wallpaper_path)

                    # CRITICAL: Clear LVGL image cache for BOTH old and new wallpapers
                    if hasattr(lv.img, "cache_invalidate_src"):
                        if old_wallpaper and _fatfs_file_exists(old_wallpaper):
                            lv.img.cache_invalidate_src(old_wallpaper)
                        if _fatfs_file_exists(wallpaper_path):
                            lv.img.cache_invalidate_src(wallpaper_path)
                        lv.img.cache_invalidate_src(None)

                    main_screen = getattr(MainScreen, "_instance", None)
                    if main_screen:
                        # Refresh the AppDrawer background
                        if hasattr(main_screen, "apps") and main_screen.apps:
                            # Remove existing background style first to avoid style stacking
                            if getattr(main_screen.apps, "_background_style", None):
                                main_screen.apps.remove_style(
                                    main_screen.apps._background_style, 0
                                )
                                main_screen.apps._background_style = None

                            # Now refresh the background which will create a new style
                            main_screen.apps.refresh_background()

                            # Force invalidate to refresh display
                            if main_screen.apps.has_flag(lv.obj.FLAG.HIDDEN):
                                main_screen.apps.invalidate()

                    # Find the root MainScreen instance
                    main_screen = (
                        MainScreen._instance
                        if hasattr(MainScreen, "_instance") and MainScreen._instance
                        else MainScreen()
                    )

                    # Use AnimScreen's load_screen method for proper navigation
                    self.load_screen(main_screen, destroy_self=True)
                    return
            else:
                # Handle button clicks for Blur button only
                if hasattr(self, "blur_button") and target == self.blur_button:
                    self.on_blur_clicked(event_obj)
