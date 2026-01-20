from typing import TYPE_CHECKING

import storage.device as storage_device
from trezor import wire
from trezor.enums import ButtonRequestType, SafetyCheckLevel
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.messages import Success
from trezor.strings import format_duration_ms
from trezor.ui.layouts import confirm_action

from apps.base import reload_settings_from_storage
from apps.common import safety_checks

if TYPE_CHECKING:
    from trezor.messages import ApplySettings


def validate_homescreen(homescreen: bytes) -> None:
    # if homescreen == b"":
    #     return

    # if len(homescreen) > storage_device.HOMESCREEN_MAXSIZE:
    #     raise wire.DataError(
    #         f"Homescreen is too large, maximum size is {storage_device.HOMESCREEN_MAXSIZE} bytes"
    #     )

    # try:
    #     w, h, grayscale = ui.display.toif_info(homescreen)
    # except ValueError:
    #     raise wire.DataError("Invalid homescreen")
    # if w != 144 or h != 144:
    #     raise wire.DataError("Homescreen must be 144x144 pixel large")
    # if grayscale:
    #     raise wire.DataError("Homescreen must be full-color TOIF image")
    internal_wallpapers = (
        "wallpaper-1.jpg",
        "wallpaper-2.jpg",
        "wallpaper-3.jpg",
        "wallpaper-4.jpg",
        "wallpaper-5.jpg",
        "wallpaper-6.jpg",
        "wallpaper-7.jpg",
    )
    if not any(name == homescreen.decode() for name in internal_wallpapers):
        raise wire.DataError("Invalid homescreen")


async def apply_settings(ctx: wire.Context, msg: ApplySettings) -> Success:
    if not storage_device.is_initialized():
        raise wire.NotInitialized("Device is not initialized")
    language = msg.language
    # homescreen = msg.homescreen
    label = msg.label
    auto_lock_delay_ms = msg.auto_lock_delay_ms
    use_passphrase = msg.use_passphrase
    passphrase_always_on_device = msg.passphrase_always_on_device
    # display_rotation = msg.display_rotation
    msg_safety_checks = msg.safety_checks
    # experimental_features = msg.experimental_features
    haptic_feedback = msg.haptic_feedback
    auto_shutdown_delay_ms = msg.auto_shutdown_delay_ms
    change_brightness = msg.change_brightness

    allowed_settings = [
        language,
        # homescreen,
        label,
        auto_lock_delay_ms,
        use_passphrase,
        passphrase_always_on_device,
        msg_safety_checks,
        haptic_feedback,
        auto_shutdown_delay_ms,
        change_brightness,
    ]
    if all(s is None for s in allowed_settings):
        raise wire.ProcessError("No setting provided")

    # if homescreen is not None:
    #     validate_homescreen(homescreen)
    #     # await confirm_set_homescreen(ctx)
    #     # storage_device.set_homescreen(f"A:/res/{msg.homescreen.decode()}")

    if label is not None:
        if len(label.encode("utf-8")) > storage_device.LABEL_MAXLENGTH:
            raise wire.DataError("Label too long")
        await require_confirm_change_label(ctx, label)
        storage_device.set_label(label)

    if use_passphrase is not None:
        await require_confirm_change_passphrase(ctx, use_passphrase)
        storage_device.set_passphrase_enabled(use_passphrase)

        if not use_passphrase and storage_device.is_passphrase_pin_enabled():
            from apps.base import lock_device

            storage_device.set_passphrase_pin_enabled(False)
            lock_device()

    if passphrase_always_on_device is not None:
        if not storage_device.is_passphrase_enabled():
            raise wire.DataError("Passphrase is not enabled")
        # else:
        #     if not msg.passphrase_always_on_device:
        #         raise wire.DataError("Only support passphrase input on device")
        await require_confirm_change_passphrase_source(ctx, passphrase_always_on_device)
        storage_device.set_passphrase_always_on_device(passphrase_always_on_device)

    if auto_lock_delay_ms is not None:
        if auto_lock_delay_ms < storage_device.AUTOLOCK_DELAY_MINIMUM:
            raise wire.ProcessError("Auto-lock delay too short")
        if auto_lock_delay_ms > storage_device.AUTOLOCK_DELAY_MAXIMUM:
            raise wire.ProcessError("Auto-lock delay too long")
        await require_confirm_change_autolock_delay(ctx, auto_lock_delay_ms)
        storage_device.set_autolock_delay_ms(auto_lock_delay_ms)

    if auto_shutdown_delay_ms is not None:
        if auto_shutdown_delay_ms < storage_device.AUTOSHUTDOWN_DELAY_MINIMUM:
            raise wire.ProcessError("Auto-shutdown delay too short")
        if auto_shutdown_delay_ms > storage_device.AUTOSHUTDOWN_DELAY_MAXIMUM:
            raise wire.ProcessError("Auto-shutdown delay too long")
        await require_confirm_change_autoshutdown_delay(ctx, auto_shutdown_delay_ms)
        storage_device.set_autoshutdown_delay_ms(auto_shutdown_delay_ms)

    if msg_safety_checks is not None:
        await require_confirm_safety_checks(ctx, msg_safety_checks)
        safety_checks.apply_setting(msg_safety_checks)

    # if display_rotation is not None:
    #     raise wire.ProcessError("Not support yet")
    #     # await require_confirm_change_display_rotation(ctx, msg.display_rotation)
    #     # storage_device.set_rotation(msg.display_rotation)
    if haptic_feedback is not None:
        await require_confirm_haptic_feedback(ctx, haptic_feedback)
        storage_device.toggle_keyboard_haptic(haptic_feedback)
    if change_brightness is not None:
        from trezor.ui.layouts import request_change_brightness

        await request_change_brightness(ctx)
    # if experimental_features is not None:
    #     raise wire.ProcessError("Not support yet")
    #     # await require_confirm_experimental_features(ctx, experimental_features)
    #     # storage_device.set_experimental_features(experimental_features)

    if language:
        from trezor.langs import langs_keys, langs
        from trezor.lvglui.i18n import i18n_refresh

        if language not in langs_keys:
            raise wire.DataError(
                f"all support ISO_639-1 language keys include {' '.join(langs_keys)})"
            )
        index = langs_keys.index(language)
        language_str = langs[index][1]
        await require_confirm_change_language(ctx, language_str)
        storage_device.set_language(language)
        i18n_refresh(language)

    reload_settings_from_storage()

    storage_device._LABEL_VALUE = None
    updated_label = storage_device.get_label()

    from trezor.lvglui.scrs.homescreen import MainScreen

    if hasattr(MainScreen, "_instance") and MainScreen._instance:
        main_screen = MainScreen._instance
        if (
            hasattr(main_screen, "title")
            and main_screen.title
            and storage_device.is_device_name_display_enabled()
        ):
            main_screen.title.set_text(updated_label)

    from trezor.lvglui.scrs.lockscreen import LockScreen

    _visible, lock_screen = LockScreen.retrieval()
    if lock_screen:
        if (
            hasattr(lock_screen, "title")
            and lock_screen.title
            and storage_device.is_device_name_display_enabled()
        ):
            lock_screen.title.set_text(updated_label)

    return Success(message="Settings applied")


async def require_confirm_change_homescreen(ctx: wire.GenericContext) -> None:
    await confirm_action(
        ctx,
        "set_homescreen",
        "Set homescreen",
        description="Do you really want to change the homescreen image?",
        br_code=ButtonRequestType.ProtectCall,
    )


async def require_confirm_change_label(ctx: wire.GenericContext, label: str) -> None:
    await confirm_action(
        ctx,
        "set_label",
        _(i18n_keys.TITLE__CHANGE_LABEL),
        description=_(i18n_keys.SUBTITLE__SET_LABEL_CHANGE_LABEL),
        description_param=label,
        br_code=ButtonRequestType.ProtectCall,
        anim_dir=2,
        icon=None,
    )


async def require_confirm_change_passphrase(
    ctx: wire.GenericContext, use: bool
) -> None:
    if use:
        description = _(i18n_keys.SUBTITLE__ENABLE_PASSPHRASE)
    else:
        from trezor.crypto import se_thd89
        from apps.common.pin_constants import AttachCommon

        current_space = se_thd89.get_pin_passphrase_space()

        if current_space < AttachCommon.MAX_PASSPHRASE_PIN_NUM:
            description = _(i18n_keys.TITLE__DISABLE_PASSPHRASE_DESC)
        else:
            description = _(i18n_keys.SUBTITLE__DISABLE_PASSPHRASE)
    await confirm_action(
        ctx,
        "set_passphrase",
        _(i18n_keys.TITLE__ENABLE_PASSPHRASE)
        if use
        else _(i18n_keys.TITLE__DISABLE_PASSPHRASE),
        description=description,
        br_code=ButtonRequestType.ProtectCall,
        anim_dir=2,
    )


async def require_confirm_change_passphrase_source(
    ctx: wire.GenericContext, passphrase_always_on_device: bool
) -> None:
    if passphrase_always_on_device:
        description = _(i18n_keys.SUBTITLE__SET_PASSPHRASE_ENABLED_FORCE_ON_DEVICE)
    else:
        description = _(i18n_keys.SUBTITLE__SET_PASSPHRASE_ENABLED_NO_FORCE_ON_DEVICE)
    await confirm_action(
        ctx,
        "set_passphrase_source",
        _(i18n_keys.TITLE__PASSPHRASE_SOURCE),
        description=description,
        br_code=ButtonRequestType.ProtectCall,
    )


async def require_confirm_change_display_rotation(
    ctx: wire.GenericContext, rotation: int
) -> None:
    if rotation == 0:
        label = "north"
    elif rotation == 90:
        label = "east"
    elif rotation == 180:
        label = "south"
    elif rotation == 270:
        label = "west"
    else:
        raise wire.DataError("Unsupported display rotation")
    await confirm_action(
        ctx,
        "set_rotation",
        "Change rotation",
        description="Do you really want to change display rotation to {}?",
        description_param=label,
        br_code=ButtonRequestType.ProtectCall,
    )


async def require_confirm_change_autolock_delay(
    ctx: wire.GenericContext, delay_ms: int
) -> None:
    await confirm_action(
        ctx,
        "set_autolock_delay",
        _(i18n_keys.TITLE__AUTO_LOCK),
        description=_(i18n_keys.SUBTITLE__SET_AUTO_LOCK),
        description_param=format_duration_ms(
            delay_ms, storage_device.AUTOLOCK_DELAY_MAXIMUM
        ),
        icon=None,
        verb=_(i18n_keys.BUTTON__CHANGE),
        br_code=ButtonRequestType.ProtectCall,
        anim_dir=2,
    )


async def require_confirm_change_autoshutdown_delay(
    ctx: wire.GenericContext, delay_ms: int
) -> None:
    await confirm_action(
        ctx,
        "set_autoshutdown_delay",
        _(i18n_keys.TITLE__SET_AUTO_SHUTDOWN),
        description=_(i18n_keys.SUBTITLE__SET_AUTO_SHUTDOWN),
        description_param=format_duration_ms(
            delay_ms, storage_device.AUTOSHUTDOWN_DELAY_MAXIMUM
        ),
        icon=None,
        verb=_(i18n_keys.BUTTON__CHANGE),
        br_code=ButtonRequestType.ProtectCall,
        anim_dir=2,
    )


async def require_confirm_safety_checks(
    ctx: wire.GenericContext, level: SafetyCheckLevel
) -> None:
    if level == SafetyCheckLevel.PromptAlways:
        await confirm_action(
            ctx,
            "set_safety_checks",
            _(i18n_keys.TITLE__DISABLE_SAFETY_CHECKS),
            hold=True,
            verb=_(i18n_keys.BUTTON__SLIDE_TO_DISABLE),
            description=_(i18n_keys.SUBTITLE__SET_SAFETY_CHECKS_TO_PROMPT),
            action="",
            reverse=True,
            larger_vspace=True,
            br_code=ButtonRequestType.ProtectCall,
            anim_dir=2,
            hold_level=2,
            icon=None,
        )
    elif level == SafetyCheckLevel.PromptTemporarily:
        await confirm_action(
            ctx,
            "set_safety_checks",
            _(i18n_keys.TITLE__DISABLE_SAFETY_CHECKS),
            hold=True,
            verb=_(i18n_keys.BUTTON__SLIDE_TO_DISABLE),
            description=_(i18n_keys.SUBTITLE__SET_SAFETY_CHECKS_TO_PROMPT),
            action="",
            reverse=True,
            br_code=ButtonRequestType.ProtectCall,
            anim_dir=2,
            hold_level=1,
            icon=None,
        )
    elif level == SafetyCheckLevel.Strict:
        await confirm_action(
            ctx,
            "set_safety_checks",
            _(i18n_keys.TITLE__ENABLE_SAFETY_CHECKS),
            description=_(i18n_keys.SUBTITLE__ENABLE_SAFETY_CHECKS),
            br_code=ButtonRequestType.ProtectCall,
            anim_dir=2,
        )
    else:
        raise ValueError  # enum value out of range


async def require_confirm_experimental_features(
    ctx: wire.GenericContext, enable: bool
) -> None:
    if enable:
        await confirm_action(
            ctx,
            "set_experimental_features",
            _(i18n_keys.TITLE__EXPERIMENTAL_MODE),
            description=_(i18n_keys.SUBTITLE__ENABLE_EXPERIMENTAL_FEATURES),
            action="",  # Only for development and beta testing!
            reverse=True,
            br_code=ButtonRequestType.ProtectCall,
            anim_dir=2,
        )


async def require_confirm_haptic_feedback(
    ctx: wire.GenericContext, enable: bool
) -> None:
    if enable:
        description = _(i18n_keys.SUBTITLE__OPEN_VIBRATION_HAPTIC)
        confirm_text = _(i18n_keys.BUTTON__OPEN)
    else:
        description = _(i18n_keys.SUBTITLE__CLOSE_VIBRATION_HAPTIC)
        confirm_text = _(i18n_keys.BUTTON__CLOSE)
    await confirm_action(
        ctx,
        "set_haptic_feedback",
        _(i18n_keys.TITLE__VIBRATION_AND_HAPTIC),
        description=description,
        icon=None,
        verb=confirm_text,
        br_code=ButtonRequestType.ProtectCall,
    )


async def require_confirm_change_language(
    ctx: wire.GenericContext, language_str
) -> None:
    await confirm_action(
        ctx,
        "set_language",
        _(i18n_keys.TITLE__SET_LANGUAGE),
        description=_(i18n_keys.SUBTITLE__SET_LANGUAGE),
        description_param=language_str,
        icon=None,
        verb=_(i18n_keys.BUTTON__CHANGE),
        br_code=ButtonRequestType.ProtectCall,
        anim_dir=2,
    )
