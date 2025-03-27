import storage
from trezor import ui, utils
from trezor.lvglui import get_elapsed, retrieve_icon_path
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.lvglui.lv_colors import lv_colors
from trezor.lvglui.scrs import font_GeistRegular20, font_GeistSemiBold48, lv
from trezor.lvglui.scrs.widgets.style import StyleWrapper


class ChargingPromptScr(lv.obj):
    _instance = None

    @classmethod
    def get_instance(cls) -> "ChargingPromptScr":
        if cls._instance is None:
            cls._instance = ChargingPromptScr(utils.BATTERY_CAP)
        return cls._instance

    @classmethod
    def has_instance(cls) -> bool:
        return cls._instance is not None

    @classmethod
    def reset(cls) -> None:
        if cls._instance is not None:
            cls._instance = None

    def __init__(self, battery_level) -> None:
        super().__init__(lv.layer_sys())
        self.set_size(lv.pct(100), lv.pct(100))
        self.add_style(
            StyleWrapper().bg_color(lv_colors.BLACK).bg_opa().radius(0).border_width(0),
            lv.PART.MAIN | lv.STATE.DEFAULT,
        )
        self.add_flag(lv.obj.FLAG.CLICKABLE)

        self.charging_bg = lv.img(self)
        self.charging_bg.set_src("A:/res/charging-bg.png")
        self.charging_bg.align(lv.ALIGN.CENTER, 0, 0)

        self.charging_fg = lv.img(self)
        self.charging_fg.set_src("A:/res/charging-fg.png")
        self.charging_fg.align(lv.ALIGN.CENTER, 0, 0)

        self.percent = lv.label(self)
        self.percent.add_style(
            StyleWrapper()
            .text_font(font_GeistSemiBold48)
            .text_color(lv_colors.WHITE)
            .pad_all(0),
            lv.PART.MAIN | lv.STATE.DEFAULT,
        )
        self.percent.set_text(f"{battery_level or 50}%")
        self.percent.align_to(self.charging_fg, lv.ALIGN.TOP_MID, 0, 122)

        self.prompt = lv.label(self)
        self.prompt.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular20)
            .text_color(lv_colors.WHITE)
            .pad_all(0),
            lv.PART.MAIN | lv.STATE.DEFAULT,
        )
        self.prompt.set_text(_(i18n_keys.MSG__CHARGING))
        self.prompt.align_to(self.percent, lv.ALIGN.OUT_BOTTOM_MID, 0, 4)

        if utils.BATTERY_CAP:
            self.battery = lv.img(self)
            self.battery.set_src(retrieve_icon_path(utils.BATTERY_CAP, True))
            self.battery.align_to(self.prompt, lv.ALIGN.OUT_BOTTOM_MID, 0, 12)

        self.add_event_cb(self.on_event, lv.EVENT.CLICKED, None)
        self.add_event_cb(self.on_event, lv.EVENT.DELETE, None)
        self.del_delayed(10000)

        self.anim = lv.anim_t()
        self.anim.init()
        self.anim.set_var(self.charging_bg)
        self.anim.set_values(256, 320)
        self.anim.set_time(2000)
        self.anim.set_playback_delay(0)
        self.anim.set_playback_time(2000)
        self.anim.set_repeat_delay(0)
        self.anim.set_repeat_count(0xFFFF)
        self.anim.set_path_cb(lv.anim_t.path_ease_in_out)
        self.anim.set_custom_exec_cb(lambda _a, val: self.anim_scale(val))
        self.anim_r = lv.anim_t.start(self.anim)

    def anim_scale(self, scale):
        try:
            self.charging_bg.set_zoom(scale)
        except Exception:
            pass

    def show(self):

        ui.display.backlight(storage.device.get_brightness() - 10)

    def on_event(self, event_obj: lv.event_t):
        code = event_obj.code
        if code == lv.EVENT.CLICKED:
            lv.anim_del(self.anim_r.var, None)
            self.destroy()
        elif code == lv.EVENT.DELETE:
            ChargingPromptScr.reset()
            if get_elapsed() > 10000:
                ui.display.backlight(0)
            if __debug__:
                print("delete .......")

    def destroy(self):
        self.clear_flag(lv.obj.FLAG.CLICKABLE)
        self.delete()
