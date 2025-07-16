import utime
from typing import Any, NoReturn

import storage.cache
import storage.sd_salt
from trezor import config, loop, wire
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.lvglui.lv_colors import lv_colors
from trezor.lvglui.scrs import fingerprints

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
            result = config.unlock(pin, salt, pin_use_type)
            if isinstance(result, tuple):
                verified, usertype = result
            else:
                verified = result
                usertype = PinResult.USER_PIN_ENTERED
            print("usertype", usertype)

            if verified:
                if usertype == PinResult.PASSPHRASE_PIN_ENTERED:
                    device.set_passphrase_pin_enabled(True)
                elif usertype == PinResult.USER_PIN_ENTERED:
                    device.set_passphrase_pin_enabled(False)

        except Exception:
            raise wire.PinCancelled("cancle")
    else:
        try:
            result = config.check_pin(pin, salt, pin_use_type)
            if isinstance(result, tuple):
                verified, usertype = result
            else:
                verified = result
                usertype = PinResult.USER_PIN_ENTERED
            if verified:
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
                result = config.unlock(pin, salt, pin_use_type)
                if isinstance(result, tuple):
                    verified, usertype = result
                else:
                    verified = result
                    usertype = PinResult.USER_PIN_ENTERED
            else:
                result = config.check_pin(pin, salt, pin_use_type)
            if isinstance(result, tuple):
                verified, usertype = result
            else:
                verified = result
                usertype = PinResult.USER_PIN_ENTERED
        except Exception:
            raise wire.PinCancelled("cal cale ..")

        if verified:
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
        import storage.device as device

        device.set_passphrase_pin_enabled(False)
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
