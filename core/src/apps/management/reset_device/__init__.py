from typing import TYPE_CHECKING, Sequence

import storage
import storage.device as storage_device
from trezor import wire
from trezor.crypto import bip39, random, slip39
from trezor.enums import BackupType
from trezor.lvglui.i18n import i18n_refresh
from trezor.messages import EntropyAck, EntropyRequest, Success

from apps.common import backup_types

from . import layout

if __debug__:
    import storage.debug

if TYPE_CHECKING:
    from trezor.messages import ResetDevice

B39 = BackupType.Bip39
S39_B = BackupType.Slip39_Basic
S39_A = BackupType.Slip39_Advanced
S39_SE = BackupType.Slip39_Single_Extendable
S39_BE = BackupType.Slip39_Basic_Extendable
S39_AE = BackupType.Slip39_Advanced_Extendable
_DEFAULT_BACKUP_TYPE = B39


async def reset_device(
    ctx: wire.GenericContext, msg: ResetDevice, use_multiple_entropy: bool = False
) -> Success:
    # validate parameters and device state
    _validate_reset_device(msg)

    if msg.language is not None:
        storage_device.set_language(msg.language)
        i18n_refresh()

    int_entropy = random.bytes(32)
    if __debug__:
        storage.debug.reset_internal_entropy = int_entropy
    if msg.display_random:
        await layout.show_internal_entropy(ctx, int_entropy)

    # request external entropy and compute the master secret
    if use_multiple_entropy:
        # Use MCU random number generator (source=0) for external entropy
        ext_entropy = random.bytes(32, 0)
    else:
        # Request external entropy from host
        entropy_ack = await ctx.call(EntropyRequest(), EntropyAck)
        ext_entropy = entropy_ack.entropy if entropy_ack else b""
    # If either of skip_backup or no_backup is specified, we are not doing backup now.
    # Otherwise, we try to do it.
    perform_backup = not msg.no_backup and not msg.skip_backup
    # If doing backup, ask the user to confirm.
    # if perform_backup:
    #     perform_backup = await confirm_backup(ctx)

    backup_type = msg.backup_type or _DEFAULT_BACKUP_TYPE
    if backup_type == S39_B:
        backup_type = S39_SE
    elif backup_type == S39_A:
        backup_type = S39_AE

    secret = _compute_secret_from_entropy(int_entropy, ext_entropy, msg.strength)

    # Check backup type, perform type-specific handling
    if backup_type == B39:
        # in BIP-39 we store mnemonic string instead of the secret
        secret = bip39.from_data(secret).encode()
    elif backup_types.is_slip39_backup_type(backup_type):
        # only not extendable backup type need to generate identifier
        # storage_device.set_slip39_identifier(slip39.generate_random_identifier())
        storage_device.set_slip39_iteration_exponent(slip39.DEFAULT_ITERATION_EXPONENT)
    else:
        # Unknown backup type.
        raise RuntimeError

    # generate and display backup information for the master secret
    if perform_backup:
        await backup_seed(ctx, backup_type, secret)
        await layout.show_backup_success(ctx)

    # write settings and master secret into storage
    if msg.label is not None:
        storage_device.set_label(msg.label)

    storage_device.set_passphrase_enabled(bool(msg.passphrase_protection))
    storage_device.store_mnemonic_secret(
        secret,  # for SLIP-39, this is the EMS
        backup_type,
        needs_backup=not perform_backup,
        no_backup=bool(msg.no_backup),
        identifier=storage_device.get_slip39_identifier(),
        iteration_exponent=storage_device.get_slip39_iteration_exponent(),
    )
    return Success(message="Initialized")


async def _backup_slip39_single(
    ctx: wire.GenericContext,
    encrypted_master_secret: bytes,
    extendable: bool,
    skip_backup_warning: bool = False,
) -> None:
    mnemonics = _get_slip39_mnemonics(encrypted_master_secret, 1, ((1, 1),), extendable)

    # for a single 1-of-1 group, we use the same layouts as for BIP39
    # await layout.show_backup_intro(single_share=True, num_of_words=len(words))
    await layout.bip39_show_and_confirm_mnemonic(
        ctx, mnemonics[0][0], skip_backup_warning
    )


async def _backup_slip39_basic(
    ctx: wire.GenericContext,
    encrypted_master_secret: bytes,
    num_of_words: int,
    extendable: bool,
    skip_backup_warning: bool = False,
) -> None:
    group_threshold = 1

    # await layout.show_backup_intro(single_share=False)

    # get number of shares
    # await layout.slip39_show_checklist(0, advanced=False)
    from trezor.lvglui.scrs.reset_device import Slip39BasicConfig

    src = Slip39BasicConfig(navigable=False)
    # share_count = await layout.slip39_prompt_number_of_shares(ctx, num_of_words)
    share_count, share_threshold = await src.request()
    # get threshold
    # await layout.slip39_show_checklist(1, advanced=False, count=share_count)
    # share_threshold = await layout.slip39_prompt_threshold(share_count)

    mnemonics = _get_slip39_mnemonics(
        encrypted_master_secret,
        group_threshold,
        ((share_threshold, share_count),),
        extendable,
    )

    # show and confirm individual shares
    # await layout.slip39_show_checklist(
    #     2, advanced=False, count=share_count, threshold=share_threshold
    # )
    await layout.slip39_basic_show_and_confirm_shares(
        ctx, mnemonics[0], skip_backup_warning
    )


# region
# async def _backup_slip39_advanced(
#     ctx: wire.Context,
#     encrypted_master_secret: bytes,
#     num_of_words: int,
#     extendable: bool,
# ) -> None:
#     # await layout.show_backup_intro(single_share=False)

#     # get number of groups
#     # await layout.slip39_show_checklist(0, advanced=True)
#     groups_count = await layout.slip39_advanced_prompt_number_of_groups(ctx)

#     # get group threshold
#     # await layout.slip39_show_checklist(1, advanced=True, count=groups_count)
#     group_threshold = await layout.slip39_advanced_prompt_group_threshold(
#         ctx, groups_count
#     )

#     # get shares and thresholds
#     # await layout.slip39_show_checklist(
#     #     2, advanced=True, count=groups_count, threshold=group_threshold
#     # )
#     groups = []
#     for i in range(groups_count):
#         share_count = await layout.slip39_prompt_number_of_shares(num_of_words, i)
#         share_threshold = await layout.slip39_prompt_threshold(share_count, i)
#         groups.append((share_threshold, share_count))

#     mnemonics = _get_slip39_mnemonics(
#         encrypted_master_secret, group_threshold, groups, extendable
#     )

#     # show and confirm individual shares
#     await layout.slip39_advanced_show_and_confirm_shares(mnemonics)
# endregion

# region
# async def backup_slip39_custom(
#     encrypted_master_secret: bytes,
#     group_threshold: int,
#     groups: Sequence[tuple[int, int]],
#     extendable: bool,
# ) -> None:
#     # show and confirm individual shares
#     if len(groups) == 1 and groups[0][0] == 1 and groups[0][1] == 1:
#         await _backup_slip39_single(encrypted_master_secret, extendable)
#     else:
#         mnemonics = _get_slip39_mnemonics(
#             encrypted_master_secret, group_threshold, groups, extendable
#         )
#         await confirm_action(
#             "warning_shamir_backup",
#             TR.reset__title_shamir_backup,
#             description=TR.reset__create_x_of_y_multi_share_backup_template.format(
#                 groups[0][0], groups[0][1]
#             ),
#             verb=TR.buttons__continue,
#         )
#         if len(groups) == 1:
#             await layout.slip39_basic_show_and_confirm_shares(mnemonics[0])
#         else:
#             await layout.slip39_advanced_show_and_confirm_shares(mnemonics)
# endregion


def _get_slip39_mnemonics(
    encrypted_master_secret: bytes,
    group_threshold: int,
    groups: Sequence[tuple[int, int]],
    extendable: bool,
) -> list[list[str]]:
    if extendable:
        identifier = slip39.generate_random_identifier()
    else:
        identifier = storage_device.get_slip39_identifier()

    iteration_exponent = storage_device.get_slip39_iteration_exponent()
    if identifier is None or iteration_exponent is None:
        raise ValueError

    # generate the mnemonics
    return slip39.split_ems(
        group_threshold,
        groups,
        identifier,
        extendable,
        iteration_exponent,
        encrypted_master_secret,
    )


def _validate_reset_device(msg: ResetDevice) -> None:
    from trezor.wire import UnexpectedMessage

    backup_type = msg.backup_type or _DEFAULT_BACKUP_TYPE
    if backup_types.is_slip39_backup_type(backup_type):
        if msg.strength not in (128, 256):
            raise wire.ProcessError("Invalid strength (has to be 128 or 256 bits)")
    elif backup_type == B39:  # BIP-39
        if msg.strength not in (128, 192, 256):
            raise wire.ProcessError("Invalid strength (has to be 128, 192 or 256 bits)")
    else:
        raise wire.ProcessError("Backup type not implemented.")

    if storage_device.is_initialized():
        raise UnexpectedMessage("Already initialized")


def _compute_secret_from_entropy(
    int_entropy: bytes, ext_entropy: bytes, strength_bits: int
) -> bytes:
    from trezor.crypto import hashlib

    # combine internal and external entropy
    ehash = hashlib.sha256()
    ehash.update(int_entropy)
    ehash.update(ext_entropy)
    entropy = ehash.digest()
    # take a required number of bytes
    strength = strength_bits // 8
    secret = entropy[:strength]
    return secret


async def backup_seed(
    ctx: wire.GenericContext,
    backup_type: BackupType,
    mnemonic_secret: bytes,
    skip_backup_warning: bool = False,
) -> None:
    if backup_types.is_slip39_backup_type(backup_type):
        num_of_words = backup_types.get_num_of_words_per_share(
            backup_type, len(mnemonic_secret)
        )
        extendable = backup_types.is_extendable_backup_type(backup_type)
        if backup_types.is_slip39_advanced_backup_type(backup_type):
            # return await _backup_slip39_advanced(
            #     ctx, mnemonic_secret, num_of_words, extendable
            # )
            pass
        elif backup_type == S39_SE:
            await _backup_slip39_single(
                ctx, mnemonic_secret, extendable, skip_backup_warning
            )
        else:
            if __debug__:
                print(
                    f"backup_type: {backup_type}, num_of_words: {num_of_words}, extendable: {extendable}"
                )
            await _backup_slip39_basic(
                ctx, mnemonic_secret, num_of_words, extendable, skip_backup_warning
            )
    else:
        await layout.bip39_show_and_confirm_mnemonic(
            ctx, mnemonic_secret.decode(), skip_backup_warning
        )
