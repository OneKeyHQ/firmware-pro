import utime
from typing import Any, NoReturn

import storage.cache
import storage.sd_salt
from trezor import config, loop, wire
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.lvglui.lv_colors import lv_colors
from trezor.lvglui.scrs import fingerprints, font_GeistRegular30, font_GeistSemiBold64
from trezor.lvglui.scrs.common import FullSizeWindow, lv

from apps.common.pin_constants import PinResult, PinType

from .sdcard import SdCardUnavailable, request_sd_salt


def can_lock_device() -> bool:
    """Return True if the device has a PIN set or SD-protect enabled."""
    return config.has_pin() or storage.sd_salt.is_enabled()


async def request_pin(
    ctx: wire.GenericContext,
    prompt: str = "",
    attempts_remaining: int | None = None,
    allow_cancel: bool = True,
    allow_fingerprint: bool = True,
    standy_wall_only: bool = False,
    **kwargs: Any,
) -> str:
    from trezor.ui.layouts import request_pin_on_device

    return await request_pin_on_device(
        ctx,
        prompt,
        attempts_remaining,
        allow_cancel,
        allow_fingerprint,
        standy_wall_only=standy_wall_only,
    )


async def request_pin_confirm(ctx: wire.Context, *args: Any, **kwargs: Any) -> str:
    while True:
        if kwargs.get("show_tip", True):
            from trezor.ui.layouts import request_pin_tips

            await request_pin_tips(ctx)
        pin1 = await request_pin(
            ctx, _(i18n_keys.TITLE__ENTER_NEW_PIN), *args, **kwargs
        )
        pin2 = await request_pin(
            ctx, _(i18n_keys.TITLE__ENTER_PIN_AGAIN), *args, **kwargs
        )
        if pin1 == pin2:
            return pin1
        await pin_mismatch(ctx)


async def pin_mismatch(ctx) -> None:
    from trezor.ui.layouts import show_warning

    await show_warning(
        ctx=ctx,
        br_type="pin_not_match",
        header=_(i18n_keys.TITLE__NOT_MATCH),
        content=_(i18n_keys.SUBTITLE__SETUP_SET_PIN_PIN_NOT_MATCH),
        icon="A:/res/danger.png",
        btn_yes_bg_color=lv_colors.ONEKEY_BLACK,
    )


async def request_pin_and_sd_salt(
    ctx: wire.Context,
    prompt: str = "",
    allow_cancel: bool = True,
    allow_fingerprint: bool = True,
    standy_wall_only: bool = False,
) -> tuple[str, bytearray | None]:
    if config.has_pin():
        pin = await request_pin(
            ctx,
            prompt,
            config.get_pin_rem(),
            allow_cancel,
            allow_fingerprint,
            standy_wall_only,
        )
        config.ensure_not_wipe_code(pin)
    else:
        pin = ""

    salt = await request_sd_salt(ctx)

    return pin, salt


def _set_last_unlock_time() -> None:
    now = utime.ticks_ms()
    storage.cache.set_int(storage.cache.APP_COMMON_REQUEST_PIN_LAST_UNLOCK, now)


def _get_last_unlock_time() -> int:
    return storage.cache.get_int(storage.cache.APP_COMMON_REQUEST_PIN_LAST_UNLOCK) or 0


async def verify_user_pin(
    ctx: wire.GenericContext = wire.DUMMY_CONTEXT,
    prompt: str = "",
    allow_cancel: bool = True,
    retry: bool = True,
    cache_time_ms: int = 0,
    re_loop: bool = False,
    callback=None,
    allow_fingerprint: bool = True,
    close_others: bool = True,
    attach_wall_only: bool = False,
    pin_use_type: int = PinType.USER_AND_PASSPHRASE_PIN,
    standy_wall_only: bool = False,
) -> None:
    from storage import device

    pin_use_type = int(pin_use_type)
    if not device.is_passphrase_enabled():
        pin_use_type = PinType.USER
        pin_use_type = int(pin_use_type)
    if pin_use_type is PinType.PASSPHRASE_PIN:
        prompt = f"{_(i18n_keys.TITLE__ENTER_HIDDEN_WALLET_PIN)}"
    last_unlock = _get_last_unlock_time()
    if (
        cache_time_ms
        and last_unlock
        and utime.ticks_ms() - last_unlock <= cache_time_ms
        and config.is_unlocked()
        and fingerprints.is_unlocked()
    ):
        return

    if config.has_pin():
        from trezor.ui.layouts import request_pin_on_device

        try:
            pin = await request_pin_on_device(
                ctx,
                prompt,
                config.get_pin_rem(),
                allow_cancel,
                allow_fingerprint,
                close_others=close_others,
                standy_wall_only=standy_wall_only,
                attach_wall_only=attach_wall_only,
            )
            config.ensure_not_wipe_code(pin)
        except Exception:
            raise wire.PinCancelled("cancle")
    else:
        pin = ""
    try:
        salt = await request_sd_salt(ctx)
    except SdCardUnavailable:
        raise wire.PinCancelled("SD salt is unavailable")
    except Exception:
        raise wire.PinCancelled("cancle")

    if not config.is_unlocked():
        try:
            verified, usertype = config.unlock(pin, salt, pin_use_type)
            if verified and pin_use_type in (
                PinType.USER,
                PinType.PASSPHRASE_PIN,
                PinType.USER_AND_PASSPHRASE_PIN,
            ):
                if usertype == PinResult.PASSPHRASE_PIN_ENTERED:
                    device.set_passphrase_pin_enabled(True)
                elif usertype == PinResult.USER_PIN_ENTERED:
                    device.set_passphrase_pin_enabled(False)

        except Exception:
            raise wire.PinCancelled("cancle")
    else:
        try:
            verified, usertype = config.check_pin(
                pin, salt, pin_use_type, auto_vibrate=True
            )
            if verified and pin_use_type in (
                PinType.USER,
                PinType.PASSPHRASE_PIN,
                PinType.USER_AND_PASSPHRASE_PIN,
            ):
                if usertype == PinResult.PASSPHRASE_PIN_ENTERED:
                    device.set_passphrase_pin_enabled(True)
                elif usertype == PinResult.USER_PIN_ENTERED:
                    device.set_passphrase_pin_enabled(False)
        except Exception:
            raise wire.PinCancelled("cancle")

    if verified:
        if re_loop:
            loop.clear()
        elif callback:
            callback()
        _set_last_unlock_time()
        return
    elif not config.has_pin():
        raise RuntimeError
    while retry:
        pin_rem = config.get_pin_rem()
        try:

            pin = await request_pin_on_device(  # type: ignore ["request_pin_on_device" is possibly unbound]  # 再次请求PIN码
                ctx,
                # _(i18n_keys.TITLE__ENTER_PIN),
                prompt,
                pin_rem,
                allow_cancel,
                allow_fingerprint,
                close_others=close_others,
                standy_wall_only=standy_wall_only,
                attach_wall_only=attach_wall_only,
            )
        except Exception:
            raise wire.PinCancelled("cancle")

        try:
            if not config.is_unlocked():
                verified, usertype = config.unlock(pin, salt, pin_use_type)
            else:
                verified, usertype = config.check_pin(
                    pin, salt, pin_use_type, auto_vibrate=True
                )
        except Exception:
            raise wire.PinCancelled("cal cale ..")

        if verified and pin_use_type in (
            PinType.USER,
            PinType.PASSPHRASE_PIN,
            PinType.USER_AND_PASSPHRASE_PIN,
        ):
            if usertype == PinResult.PASSPHRASE_PIN_ENTERED:
                device.set_passphrase_pin_enabled(True)
            elif usertype == PinResult.USER_PIN_ENTERED:
                device.set_passphrase_pin_enabled(False)

            if re_loop:
                loop.clear()
            elif callback:
                callback()
            _set_last_unlock_time()
            return
        else:
            continue

    raise wire.PinInvalid


async def verify_user_fingerprint(
    ctx: wire.GenericContext = wire.DUMMY_CONTEXT,
    re_loop: bool = False,
    callback=None,
):
    if fingerprints.is_unlocked():
        return
    if await fingerprints.request():
        fingerprints.unlock()
        if re_loop:
            loop.clear()
        elif callback:
            callback()
        _set_last_unlock_time


async def error_pin_invalid(ctx: wire.Context) -> NoReturn:
    from trezor.ui.layouts import show_error_and_raise

    await show_error_and_raise(
        ctx,
        "warning_wrong_pin",
        header=_(i18n_keys.TITLE__WRONG_PIN),
        content=_(i18n_keys.SUBTITLE__SET_PIN_WRONG_PIN),
        red=True,
        exc=wire.PinInvalid,
    )
    assert False


async def error_pin_used(ctx: wire.Context) -> NoReturn:
    from trezor.ui.layouts import show_error_and_raise

    await show_error_and_raise(
        ctx,
        "warning_wrong_pin",
        header=_(i18n_keys.PASSPHRASE__PIN_USED),
        content=_(i18n_keys.PASSPHRASE__PIN_USED_DESC),
        red=True,
        exc=wire.PinInvalid,
    )
    assert False


async def passphrase_pin_used(ctx: wire.Context):
    screen = FullSizeWindow(
        None,
        None,
        confirm_text=_(i18n_keys.BUTTON__OVERWRITE),
        anim_dir=0,
    )

    close_btn = lv.btn(screen)
    close_btn.set_size(48, 48)
    close_btn.align(lv.ALIGN.TOP_RIGHT, -12, 56)
    close_btn.set_style_bg_color(lv_colors.BLACK, 0)
    close_btn.set_style_bg_opa(0, 0)
    close_btn.set_style_border_width(0, 0)
    close_btn.set_style_shadow_width(0, 0)
    close_btn.add_flag(lv.obj.FLAG.CLICKABLE)
    close_btn.set_ext_click_area(100)

    close_img = lv.img(close_btn)
    close_img.set_src("A:/res/nav-icon.png")
    close_img.center()

    title_label = lv.label(screen.content_area)
    title_label.set_text(_(i18n_keys.PASSPHRASE__PIN_USED))
    title_label.set_style_text_font(font_GeistSemiBold64, 0)
    title_label.set_style_text_color(lv_colors.WHITE, 0)
    title_label.set_style_text_letter_space(-6, 0)
    title_label.set_style_text_line_space(-6, 0)
    title_label.set_long_mode(lv.label.LONG.WRAP)
    title_label.set_size(456, lv.SIZE.CONTENT)
    title_label.align(lv.ALIGN.TOP_MID, 0, 72)

    if hasattr(screen, "subtitle"):
        screen.subtitle.delete()

    subtitle_label = lv.label(screen.content_area)
    subtitle_label.set_text(_(i18n_keys.TITLE__PIN_ALREADY_USED_DESC))
    subtitle_label.set_style_text_font(font_GeistRegular30, 0)
    subtitle_label.set_style_text_color(lv_colors.LIGHT_GRAY, 0)
    subtitle_label.set_style_text_letter_space(-2, 0)
    subtitle_label.set_style_text_line_space(5, 0)
    subtitle_label.set_long_mode(lv.label.LONG.WRAP)
    subtitle_label.set_size(456, lv.SIZE.CONTENT)
    subtitle_label.align_to(title_label, lv.ALIGN.OUT_BOTTOM_MID, 0, 16)

    # Set confirm button text color to gray
    if hasattr(screen, "btn_yes"):
        screen.btn_yes.enable(lv_colors.ONEKEY_GRAY_3, text_color=lv_colors.WHITE)

    processing = False

    def on_close_clicked(e):
        nonlocal processing
        if e.code == lv.EVENT.CLICKED and not processing:
            processing = True
            screen.show_dismiss_anim()
            screen.channel.publish(0)

    close_btn.add_event_cb(on_close_clicked, lv.EVENT.CLICKED, None)

    screen.btn_layout_ver()
    result = await ctx.wait(screen.request())
    return result


# async def passphrase_pin_used(ctx: wire.Context) -> NoReturn:
#     from trezor.ui.layouts import show_error_and_raise

#     await show_error_and_raise(
#         ctx,
#         "warning_wrong_pin",
#         header=_(i18n_keys.PASSPHRASE__PIN_USED),
#         content=_(i18n_keys.PASSPHRASE__PIN_USED_DESC),  #i18n待更改
#         red=True,
#         exc=wire.PinInvalid,
#     )
#     assert False


async def error_pin_matches_wipe_code(ctx: wire.Context) -> NoReturn:
    from trezor.ui.layouts import show_error_and_raise

    await show_error_and_raise(
        ctx,
        "warning_invalid_new_pin",
        header="Invalid PIN",
        content="The new PIN must be different from your\nwipe code.",
        red=True,
        exc=wire.PinInvalid,
    )
    assert False
