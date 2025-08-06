from typing import TYPE_CHECKING

from storage.device import is_initialized
from trezor import config, wire
from trezor.crypto import se_thd89
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.lvglui.lv_colors import lv_colors
from trezor.lvglui.scrs.pinscreen import request_change_passphrase_pin
from trezor.messages import Success
from trezor.ui.layouts import confirm_action, show_success

from apps.common.request_pin import (
    error_pin_invalid,
    error_pin_matches_wipe_code,
    error_pin_used,
    passphrase_pin_used,
    request_pin_and_sd_salt,
    request_pin_confirm,
)

PIN_CANCEL = 0
PIN_OVERWRITE = 1

if TYPE_CHECKING:
    from typing import Awaitable

    from trezor.messages import ChangePin


async def change_pin(ctx: wire.Context, msg: ChangePin) -> Success:
    if not is_initialized():
        raise wire.NotInitialized("Device is not initialized")
    await require_confirm_change_pin(ctx, msg)
    curpin, salt = await request_pin_and_sd_salt(
        ctx,
        _(i18n_keys.TITLE__ENTER_OLD_PIN),
        allow_fingerprint=False,
    )
    from apps.common.pin_constants import PinType, PinResult

    if curpin and not msg.remove:
        verified, usertype = config.check_pin(
            curpin, salt, PinType.USER_AND_PASSPHRASE_PIN_CHECK
        )
        if not verified:
            await error_pin_invalid(ctx)
        if usertype == PinResult.USER_PIN_ENTERED:
            if not msg.remove:
                newpin = await request_pin_confirm(
                    ctx, show_tip=(not bool(curpin)), allow_fingerprint=False
                )
            else:
                newpin = ""
            is_current = False
            remove_result = False
            if newpin:
                verified, usertype = config.check_pin(
                    newpin, salt, PinType.PASSPHRASE_PIN_CHECK
                )
                if usertype == PinResult.PASSPHRASE_PIN_ENTERED:
                    result = await passphrase_pin_used(ctx)
                    if result == PIN_CANCEL:
                        return Success(message="Operation cancelled")
                    elif result == PIN_OVERWRITE:
                        passphrase_pin_str = (
                            str(newpin) if not isinstance(newpin, str) else newpin
                        )
                        (
                            remove_result,
                            is_current,
                        ) = se_thd89.delete_pin_passphrase(passphrase_pin_str)
            # write into storage
            if not config.change_pin(curpin, newpin, salt, salt):
                if newpin:
                    await error_pin_matches_wipe_code(ctx)
                else:
                    await error_pin_invalid(ctx)
            if remove_result and is_current:
                verified, usertype = config.check_pin(newpin, salt, PinType.USER)
                if usertype == PinResult.USER_PIN_ENTERED:
                    import storage.device

                    storage.device.set_passphrase_pin_enabled(False)

            if newpin:
                if curpin:
                    msg_screen = _(i18n_keys.SUBTITLE__SET_PIN_PIN_CHANGED)
                    msg_wire = _(i18n_keys.TITLE__PIN_CHANGED)
                else:
                    msg_screen = _(i18n_keys.SUBTITLE__SETUP_SET_PIN_PIN_ENABLED)
                    msg_wire = _(i18n_keys.TITLE__PIN_ENABLED)
            else:
                msg_screen = _(i18n_keys.SUBTITLE__SET_PIN_PIN_DISABLED)
                msg_wire = _(i18n_keys.TITLE__PIN_DISABLED)

            await show_success(
                ctx,
                "success_pin",
                msg_screen,
                header=msg_wire,
                button=_(i18n_keys.BUTTON__DONE),
            )
            return Success(message=msg_wire)
        elif usertype == PinResult.PASSPHRASE_PIN_ENTERED:
            # get new pin
            if not msg.remove:
                newpin = await request_change_passphrase_pin(ctx)
            else:
                newpin = ""
            if newpin:
                verified, usertype = config.check_pin(
                    newpin, salt, PinType.USER_AND_PASSPHRASE_PIN_CHECK
                )
                if usertype == PinResult.USER_PIN_ENTERED:
                    return await error_pin_used(ctx)
                if usertype == PinResult.PASSPHRASE_PIN_ENTERED:
                    if newpin == curpin:
                        await show_success(
                            ctx,
                            "success_pin",
                            _(i18n_keys.SUBTITLE__SET_PIN_PIN_CHANGED),
                            header=_(i18n_keys.TITLE__PIN_CHANGED),
                            button=_(i18n_keys.BUTTON__DONE),
                        )
                        return Success(message=_(i18n_keys.TITLE__PIN_CHANGED))
                    result = await passphrase_pin_used(ctx)
                    if result == PIN_CANCEL:
                        return Success(message="Operation cancelled")
                    elif result == PIN_OVERWRITE:
                        # Define messages for passphrase PIN change
                        if curpin and not msg.remove:
                            msg_screen = _(i18n_keys.SUBTITLE__SET_PIN_PIN_CHANGED)
                            msg_wire = _(i18n_keys.TITLE__PIN_CHANGED)
                        elif not curpin and not msg.remove:
                            msg_screen = _(
                                i18n_keys.SUBTITLE__SETUP_SET_PIN_PIN_ENABLED
                            )
                            msg_wire = _(i18n_keys.TITLE__PIN_ENABLED)
                        else:
                            msg_screen = _(i18n_keys.SUBTITLE__SET_PIN_PIN_DISABLED)
                            msg_wire = _(i18n_keys.TITLE__PIN_DISABLED)

                        result = se_thd89.change_pin_passphrase(curpin, newpin)

                        await show_success(
                            ctx,
                            "success_pin",
                            msg_screen,
                            header=msg_wire,
                            button=_(i18n_keys.BUTTON__DONE),
                        )
                        return Success(message=msg_wire)
                else:
                    if curpin and not msg.remove:
                        msg_screen = _(i18n_keys.SUBTITLE__SET_PIN_PIN_CHANGED)
                        msg_wire = _(i18n_keys.TITLE__PIN_CHANGED)
                    elif not curpin and not msg.remove:
                        msg_screen = _(i18n_keys.SUBTITLE__SETUP_SET_PIN_PIN_ENABLED)
                        msg_wire = _(i18n_keys.TITLE__PIN_ENABLED)
                    else:
                        msg_screen = _(i18n_keys.SUBTITLE__SET_PIN_PIN_DISABLED)
                        msg_wire = _(i18n_keys.TITLE__PIN_DISABLED)

                    result = se_thd89.change_pin_passphrase(curpin, newpin)
                    if result:
                        await show_success(
                            ctx,
                            "success_pin",
                            msg_screen,
                            header=msg_wire,
                            button=_(i18n_keys.BUTTON__DONE),
                        )
                        return Success(message=msg_wire)
                    else:
                        return await error_pin_used(ctx)

    return Success(message="PIN operation completed")


def require_confirm_change_pin(ctx: wire.Context, msg: ChangePin) -> Awaitable[None]:
    has_pin = config.has_pin()

    if msg.remove and has_pin:  # removing pin
        return confirm_action(
            ctx,
            "set_pin",
            _(i18n_keys.TITLE__DISABLE_PIN_PROTECTION),
            description=_(i18n_keys.SUBTITLE__DISABLE_PIN_PROTECTION),
            action="",
            anim_dir=2,
            verb=_(i18n_keys.BUTTON__REMOVE),
            primary_color=lv_colors.ONEKEY_YELLOW,
        )

    if not msg.remove:  # changing pin

        return confirm_action(
            ctx,
            "set_pin",
            _(i18n_keys.TITLE__CHANGE_PIN),
            description=_(i18n_keys.SUBTITLE__SETUP_CREATE_ENABLE_PIN_PROTECTION),
            action="",
            anim_dir=2,
            primary_color=lv_colors.ONEKEY_YELLOW,
        )
    # if not msg.remove and has_pin:  # changing pin
    #     return confirm_action(
    #         ctx,
    #         "set_pin",
    #         _(i18n_keys.TITLE__CHANGE_PIN),
    #         description=_(i18n_keys.SUBTITLE__SET_PIN_CHANGE_PIN),
    #         action="",
    #         reverse=True,
    #         anim_dir=2,
    #     )

    # if not msg.remove and not has_pin:  # setting new pin
    #     return confirm_action(
    #         ctx,
    #         "set_pin",
    #         _(i18n_keys.TITLE__ENABLED_PIN),
    #         description=_(i18n_keys.SUBTITLE__SET_PIN_ENABLE_PIN),
    #         action="",
    #         reverse=True,
    #         anim_dir=2,
    #     )

    # removing non-existing PIN
    raise wire.ProcessError("PIN protection already disabled")
