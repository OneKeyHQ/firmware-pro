import utime

from trezor import motor
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys

from .. import font_GeistSemiBold30, lv, lv_colors
from ..widgets.style import StyleWrapper

MAX_VISIBLE_VALUE = 175
MIN_VISIBLE_VALUE = 20
START_VALUE_TOLERANCE = 5
# slider 456px, knob 82px, pad 16px => track ~358px, range 155 units => ~2.3px/unit
MAX_FORWARD_JUMP = 60  # ~138px/frame
MIN_PROGRESS_STEPS = 3  # normal slide produces 15-60 events
MAX_DRAG_DURATION_MS = 3000  # max 3s per drag, blocks slow random touch accumulation
TOUCH_BOUNDS_MARGIN_HORIZONTAL_PX = 12
TOUCH_BOUNDS_MARGIN_VERTICAL_PX = 36


class Slider(lv.slider):
    SLIDER_DISABLE_ARROW_IMG_SRC = "A:/res/slide-arrow-disable.png"
    SLIDER_DEFAULT_ARROW_IMG_SRC = "A:/res/slide-arrow-black.png"
    SLIDER_DEFAULT_DONE_IMG_SRC = "A:/res/slider-done-black.png"
    SLIDER_ARROW_WHITE_IMG_SRC = "A:/res/slide-arrow-white.png"
    SLIDER_DONE_WHITE_IMG_SRC = "A:/res/slide-done-white.png"

    def __init__(self, parent, text, relative_y=-114) -> None:
        super().__init__(parent)
        self.remove_style_all()
        self.disable = False
        self.arrow_img_src = Slider.SLIDER_DEFAULT_ARROW_IMG_SRC
        self.done_img_src = Slider.SLIDER_DEFAULT_DONE_IMG_SRC
        self.set_size(456, 114)
        self.add_flag(lv.obj.FLAG.ADV_HITTEST)
        self.align(lv.ALIGN.BOTTOM_MID, 0, relative_y)
        self.set_range(0, 200)
        self.set_value(MIN_VISIBLE_VALUE, lv.ANIM.OFF)
        self.set_style_anim_time(100, lv.PART.MAIN)

        self.add_style(
            StyleWrapper()
            # .border_width(2)
            # .border_color(lv_colors.WHITE)
            .bg_color(lv_colors.ONEKEY_BLACK_3)
            .bg_opa()
            .pad_ver(20)
            .pad_hor(8)
            .radius(98),
            lv.PART.MAIN | lv.STATE.DEFAULT,
        )
        self.add_style(
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_GREEN_1)
            .bg_opa()
            .width(82)
            .height(82)
            .pad_all(-16)
            .radius(lv.RADIUS.CIRCLE),
            lv.PART.KNOB | lv.STATE.DEFAULT,
        )
        self.add_style(
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_BLACK_3)
            .bg_opa(lv.OPA.COVER)
            .radius(98),
            lv.PART.INDICATOR | lv.STATE.DEFAULT,
        )
        self.text = text

        self.tips = lv.label(self)
        self.tips.add_style(
            StyleWrapper()
            .text_font(font_GeistSemiBold30)
            .text_letter_space(-1)
            .text_color(lv_colors.ONEKEY_WHITE_4),
            0,
        )
        self.tips.set_text(_(i18n_keys.BUTTON__PROCESSING))
        self.tips.center()
        self.tips.add_flag(lv.obj.FLAG.HIDDEN)

        self.add_event_cb(self.on_event, lv.EVENT.PRESSING, None)
        self.add_event_cb(self.on_event, lv.EVENT.PRESSED, None)
        self.add_event_cb(self.on_event, lv.EVENT.RELEASED, None)
        self.add_event_cb(self.on_event, lv.EVENT.DRAW_PART_BEGIN, None)
        self.add_event_cb(self.on_event, lv.EVENT.DRAW_PART_END, None)

        self._drag_active = False
        self._start_value = MIN_VISIBLE_VALUE
        self._last_value = MIN_VISIBLE_VALUE
        self._reached_end = False
        self._progress_count = 0
        self._invalid_jump = False
        self._passed = False
        self._drag_start_ms = 0
        self._blocked_until_release = False
        self._touch_point = lv.point_t()
        self._slider_area = lv.area_t()

    def enable(self, enable: bool = True):
        if enable:
            self.disable = False
            self.set_style_bg_color(
                lv_colors.ONEKEY_GRAY_2, lv.PART.MAIN | lv.STATE.DEFAULT
            )
            self.set_style_bg_color(
                lv_colors.ONEKEY_GRAY_2, lv.PART.INDICATOR | lv.STATE.DEFAULT
            )
            # self.arrow_img_src = Slider.SLIDER_ARROW_BLACK_IMG_SRC
            # self.done_img_src = Slider.SLIDER_DONE_WHITE_IMG_SRC
            self.set_style_bg_color(
                lv_colors.ONEKEY_RED_1, lv.PART.KNOB | lv.STATE.DEFAULT
            )
            # self.set_style_border_color(lv_colors.ONEKEY_GRAY, 0)
            self.tips.set_style_text_color(lv_colors.ONEKEY_WHITE_4, 0)
        else:
            self.disable = True
            self.set_style_bg_color(
                lv_colors.ONEKEY_GRAY_1, lv.PART.KNOB | lv.STATE.DEFAULT
            )
            self.set_style_bg_color(
                lv_colors.ONEKEY_BLACK_3, lv.PART.MAIN | lv.STATE.DEFAULT
            )
            self.set_style_bg_color(
                lv_colors.ONEKEY_BLACK_3, lv.PART.INDICATOR | lv.STATE.DEFAULT
            )
            # self.set_style_border_color(lv_colors.ONEKEY_GRAY_1, 0)
            self.tips.set_style_text_color(lv_colors.WHITE_2, 0)

    def change_knob_style(self, level):
        if level == 1:
            self.add_style(
                StyleWrapper().bg_color(lv_colors.WHITE),
                lv.PART.KNOB | lv.STATE.DEFAULT,
            )
            # self.arrow_img_src = "A:/res/slide-arrow-black.png"
            # self.done_img_src = "A:/res/slider-done-black.png"
        elif level == 2:
            self.add_style(
                StyleWrapper().bg_color(lv_colors.ONEKEY_RED_1),
                lv.PART.KNOB | lv.STATE.DEFAULT,
            )

    def _clamp_value(self, value):
        if value > MAX_VISIBLE_VALUE:
            self.set_value(MAX_VISIBLE_VALUE, lv.ANIM.OFF)
            return MAX_VISIBLE_VALUE
        if value < MIN_VISIBLE_VALUE:
            self.set_value(MIN_VISIBLE_VALUE, lv.ANIM.OFF)
            return MIN_VISIBLE_VALUE
        return value

    def _reset_drag_state(self, value):
        self._drag_active = True
        self._start_value = value
        self._last_value = value
        self._reached_end = value >= MAX_VISIBLE_VALUE
        self._progress_count = 0
        self._invalid_jump = False
        self._passed = False
        self._drag_start_ms = utime.ticks_ms()
        self._blocked_until_release = False

    def _can_pass(self, current_value):
        return (
            self._drag_active
            and self._start_value <= MIN_VISIBLE_VALUE + START_VALUE_TOLERANCE
            and current_value >= MAX_VISIBLE_VALUE
            and self._reached_end
            and self._progress_count >= MIN_PROGRESS_STEPS
            and not self._invalid_jump
            and utime.ticks_diff(utime.ticks_ms(), self._drag_start_ms)
            <= MAX_DRAG_DURATION_MS
        )

    def _abort_drag(self):
        self._drag_active = False
        self._blocked_until_release = True
        self._invalid_jump = True
        self._passed = False
        self.tips.add_flag(lv.obj.FLAG.HIDDEN)
        self.set_value(MIN_VISIBLE_VALUE, lv.ANIM.ON)
        indev = lv.indev_get_act()
        if indev:
            indev.wait_release()

    def on_event(self, event):
        code = event.code
        current_value = event.get_target().get_value()
        if code == lv.EVENT.PRESSING:
            current_value = self._clamp_value(current_value)
            if self._blocked_until_release:
                return
            if not self._drag_active:
                return
            # timeout: reset slider immediately instead of waiting for RELEASED
            if (
                utime.ticks_diff(utime.ticks_ms(), self._drag_start_ms)
                > MAX_DRAG_DURATION_MS
            ):
                self._abort_drag()
                return
            # check if touch is within slider component bounds (not knob-only hit_test)
            indev = lv.indev_get_act()
            if indev:
                indev.get_point(self._touch_point)
                self.get_coords(self._slider_area)
                if (
                    self._touch_point.x
                    < self._slider_area.x1 - TOUCH_BOUNDS_MARGIN_HORIZONTAL_PX
                    or self._touch_point.x
                    > self._slider_area.x2 + TOUCH_BOUNDS_MARGIN_HORIZONTAL_PX
                    or self._touch_point.y
                    < self._slider_area.y1 - TOUCH_BOUNDS_MARGIN_VERTICAL_PX
                    or self._touch_point.y
                    > self._slider_area.y2 + TOUCH_BOUNDS_MARGIN_VERTICAL_PX
                ):
                    self._abort_drag()
                    return
            delta = current_value - self._last_value
            if delta > 0:
                self._progress_count += 1
                if delta > MAX_FORWARD_JUMP:
                    self._invalid_jump = True
            self._last_value = current_value
            if current_value >= MAX_VISIBLE_VALUE:
                self._reached_end = True
        elif code == lv.EVENT.PRESSED:
            motor.vibrate(motor.WHISPER)
            current_value = self._clamp_value(current_value)
            self._reset_drag_state(current_value)
        elif code == lv.EVENT.RELEASED:
            self._blocked_until_release = False
            self._passed = self._can_pass(current_value)
            if self._passed:
                self.tips.clear_flag(lv.obj.FLAG.HIDDEN)
                if self.has_flag(lv.obj.FLAG.CLICKABLE):
                    self.clear_flag(lv.obj.FLAG.CLICKABLE)
                motor.vibrate(motor.SUCCESS)
                lv.event_send(self, lv.EVENT.READY, None)
            else:
                motor.vibrate(motor.ERROR)
                self.set_value(MIN_VISIBLE_VALUE, lv.ANIM.ON)
                self.tips.add_flag(lv.obj.FLAG.HIDDEN)
            self._drag_active = False
        elif code == lv.EVENT.DRAW_PART_BEGIN:
            dsc = lv.obj_draw_part_dsc_t.__cast__(event.get_param())
            if dsc.part == lv.PART.KNOB and dsc.id == 0:
                show_done = self._passed or self._can_pass(current_value)
                if show_done:
                    self.tips.clear_flag(lv.obj.FLAG.HIDDEN)
                    dsc.rect_dsc.bg_img_src = self.done_img_src
                else:
                    self.tips.add_flag(lv.obj.FLAG.HIDDEN)
                    dsc.rect_dsc.bg_img_src = self.arrow_img_src
        elif code == lv.EVENT.DRAW_PART_END:
            dsc = lv.obj_draw_part_dsc_t.__cast__(event.get_param())
            if dsc.part == lv.PART.MAIN:
                label_text = self.text
                label_size = lv.point_t()
                lv.txt_get_size(
                    label_size, label_text, font_GeistSemiBold30, -1, 0, lv.COORD.MAX, 0
                )
                label_area = lv.area_t()
                label_area.x1 = (
                    dsc.draw_area.x1
                    + dsc.draw_area.get_width() // 2
                    - min(98, label_size.x // 2)
                )
                label_area.x2 = dsc.draw_area.x2 - 8
                label_area.y1 = (
                    dsc.draw_area.y1
                    + dsc.draw_area.get_height() // 2
                    - (label_size.y if label_size.x > 300 else label_size.y // 2)
                )
                label_area.y2 = dsc.draw_area.y2 - 8
                label_draw_dsc = lv.draw_label_dsc_t()
                label_draw_dsc.init()
                label_draw_dsc.color = (
                    lv_colors.WHITE_2 if self.disable else lv_colors.ONEKEY_WHITE_4
                )
                label_draw_dsc.font = font_GeistSemiBold30
                dsc.draw_ctx.label(label_draw_dsc, label_area, label_text, None)
