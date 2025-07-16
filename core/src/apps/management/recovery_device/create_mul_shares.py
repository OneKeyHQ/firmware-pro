from trezor import utils
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.lvglui.scrs.common import FullSizeWindow
from trezor.wire import DUMMY_CONTEXT


async def create_multi_share_backup() -> None:
    """
    Create a multi-share backup.
    """
    import storage.device as storage_device
    import storage.recovery as storage_recovery
    from apps.common import backup_types

    if not backup_types.is_extendable_backup_type(storage_device.get_backup_type()):
        raise RuntimeError

    from trezor.lvglui.scrs.reset_device import (
        CreateMultiShareBackup,
        CreateMultiShareBackupTips,
    )

    scr = CreateMultiShareBackup()
    result = await scr.request()
    if not result:
        return

    scr = CreateMultiShareBackupTips()
    result = await scr.request()
    if not result:
        return

    mods = utils.unimport_begin()

    from apps.management.recovery_device import recovery_device
    from trezor.messages import RecoveryDevice

    storage_recovery.set_extend_share()
    try:
        # pyright: off
        await recovery_device(
            DUMMY_CONTEXT,
            RecoveryDevice(dry_run=True, enforce_wordlist=True),
        )
        # pyright: on
    except BaseException:
        storage_recovery.clear_slip39_extend_share_state()
        return
    finally:
        utils.unimport_end(mods)

    scr = FullSizeWindow(
        title=_(i18n_keys.TITLE__CREATE_ADDITIONAL_BACKUP),
        subtitle=_(i18n_keys.TITLE__CREATE_ADDITIONAL_BACKUP_DESC),
        confirm_text=_(i18n_keys.BUTTON__CONTINUE),
        cancel_text=_(i18n_keys.BUTTON__CANCEL),
    )
    if not await scr.request():
        return

    try:
        await backup_multi_share(storage_recovery.get_slip39_checked_secret())
    except BaseException as e:
        if __debug__:
            import sys

            sys.print_exception(e)  # type: ignore["print_exception" is not a known member of module]
    finally:
        storage_recovery.clear_slip39_extend_share_state()


async def backup_multi_share(encrypted_master_secret: bytes) -> None:
    from apps.management.reset_device import _get_slip39_mnemonics
    from trezor.lvglui.scrs.reset_device import Slip39BasicConfig
    from trezor.wire import ActionCancelled
    from trezor.ui.layouts import confirm_action
    from trezor.enums import ButtonRequestType
    from trezor.lvglui.lv_colors import lv_colors

    while True:
        src = Slip39BasicConfig(navigable=False, min_num=1)
        result = await src.request()
        share_count, share_threshold = result
        if share_count == 0 and share_threshold == 0:
            title = _(i18n_keys.TITLE__ABORT_RECOVERY_PHRASE_CREATION)
            subtitle = _(i18n_keys.SUBTITLE__ABORT_PROCESSING)
            try:
                await confirm_action(
                    DUMMY_CONTEXT,
                    "abort_create_multi_share",
                    title,
                    description=subtitle,
                    icon=None,
                    br_code=ButtonRequestType.ProtectCall,
                    anim_dir=0,
                    primary_color=lv_colors.ONEKEY_YELLOW,
                )
            except ActionCancelled:
                continue
            else:
                raise ActionCancelled()

        mnemonics = _get_slip39_mnemonics(
            encrypted_master_secret,
            1,
            ((share_threshold, share_count),),
            True,
        )
        try:
            await show_and_confirm_shares(mnemonics[0])
        except ActionCancelled:
            continue
        else:
            break


async def show_and_confirm_shares(shares: list[str]) -> None:
    from trezor.ui.layouts.lvgl.reset import show_share_words
    from apps.management.reset_device.layout import (
        show_check_word_tips,
        _confirm_share_words,
        _show_confirmation_success,
        _show_confirmation_failure,
        show_backup_success,
    )
    from trezor.wire import ActionCancelled

    share_count = len(shares)
    i = 0
    while i < share_count:
        mods = utils.unimport_begin()
        share_words = shares[i].split(" ")
        while True:
            # display paginated share on the screen
            result = await show_share_words(
                DUMMY_CONTEXT,
                share_words,
                i,
                share_count=share_count,
                show_indicator=False,
            )
            if not result:
                if i >= 1:
                    i -= 1
                    continue
                else:
                    raise ActionCancelled()
            # description before check word
            try:
                await show_check_word_tips(DUMMY_CONTEXT)
            except ActionCancelled:
                continue
            # make the user confirm words from the share
            if await _confirm_share_words(DUMMY_CONTEXT, i, share_words):
                await _show_confirmation_success(
                    DUMMY_CONTEXT, share_index=i, num_of_shares=len(shares)
                )
                i += 1
                break  # this share is confirmed, go to next one
            else:
                await _show_confirmation_failure(DUMMY_CONTEXT, i)
        utils.unimport_end(mods)
    await show_backup_success(DUMMY_CONTEXT)
