from typing import TYPE_CHECKING

import storage.device as storage_device
from trezor import config, loop, utils, wire, workflow
from trezor.enums import BackupType

if TYPE_CHECKING:
    from trezor.lvglui.scrs.device_transfer import TransferProgress

_active = False
_mnemonic: bytearray | None = None
_LISTEN_MS = 60_000
_TURN_MS = 30_000
_MNEMONIC_MAX_LEN = 240
_RESULT_NO_READER = 4


def _wipe(buf: bytearray) -> None:
    for i in range(len(buf)):
        buf[i] = 0


def abandon() -> None:
    """Release what a transfer holds when loop.clear() dropped its task.

    The cleanup in device_transfer() never runs then, so this closes the
    exporter, wipes the phrase and resumes normal wire state. The next
    session sets wire handling up again.
    """
    global _active, _mnemonic
    if not _active:
        return
    from trezorio import nfc

    nfc.p2p_export_close()
    if _mnemonic is not None:
        _wipe(_mnemonic)
        _mnemonic = None
    utils.set_backup_none()
    utils.mark_initialization_done()
    _active = False


def failure_kind(result: int, frames_in: int) -> str:
    if result == _RESULT_NO_READER:
        return "timeout"
    if frames_in:
        return "interrupted"
    return "connection"


async def run_session(screen: "TransferProgress", mnemonic: bytearray) -> str:
    from trezorio import nfc

    if config.get_mnemonic_type() != 0:
        raise wire.ProcessError("Only a BIP39 seed can be transferred")
    if not config.is_unlocked():
        raise wire.ProcessError("Device is locked")
    if not 1 <= len(mnemonic) <= _MNEMONIC_MAX_LEN:
        raise wire.ProcessError("Verified mnemonic required")
    try:
        screen.show_ready()
        result = nfc.p2p_export_start(mnemonic, _LISTEN_MS, _TURN_MS)
        if result:
            return failure_kind(result, 0)
        # The C adapter keeps its own copy of the verified text and wipes it after
        # the last envelope, on failure and on close.
        ticks = 0
        while True:
            if screen.cancelled:
                return "cancelled"
            result, frames_in, _frames_out, _export_sw, _state = nfc.p2p_export_poll()
            if frames_in:
                screen.show_holding()
            if result != -1:
                return "success" if result == 0 else failure_kind(result, frames_in)
            if ticks % 200 == 0:
                workflow.idle_timer.touch()
            ticks += 1
            await loop.sleep(5)
    finally:
        nfc.p2p_export_close()


async def device_transfer(ctx: wire.GenericContext) -> None:
    global _active, _mnemonic
    if _active:
        return
    if not storage_device.is_initialized():
        raise wire.NotInitialized("Device is not initialized")
    if storage_device.is_airgap_mode():
        raise wire.ProcessError("NFC is disabled in air gap mode")
    if storage_device.get_backup_type() != BackupType.Bip39:
        raise wire.ProcessError("Only a BIP39 seed can be transferred")

    from trezor.lvglui.scrs.device_transfer import (
        TransferIntroduction,
        TransferProgress,
        TransferResult,
    )
    from trezor.messages import RecoveryDevice
    from apps.management.recovery_device import recovery_device

    _active = True
    screen = None
    verified_mnemonic = bytearray()
    _mnemonic = verified_mnemonic
    # Wire handling stays suspended for the whole flow. A host message would
    # end in loop.clear(), which drops this task without running the cleanup
    # below and leaves the exporter and NFC running.
    utils.mark_initialization_processing()
    utils.play_dead()
    try:
        screen = TransferIntroduction()
        if not await ctx.wait(screen.request()):
            return
        screen.destroy()
        screen = None
        utils.set_backup_none()
        await recovery_device(
            ctx,
            RecoveryDevice(dry_run=True, enforce_wordlist=True),
            on_verified=verified_mnemonic.extend,
        )
        while True:
            screen = TransferProgress()
            try:
                outcome = await run_session(screen, verified_mnemonic)
            except Exception:  # noqa: BLE001 - present a retryable failure
                outcome = "interrupted" if screen.holding else "connection"
            screen.destroy()
            screen = None
            if outcome == "cancelled":
                return
            if outcome == "success":
                # No retry is possible from the success page; discard the
                # entered phrase before waiting for the acknowledgement tap.
                _wipe(verified_mnemonic)
            screen = TransferResult(outcome)
            retry = await ctx.wait(screen.request())
            screen.destroy()
            screen = None
            if outcome == "success" or not retry:
                return
    except (wire.ActionCancelled, wire.PinCancelled):
        return
    except wire.ProcessError:
        # The existing recovery flow has already shown the mismatch result.
        return
    finally:
        _wipe(verified_mnemonic)
        _mnemonic = None
        if screen is not None:
            screen.destroy()
        utils.set_backup_none()
        utils.mark_initialization_done()
        utils.set_up()
        _active = False
