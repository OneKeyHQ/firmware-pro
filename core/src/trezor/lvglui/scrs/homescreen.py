import gc
import math
from micropython import const

import storage.cache
from storage import device
from trezor import io, loop, uart, utils, wire, workflow
from trezor.enums import SafetyCheckLevel
from trezor.langs import langs, langs_keys
from trezor.lvglui.i18n import gettext as _, i18n_refresh, keys as i18n_keys
from trezor.lvglui.lv_colors import lv_colors
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

import ujson as json
from apps.common import passphrase, safety_checks

from ..lv_symbols import LV_SYMBOLS
from . import font_GeistRegular26, font_GeistRegular30, font_GeistSemiBold26
from .address import AddressManager, chain_list
from .common import AnimScreen, FullSizeWindow, Screen, lv  # noqa: F401, F403, F405
from .components.anim import Anim
from .components.banner import LEVEL, Banner
from .components.button import ListItemBtn, ListItemBtnWithSwitch, NormalButton
from .components.container import ContainerFlexCol, ContainerFlexRow, ContainerGrid
from .components.listitem import (
    DisplayItemWithFont_30,
    DisplayItemWithFont_TextPairs,
    ImgGridItem,
)
from .deviceinfo import DeviceInfoManager
from .widgets.style import StyleWrapper


def brightness2_percent_str(brightness: int) -> str:
    return f"{int(brightness / style.BACKLIGHT_MAX * 100)}%"


GRID_CELL_SIZE_ROWS = const(240)
GRID_CELL_SIZE_COLS = const(144)

if __debug__:
    APP_DRAWER_UP_TIME = 50
    APP_DRAWER_DOWN_TIME = 150
    APP_DRAWER_UP_DELAY = 15
    APP_DRAWER_DOWN_DELAY = 0
    PATH_OVER_SHOOT = lv.anim_t.path_overshoot
    PATH_BOUNCE = lv.anim_t.path_bounce
    PATH_LINEAR = lv.anim_t.path_linear
    PATH_EASE_IN_OUT = lv.anim_t.path_ease_in_out
    PATH_EASE_IN = lv.anim_t.path_ease_in
    PATH_EASE_OUT = lv.anim_t.path_ease_out
    PATH_STEP = lv.anim_t.path_step
    APP_DRAWER_UP_PATH_CB = PATH_EASE_OUT
    APP_DRAWER_DOWN_PATH_CB = PATH_EASE_IN_OUT


def change_state(is_busy: bool = False):
    if hasattr(MainScreen, "_instance"):
        if MainScreen._instance:
            MainScreen._instance.change_state(is_busy)


class MainScreen(Screen):
    def __init__(self, device_name=None, ble_name=None, dev_state=None):
        homescreen = device.get_homescreen()
        if not hasattr(self, "_init"):
            self._init = True
            super().__init__(
                title=device_name, subtitle=ble_name or uart.get_ble_name()
            )
            self.title.add_style(StyleWrapper().text_align_center(), 0)
            self.subtitle.add_style(
                StyleWrapper().text_align_center().text_color(lv_colors.WHITE), 0
            )
        else:
            self.add_style(
                StyleWrapper().bg_img_src(homescreen),
                0,
            )
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
            return
        self.title.align_to(self.content_area, lv.ALIGN.TOP_MID, 0, 76)
        self.subtitle.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, 16)
        if dev_state:
            self.dev_state = MainScreen.DevStateTipsBar(self)
            self.dev_state.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 48)
            self.dev_state.show(dev_state)

        self.add_style(
            StyleWrapper().bg_img_src(homescreen),
            0,
        )

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
        self.add_event_cb(self.on_slide_up, lv.EVENT.GESTURE, None)
        save_app_obj(self)

    def hidden_others(self, hidden: bool = True):
        # if hidden:
        #     # self.subtitle.add_flag(lv.obj.FLAG.HIDDEN)
        #     # self.title.add_flag(lv.obj.FLAG.HIDDEN)
        #     self.bottom_bar.add_flag(lv.obj.FLAG.HIDDEN)

        # else:
        #     # self.subtitle.clear_flag(lv.obj.FLAG.HIDDEN)
        #     # self.title.clear_flag(lv.obj.FLAG.HIDDEN)
        #     self.bottom_bar.clear_flag(lv.obj.FLAG.HIDDEN)
        pass

    def change_state(self, busy: bool):
        if busy:
            self.clear_flag(lv.obj.FLAG.CLICKABLE)
            self.up_arrow.add_flag(lv.obj.FLAG.HIDDEN)
            self.bottom_tips.set_text(_(i18n_keys.BUTTON__PROCESSING))
        else:
            self.add_flag(lv.obj.FLAG.CLICKABLE)
            self.up_arrow.clear_flag(lv.obj.FLAG.HIDDEN)
            self.bottom_tips.set_text(_(i18n_keys.BUTTON__SWIPE_TO_SHOW_APPS))

    def on_slide_up(self, event_obj):
        code = event_obj.code
        if code == lv.EVENT.GESTURE:
            _dir = lv.indev_get_act().get_gesture_dir()
            if _dir == lv.DIR.TOP:
                # child_cnt == 5 in common if in homepage
                if self.get_child_cnt() > 5:
                    return
                if self.is_visible():
                    # self.hidden_others()
                    # if hasattr(self, "dev_state"):
                    #     self.dev_state.hidden()
                    self.apps.show()
            elif _dir == lv.DIR.BOTTOM:
                lv.event_send(self.apps, lv.EVENT.GESTURE, None)

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

        def __init__(self, parent) -> None:
            super().__init__(parent)
            self.parent = parent
            self.remove_style_all()
            self.set_pos(0, 800)
            self.set_size(lv.pct(100), lv.pct(100))
            self.add_style(
                StyleWrapper().bg_color(lv_colors.BLACK).bg_opa().border_width(0),
                0,
            )
            self.img_down = lv.imgbtn(self)
            self.img_down.set_size(40, 40)
            self.img_down.set_style_bg_img_src("A:/res/slide-down.jpg", 0)
            self.img_down.align(lv.ALIGN.TOP_MID, 0, 64)
            self.img_down.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            self.img_down.set_ext_click_area(100)

            # buttons
            click_style = (
                StyleWrapper()
                .bg_img_recolor_opa(lv.OPA._30)
                .bg_img_recolor(lv_colors.BLACK)
            )
            default_desc_style = (
                StyleWrapper()
                .width(170)
                .text_font(font_GeistSemiBold26)
                .text_color(lv_colors.WHITE)
                .text_align_center()
            )
            pressed_desc_style = StyleWrapper().text_opa(lv.OPA._70)

            self.connect = lv.imgbtn(self)
            self.connect.set_size(216, 216)
            self.connect.set_pos(16, 148)
            self.connect.set_style_bg_img_src("A:/res/app-connect.jpg", 0)
            self.connect.add_style(click_style, lv.PART.MAIN | lv.STATE.PRESSED)
            self.connect.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            self.connect_desc = lv.label(self)
            self.connect_desc.set_text(_(i18n_keys.APP__CONNECT_WALLET))
            self.connect_desc.add_style(default_desc_style, 0)
            self.connect_desc.add_style(
                pressed_desc_style, lv.PART.MAIN | lv.STATE.PRESSED
            )
            self.connect_desc.align_to(self.connect, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)

            self.scan = lv.imgbtn(self)
            self.scan.set_size(216, 216)
            self.scan.align_to(self.connect, lv.ALIGN.OUT_RIGHT_MID, 16, 0)
            self.scan.set_style_bg_img_src("A:/res/app-scan.jpg", 0)
            self.scan.add_style(click_style, lv.PART.MAIN | lv.STATE.PRESSED)
            self.scan.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            self.scan_desc = lv.label(self)
            self.scan_desc.set_text(_(i18n_keys.APP__SCAN))
            self.scan_desc.add_style(default_desc_style, 0)
            self.scan_desc.add_style(
                pressed_desc_style, lv.PART.MAIN | lv.STATE.PRESSED
            )
            self.scan_desc.align_to(self.scan, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)

            self.my_address = lv.imgbtn(self)
            self.my_address.set_size(216, 216)
            self.my_address.align_to(self.connect, lv.ALIGN.OUT_BOTTOM_MID, 0, 77)
            self.my_address.set_style_bg_img_src("A:/res/app-address.jpg", 0)
            self.my_address.add_style(click_style, lv.PART.MAIN | lv.STATE.PRESSED)
            self.my_address.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            self.my_address_desc = lv.label(self)
            self.my_address_desc.set_text(_(i18n_keys.APP__ADDRESS))
            self.my_address_desc.add_style(default_desc_style, 0)
            self.my_address_desc.add_style(
                pressed_desc_style, lv.PART.MAIN | lv.STATE.PRESSED
            )
            self.my_address_desc.align_to(
                self.my_address, lv.ALIGN.OUT_BOTTOM_MID, 0, 8
            )

            self.settings = lv.imgbtn(self)
            self.settings.set_size(216, 216)
            self.settings.align_to(self.scan, lv.ALIGN.OUT_BOTTOM_MID, 0, 77)
            self.settings.set_style_bg_img_src("A:/res/app-settings.jpg", 0)
            self.settings.add_style(click_style, lv.PART.MAIN | lv.STATE.PRESSED)
            self.settings.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            self.settings_desc = lv.label(self)
            self.settings_desc.set_text(_(i18n_keys.APP__SETTINGS))
            self.settings_desc.add_style(default_desc_style, 0)
            self.settings_desc.add_style(
                pressed_desc_style, lv.PART.MAIN | lv.STATE.PRESSED
            )
            self.settings_desc.align_to(self.settings, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)

            self.passkey = lv.imgbtn(self)
            self.passkey.set_size(216, 216)
            self.passkey.set_pos(16, 148)
            self.passkey.set_style_bg_img_src("A:/res/app-keys.jpg", 0)
            self.passkey.add_style(click_style, lv.PART.MAIN | lv.STATE.PRESSED)
            self.passkey.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            self.passkey_desc = lv.label(self)
            self.passkey_desc.set_text(_(i18n_keys.FIDO_FIDO_KEYS_LABEL))
            self.passkey_desc.add_style(default_desc_style, 0)
            self.passkey_desc.add_style(
                pressed_desc_style, lv.PART.MAIN | lv.STATE.PRESSED
            )
            self.passkey_desc.align_to(self.passkey, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)
            self.passkey.add_flag(lv.obj.FLAG.HIDDEN)
            self.passkey_desc.add_flag(lv.obj.FLAG.HIDDEN)

            self.backup = lv.imgbtn(self)
            self.backup.set_size(216, 216)
            self.backup.set_style_bg_img_src("A:/res/app-backup.jpg", 0)
            self.backup.add_style(click_style, lv.PART.MAIN | lv.STATE.PRESSED)
            self.backup.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            self.backup.align_to(self.passkey, lv.ALIGN.OUT_RIGHT_MID, 16, 0)
            self.backup_desc = lv.label(self)
            self.backup_desc.set_text(_(i18n_keys.APP__BACK_UP))
            self.backup_desc.add_style(default_desc_style, 0)
            self.backup_desc.add_style(
                pressed_desc_style, lv.PART.MAIN | lv.STATE.PRESSED
            )
            self.backup_desc.align_to(self.backup, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)
            self.backup.add_flag(lv.obj.FLAG.HIDDEN)
            self.backup_desc.add_flag(lv.obj.FLAG.HIDDEN)

            self.nft = lv.imgbtn(self)
            self.nft.set_size(216, 216)
            self.nft.align_to(self.passkey, lv.ALIGN.OUT_BOTTOM_MID, 0, 77)
            self.nft.set_style_bg_img_src("A:/res/app-nft.jpg", 0)
            self.nft.add_style(click_style, lv.PART.MAIN | lv.STATE.PRESSED)
            self.nft.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            self.nft_desc = lv.label(self)
            self.nft_desc.set_text(_(i18n_keys.APP__NFT_GALLERY))
            self.nft_desc.add_style(default_desc_style, 0)
            self.nft_desc.add_style(pressed_desc_style, lv.PART.MAIN | lv.STATE.PRESSED)
            self.nft_desc.align_to(self.nft, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)
            self.nft.add_flag(lv.obj.FLAG.HIDDEN)
            self.nft_desc.add_flag(lv.obj.FLAG.HIDDEN)

            self.guide = lv.imgbtn(self)
            self.guide.set_size(216, 216)
            self.guide.align_to(self.backup, lv.ALIGN.OUT_BOTTOM_MID, 0, 77)
            self.guide.set_style_bg_img_src("A:/res/app-tips.jpg", 0)
            self.guide.add_style(click_style, lv.PART.MAIN | lv.STATE.PRESSED)
            self.guide.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            self.guide_desc = lv.label(self)
            self.guide_desc.set_text(_(i18n_keys.APP__TIPS))
            self.guide_desc.add_style(default_desc_style, 0)
            self.guide_desc.add_style(
                pressed_desc_style, lv.PART.MAIN | lv.STATE.PRESSED
            )
            self.guide_desc.align_to(self.guide, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)
            self.guide.add_flag(lv.obj.FLAG.HIDDEN)
            self.guide_desc.add_flag(lv.obj.FLAG.HIDDEN)

            self.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
            self.add_event_cb(self.on_pressed, lv.EVENT.PRESSED, None)
            self.add_event_cb(self.on_released, lv.EVENT.RELEASED, None)
            self.show_anim = Anim(
                800,
                0,
                self.set_position,
                start_cb=self.show_anim_start_cb,
                delay=15 if not __debug__ else APP_DRAWER_UP_DELAY,
                del_cb=self.show_anim_del_cb,
                time=130 if not __debug__ else APP_DRAWER_UP_TIME,
                path_cb=lv.anim_t.path_ease_in
                if not __debug__
                else APP_DRAWER_UP_PATH_CB,
            )
            self.dismiss_anim = Anim(
                0,
                800,
                self.set_position,
                path_cb=lv.anim_t.path_ease_out
                if not __debug__
                else APP_DRAWER_DOWN_PATH_CB,
                time=50 if not __debug__ else APP_DRAWER_DOWN_TIME,
                start_cb=self.dismiss_anim_start_cb,
                del_cb=self.dismiss_anim_del_cb,
                delay=0 if not __debug__ else APP_DRAWER_DOWN_DELAY,
            )
            self.slide = False
            self.visible = False
            # page indicator
            self.container = ContainerFlexRow(self, None, padding_col=0)
            self.container.align(lv.ALIGN.BOTTOM_MID, 0, -32)
            # indicator dots
            self.select_page_index = 0
            self.indicators = []
            for i in range(self.PAGE_SIZE):
                self.indicators.append(Indicator(self.container, i))
            self.clear_flag(lv.obj.FLAG.GESTURE_BUBBLE)
            self.add_event_cb(self.on_gesture, lv.EVENT.GESTURE, None)
            self.group_1 = [
                self.container,
                self.settings,
                self.settings_desc,
                self.my_address,
                self.my_address_desc,
                self.scan,
                self.scan_desc,
                self.connect,
                self.connect_desc,
                self.img_down,
            ]
            self.group_2 = [
                self.container,
                self.passkey,
                self.passkey_desc,
                self.guide,
                self.guide_desc,
                self.nft,
                self.nft_desc,
                self.backup,
                self.backup_desc,
                self.guide,
                self.guide_desc,
                self.img_down,
            ]

        def set_position(self, val):
            self.set_y(val)

        def on_gesture(self, event_obj):
            code = event_obj.code
            if code == lv.EVENT.GESTURE:
                indev = lv.indev_get_act()
                _dir = indev.get_gesture_dir()
                if _dir == lv.DIR.BOTTOM:
                    self.slide = True
                    self.dismiss()
                    return
                if _dir not in [lv.DIR.RIGHT, lv.DIR.LEFT]:
                    return
                self.indicators[self.select_page_index].set_active(False)
                if _dir == lv.DIR.LEFT:
                    self.select_page_index = (
                        self.select_page_index + 1
                    ) % self.PAGE_SIZE

                elif _dir == lv.DIR.RIGHT:
                    self.select_page_index = (
                        self.select_page_index - 1 + self.PAGE_SIZE
                    ) % self.PAGE_SIZE
                self.indicators[self.select_page_index].set_active(True)
                self.show_page(self.select_page_index)

        def show_page(self, index: int):
            if index == 0:
                for obj in self.group_2:
                    obj.add_flag(lv.obj.FLAG.HIDDEN)
                for obj in self.group_1:
                    obj.clear_flag(lv.obj.FLAG.HIDDEN)
            elif index == 1:
                for obj in self.group_1:
                    obj.add_flag(lv.obj.FLAG.HIDDEN)
                for obj in self.group_2:
                    obj.clear_flag(lv.obj.FLAG.HIDDEN)

        def hidden_page(self, index: int):
            # if index == 0:
            #     for obj in self.group_1:
            #         # obj.add_flag(lv.obj.FLAG.HIDDEN)
            #         pass
            # elif index == 1:
            #     for obj in self.group_2:
            #         # obj.add_flag(lv.obj.FLAG.HIDDEN)
            #         pass
            pass

        def show_anim_start_cb(self, _anim):
            self.parent.hidden_others()
            self.hidden_page(self.select_page_index)
            self.parent.clear_state(lv.STATE.USER_1)

        def show_anim_del_cb(self, _anim):
            self.show_page(self.select_page_index)

        def dismiss_anim_start_cb(self, _anim):
            self.hidden_page(self.select_page_index)

        def dismiss_anim_del_cb(self, _anim):
            self.parent.hidden_others(False)

        def show(self):
            if self.visible:
                return
            self.parent.add_state(lv.STATE.USER_1)
            self.show_anim.start()
            # if self.header.has_flag(lv.obj.FLAG.HIDDEN):
            #     self.header.clear_flag(lv.obj.FLAG.HIDDEN)
            self.slide = False
            self.visible = True

        def dismiss(self):
            if not self.visible:
                return
            # self.parent.hidden_others(False)
            if hasattr(self.parent, "dev_state"):
                self.parent.dev_state.show()
            # self.header.add_flag(lv.obj.FLAG.HIDDEN)
            self.dismiss_anim.start()
            self.visible = False

        def on_click(self, event_obj):
            code = event_obj.code
            target = event_obj.get_target()
            if code == lv.EVENT.CLICKED:
                if utils.lcd_resume():
                    return
                if self.slide:
                    return
                if target == self.settings:
                    SettingsScreen(self.parent)
                elif target == self.guide:
                    UserGuide(self.parent)
                elif target == self.nft:
                    NftGallery(self.parent)
                elif target == self.backup:
                    BackupWallet(self.parent)
                elif target == self.scan:
                    ScanScreen(self.parent)
                elif target == self.connect:
                    ConnectWalletWays(self.parent)
                elif target == self.my_address:
                    ShowAddress(self.parent)
                elif target == self.passkey:
                    PasskeysManager(self.parent)
                elif target == self.img_down:
                    self.dismiss()

        def on_pressed(self, event_obj):
            code = event_obj.code
            target = event_obj.get_target()
            if code == lv.EVENT.PRESSED:
                if utils.lcd_resume():
                    return
                if target == self.settings:
                    self.settings_desc.add_state(lv.STATE.PRESSED)
                elif target == self.guide:
                    self.guide_desc.add_state(lv.STATE.PRESSED)
                elif target == self.nft:
                    self.nft_desc.add_state(lv.STATE.PRESSED)
                elif target == self.backup:
                    self.backup_desc.add_state(lv.STATE.PRESSED)
                elif target == self.scan:
                    self.scan_desc.add_state(lv.STATE.PRESSED)
                elif target == self.connect:
                    self.connect_desc.add_state(lv.STATE.PRESSED)
                elif target == self.my_address:
                    self.my_address_desc.add_state(lv.STATE.PRESSED)
                elif target == self.passkey:
                    self.passkey_desc.add_state(lv.STATE.PRESSED)

        def on_released(self, event_obj):
            code = event_obj.code
            target = event_obj.get_target()
            if code == lv.EVENT.RELEASED:
                if utils.lcd_resume():
                    return
                if target == self.settings:
                    self.settings_desc.clear_state(lv.STATE.PRESSED)
                elif target == self.guide:
                    self.guide_desc.clear_state(lv.STATE.PRESSED)
                elif target == self.nft:
                    self.nft_desc.clear_state(lv.STATE.PRESSED)
                elif target == self.backup:
                    self.backup_desc.clear_state(lv.STATE.PRESSED)
                elif target == self.scan:
                    self.scan_desc.clear_state(lv.STATE.PRESSED)
                elif target == self.connect:
                    self.connect_desc.clear_state(lv.STATE.PRESSED)
                elif target == self.my_address:
                    self.my_address_desc.clear_state(lv.STATE.PRESSED)
                elif target == self.passkey:
                    self.passkey_desc.clear_state(lv.STATE.PRESSED)

        def refresh_text(self):
            self.settings_desc.set_text(_(i18n_keys.APP__SETTINGS))
            self.guide_desc.set_text(_(i18n_keys.APP__TIPS))
            self.nft_desc.set_text(_(i18n_keys.APP__NFT_GALLERY))
            self.backup_desc.set_text(_(i18n_keys.APP__BACK_UP))
            self.scan_desc.set_text(_(i18n_keys.APP__SCAN))
            self.connect_desc.set_text(_(i18n_keys.APP__CONNECT_WALLET))
            self.my_address_desc.set_text(_(i18n_keys.APP__ADDRESS))
            self.passkey_desc.set_text(_(i18n_keys.FIDO_FIDO_KEYS_LABEL))


class PasskeysManager(Screen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            if not self.is_visible():
                if hasattr(self, "container") and self.container:
                    self.container.delete()
                    del self.container
                self.fresh_show()
                lv.scr_load(self)
            return
        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.FIDO_FIDO_KEYS_LABEL), nav_back=True
        )

        self.fresh_show()
        self.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        # self.add_event_cb(self.on_scroll, lv.EVENT.SCROLL_BEGIN, None)

    async def list_credential(self):
        from .app_passkeys import PasskeysListItemBtn

        BATCH_SIZE = 5
        stored_credentials = []
        for i, credential in enumerate(self.credentials):
            stored_credentials.append(
                (
                    credential.app_name(),
                    credential.account_name(),
                    credential.index,
                    credential.creation_time,
                )
            )
            self.overlay.set_value(i + 1)
            if (i < BATCH_SIZE) or ((i + 1) % BATCH_SIZE == 0):
                await loop.sleep(10)
        stored_credentials.sort(key=lambda x: x[3])
        for i, credential in enumerate(stored_credentials):
            self.listed_credentials.append(
                PasskeysListItemBtn(
                    self.container,
                    credential[0],
                    credential[1] or "",
                    credential[2],
                )
            )
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
            self.listed_credentials = []

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
            self.learn_more = NormalButton(self, text=_(i18n_keys.ACTION__LEARN_MORE))
        else:
            if not hasattr(self, "container"):
                self.container = ContainerFlexCol(
                    self.content_area, self.title, padding_row=2
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
        credential = self.listed_credentials.pop(i)
        from .app_passkeys import delete_credential

        delete_credential(credential.credential_index)
        item_height = credential.get_height()
        credential.delete()
        self.fresh_show()
        self.auto_adjust_scroll(item_height)

    def on_click(self, event_obj):
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
                        workflow.spawn(
                            app_passkeys.request_credential_details(
                                credential.app_name,
                                credential.account_name,
                                on_remove=lambda index=i: self.on_remove(index),
                            )
                        )

    # def on_scroll(self, event_obj):
    #     code = event_obj.code
    #     if code == lv.EVENT.SCROLL_BEGIN:
    #         if self.count < 5:
    #             self.content_area.clear_flag(lv.obj.FLAG.SCROLL_ELASTIC)

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)


class ShowAddress(AnimScreen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self.prev_session_id = storage.cache.get_session_id()
            self.curr_session_id = storage.cache.start_session()

            if passphrase.is_enabled():
                workflow.spawn(
                    self._get_passphrase_from_user(init=True, prev_scr=prev_scr)
                )
            else:
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
            # self.container.delete()
            # self.init_ui()
            gc.collect()

    async def _get_passphrase_from_user(self, init=False, prev_scr=None):
        try:
            from apps.ethereum.onekey.get_address import get_address as eth_get_address
            from trezor import messages

            msg = messages.EthereumGetAddressOneKey(
                address_n=[0x80000000 + 44, 0x80000000 + 60, 0x80000000 + 0, 0, 0],
                show_display=False,
            )
            # pyright: off
            await eth_get_address(wire.DummyContext(), msg)
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
        # def create_fade_cb_container(obj, item_index):
        #     def cb(value):
        #         self.container.set_style_text_opa(value, 0)

        #         for btn in self.chain_buttons:
        #             btn.set_style_bg_opa(value, 0)
        #             btn.img_left.set_style_img_opa(value, 0)

        #     return cb

        def create_move_cb_container(obj, item_index):
            def cb(value):
                obj.set_style_translate_x(value, 0)
                obj.invalidate()

            return cb

        # container_fade_anim = Anim(
        #     150,
        #     255,
        #     create_fade_cb_container(self.container, 0),
        #     time=110,
        #     delay=0,
        #     path_cb=lv.anim_t.path_ease_out,
        # )

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

        # self.animations_next.append(container_fade_anim)
        # container_fade_anim.start()
        self.animations_next.append(container_move_anim)
        # container_move_anim.start()

        # self.animations_prev.append(container_fade_anim)
        self.animations_prev.append(container_move_back_anim)

    def init_ui(self):
        """Initialize UI components"""

        if passphrase.is_enabled():
            from .components.navigation import GeneralNavigation

            self.nav_passphrase = GeneralNavigation(
                self.content_area, img="A:/res/repeat.png"
            )
            self.nav_passphrase.align(lv.ALIGN.TOP_RIGHT, 0, 44)

            # self.nav_passphrase.align_to(self.nav_back, lv.ALIGN.RIGHT_MID, 222, 0)

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
        self.chains = chain_list
        self.visible_chains_count = 8

        self.is_expanded = False
        self.chain_buttons = []
        self.created_count = 0
        self.current_page = 0
        self.items_per_page = 5

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

        self.update_page_buttons()
        self.next_btn.add_style(self.enable_style, 0)

        self.animations_next = []
        self.animations_prev = []
        self.list_items = self.chain_buttons

        if device.is_animation_enabled():
            self.animate_list_items()

    def _create_visible_chain_buttons(self):
        start_idx = self.current_page * self.items_per_page
        end_idx = min(start_idx + self.items_per_page, len(self.chains))

        for i, btn in enumerate(self.chain_buttons):
            btn.remove_event_cb(None)
            # btn.set_style_opa(0, 0)
            if i < (end_idx - start_idx):
                chain = self.chains[start_idx + i]
                btn.label_left.set_text(chain["name"])
                btn.img_left.set_src(chain["icon_48"])
                btn.add_event_cb(
                    lambda e, name=chain["name"]: self.on_chain_click(e, name),
                    lv.EVENT.CLICKED,
                    None,
                )
                btn.clear_flag(lv.obj.FLAG.HIDDEN)

                if chain["name"] == "Ethereum" and not hasattr(btn, "img_right"):
                    btn.img_right = lv.img(btn)
                    btn.img_right.set_src("A:/res/stacked-chains.png")
                    btn.img_right.set_align(lv.ALIGN.RIGHT_MID)
                elif chain["name"] == "Ethereum" and hasattr(btn, "img_right"):
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
        max_pages = (len(self.chains) - 1) // self.items_per_page
        if self.current_page == 0:
            self.disable_page_buttons(self.back_btn)
            if not self.next_btn.has_flag(lv.btn.FLAG.CLICKABLE):
                self.enable_page_buttons(self.next_btn)

        elif self.current_page == max_pages:
            self.disable_page_buttons(self.next_btn)
            if not self.back_btn.has_flag(lv.btn.FLAG.CLICKABLE):
                self.enable_page_buttons(self.back_btn)

        else:
            if not self.next_btn.has_flag(lv.btn.FLAG.CLICKABLE):
                self.enable_page_buttons(self.next_btn)
            if not self.back_btn.has_flag(lv.btn.FLAG.CLICKABLE):
                self.enable_page_buttons(self.back_btn)

    def next_page(self):
        max_pages = (len(self.chains) - 1) // self.items_per_page
        if self.current_page < max_pages:
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
        """Handle account selection click"""
        IndexSelectionScreen(self)

    def on_chain_click(self, event, name):
        """Handle chain selection click"""
        if utils.lcd_resume():
            return

        workflow.spawn(self.addr_manager.generate_address(name, self.current_index))

    def update_index_btn_text(self):
        """Update account button text"""
        self.index_btn.label_left.set_text(f"Account #{self.current_index + 1}")
        # pass

    # def _load_scr(self, scr: "Screen", back: bool = False) -> None:
    #     lv.scr_load(scr)

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

                elif (
                    passphrase.is_enabled() and target == self.nav_passphrase.select_btn
                ):
                    # enter new passphrase
                    storage.cache.end_current_session()
                    self.curr_session_id = storage.cache.start_session()
                    workflow.spawn(self._get_passphrase_from_user(init=False))

            else:
                gc.collect()
                if target == self.back_btn:
                    self.prev_page()
                elif target == self.next_btn:
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

        from .components.navigation import GeneralNavigation

        # # navi
        self.nav_opt = GeneralNavigation(self.content_area)
        # self.nav_opt.align_to(self.nav_back, lv.ALIGN.RIGHT_MID, 222, 0)
        self.nav_opt.align(lv.ALIGN.TOP_RIGHT, 0, 44)

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
        if device.is_animation_enabled():
            self.animate_list_items()

    def animate_list_items(self):
        # def create_fade_cb_container(obj, item_index):
        #     def cb(value):
        #         self.container.set_style_text_opa(value, 0)

        #         for btn in self.chain_buttons:
        #             btn.set_style_bg_opa(value, 0)
        #             # btn.img_left.set_style_img_opa(value, 0)

        #     return cb

        def create_move_cb_container(obj, item_index):
            def cb(value):
                obj.set_style_translate_x(value, 0)
                obj.invalidate()

            return cb

        # container_fade_anim = Anim(
        #     150,
        #     255,
        #     create_fade_cb_container(self.container, 0),
        #     time=110,
        #     delay=0,
        #     path_cb=lv.anim_t.path_ease_out,
        # )

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

        # self.animations_next.append(container_fade_anim)
        # container_fade_anim.start()
        self.animations_next.append(container_move_anim)
        container_move_anim.start()

        # self.animations_prev.append(container_fade_anim)
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
                elif target == self.nav_opt.select_btn:
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


class NftGallery(Screen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
            kwargs = {
                "prev_scr": prev_scr,
                "title": _(i18n_keys.TITLE__NFT_GALLERY),
                "nav_back": True,
            }
            super().__init__(**kwargs)
        else:
            self.overview.delete()
            self.container.delete()

        nft_counts = 0
        file_name_list = []
        if not utils.EMULATOR:
            for size, _attrs, name in io.fatfs.listdir("1:/res/nfts/zooms"):
                if nft_counts >= 24:
                    break
                if size > 0:
                    nft_counts += 1
                    file_name_list.append(name)
        if nft_counts == 0:
            self.empty()
        else:
            rows_num = math.ceil(nft_counts / 2)
            row_dsc = [238] * rows_num
            row_dsc.append(lv.GRID_TEMPLATE.LAST)
            # 2 columns
            col_dsc = [
                238,
                238,
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
                    path_dir = "A:1:/res/nfts/zooms/"
                    current_nft = ImgGridItem(
                        self.container,
                        (i) % 2,
                        (i) // 2,
                        file_name,
                        path_dir,
                        is_internal=False,
                    )
                    self.nfts.append(current_nft)

            self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

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
                    with io.fatfs.open(desc_file_path, "r") as f:
                        description = bytearray(2048)
                        n = f.read(description)
                        if 0 < n < 2048:
                            try:
                                metadata_load = json.loads(
                                    (description[:n]).decode("utf-8")
                                )
                            except BaseException as e:
                                if __debug__:
                                    print(f"Invalid json {e}")
                            else:
                                if all(
                                    key in metadata_load.keys()
                                    for key in metadata.keys()
                                ):
                                    metadata = metadata_load
                    NftManager(self, metadata, nft.file_name)

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)


class NftManager(Screen):
    def __init__(self, prev_scr, nft_config, file_name):
        self.zoom_path = f"A:1:/res/nfts/zooms/{file_name}"
        self.file_name = file_name.replace("zoom-", "")
        self.img_path = f"A:1:/res/nfts/imgs/{self.file_name}"
        super().__init__(
            prev_scr,
            title=nft_config["header"],
            subtitle=nft_config["subheader"],
            icon_path=self.img_path,
            nav_back=True,
        )
        self.nft_config = nft_config
        self.content_area.set_style_max_height(756, 0)
        self.icon.align_to(self.nav_back, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 8)
        self.title.align_to(self.icon, lv.ALIGN.OUT_BOTTOM_LEFT, 8, 16)
        self.subtitle.align_to(self.title, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 16)
        # self.icon.add_style(StyleWrapper().radius(40).clip_corner(True), 0)
        self.btn_yes = NormalButton(self.content_area)
        self.btn_yes.set_size(456, 98)
        self.btn_yes.enable(lv_colors.ONEKEY_PURPLE, lv_colors.BLACK)
        self.btn_yes.label.set_text(_(i18n_keys.BUTTON__SET_AS_HOMESCREEN))
        self.btn_yes.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 32)

        self.btn_del = NormalButton(self.content_area, "")
        self.btn_del.set_size(456, 98)
        self.btn_del.align_to(self.btn_yes, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)
        self.panel = lv.obj(self.btn_del)
        self.panel.remove_style_all()
        self.panel.set_size(lv.SIZE.CONTENT, lv.SIZE.CONTENT)
        self.panel.clear_flag(lv.obj.FLAG.CLICKABLE)
        self.panel.add_style(
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_BLACK)
            .text_color(lv_colors.ONEKEY_RED_1)
            .bg_opa(lv.OPA.TRANSP)
            .border_width(0)
            .align(lv.ALIGN.CENTER),
            0,
        )
        self.btn_del_img = lv.img(self.panel)
        self.btn_del_img.set_src("A:/res/btn-del.png")
        self.btn_label = lv.label(self.panel)
        self.btn_label.set_text(_(i18n_keys.BUTTON__DELETE))
        self.btn_label.align_to(self.btn_del_img, lv.ALIGN.OUT_RIGHT_MID, 4, 1)

    def del_callback(self):
        io.fatfs.unlink(self.zoom_path[2:])
        io.fatfs.unlink(self.img_path[2:])
        io.fatfs.unlink("1:/res/nfts/desc/" + self.file_name.split(".")[0] + ".json")
        if device.get_homescreen() == self.img_path:
            device.set_homescreen(utils.get_default_wallpaper())
        self.load_screen(self.prev_scr, destroy_self=True)

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)

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
            else:
                if target == self.btn_yes:
                    NftManager.ConfirmSetHomeScreen(self.img_path)

                elif target == self.btn_del:
                    from trezor.ui.layouts import confirm_remove_nft
                    from trezor.wire import DUMMY_CONTEXT

                    workflow.spawn(
                        confirm_remove_nft(
                            DUMMY_CONTEXT,
                            self.del_callback,
                            self.zoom_path,
                        )
                    )

    class ConfirmSetHomeScreen(FullSizeWindow):
        def __init__(self, homescreen):
            super().__init__(
                title=_(i18n_keys.TITLE__SET_AS_HOMESCREEN),
                subtitle=_(i18n_keys.SUBTITLE__SET_AS_HOMESCREEN),
                confirm_text=_(i18n_keys.BUTTON__CONFIRM),
                cancel_text=_(i18n_keys.BUTTON__CANCEL),
            )
            self.homescreen = homescreen

        def eventhandler(self, event_obj):
            code = event_obj.code
            target = event_obj.get_target()
            if code == lv.EVENT.CLICKED:
                if utils.lcd_resume():
                    return
                if target == self.btn_yes:
                    device.set_homescreen(self.homescreen)
                    self.destroy(0)
                    workflow.spawn(utils.internal_reloop())
                elif target == self.btn_no:
                    self.destroy()


class SettingsScreen(AnimScreen):
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
                "title": _(i18n_keys.TITLE__SETTINGS),
                "nav_back": True,
            }
            super().__init__(**kwargs)
        else:
            self.from_appdrawer = False
            self.refresh_text()
            return
        # if __debug__:
        #     self.add_style(StyleWrapper().bg_color(lv_colors.ONEKEY_GREEN_1), 0)
        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.general = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__GENERAL),
            left_img_src="A:/res/general.png",
        )
        # self.connect = ListItemBtn(
        #     self.container,
        #     _(i18n_keys.ITEM__CONNECT),
        #     left_img_src="A:/res/connect.png",
        # )
        self.air_gap = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__AIR_GAP_MODE),
            left_img_src="A:/res/connect.png",
        )
        self.home_scr = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__HOMESCREEN),
            left_img_src="A:/res/homescreen.png",
        )
        self.security = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__SECURITY),
            left_img_src="A:/res/security.png",
        )
        self.wallet = ListItemBtn(
            self.container, _(i18n_keys.ITEM__WALLET), left_img_src="A:/res/wallet.png"
        )
        self.about = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__ABOUT_DEVICE),
            left_img_src="A:/res/about.png",
        )
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.load_screen(self)

    def refresh_text(self):
        self.title.set_text(_(i18n_keys.TITLE__SETTINGS))
        self.general.label_left.set_text(_(i18n_keys.ITEM__GENERAL))
        # self.connect.label_left.set_text(_(i18n_keys.ITEM__CONNECT))
        self.air_gap.label_left.set_text(_(i18n_keys.ITEM__AIR_GAP_MODE))
        self.home_scr.label_left.set_text(_(i18n_keys.ITEM__HOMESCREEN))
        self.security.label_left.set_text(_(i18n_keys.ITEM__SECURITY))
        self.wallet.label_left.set_text(_(i18n_keys.ITEM__WALLET))
        self.about.label_left.set_text(_(i18n_keys.ITEM__ABOUT_DEVICE))

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.general:
                GeneralScreen(self)
            # elif target == self.connect:
            #     ConnectSetting(self)
            elif target == self.home_scr:
                HomeScreenSetting(self)
            elif target == self.security:
                SecurityScreen(self)
            elif target == self.wallet:
                WalletScreen(self)
            elif target == self.about:
                AboutSetting(self)
            # elif target == self.boot_loader:
            #     Go2UpdateMode(self)
            elif target == self.air_gap:
                AirGapSetting(self)

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        if self.from_appdrawer:
            scr.set_pos(0, 0)
            lv.scr_load(scr)
        else:
            super()._load_scr(scr, back)


class ConnectWalletWays(Screen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
            kwargs = {
                "prev_scr": prev_scr,
                "title": _(i18n_keys.TITLE__CONNECT_APP_WALLET),
                "subtitle": _(i18n_keys.TITLE__CONNECT_APP_WALLET_DESC),
                "nav_back": True,
            }
            super().__init__(**kwargs)
        else:
            return
        airgap_enabled = device.is_airgap_mode()
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

        self.by_qrcode = ListItemBtn(
            self.container,
            _(i18n_keys.BUTTON__QRCODE),
            left_img_src="A:/res/connect-way-qrcode.png",
        )
        self.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.by_ble:
                ConnectWalletGuide("ble", self)
            elif target == self.by_usb:
                ConnectWalletGuide("usb", self)
            elif target == self.by_qrcode:
                WalletList(self)
            else:
                return

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)


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
            "BTC·ETH·TRON·SOL·NEAR ...",
            left_img_src="A:/res/ok-logo-48.png",
        )
        self.onekey.text_layout_vertical(pad_top=17, pad_ver=20)

        self.mm = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__METAMASK_WALLET),
            _(i18n_keys.CONTENT__ETH_AND_EVM_POWERED_NETWORK),
            left_img_src="A:/res/mm-logo-48.png",
        )
        self.mm.text_layout_vertical()
        if self.connect_type == "ble":
            self.mm.add_flag(lv.obj.FLAG.HIDDEN)

        self.okx = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__OKX_WALLET),
            "BTC·ETH·TRON·SOL·NEAR ...",
            left_img_src="A:/res/okx-logo-48.png",
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
                    "https://help.onekey.so/hc/articles/9541173629455"
                    if self.connect_type == "ble"
                    else "https://help.onekey.so/hc/articles/9402090066319"
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
                url = "https://help.onekey.so/hc/articles/8910581291151#tab-item-generated-0"
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
                    "https://help.onekey.so/hc/articles/8925272484111#tab-item-generated-1"
                    if self.connect_type == "ble"
                    else "https://help.onekey.so/hc/articles/8925272484111#tab-item-generated-0"
                )
            ConnectWalletTutorial(title, subtitle, steps, url, logo)

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)


class WalletList(Screen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
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
            _(i18n_keys.CONTENT__BTC_AND_EVM_COMPATIBLE_NETWORKS),
            left_img_src="A:/res/ok-logo-48.png",
        )
        self.onekey.text_layout_vertical()

        self.mm = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__METAMASK_WALLET),
            _(i18n_keys.CONTENT__ETH_AND_EVM_POWERED_NETWORK),
            left_img_src="A:/res/mm-logo-48.png",
        )
        self.mm.text_layout_vertical(pad_top=17, pad_ver=20)

        self.okx = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__OKX_WALLET),
            # "BTC·ETH·TRON·SOL·NEAR ...",
            _(i18n_keys.CONTENT__COMING_SOON),
            left_img_src="A:/res/okx-logo-48.png",
        )
        self.okx.text_layout_vertical(pad_top=17, pad_ver=20)
        self.okx.disable()

        self.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

        if not device.is_passphrase_enabled():
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
            if target not in [self.onekey, self.mm, self.okx]:
                return
            if target == self.onekey:
                from trezor.qr import gen_multi_accounts, get_encoder

                if passphrase.is_enabled():
                    encoder = retrieval_encoder()
                else:
                    encoder = get_encoder()
                if encoder is None:
                    workflow.spawn(
                        gen_multi_accounts(
                            lambda: lv.event_send(target, lv.EVENT.CLICKED, None)
                        )
                    )
                    return
                ConnectWallet(
                    None,
                    None,
                    None,
                    encoder=encoder,
                    subtitle=_(i18n_keys.CONTENT__OPEN_ONEKEY_SCAN_THE_QRCODE),
                )

            elif target == self.mm:
                qr_data = (
                    retrieval_hd_key()
                    if device.is_passphrase_enabled()
                    else get_hd_key()
                )
                if qr_data is None:
                    from trezor.qr import gen_hd_key

                    workflow.spawn(
                        gen_hd_key(
                            lambda: lv.event_send(target, lv.EVENT.CLICKED, None)
                        )
                    )
                    return
                ConnectWallet(
                    _(i18n_keys.ITEM__METAMASK_WALLET),
                    _(i18n_keys.CONTENT__ETH_AND_EVM_POWERED_NETWORK),
                    qr_data,
                    "A:/res/mm-logo-96.png",
                )
            elif target == self.okx:
                qr_data = b""
                ConnectWallet(
                    _(i18n_keys.ITEM__OKX_WALLET),
                    "Ethereum, Bitcoin, Polygon, Solana, OKT Chain, TRON and other networks.",
                    qr_data,
                    "A:/res/okx-logo-96.png",
                )

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)


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
            return

        self.container = ContainerFlexCol(
            self.content_area, self.subtitle, padding_row=2
        )

        self.lite = ListItemBtn(
            self.container,
            "OneKey Lite",
            left_img_src="A:/res/icon-lite-48.png",
        )
        # hide lite backup for now
        # self.lite.add_flag(lv.obj.FLAG.HIDDEN)

        self.keytag = ListItemBtn(
            self.container,
            "OneKey Keytag",
            left_img_src="A:/res/icon-dot-48.png",
        )
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
                    airgap_enabled = device.is_airgap_mode()
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
                # pyright: off
                workflow.spawn(
                    recovery_device(
                        DUMMY_CONTEXT,
                        RecoveryDevice(dry_run=True, enforce_wordlist=True),
                    )
                )
                # pyright: on

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
        wallet_name,
        support_chains,
        qr_data,
        icon_path=None,
        encoder=None,
        subtitle=None,
    ):
        super().__init__(
            _(i18n_keys.TITLE__CONNECT_STR_WALLET).format(wallet_name)
            if wallet_name
            else None,
            _(i18n_keys.CONTENT__OPEN_STR_WALLET_AND_SCAN_THE_QR_CODE_BELOW).format(
                wallet_name
            )
            if wallet_name
            else subtitle,
            anim_dir=0,
        )
        self.content_area.set_style_max_height(684, 0)
        self.add_nav_back()

        gc.collect()
        gc.threshold(int(18248 * 1.5))  # type: ignore["threshold" is not a known member of module]
        from trezor.lvglui.scrs.components.qrcode import QRCode

        self.encoder = encoder
        data = qr_data if encoder is None else encoder.next_part()
        self.qr = QRCode(
            self.content_area,
            data,
            icon_path=icon_path,
        )
        self.qr.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 40)

        if wallet_name and support_chains:
            self.panel = lv.obj(self.content_area)
            self.panel.set_size(456, lv.SIZE.CONTENT)
            self.panel.add_style(
                StyleWrapper()
                .bg_color(lv_colors.ONEKEY_GRAY_3)
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
            self.label_bottom.set_width(408)
            self.label_bottom.add_style(
                StyleWrapper().text_font(font_GeistRegular26).pad_ver(12).pad_hor(0),
                0,
            )
            # self.content_area.clear_flag(lv.obj.FLAG.SCROLL_ELASTIC)
            # self.content_area.clear_flag(lv.obj.FLAG.SCROLL_MOMENTUM)
            self.content_area.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
            self.label_bottom.set_long_mode(lv.label.LONG.WRAP)
            self.label_bottom.set_text(support_chains)
            self.label_bottom.align_to(self.line, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 0)
            self.panel.align_to(self.qr, lv.ALIGN.OUT_BOTTOM_MID, 0, 32)
        self.nav_back.add_event_cb(self.on_nav_back, lv.EVENT.CLICKED, None)
        self.add_event_cb(self.on_nav_back, lv.EVENT.GESTURE, None)

        if encoder is not None:
            workflow.spawn(self.update_qr())

    # def on_scroll_begin(self, event_obj):
    #     self.scrolling = True

    # def on_scroll_end(self, event_obj):
    #     self.scrolling = False

    def on_nav_back(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.nav_back.nav_btn:
                if self.encoder is not None:
                    self.channel.publish(1)
                else:
                    self.destroy()
        elif code == lv.EVENT.GESTURE:
            _dir = lv.indev_get_act().get_gesture_dir()
            if _dir == lv.DIR.RIGHT:
                lv.event_send(self.nav_back.nav_btn, lv.EVENT.CLICKED, None)

    def destroy(self, delay_ms=200):
        self.delete()

    async def update_qr(self):
        while True:
            stop_single = self.request()
            racer = loop.race(stop_single, loop.sleep(100))
            await racer
            if stop_single in racer.finished:
                self.destroy()
                return
            # if self.scrolling:
            #     await loop.sleep(5000)
            #     continue
            assert self.encoder is not None
            qr_data = self.encoder.next_part()
            self.qr.update(qr_data, len(qr_data))


class ScanScreen(Screen):
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

        self.nav_back.align(lv.ALIGN.TOP_RIGHT, 0, 44)
        self.nav_back.nav_btn.add_style(
            StyleWrapper().bg_img_src("A:/res/nav-close.png"), 0
        )
        self.nav_back.nav_btn.align(lv.ALIGN.RIGHT_MID, 0, 0)

        self.camera_bg = lv.img(self.content_area)
        self.camera_bg.set_src("A:/res/camera-bg.png")
        self.camera_bg.align(lv.ALIGN.TOP_MID, 0, 148)

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
        self.desc.set_text(_(i18n_keys.CONTENT__SCAN_THE_QR_CODE_DISPLAYED_ON_THE_APP))
        self.desc.align_to(self.camera_bg, lv.ALIGN.OUT_BOTTOM_MID, 0, 14)

        self.btn = NormalButton(self, f"{LV_SYMBOLS.LV_SYMBOL_LIGHTBULB}")
        self.btn.set_size(115, 115)
        self.btn.add_style(StyleWrapper().radius(lv.RADIUS.CIRCLE), 0)
        self.btn.align(lv.ALIGN.BOTTOM_MID, 0, -8)
        self.btn.add_state(lv.STATE.CHECKED)
        self.add_event_cb(self.on_event, lv.EVENT.CLICKED, None)

        scan_qr(self)

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

    async def error_feedback(self):
        from trezor.ui.layouts import show_error_no_interact

        await show_error_no_interact(
            _(i18n_keys.TITLE__DATA_FORMAT_NOT_SUPPORT),
            _(i18n_keys.CONTENT__QR_CODE_TYPE_NOT_SUPPORT_PLEASE_TRY_AGAIN),
        )

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)

    @classmethod
    def notify_close(cls):
        if hasattr(cls, "_instance") and cls._instance._init:
            lv.event_send(cls._instance.nav_back.nav_btn, lv.EVENT.CLICKED, None)


if __debug__:
    from .common import SETTINGS_MOVE_TIME, SETTINGS_MOVE_DELAY

    class UITest(lv.obj):
        def __init__(self) -> None:
            super().__init__(lv.layer_sys())
            self.set_size(lv.pct(100), lv.pct(100))
            self.align(lv.ALIGN.TOP_LEFT, 0, 0)
            self.set_style_bg_color(lv_colors.BLACK, 0)
            self.set_style_pad_all(0, 0)
            self.set_style_border_width(0, 0)
            self.set_style_radius(0, 0)
            self.set_style_bg_img_src("A:/res/wallpaper-test.png", 0)
            self.add_flag(lv.obj.FLAG.CLICKABLE)
            self.clear_flag(lv.obj.FLAG.SCROLLABLE)
            self.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

        def on_click(self, _event_obj):
            self.delete()

    class AnimationSettings(Screen):
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
            self.app_drawer_up.set_text("主页上滑动画时间:")
            self.app_drawer_up.align_to(self.nav_back, lv.ALIGN.OUT_BOTTOM_LEFT, 12, 20)

            self.slider = lv.slider(self.content_area)
            self.slider.set_size(456, 80)
            self.slider.set_ext_click_area(20)
            self.slider.set_range(20, 400)
            self.slider.set_value(APP_DRAWER_UP_TIME, lv.ANIM.OFF)
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
                StyleWrapper()
                .text_font(font_GeistRegular30)
                .text_color(lv_colors.BLUE),
                0,
            )
            self.percent.set_text(f"{APP_DRAWER_UP_TIME} ms")
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
            self.app_drawer_up_delay.set_text("主页上滑动画延时:")
            self.app_drawer_up_delay.align_to(
                self.slider, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 20
            )

            self.slider1 = lv.slider(self.content_area)
            self.slider1.set_size(456, 80)
            self.slider1.set_ext_click_area(20)
            self.slider1.set_range(0, 80)
            self.slider1.set_value(APP_DRAWER_UP_DELAY, lv.ANIM.OFF)
            self.slider1.align_to(
                self.app_drawer_up_delay, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 20
            )

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
                StyleWrapper()
                .text_font(font_GeistRegular30)
                .text_color(lv_colors.BLUE),
                0,
            )
            self.percent1.set_text(f"{APP_DRAWER_UP_DELAY} ms")
            self.slider1.clear_flag(lv.obj.FLAG.GESTURE_BUBBLE)
            self.slider1.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            # endregion
            # region

            self.app_drawer_down = lv.label(self.content_area)
            self.app_drawer_down.set_size(456, lv.SIZE.CONTENT)
            self.app_drawer_down.add_style(
                StyleWrapper()
                .pad_all(12)
                .text_font(font_GeistRegular30)
                .text_color(lv_colors.WHITE),
                0,
            )
            self.app_drawer_down.set_text("主页下滑动画时间:")
            self.app_drawer_down.align_to(self.slider1, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 20)

            self.slider2 = lv.slider(self.content_area)
            self.slider2.set_size(456, 80)
            self.slider2.set_ext_click_area(20)
            self.slider2.set_range(20, 400)
            self.slider2.set_value(APP_DRAWER_DOWN_TIME, lv.ANIM.OFF)
            self.slider2.align_to(self.app_drawer_down, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 20)

            self.slider2.add_style(
                StyleWrapper().border_width(0).radius(40).bg_color(lv_colors.GRAY_1), 0
            )
            self.slider2.add_style(
                StyleWrapper().bg_color(lv_colors.WHITE).pad_all(-50), lv.PART.KNOB
            )
            self.slider2.add_style(
                StyleWrapper().radius(0).bg_color(lv_colors.WHITE), lv.PART.INDICATOR
            )
            self.percent2 = lv.label(self.slider2)
            self.percent2.align(lv.ALIGN.CENTER, 0, 0)
            self.percent2.add_style(
                StyleWrapper()
                .text_font(font_GeistRegular30)
                .text_color(lv_colors.BLUE),
                0,
            )
            self.percent2.set_text(f"{APP_DRAWER_DOWN_TIME} ms")
            self.slider2.clear_flag(lv.obj.FLAG.GESTURE_BUBBLE)
            self.slider2.add_flag(lv.obj.FLAG.EVENT_BUBBLE)

            self.app_drawer_down_delay = lv.label(self.content_area)
            self.app_drawer_down_delay.set_size(456, lv.SIZE.CONTENT)
            self.app_drawer_down_delay.add_style(
                StyleWrapper()
                .pad_all(12)
                .text_font(font_GeistRegular30)
                .text_color(lv_colors.WHITE),
                0,
            )
            self.app_drawer_down_delay.set_text("主页下滑动画延时:")
            self.app_drawer_down_delay.align_to(
                self.slider2, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 20
            )

            self.slider3 = lv.slider(self.content_area)
            self.slider3.set_size(456, 80)
            self.slider3.set_ext_click_area(20)
            self.slider3.set_range(0, 80)
            self.slider3.set_value(APP_DRAWER_DOWN_DELAY, lv.ANIM.OFF)
            self.slider3.align_to(
                self.app_drawer_down_delay, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 20
            )

            self.slider3.add_style(
                StyleWrapper().border_width(0).radius(40).bg_color(lv_colors.GRAY_1), 0
            )
            self.slider3.add_style(
                StyleWrapper().bg_color(lv_colors.WHITE).pad_all(-50), lv.PART.KNOB
            )
            self.slider3.add_style(
                StyleWrapper().radius(0).bg_color(lv_colors.WHITE), lv.PART.INDICATOR
            )
            self.percent3 = lv.label(self.slider3)
            self.percent3.align(lv.ALIGN.CENTER, 0, 0)
            self.percent3.add_style(
                StyleWrapper()
                .text_font(font_GeistRegular30)
                .text_color(lv_colors.BLUE),
                0,
            )
            self.percent3.set_text(f"{APP_DRAWER_DOWN_DELAY} ms")
            self.slider3.clear_flag(lv.obj.FLAG.GESTURE_BUBBLE)
            self.slider3.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            # endregion
            # region
            self.cur_up_path_cb_type = lv.label(self.content_area)
            self.cur_up_path_cb_type.set_size(456, lv.SIZE.CONTENT)
            self.cur_up_path_cb_type.add_style(
                StyleWrapper()
                .pad_all(12)
                .text_font(font_GeistRegular30)
                .text_color(lv_colors.WHITE),
                0,
            )
            self.set_cur_path_cb_type(0)
            self.cur_up_path_cb_type.align_to(
                self.slider3, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 20
            )

            self.cur_sown_path_cb_type = lv.label(self.content_area)
            self.cur_sown_path_cb_type.set_size(456, lv.SIZE.CONTENT)
            self.cur_sown_path_cb_type.add_style(
                StyleWrapper()
                .pad_all(12)
                .text_font(font_GeistRegular30)
                .text_color(lv_colors.WHITE),
                0,
            )
            self.set_cur_path_cb_type(1)
            self.cur_sown_path_cb_type.align_to(
                self.cur_up_path_cb_type, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 20
            )

            self.container = ContainerFlexCol(
                self.content_area,
                self.cur_sown_path_cb_type,
                padding_row=2,
                pos=(0, 20),
            )

            from .components.listitem import ListItemWithLeadingCheckbox

            self.path_up = ListItemWithLeadingCheckbox(
                self.container,
                "修改主页上滑动画类型",
            )
            self.path_up.enable_bg_color(False)
            self.path_down = ListItemWithLeadingCheckbox(
                self.container,
                "修改主页下滑动画类型",
            )
            self.path_down.enable_bg_color(False)
            self.path_liner = ListItemBtn(
                self.container,
                "path liner",
            )
            self.path_ease_in = ListItemBtn(
                self.container,
                "path ease in(slow at the beginning)",
            )
            self.path_ease_out = ListItemBtn(
                self.container,
                "path ease out(slow at the end)",
            )
            self.path_ease_in_out = ListItemBtn(
                self.container,
                "path ease in out(slow at the beginning and end)",
            )
            self.path_over_shoot = ListItemBtn(
                self.container,
                "path over shoot(overshoot the end value)",
            )
            self.path_bounce = ListItemBtn(
                self.container,
                "path bounce(bounce back a little from the end value (like hitting a wall))",
            )
            self.path_step = ListItemBtn(
                self.container,
                "path step(change in one step at the end)",
            )
            # endregion

            # region
            self.setting_scr = lv.label(self.content_area)
            self.setting_scr.set_size(456, lv.SIZE.CONTENT)
            self.setting_scr.add_style(
                StyleWrapper()
                .pad_all(12)
                .text_font(font_GeistRegular30)
                .text_color(lv_colors.WHITE),
                0,
            )
            self.setting_scr.set_text("设置页面动画时间:")
            self.setting_scr.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 40)

            self.slider4 = lv.slider(self.content_area)
            self.slider4.set_size(456, 80)
            self.slider4.set_ext_click_area(20)
            self.slider4.set_range(20, 400)

            self.slider4.set_value(SETTINGS_MOVE_TIME, lv.ANIM.OFF)
            self.slider4.align_to(self.setting_scr, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 20)

            self.slider4.add_style(
                StyleWrapper().border_width(0).radius(40).bg_color(lv_colors.GRAY_1), 0
            )
            self.slider4.add_style(
                StyleWrapper().bg_color(lv_colors.WHITE).pad_all(-50), lv.PART.KNOB
            )
            self.slider4.add_style(
                StyleWrapper().radius(0).bg_color(lv_colors.WHITE), lv.PART.INDICATOR
            )
            self.percent4 = lv.label(self.slider4)
            self.percent4.align(lv.ALIGN.CENTER, 0, 0)
            self.percent4.add_style(
                StyleWrapper()
                .text_font(font_GeistRegular30)
                .text_color(lv_colors.BLUE),
                0,
            )
            self.percent4.set_text(f"{SETTINGS_MOVE_TIME} ms")
            self.slider4.clear_flag(lv.obj.FLAG.GESTURE_BUBBLE)
            self.slider4.add_flag(lv.obj.FLAG.EVENT_BUBBLE)

            self.setting_scr_delay = lv.label(self.content_area)
            self.setting_scr_delay.set_size(456, lv.SIZE.CONTENT)
            self.setting_scr_delay.add_style(
                StyleWrapper()
                .pad_all(12)
                .text_font(font_GeistRegular30)
                .text_color(lv_colors.WHITE),
                0,
            )
            self.setting_scr_delay.set_text("设置页面动画延时:")
            self.setting_scr_delay.align_to(
                self.slider4, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 20
            )

            self.slider5 = lv.slider(self.content_area)
            self.slider5.set_size(456, 80)
            self.slider5.set_ext_click_area(20)
            self.slider5.set_range(0, 80)
            self.slider5.set_value(SETTINGS_MOVE_DELAY, lv.ANIM.OFF)
            self.slider5.align_to(
                self.setting_scr_delay, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 20
            )

            self.slider5.add_style(
                StyleWrapper().border_width(0).radius(40).bg_color(lv_colors.GRAY_1), 0
            )
            self.slider5.add_style(
                StyleWrapper().bg_color(lv_colors.WHITE).pad_all(-50), lv.PART.KNOB
            )
            self.slider5.add_style(
                StyleWrapper().radius(0).bg_color(lv_colors.WHITE), lv.PART.INDICATOR
            )
            self.percent5 = lv.label(self.slider5)
            self.percent5.align(lv.ALIGN.CENTER, 0, 0)
            self.percent5.add_style(
                StyleWrapper()
                .text_font(font_GeistRegular30)
                .text_color(lv_colors.BLUE),
                0,
            )
            self.percent5.set_text(f"{SETTINGS_MOVE_DELAY} ms")
            self.slider5.clear_flag(lv.obj.FLAG.GESTURE_BUBBLE)
            self.slider5.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            # endregion

            self.add_event_cb(self.on_value_changed, lv.EVENT.VALUE_CHANGED, None)
            self.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

        def on_nav_back(self, event_obj):
            pass

        def on_click(self, event_obj):
            global APP_DRAWER_UP_PATH_CB, APP_DRAWER_DOWN_PATH_CB
            # _code = event_obj.code
            target = event_obj.get_target()
            if target == self.path_liner:
                print("path_liner clicked")
                if self.path_up.checkbox.get_state() & lv.STATE.CHECKED:
                    print("path_up checked")
                    APP_DRAWER_UP_PATH_CB = PATH_LINEAR
                if self.path_down.checkbox.get_state() & lv.STATE.CHECKED:
                    APP_DRAWER_DOWN_PATH_CB = PATH_LINEAR
            elif target == self.path_ease_in:
                print("path_ease_in clicked")
                if self.path_up.checkbox.get_state() & lv.STATE.CHECKED:
                    APP_DRAWER_UP_PATH_CB = PATH_EASE_IN
                if self.path_down.checkbox.get_state() & lv.STATE.CHECKED:
                    APP_DRAWER_DOWN_PATH_CB = PATH_EASE_IN
            elif target == self.path_ease_out:
                print("path_ease_out clicked")
                if self.path_up.checkbox.get_state() & lv.STATE.CHECKED:
                    APP_DRAWER_UP_PATH_CB = PATH_EASE_OUT
                if self.path_down.checkbox.get_state() & lv.STATE.CHECKED:
                    APP_DRAWER_DOWN_PATH_CB = PATH_EASE_OUT
            elif target == self.path_ease_in_out:
                print("path_ease_in_out clicked")
                if self.path_up.checkbox.get_state() & lv.STATE.CHECKED:
                    APP_DRAWER_UP_PATH_CB = PATH_EASE_IN_OUT
                if self.path_down.checkbox.get_state() & lv.STATE.CHECKED:
                    APP_DRAWER_DOWN_PATH_CB = PATH_EASE_IN_OUT
            elif target == self.path_over_shoot:
                print("path_over_shoot clicked")
                if self.path_up.checkbox.get_state() & lv.STATE.CHECKED:
                    APP_DRAWER_UP_PATH_CB = PATH_OVER_SHOOT
                if self.path_down.checkbox.get_state() & lv.STATE.CHECKED:
                    APP_DRAWER_DOWN_PATH_CB = PATH_OVER_SHOOT
            elif target == self.path_bounce:
                print("path_bounce clicked")
                if self.path_up.checkbox.get_state() & lv.STATE.CHECKED:
                    APP_DRAWER_UP_PATH_CB = PATH_BOUNCE
                if self.path_down.checkbox.get_state() & lv.STATE.CHECKED:
                    APP_DRAWER_DOWN_PATH_CB = PATH_BOUNCE
            elif target == self.path_step:
                print("path_step clicked")
                if self.path_up.checkbox.get_state() & lv.STATE.CHECKED:
                    APP_DRAWER_UP_PATH_CB = PATH_STEP
                if self.path_down.checkbox.get_state() & lv.STATE.CHECKED:
                    APP_DRAWER_DOWN_PATH_CB = PATH_STEP

            if self.path_up.checkbox.get_state() & lv.STATE.CHECKED:
                self.set_cur_path_cb_type(0)
                MainScreen._instance.apps.show_anim.set_path_cb(APP_DRAWER_UP_PATH_CB)
            if self.path_down.checkbox.get_state() & lv.STATE.CHECKED:
                self.set_cur_path_cb_type(1)
                MainScreen._instance.apps.dismiss_anim.set_path_cb(
                    APP_DRAWER_DOWN_PATH_CB
                )

        def get_path_cb_str(self, path_cb):
            if path_cb is PATH_LINEAR:
                return "path_linear"
            elif path_cb is PATH_EASE_IN:
                return "path_ease_in"
            elif path_cb is PATH_EASE_OUT:
                return "path_ease_out"
            elif path_cb is PATH_EASE_IN_OUT:
                return "path_ease_in_out"
            elif path_cb is PATH_OVER_SHOOT:
                return "path_overshoot"
            elif path_cb is PATH_BOUNCE:
                return "path_bounce"
            elif path_cb is PATH_STEP:
                return "path_step"
            else:
                return "path_linear"

        def set_cur_path_cb_type(self, type: int):
            global APP_DRAWER_UP_PATH_CB, APP_DRAWER_DOWN_PATH_CB
            if type == 0:
                self.cur_up_path_cb_type.set_text(
                    f"current up anim type : {self.get_path_cb_str(APP_DRAWER_UP_PATH_CB)}"
                )
            elif type == 1:
                self.cur_sown_path_cb_type.set_text(
                    f"current down anim type: {self.get_path_cb_str(APP_DRAWER_DOWN_PATH_CB)}"
                )
            else:
                raise ValueError("type is not valid")

        def on_value_changed(self, event_obj):
            global APP_DRAWER_UP_TIME, APP_DRAWER_UP_DELAY, APP_DRAWER_DOWN_TIME, APP_DRAWER_DOWN_DELAY, SETTINGS_MOVE_TIME, SETTINGS_MOVE_DELAY

            target = event_obj.get_target()
            if target == self.slider:
                value = target.get_value()
                APP_DRAWER_UP_TIME = value
                MainScreen._instance.apps.show_anim.set_time(value)
                self.percent.set_text(f"{value} ms")
            elif target == self.slider1:
                value = target.get_value()
                APP_DRAWER_UP_DELAY = value
                MainScreen._instance.apps.show_anim.set_delay(value)
                self.percent1.set_text(f"{value} ms")
            elif target == self.slider2:
                value = target.get_value()
                APP_DRAWER_DOWN_TIME = value
                MainScreen._instance.apps.dismiss_anim.set_time(value)
                self.percent2.set_text(f"{value} ms")
            elif target == self.slider3:
                value = target.get_value()
                APP_DRAWER_DOWN_DELAY = value
                MainScreen._instance.apps.dismiss_anim.set_delay(value)
                self.percent3.set_text(f"{value} ms")
            elif target == self.slider4:
                value = target.get_value()
                SETTINGS_MOVE_TIME = value
                self.percent4.set_text(f"{value} ms")
            elif target == self.slider5:
                value = target.get_value()
                SETTINGS_MOVE_DELAY = value
                self.percent5.set_text(f"{value} ms")


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
            self.backlight.label_right.set_text(
                brightness2_percent_str(device.get_brightness())
            )
            self.refresh_text()
            return
        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__GENERAL), nav_back=True
        )

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        GeneralScreen.cur_language = langs[langs_keys.index(device.get_language())][1]
        self.language = ListItemBtn(
            self.container, _(i18n_keys.ITEM__LANGUAGE), GeneralScreen.cur_language
        )
        self.backlight = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__BRIGHTNESS),
            brightness2_percent_str(device.get_brightness()),
        )
        self.animation = ListItemBtn(self.container, _(i18n_keys.ITEM__ANIMATIONS))
        self.tap_awake = ListItemBtn(self.container, _(i18n_keys.ITEM__LOCK_SCREEN))
        self.autolock_and_shutdown = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__AUTO_LOCK_AND_SHUTDOWN),
        )
        self.power = ListItemBtn(
            self.content_area,
            _(i18n_keys.ITEM__POWER_OFF),
            left_img_src="A:/res/poweroff.png",
            has_next=False,
        )
        self.power.label_left.set_style_text_color(lv_colors.ONEKEY_RED_1, 0)
        self.power.align_to(self.container, lv.ALIGN.OUT_BOTTOM_MID, 0, 12)
        self.power.set_style_radius(40, 0)
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.content_area.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.load_screen(self)
        gc.collect()

    def refresh_text(self):
        self.title.set_text(_(i18n_keys.TITLE__GENERAL))
        self.language.label_left.set_text(_(i18n_keys.ITEM__LANGUAGE))
        self.backlight.label_left.set_text(_(i18n_keys.ITEM__BRIGHTNESS))
        self.animation.label_left.set_text(_(i18n_keys.ITEM__ANIMATIONS))
        self.tap_awake.label_left.set_text(_(i18n_keys.ITEM__LOCK_SCREEN))
        self.autolock_and_shutdown.label_left.set_text(
            _(i18n_keys.ITEM__AUTO_LOCK_AND_SHUTDOWN)
        )
        self.power.label_left.set_text(_(i18n_keys.ITEM__POWER_OFF))
        self.container.update_layout()
        self.power.align_to(self.container, lv.ALIGN.OUT_BOTTOM_MID, 0, 12)

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.language:
                LanguageSetting(self)
            elif target == self.backlight:
                BacklightSetting(self)
            elif target == self.animation:
                Animations(self)
            elif target == self.tap_awake:
                TapAwakeSetting(self)
            elif target == self.autolock_and_shutdown:
                Autolock_and_ShutingDown(self)
            elif target == self.power:
                PowerOff()
            else:
                pass


class Animations(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        return targets

    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            self.refresh_text()
            return

        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__ANIMATIONS), nav_back=True
        )

        self.container = ContainerFlexCol(
            self.content_area, self.title, padding_row=2, pos=(0, 40)
        )
        GeneralScreen.cur_language = langs[langs_keys.index(device.get_language())][1]
        self.keyboard_haptic = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__VIBRATION_AND_HAPTIC),
        )
        self.animation = ListItemBtn(self.container, _(i18n_keys.ITEM__ANIMATIONS))
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.load_screen(self)
        gc.collect()

    def refresh_text(self):
        self.title.set_text(_(i18n_keys.TITLE__ANIMATIONS))
        self.keyboard_haptic.label_left.set_text(
            _(i18n_keys.ITEM__VIBRATION_AND_HAPTIC)
        )
        self.animation.label_left.set_text(_(i18n_keys.ITEM__ANIMATIONS))

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.keyboard_haptic:
                KeyboardHapticSetting(self)
            elif target == self.animation:
                AnimationSetting(self)


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
        Autolock_and_ShutingDown.cur_auto_lock_ms = device.get_autolock_delay_ms()
        Autolock_and_ShutingDown.cur_auto_shutdown_ms = (
            device.get_autoshutdown_delay_ms()
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
        if time_ms == device.AUTOLOCK_DELAY_MAXIMUM:
            return _(i18n_keys.ITEM__STATUS__NEVER)
        auto_lock_time = time_ms / 1000 // 60
        if auto_lock_time > 60:
            value = str(auto_lock_time // 60).split(".")[0]
            text = _(
                i18n_keys.OPTION__STR_HOUR
                if value == "1"
                else i18n_keys.OPTION__STR_HOURS
            ).format(value)
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
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__AUTO_LOCK), nav_back=True
        )

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.setting_items = [1, 2, 5, 10, 30, "Never", None]
        has_custom = True
        self.checked_index = 0
        self.btns: [ListItemBtn] = [None] * (len(self.setting_items))
        for index, item in enumerate(self.setting_items):
            if item is None:
                break
            if not item == "Never":  # last item
                item = _(
                    i18n_keys.ITEM__STATUS__STR_MINUTES
                    if item != 1
                    else i18n_keys.OPTION__STR_MINUTE
                ).format(item)
            else:
                item = _(i18n_keys.ITEM__STATUS__NEVER)
            self.btns[index] = ListItemBtn(
                self.container, item, has_next=False, use_transition=False
            )
            # self.btns[index].label_left.add_style(
            #     StyleWrapper().text_font(font_GeistRegular30), 0
            # )
            self.btns[index].add_check_img()
            if item == Autolock_and_ShutingDown.cur_auto_lock:
                has_custom = False
                self.btns[index].set_checked()
                self.checked_index = index

        if has_custom:
            self.custom = device.get_autolock_delay_ms()
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
                        if index == 5:
                            auto_lock_time = device.AUTOLOCK_DELAY_MAXIMUM
                        elif index == 6:
                            auto_lock_time = self.custom
                        else:
                            auto_lock_time = self.setting_items[index] * 60 * 1000
                        device.set_autolock_delay_ms(auto_lock_time)
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
            # lang_button.label_left.add_style(StyleWrapper().text_font(font_GeistRegular30), 0)
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
                    device.set_language(langs_keys[idx])
                    GeneralScreen.cur_language = langs[idx][1]
                    i18n_refresh()
                    self.title.set_text(_(i18n_keys.TITLE__LANGUAGE))
                    self.check_index = idx
                    button.set_checked()


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

        self.current_brightness = device.get_brightness()
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
                        device.set_brightness(self.temp_brightness)
            super().eventhandler(event_obj)


class KeyboardHapticSetting(AnimScreen):
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
            title=_(i18n_keys.TITLE__VIBRATION_AND_HAPTIC),
            nav_back=True,
        )
        self.container = ContainerFlexCol(
            self.content_area,
            self.title,
        )

        self.keyboard = ListItemBtnWithSwitch(
            self.container, _(i18n_keys.ITEM__KEYBOARD_HAPTIC)
        )
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
        self.tips.set_text(_(i18n_keys.CONTENT__VIBRATION_HAPTIC__HINT))
        if device.keyboard_haptic_enabled():
            self.keyboard.add_state()
        else:
            self.keyboard.clear_state()

        self.container.add_event_cb(self.on_value_changed, lv.EVENT.VALUE_CHANGED, None)
        self.load_screen(self)
        gc.collect()

    def on_value_changed(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.keyboard.switch:
                if target.has_state(lv.STATE.CHECKED):
                    device.toggle_keyboard_haptic(True)
                else:
                    device.toggle_keyboard_haptic(False)


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
        if device.is_animation_enabled():
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
                    device.set_animation_enable(True)
                    self.tips.set_text(_(i18n_keys.CONTENT__ANIMATIONS__ENABLED_HINT))
                else:
                    device.set_animation_enable(False)
                    self.tips.set_text(_(i18n_keys.CONTENT__ANIMATIONS__DISABLED_HINT))


class TapAwakeSetting(AnimScreen):
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
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__LOCK_SCREEN), nav_back=True
        )

        self.container = ContainerFlexCol(self.content_area, self.title)
        self.tap_awake = ListItemBtnWithSwitch(
            self.container, _(i18n_keys.ITEM__TAP_TO_WAKE)
        )
        self.description = lv.label(self.content_area)
        self.description.set_size(456, lv.SIZE.CONTENT)
        self.description.set_long_mode(lv.label.LONG.WRAP)
        self.description.set_style_text_color(lv_colors.ONEKEY_GRAY, lv.STATE.DEFAULT)
        self.description.set_style_text_font(font_GeistRegular26, lv.STATE.DEFAULT)
        self.description.set_style_text_line_space(3, 0)
        self.description.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 8, 16)

        if device.is_tap_awake_enabled():
            self.tap_awake.add_state()
            self.description.set_text(_(i18n_keys.CONTENT__TAP_TO_WAKE_ENABLED__HINT))
        else:
            self.tap_awake.clear_state()
            self.description.set_text(_(i18n_keys.CONTENT__TAP_TO_WAKE_DISABLED__HINT))
        self.container.add_event_cb(self.on_value_changed, lv.EVENT.VALUE_CHANGED, None)
        self.load_screen(self)
        gc.collect()

    def on_value_changed(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.tap_awake.switch:
                if target.has_state(lv.STATE.CHECKED):
                    self.description.set_text(
                        _(i18n_keys.CONTENT__TAP_TO_WAKE_ENABLED__HINT)
                    )
                    device.set_tap_awake_enable(True)
                else:
                    self.description.set_text(
                        _(i18n_keys.CONTENT__TAP_TO_WAKE_DISABLED__HINT)
                    )
                    device.set_tap_awake_enable(False)


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

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.setting_items = [1, 2, 5, 10, "Never", None]
        has_custom = True
        self.checked_index = 0
        # pyright: off
        self.btns: [ListItemBtn] = [None] * (len(self.setting_items))
        for index, item in enumerate(self.setting_items):
            if item is None:
                break
            if not item == "Never":  # last item
                item = _(
                    i18n_keys.ITEM__STATUS__STR_MINUTES
                    if item != 1
                    else i18n_keys.OPTION__STR_MINUTE
                ).format(item)
            else:
                item = _(i18n_keys.ITEM__STATUS__NEVER)
            self.btns[index] = ListItemBtn(
                self.container, item, has_next=False, use_transition=False
            )
            # self.btns[index].label_left.add_style(
            #     StyleWrapper().text_font(font_GeistRegular30), 0
            # )
            self.btns[index].add_check_img()
            if item == Autolock_and_ShutingDown.cur_auto_shutdown:
                has_custom = False
                self.btns[index].set_checked()
                self.checked_index = index

        if has_custom:
            self.custom = device.get_autoshutdown_delay_ms()
            self.btns[-1] = ListItemBtn(
                self.container,
                f"{Autolock_and_ShutingDown.cur_auto_shutdown}({_(i18n_keys.OPTION__CUSTOM__INSERT)})",
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
                    item_text or Autolock_and_ShutingDown.cur_auto_shutdown[:1]
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
                            auto_shutdown_time = device.AUTOSHUTDOWN_DELAY_MAXIMUM
                        elif index == 5:
                            auto_shutdown_time = self.custom
                        else:
                            auto_shutdown_time = self.setting_items[index] * 60 * 1000
                        device.set_autoshutdown_delay_ms(auto_shutdown_time)
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
        if device.is_random_pin_map_enabled():
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
                if not device.is_random_pin_map_enabled():
                    device.set_random_pin_map_enable(True)
            elif target == self.order:
                self.random.set_uncheck()
                self.order.set_checked()
                if device.is_random_pin_map_enabled():
                    device.set_random_pin_map_enable(False)
            else:
                return
            self.fresh_tips()


class ConnectSetting(Screen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        super().__init__(
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__CONNECT), nav_back=True
        )

        self.container = ContainerFlexCol(self.content_area, self.title)
        self.ble = ListItemBtnWithSwitch(self.container, _(i18n_keys.ITEM__BLUETOOTH))

        self.description = lv.label(self.content_area)
        self.description.set_size(456, lv.SIZE.CONTENT)
        self.description.set_long_mode(lv.label.LONG.WRAP)
        self.description.set_style_text_color(lv_colors.ONEKEY_GRAY, lv.STATE.DEFAULT)
        self.description.set_style_text_font(font_GeistRegular26, lv.STATE.DEFAULT)
        self.description.set_style_text_line_space(3, 0)
        self.description.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 8, 16)

        if uart.is_ble_opened():
            self.ble.add_state()
            self.description.set_text(
                _(i18n_keys.CONTENT__CONNECT_BLUETOOTH_ENABLED__HINT).format(
                    device.get_ble_name()
                )
            )
        else:
            self.ble.clear_state()
            self.description.set_text(
                _(i18n_keys.CONTENT__CONNECT_BLUETOOTH_DISABLED__HINT)
            )
        # self.usb = ListItemBtnWithSwitch(self.container, _(i18n_keys.ITEM__USB))
        self.container.add_event_cb(self.on_value_changed, lv.EVENT.VALUE_CHANGED, None)

    def on_value_changed(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:

            if target == self.ble.switch:
                if target.has_state(lv.STATE.CHECKED):
                    self.description.set_text(
                        self.description.set_text(
                            _(
                                i18n_keys.CONTENT__CONNECT_BLUETOOTH_ENABLED__HINT
                            ).format(device.get_ble_name())
                        )
                    )
                    uart.ctrl_ble(enable=True)
                else:
                    self.description.set_text(
                        _(i18n_keys.CONTENT__CONNECT_BLUETOOTH_DISABLED__HINT)
                    )
                    uart.ctrl_ble(enable=False)
            # else:
            #     if target.has_state(lv.STATE.CHECKED):
            #         print("USB is on")
            #     else:
            #         print("USB is off")


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
            air_gap_enabled = device.is_airgap_mode()
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
            prev_scr=prev_scr, title=_(i18n_keys.ITEM__AIR_GAP_MODE), nav_back=True
        )

        self.container = ContainerFlexCol(self.content_area, self.title)
        self.air_gap = ListItemBtnWithSwitch(self.container, _(i18n_keys.ITEM__AIR_GAP))

        self.description = lv.label(self.content_area)
        self.description.set_size(456, lv.SIZE.CONTENT)
        self.description.set_long_mode(lv.label.LONG.WRAP)
        self.description.set_style_text_color(lv_colors.ONEKEY_GRAY, lv.STATE.DEFAULT)
        self.description.set_style_text_font(font_GeistRegular26, lv.STATE.DEFAULT)
        self.description.set_style_text_line_space(3, 0)
        self.description.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 8, 16)
        air_gap_enabled = device.is_airgap_mode()
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
            if not device.is_airgap_mode():
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
            if device.is_airgap_mode():
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
        self.fcc_id = DisplayItemWithFont_30(self.container, "FCC ID", "2BB8VP1")

        self.fcc_icon = lv.img(self.fcc_id)
        self.fcc_icon.set_src("A:/res/fcc-logo.png")
        self.fcc_icon.align(lv.ALIGN.RIGHT_MID, 0, -5)
        self.container.add_dummy()

        self.firmware_update = NormalButton(
            self.content_area, _(i18n_keys.BUTTON__SYSTEM_UPDATE)
        )

        self.firmware_update.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 8)
        self.serial.add_event_cb(self.on_long_pressed, lv.EVENT.LONG_PRESSED, None)
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.firmware_update.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.load_screen(self)
        gc.collect()

    def on_click(self, event_obj):
        target = event_obj.get_target()
        # if target == self.board_loader:
        #     GO2BoardLoader()
        if target == self.firmware_update:
            Go2UpdateMode(self)

    def on_long_pressed(self, event_obj):
        target = event_obj.get_target()
        if target == self.serial:
            # if self.board_loader.has_flag(lv.obj.FLAG.HIDDEN):
            #     self.board_loader.clear_flag(lv.obj.FLAG.HIDDEN)
            # else:
            #     self.board_loader.add_flag(lv.obj.FLAG.HIDDEN)
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

                device.enable_trezor_compatible(self.enable)
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
        if self.has_pin and device.is_initialized():
            # from trezor.lvglui.scrs import fingerprints

            # if fingerprints.is_available() and fingerprints.is_unlocked():
            #         fingerprints.lock()
            # else:
            #     config.lock()
            config.lock()

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
                    and device.is_initialized()
                ):
                    from apps.common.request_pin import verify_user_pin

                    workflow.spawn(
                        verify_user_pin(
                            re_loop=self.re_loop,
                            allow_cancel=False,
                            callback=self.back,
                            allow_fingerprint=False,
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


class HomeScreenSetting(AnimScreen):
    def collect_animation_targets(self) -> list:
        targets = []
        if hasattr(self, "container") and self.container:
            targets.append(self.container)
        if hasattr(self, "wps"):
            for wp in self.wps:
                targets.append(wp)
        return targets

    def __init__(self, prev_scr=None):
        homescreen = device.get_homescreen()
        if not hasattr(self, "_init"):
            self._init = True
            self.from_wallpaper = False
            super().__init__(
                prev_scr=prev_scr, title=_(i18n_keys.TITLE__HOMESCREEN), nav_back=True
            )

        else:
            self.container.delete()

        internal_wp_nums = 7
        wp_nums = internal_wp_nums
        file_name_list = []
        if not utils.EMULATOR:
            for size, _attrs, name in io.fatfs.listdir("1:/res/wallpapers"):
                if wp_nums >= 12:
                    break
                if size > 0 and name[:4] == "zoom":
                    wp_nums += 1
                    file_name_list.append(name)
        rows_num = math.ceil(wp_nums / 3)
        row_dsc = [GRID_CELL_SIZE_ROWS] * rows_num
        row_dsc.append(lv.GRID_TEMPLATE.LAST)
        # 3 columns
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
        )
        self.container.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, 40)
        self.wps = []
        for i in range(internal_wp_nums):
            path_dir = "A:/res/"
            file_name = f"zoom-wallpaper-{i+1}.jpg"

            current_wp = ImgGridItem(
                self.container,
                i % 3,
                i // 3,
                file_name,
                path_dir,
                is_internal=True,
            )
            self.wps.append(current_wp)
            if homescreen == current_wp.img_path:
                current_wp.set_checked(True)

        if not utils.EMULATOR:
            file_name_list.sort(
                key=lambda name: int(
                    name[5:].split("-")[-1][: -(len(name.split(".")[1]) + 1)]
                )
            )
            for i, file_name in enumerate(file_name_list):
                path_dir = "A:1:/res/wallpapers/"
                current_wp = ImgGridItem(
                    self.container,
                    (i + internal_wp_nums) % 3,
                    (i + internal_wp_nums) // 3,
                    file_name,
                    path_dir,
                    is_internal=False,
                )
                self.wps.append(current_wp)
                if homescreen == current_wp.img_path:
                    current_wp.set_checked(True)
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.load_screen(self)
        gc.collect()

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target not in self.wps:
                return
            for wp in self.wps:
                if target == wp:
                    WallPaperManage(
                        self,
                        img_path=wp.img_path,
                        zoom_path=wp.zoom_path,
                        is_internal=wp.is_internal,
                    )

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        if self.from_wallpaper:
            scr.set_pos(0, 0)
            lv.scr_load(scr)
        else:
            super()._load_scr(scr, back)


class WallPaperManage(Screen):
    def __init__(
        self,
        prev_scr=None,
        img_path: str = "",
        zoom_path: str = "",
        is_internal: bool = False,
    ):
        super().__init__(
            prev_scr,
            icon_path=zoom_path,
            title=_(i18n_keys.TITLE__MANAGE_WALLPAPER),
            subtitle=_(i18n_keys.SUBTITLE__MANAGE_WALLPAPER),
            nav_back=True,
        )
        self.img_path = img_path
        self.zoom_path = zoom_path

        self.btn_yes = NormalButton(self.content_area, _(i18n_keys.BUTTON__SET))
        self.btn_yes.add_style(
            StyleWrapper().bg_color(lv_colors.ONEKEY_GREEN).text_color(lv_colors.BLACK),
            0,
        )
        if not is_internal:
            # self.icon.add_style(StyleWrapper().radius(40).clip_corner(True), 0)
            # self.icon.set_style_radius(40, 0)
            # self.icon.set_style_clip_corner(True, 0)
            self.btn_yes.set_size(224, 98)
            self.btn_yes.align_to(self.content_area, lv.ALIGN.BOTTOM_RIGHT, -12, -8)
            self.btn_del = NormalButton(self.content_area, "")
            self.btn_del.set_size(224, 98)
            self.btn_del.align(lv.ALIGN.BOTTOM_LEFT, 12, -8)

            self.panel = lv.obj(self.btn_del)
            self.panel.remove_style_all()
            self.panel.set_size(lv.SIZE.CONTENT, lv.SIZE.CONTENT)
            self.panel.clear_flag(lv.obj.FLAG.CLICKABLE)

            self.btn_del_img = lv.img(self.panel)
            self.btn_del_img.set_src("A:/res/btn-del.png")
            self.btn_label = lv.label(self.panel)
            self.btn_label.set_text(_(i18n_keys.BUTTON__DELETE))
            self.btn_label.align_to(self.btn_del_img, lv.ALIGN.OUT_RIGHT_MID, 4, 1)

            self.panel.add_style(
                StyleWrapper()
                .bg_color(lv_colors.ONEKEY_BLACK)
                .text_color(lv_colors.ONEKEY_RED_1)
                .bg_opa(lv.OPA.TRANSP)
                .border_width(0)
                .align(lv.ALIGN.CENTER),
                0,
            )

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)

    def del_callback(self):
        io.fatfs.unlink(self.img_path[2:])
        io.fatfs.unlink(self.zoom_path[2:])
        if device.get_homescreen() == self.img_path:
            device.set_homescreen(utils.get_default_wallpaper())
        self.load_screen(self.prev_scr, destroy_self=True)

    # def cancel_callback(self):
    #     self.btn_del.clear_flag(lv.obj.FLAG.HIDDEN)

    def eventhandler(self, event_obj):
        event = event_obj.code
        target = event_obj.get_target()
        if event == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if isinstance(target, lv.imgbtn):
                if target == self.nav_back.nav_btn:
                    if self.prev_scr is not None:
                        self.prev_scr.from_wallpaper = True
                        self.load_screen(self.prev_scr, destroy_self=True)
                        self.prev_scr.from_wallpaper = False
            else:
                if target == self.btn_yes:
                    device.set_homescreen(self.img_path)
                    self.prev_scr.from_wallpaper = True
                    self.load_screen(self.prev_scr, destroy_self=True)
                    self.prev_scr.from_wallpaper = False
                elif hasattr(self, "btn_del") and target == self.btn_del:
                    from trezor.ui.layouts import confirm_del_wallpaper
                    from trezor.wire import DUMMY_CONTEXT

                    workflow.spawn(
                        confirm_del_wallpaper(DUMMY_CONTEXT, self.del_callback)
                    )


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

        # self.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)

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
                # from trezor.lvglui.scrs import fingerprints

                # if fingerprints.has_fingerprints():
                #     # from trezor import config

                #     # if config.has_pin():
                #     #     config.lock()
                from apps.common.request_pin import verify_user_pin

                workflow.spawn(
                    verify_user_pin(
                        re_loop=False,
                        allow_cancel=True,
                        callback=lambda: FingerprintSetting(self),
                        allow_fingerprint=False,
                    )
                )
                # else:

                #     workflow.spawn(
                #         fingerprints.add_fingerprint(
                #             0, callback=lambda: FingerprintSetting(self)
                #         )
                #     )
            elif target == self.device_auth:
                DeviceAuthScreen(self)
            elif target == self.safety_check:
                SafetyCheckSetting(self)
            else:
                if __debug__:
                    print("unknown")
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
        firmware_version = device.get_firmware_version()
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
            device.get_serial(),
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

    def on_click(self, btn):
        if utils.lcd_resume():
            return
        if btn == self.btn:
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

        if device.is_usb_lock_enabled():
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
                    device.set_usb_lock_enable(True)
                else:
                    self.description.set_text(
                        _(i18n_keys.CONTENT__USB_LOCK_DISABLED__HINT)
                    )
                    device.set_usb_lock_enable(False)


class FingerprintSetting(AnimScreen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
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
            self.tips.delete()

        from . import fingerprints

        self.fingerprint_list = fingerprints.get_fingerprint_list()

        counter = fingerprints.get_fingerprint_count()
        self.added_fingerprints = []
        if counter > 0:
            for ids in self.fingerprint_list:
                self.added_fingerprints.append(
                    ListItemBtn(
                        self.container,
                        _(i18n_keys.FORM__FINGER_STR).format(ids + 1),
                        left_img_src="A:/res/settings-fingerprint.png",
                        has_next=False,
                    )
                    if ids is not None
                    else None
                )
        self.add_fingerprint = None
        if counter < 3:
            self.add_fingerprint = ListItemBtn(
                self.container,
                _(i18n_keys.BUTTON__ADD_FINGERPRINT),
                left_img_src="A:/res/settings-plus.png",
                has_next=False,
            )
        self.tips = lv.label(self.content_area)
        self.tips.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 0)
        self.tips.set_long_mode(lv.label.LONG.WRAP)
        self.tips.set_text(
            _(
                i18n_keys.CONTENT__ALLOW_UP_TO_3_FINGERPRINTS_TO_BE_RECORDED_SIMULTANEOUSLY
            )
        )
        self.tips.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular26)
            .width(456)
            .text_color(lv_colors.WHITE_2)
            .text_letter_space(-1)
            .text_align_left()
            .pad_ver(16)
            .pad_hor(12),
            0,
        )
        self.container_fun = ContainerFlexCol(
            self.content_area, self.tips, pos=(0, 12), padding_row=2
        )
        self.unlock = ListItemBtnWithSwitch(
            self.container_fun, _(i18n_keys.FORM__UNLOCK_DEVICE)
        )
        if not device.is_fingerprint_unlock_enabled():
            self.unlock.clear_state()

    async def on_remove(self, i):
        self.added_fingerprints.pop(i).delete()
        from trezorio import fingerprint

        if self.fingerprint_list[i] is not None:
            # pyright: off
            fingerprint.remove(self.fingerprint_list[i])
            # pyright: on
        else:
            assert False
        self.fresh_show()

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            from trezor.lvglui.scrs import fingerprints

            if target == self.add_fingerprint:
                workflow.spawn(
                    fingerprints.add_fingerprint(
                        self.fingerprint_list.index(None)
                        if self.fingerprint_list
                        else 0,
                        callback=lambda: self.fresh_show(),
                    )
                )
            elif target in self.added_fingerprints:
                for i, finger in enumerate(self.added_fingerprints):
                    if target == finger:
                        select_index = i
                        # pyright: off
                        workflow.spawn(
                            fingerprints.request_delete_fingerprint(
                                _(i18n_keys.FORM__FINGER_STR).format(
                                    self.fingerprint_list[select_index] + 1
                                ),
                                on_remove=lambda: self.on_remove(select_index),
                            )
                        )
                        # pyright: on

    def on_value_changed(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.unlock.switch:

                if target.has_state(lv.STATE.CHECKED):
                    device.enable_fingerprint_unlock(True)
                else:
                    device.enable_fingerprint_unlock(False)


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
        # self.strict = ListItemBtn(
        #     self.container, _(i18n_keys.ITEM__STATUS__STRICT), has_next=False
        # )
        # self.strict.add_check_img()
        # self.prompt = ListItemBtn(
        #     self.container, _(i18n_keys.ITEM__STATUS__PROMPT), has_next=False
        # )
        self.safety_check = ListItemBtnWithSwitch(
            self.container, _(i18n_keys.ITEM__SAFETY_CHECKS)
        )
        # self.prompt.add_check_img()
        self.description = lv.label(self.content_area)
        self.description.set_size(456, lv.SIZE.CONTENT)
        self.description.set_long_mode(lv.label.LONG.WRAP)
        self.description.set_style_text_font(font_GeistRegular26, lv.STATE.DEFAULT)
        self.description.set_style_text_line_space(3, 0)
        self.description.align_to(self.container, lv.ALIGN.OUT_BOTTOM_LEFT, 8, 16)
        self.description.set_recolor(True)
        # self.set_checked()
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
        # self.status_bar = lv.obj(self)
        # self.status_bar.remove_style_all()
        # self.status_bar.set_size(lv.pct(100), 44)
        # self.status_bar.add_style(
        #     StyleWrapper()
        #     .bg_opa()
        #     .align(lv.ALIGN.TOP_LEFT)
        #     .bg_img_src("A:/res/warning_bar.png"),
        #     0,
        # )
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
        self.passphrase = ListItemBtn(self.container, _(i18n_keys.ITEM__PASSPHRASE))
        self.trezor_mode = ListItemBtnWithSwitch(
            self.container, _(i18n_keys.ITEM__COMPATIBLE_WITH_TREZOR)
        )
        self.trezor_mode.add_style(
            StyleWrapper().bg_color(lv_colors.ONEKEY_BLACK_3).bg_opa(lv.OPA.COVER), 0
        )
        if not device.is_trezor_compatible():
            self.trezor_mode.clear_state()
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.trezor_mode.add_event_cb(
            self.on_value_changed, lv.EVENT.VALUE_CHANGED, None
        )
        self.rest_device = ListItemBtn(
            self.content_area,
            _(i18n_keys.ITEM__RESET_DEVICE),
            has_next=False,
        )
        self.rest_device.label_left.set_style_text_color(lv_colors.ONEKEY_RED_1, 0)
        self.rest_device.align_to(self.trezor_mode, lv.ALIGN.OUT_BOTTOM_MID, 0, 12)
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
                # pyright: off
                workflow.spawn(
                    recovery_device(
                        DUMMY_CONTEXT,
                        RecoveryDevice(dry_run=True, enforce_wordlist=True),
                    )
                )
                # pyright: on
            elif target == self.passphrase:
                PassphraseScreen(self)
            elif target == self.rest_device:
                from apps.management.wipe_device import wipe_device
                from trezor.messages import WipeDevice

                workflow.spawn(wipe_device(DUMMY_CONTEXT, WipeDevice()))

    def on_value_changed(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.trezor_mode.switch:
                TrezorModeToggle(self, not device.is_trezor_compatible())

    def reset_switch(self):
        if device.is_trezor_compatible():
            self.trezor_mode.add_state()
        else:
            self.trezor_mode.clear_state()


class PassphraseScreen(AnimScreen):
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
            prev_scr=prev_scr, title=_(i18n_keys.TITLE__PASSPHRASE), nav_back=True
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

        passphrase_enable = device.is_passphrase_enabled()
        if passphrase_enable:
            self.passphrase.add_state()
            self.description.set_text(_(i18n_keys.CONTENT__PASSPHRASE_ENABLED__HINT))
        else:
            self.passphrase.clear_state()
            self.description.set_text(_(i18n_keys.CONTENT__PASSPHRASE_DISABLED__HINT))
        self.container.add_event_cb(self.on_value_changed, lv.EVENT.VALUE_CHANGED, None)
        self.add_event_cb(self.on_value_changed, lv.EVENT.READY, None)
        self.add_event_cb(self.on_value_changed, lv.EVENT.CANCEL, None)
        self.load_screen(self)
        gc.collect()

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
                    PassphraseTipsConfirm(
                        _(i18n_keys.TITLE__DISABLE_PASSPHRASE),
                        _(i18n_keys.SUBTITLE__DISABLE_PASSPHRASE),
                        _(i18n_keys.BUTTON__DISABLE),
                        self,
                        icon_path="",
                    )
        elif code == lv.EVENT.READY:
            if self.passphrase.switch.has_state(lv.STATE.CHECKED):
                self.description.set_text(
                    _(i18n_keys.CONTENT__PASSPHRASE_ENABLED__HINT)
                )
                device.set_passphrase_enabled(True)
                device.set_passphrase_always_on_device(False)
            else:
                self.description.set_text(
                    _(i18n_keys.CONTENT__PASSPHRASE_DISABLED__HINT)
                )
                device.set_passphrase_enabled(False)
        elif code == lv.EVENT.CANCEL:
            if self.passphrase.switch.has_state(lv.STATE.CHECKED):
                self.passphrase.clear_state()
            else:
                self.passphrase.add_state()


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


class CryptoScreen(Screen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        super().__init__(prev_scr, title=_(i18n_keys.TITLE__CRYPTO), nav_back=True)

        self.container = ContainerFlexCol(self, self.title, padding_row=2)
        self.ethereum = ListItemBtn(self.container, _(i18n_keys.TITLE__ETHEREUM))
        self.solana = ListItemBtn(self.container, _(i18n_keys.TITLE__SOLANA))
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.ethereum:
                EthereumSetting(self)
            elif target == self.solana:
                SolanaSetting(self)


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


class UserGuide(Screen):
    def __init__(self, prev_scr=None):
        if not hasattr(self, "_init"):
            self._init = True
        else:
            return
        kwargs = {
            "prev_scr": prev_scr,
            "title": _(i18n_keys.APP__USER_GUIDE),
            "nav_back": True,
        }
        super().__init__(**kwargs)

        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=2)
        self.app_tutorial = ListItemBtn(
            self.container, _(i18n_keys.ITEM__ONEKEY_APP_TUTORIAL)
        )
        self.hardware_wallet = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__HOW_HARDWARE_WALLET_WORKS),
        )
        self.recovery_phrase = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__WHAT_IS_RECOVERY_PHRASE),
        )
        self.passphrase = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__PASSPHRASE_ACCESS_HIDDEN_WALLETS),
        )
        self.passkeys = ListItemBtn(
            self.container,
            _(i18n_keys.FIDO_FIDO_KEYS_LABEL),
        )
        self.power_off = ListItemBtn(
            self.container,
            _(i18n_keys.TITLE__POWER_ON_OFF__GUIDE),
        )
        self.pin_protection = ListItemBtn(
            self.container,
            _(i18n_keys.ITEM__ENABLE_PIN_PROTECTION),
        )
        self.fingerprint = ListItemBtn(
            self.container,
            _(i18n_keys.TITLE__FINGERPRINT),
        )
        self.need_help = ListItemBtn(self.container, _(i18n_keys.ITEM__NEED_HELP))
        self.container.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

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
            elif target == self.pin_protection:
                PinProtectionDetails()
            elif target == self.hardware_wallet:
                HardwareWalletDetails()
            elif target == self.passphrase:
                PassphraseDetails()
            elif target == self.need_help:
                HelpDetails()
            elif target == self.fingerprint:
                FingerprintDetails()
            elif target == self.passkeys:
                from .app_passkeys import PasskeysRegister

                PasskeysRegister()
            else:
                if __debug__:
                    print("Unknown")

    def _load_scr(self, scr: "Screen", back: bool = False) -> None:
        lv.scr_load(scr)


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

    # def destroy(self, _delay):
    #     return self.delete()


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

    # def destroy(self, _delay):
    #     return self.delete()


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

    # def destroy(self, _delay):
    #     return self.delete()


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

    # def destroy(self, _delay):
    #     return self.delete()


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

    # def destroy(self, _delay):
    #     return self.delete()


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
        self.website.set_text("help.onekey.so/hc")
        self.website.align_to(self.item.label, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 0)
        self.underline = lv.line(self.item)
        self.underline.set_points(
            [
                {"x": 0, "y": 2},
                {"x": 245, "y": 2},
            ],
            2,
        )
        self.underline.set_style_line_color(lv_colors.WHITE_2, 0)
        self.underline.align_to(self.website, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 0)

    # def destroy(self, _delay):
    #     return self.delete()
