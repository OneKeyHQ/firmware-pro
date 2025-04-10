import utime

from .. import lv


class DoubleClickDetector:
    def __init__(self, double_callback=None, click_timeout=300, click_dist=20):
        self.double_callback = double_callback
        self.click_timeout = click_timeout
        self.click_dist = click_dist
        self.first_click_time = 0
        self.first_click_point = lv.point_t()
        self.timer = None

    def _reset(self):
        self.first_click_time = 0
        if self.timer:
            self.timer._del()
            self.timer = None

    def handle_click(self, point):
        current_time = utime.ticks_ms()

        if self.first_click_time != 0:
            time_diff = utime.ticks_diff(current_time, self.first_click_time)
            pos_match = (
                abs(point.x - self.first_click_point.x) <= self.click_dist
                and abs(point.y - self.first_click_point.y) <= self.click_dist
            )
            if time_diff <= self.click_timeout and pos_match:
                self._reset()
                if self.double_callback:
                    self.double_callback()
                return True

        self.first_click_time = current_time
        self.first_click_point.x = point.x
        self.first_click_point.y = point.y
        if self.timer:
            self.timer._del()
        self.timer = lv.timer_create(lambda t: self._reset(), self.click_timeout, None)
        self.timer.set_repeat_count(1)
        return False
