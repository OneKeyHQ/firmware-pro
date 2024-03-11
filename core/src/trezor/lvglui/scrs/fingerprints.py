from trezorio import fingerprint

from storage import device
from trezor import config, loop, motor, utils
from trezor.crypto import se_thd89

from ..i18n import gettext as _, keys as i18n_keys
from ..lv_colors import lv_colors
from . import font_GeistRegular30, lv
from .common import FullSizeWindow
from .widgets.style import StyleWrapper

FP_MAX_COLLECT_COUNT = 6
match_chan = loop.chan()


def request():
    return match_chan.take()


def signal_match():
    match_chan.publish(1)


def has_takers():
    return len(match_chan.takers) > 0


def is_available() -> bool:
    return (
        device.is_fingerprint_unlock_enabled()
        and config.is_unlocked()
        and has_fingerprints()
        and failed_count() < utils.MAX_FP_ATTEMPTS
    )


def failed_count() -> int:
    return device.finger_failed_count()


def get_fingerprint_count() -> int:
    try:
        count = fingerprint.get_template_count()
    except Exception as e:
        if __debug__:
            print(f"get fingerprint count failed: {e}")
        count = 0
    return count


def has_fingerprints() -> bool:
    return get_fingerprint_count() > 0


def get_fingerprint_list():
    try:
        fingers = fingerprint.list_template()
    except Exception as e:
        if __debug__:
            print(f"get fingerprint list failed: {e}")
        return ()
    return fingers or ()


def lock() -> bool:
    return se_thd89.fingerprint_lock()


def unlock() -> bool:
    return se_thd89.fingerprint_unlock()


def is_unlocked() -> bool:
    return se_thd89.fingerprint_is_unlocked()


class RequestAddFingerprintScreen(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__FINGERPRINT),
            _(i18n_keys.TITLE__FINGERPRINT_DESC),
            _(i18n_keys.BUTTON__ADD_FINGERPRINT),
            _(i18n_keys.BUTTON__NOT_NOW),
            icon_path="A:/res/fingerprint.png",
            anim_dir=0,
        )
        self.btn_layout_ver()

    def show_unload_anim(self):
        self.destroy(10)


class FingerprintAddedSuccess(FullSizeWindow):
    def __init__(self, ids: int):
        super().__init__(
            title=_(i18n_keys.TITLE__FINGERPRINT_ADDED),
            subtitle=_(i18n_keys.TITLE__FINGERPRINT_ADDED_DESC).format(
                _(i18n_keys.FORM__FINGER_STR).format(ids + 1)
            ),
            confirm_text=_(i18n_keys.BUTTON__CONTINUE),
            icon_path="A:/res/success.png",
            anim_dir=0,
        )

    def show_unload_anim(self):
        self.destroy(100)


class CollectFingerprintStart(FullSizeWindow):
    def __init__(self):
        super().__init__(
            title=_(i18n_keys.TITLE__GET_STARTED),
            subtitle=_(
                i18n_keys.CONTENT__PLACE_YOUR_FINGER_ON_THE_SENSOR_LOCATED_ON_THE_SIDE_OF_THE_PHONE
            ),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            anim_dir=0,
        )
        self.img = lv.img(self.content_area)
        self.img.remove_style_all()
        self.img.set_src("A:/res/finger-start.png")
        self.img.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 86)

        self.arrow = lv.img(self.content_area)
        self.arrow.remove_style_all()
        self.arrow.set_src("A:/res/finger-start-arrow.png")
        self.arrow.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_RIGHT, 0, 220)

        self.anim = lv.anim_t()
        self.anim.init()
        self.anim.set_var(self.arrow)
        self.anim.set_values(372, 278)
        self.anim.set_time(400)
        self.anim.set_playback_delay(100)
        self.anim.set_playback_time(400)
        self.anim.set_repeat_delay(100)
        self.anim.set_repeat_count(0xFFFF)  # infinite
        self.anim.set_path_cb(lv.anim_t.path_ease_in_out)
        self.anim.set_custom_exec_cb(lambda _a, val: self.anim_set_x(val))
        lv.anim_t.start(self.anim)

    def anim_set_x(self, x):
        try:
            self.arrow.set_x(x)
        except Exception:
            pass


class CollectFingerprintProgress(FullSizeWindow):
    _instance = None

    @staticmethod
    def get_instance():
        if CollectFingerprintProgress._instance is None:
            CollectFingerprintProgress._instance = CollectFingerprintProgress()
        return CollectFingerprintProgress._instance

    @staticmethod
    def has_instance():
        return CollectFingerprintProgress._instance is not None

    @staticmethod
    def reset():
        CollectFingerprintProgress._instance = None

    def __init__(self):
        super().__init__(
            title=_(i18n_keys.TITLE__PLACE_FINGER),
            subtitle=_(i18n_keys.TITLE__PLACE_FINGER_DESC),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            anim_dir=0,
        )
        self.img = lv.img(self.content_area)
        self.img.remove_style_all()
        self.img.set_src("A:/res/fingerprint-process-0.png")
        self.img.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 52)

        self.tips = lv.label(self.content_area)
        self.tips.set_long_mode(lv.label.LONG.WRAP)
        self.tips.set_text("")
        self.tips.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular30)
            .width(456)
            .text_color(lv_colors.ONEKEY_YELLOW)
            .text_letter_space(-1)
            .text_align_center()
            .pad_ver(16)
            .pad_hor(12),
            0,
        )
        self.tips.align_to(self.img, lv.ALIGN.OUT_BOTTOM_MID, 0, 6)

    def update_progress(self, progress):
        self.img.set_src(f"A:/res/fingerprint-process-{progress}.png")

    def prompt_tips(self, text: str | None = None):
        if text:
            self.tips.set_text(text)
        else:
            self.tips.set_text(_(i18n_keys.MSG__DO_NOT_PRESS_THE_POWER_BUTTON))
        self.tips.align_to(self.img, lv.ALIGN.OUT_BOTTOM_MID, 0, 16)

    def prompt_tips_clear(self):
        self.tips.set_text("")


async def request_enroll(i) -> None:

    while fingerprint.detect():
        if i != 0:
            if __debug__:
                print("move finger away")
            # motor.vibrate(weak=True)
            await loop.sleep(100)
        else:
            break
    prompt_text = ""
    while True:
        if not fingerprint.detect():
            await loop.sleep(50)
            continue
        try:
            fingerprint.enroll(i)
        except Exception as e:
            if __debug__:
                from trezor import log

                log.exception(__name__, e)
            motor.vibrate()
            if isinstance(e, fingerprint.EnrollDuplicate):
                prompt_text = _(
                    i18n_keys.MSG__LIFT_AND_FINE_TUNE_THE_POSITION_THEN_TOUCH_POWER_BUTTON_AGAIN
                )
            elif isinstance(e, fingerprint.ExtractFeatureFail):
                prompt_text = _(i18n_keys.MSG__CLEAN_FINGERPRINT_SENSOR_AND_TRY_AGAIN)
            elif isinstance(e, (fingerprint.NoFp, fingerprint.GetImageFail)):
                prompt_text = _(i18n_keys.MSG__PUT_FINGER_ON_THE_FINGERPRINT)
            if CollectFingerprintProgress.has_instance():
                CollectFingerprintProgress.get_instance().prompt_tips(prompt_text)
            await loop.sleep(100)
        else:
            motor.vibrate(weak=True)
            break


async def request_add_fingerprint() -> None:
    while True:
        scr = RequestAddFingerprintScreen()
        if await scr.request():
            success = await add_fingerprint(0)
            if __debug__:
                print("add_fingerprint success:", success)
            if success:
                break
        else:
            break


async def add_fingerprint(ids, callback=None) -> bool:

    processes = [12.5, 25, 50, 75, 87.5, 100]
    utils.mark_collecting_fingerprint()
    while True:
        abort = False
        success = True
        scr = CollectFingerprintStart()
        while True:

            if fingerprint.detect():
                motor.vibrate(weak=True)
                scr.destroy(50)
                progress = CollectFingerprintProgress.get_instance()
                for i in range(FP_MAX_COLLECT_COUNT):
                    progress.prompt_tips_clear()
                    enroll_task = request_enroll(i)
                    cancel_task = progress.request()
                    racer = loop.race(enroll_task, cancel_task)
                    await racer
                    if cancel_task in racer.finished:
                        abort = True
                        CollectFingerprintProgress.reset()
                        break
                    progress.update_progress(processes[i])

                progress.destroy(50)
                if abort:
                    success = False
                    break
                ret = fingerprint.save(ids)
                if not ret:
                    success = False
                CollectFingerprintProgress.reset()
                break
            else:
                idle = loop.sleep(100)
                cancel = scr.request()
                racer = loop.race(idle, cancel)
                await racer
                if idle in racer.finished:
                    continue
                elif cancel in racer.finished:
                    success = False
                    abort = False
                    break
        if not abort:
            break
    utils.mark_collecting_fingerprint_done()
    if success:
        await FingerprintAddedSuccess(ids).request()
        if callback and callable(callback):
            callback()
    return success


async def request_delete_fingerprint(fingerprint_name: str, on_remove) -> None:
    confirmed = await RequestRemoveFingerprint(fingerprint_name).request()
    if confirmed:
        confirmed = await ConfirmRemoveFingerprint().request()
        await loop.sleep(20)
        if confirmed:
            await on_remove()


class RequestRemoveFingerprint(FullSizeWindow):
    def __init__(self, fingerprint_name: str):
        super().__init__(
            fingerprint_name,
            None,
            confirm_text=_(i18n_keys.BUTTON__REMOVE),
            icon_path="A:/res/fingerprint.png",
        )
        self.add_nav_back()
        self.btn_yes.add_style(StyleWrapper().bg_color(lv_colors.ONEKEY_RED_1), 0)

        self.add_event_cb(self.on_nav_back, lv.EVENT.CLICKED, None)
        self.add_event_cb(self.on_nav_back, lv.EVENT.GESTURE, None)

    def on_nav_back(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.nav_back.nav_btn:
                self.destroy(50)
        elif code == lv.EVENT.GESTURE:
            _dir = lv.indev_get_act().get_gesture_dir()
            if _dir == lv.DIR.RIGHT:
                lv.event_send(self.nav_back.nav_btn, lv.EVENT.CLICKED, None)

    def show_unload_anim(self):
        # if self.anim_dir == ANIM_DIRS.HOR:
        #     Anim(0, -480, self.set_pos, time=200, y_axis=False, delay=200, del_cb=self._delete).start()
        # else:
        #     self.show_dismiss_anim()
        self.destroy(100)


class ConfirmRemoveFingerprint(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__REMOVE_THIS_FINGERPRINT),
            _(i18n_keys.TITLE__REMOVE_THIS_FINGERPRINT_DESC),
            confirm_text=_(i18n_keys.BUTTON__REMOVE),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            icon_path="A:/res/fingerprint.png",
        )
        self.add_nav_back()
        self.btn_yes.add_style(StyleWrapper().bg_color(lv_colors.ONEKEY_RED_1), 0)
        self.add_event_cb(self.on_nav_back, lv.EVENT.CLICKED, None)
        self.add_event_cb(self.on_nav_back, lv.EVENT.GESTURE, None)

    def on_nav_back(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.nav_back.nav_btn:
                self.destroy(50)
        elif code == lv.EVENT.GESTURE:
            _dir = lv.indev_get_act().get_gesture_dir()
            if _dir == lv.DIR.RIGHT:
                lv.event_send(self.nav_back.nav_btn, lv.EVENT.CLICKED, None)

    def show_unload_anim(self):
        self.destroy(10)
