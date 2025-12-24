from typing import TYPE_CHECKING

import storage.cache
import storage.device
from trezor import config, loop, protobuf, ui, utils, wire, workflow
from trezor.enums import MessageType, OneKeyRebootType
from trezor.messages import OneKeyReboot, OneKeyInfoReq, OneKeyInfoResp, Success, Failure, UnlockPath

from . import workflow_handlers

if TYPE_CHECKING:
    from trezor.messages import (
        Features,
        StartSession,
        EndSession,
        GetFeatures,
        Cancel,
        LockDevice,
        Ping,
        DoPreauthorized,
        CancelAuthorization,
        SetBusy,
    )


def get_vendor():
    return "trezor.io" if storage.device.is_trezor_compatible() else "onekey.so"


def busy_expiry_ms() -> int:
    """
    Returns the time left until the busy state expires or 0 if the device is not in the busy state.
    """

    busy_deadline_ms = storage.cache.get_int(storage.cache.APP_COMMON_BUSY_DEADLINE_MS)
    if busy_deadline_ms is None:
        return 0

    import utime

    expiry_ms = utime.ticks_diff(busy_deadline_ms, utime.ticks_ms())
    return expiry_ms if expiry_ms > 0 else 0


async def handle_OneKeyReboot(ctx: wire.Context, req: OneKeyReboot) -> Success | Failure :

    if req.reboot_type == OneKeyRebootType.Normal:
            utils.reboot()
            return Success()
    if req.reboot_type == OneKeyRebootType.Boardloader:
            utils.reboot_to_boardloader()
            return Success()
    if req.reboot_type == OneKeyRebootType.BootLoader:
            utils.reboot_to_bootloader()
            return Success()
    
    return Failure("Reboot target incorrect!")
    
def process_OneKeyInfo(req: OneKeyInfoReq) -> OneKeyInfoResp | None:
    from trezor import uart  # for bluetooth
    from trezorio import hwinfo  # for hardware version

    # onekey specific
    from trezor.enums import (
        OneKeyDeviceType,
        OneKeySeType,
        OneKeySEState,
    )

    from trezor.messages import (
        OneKeyFwImgInfo,
        OneKeyMainMcuInfo,
        OneKeyBluetoothInfo,
        OneKeySEInfo,
        OneKeyHardwareInfo,
        # OneKeyInfoTargets,
        OneKeyInfoTypes,
        # OneKeyInfoReq,
        # OneKeyInfoResp,
        OneKeyStatus,
    )

    assert req is not None

    def process_OneKeyStatus(info_type: OneKeyInfoTypes) -> OneKeyStatus:
        assert info_type is not None

        status = OneKeyStatus()

        status.language = storage.device.get_language()
        status.bt_enable = uart.is_ble_opened()
        status.init_states = storage.device.is_initialized()

        if device_is_unlocked():
            status.backup_required = storage.device.no_backup()
            status.passphrase_protection = storage.device.is_passphrase_enabled()

        status.lable = storage.device.get_label()

        return status

    def process_OneKeyHardwareInfo(
        info_type: OneKeyInfoTypes,
    ) -> OneKeyHardwareInfo:
        assert info_type is not None

        hw_info = OneKeyHardwareInfo()

        hw_info.device_type = OneKeyDeviceType.PRO
        hw_info.serial_no = storage.device.get_serial()

        if info_type.version:
            hw_info.hardware_version = hwinfo.ver()
        if info_type.specific:
            hw_info.hardware_version_raw_adc = hwinfo.ver_adc()

        return hw_info

    def process_OneKeyMainMcuInfo(
        info_type: OneKeyInfoTypes,
    ) -> OneKeyMainMcuInfo:
        assert info_type is not None

        fw_info = OneKeyMainMcuInfo()

        fw_info.board = OneKeyFwImgInfo()
        fw_info.boot = OneKeyFwImgInfo()
        fw_info.app = OneKeyFwImgInfo()

        if info_type.version:
            fw_info.board.version = utils.board_version()
            fw_info.boot.version = utils.boot_version()
            fw_info.app.version = utils.ONEKEY_VERSION

        if info_type.build_id:
            fw_info.board.build_id = utils.board_build_id()
            fw_info.boot.build_id = utils.boot_build_id()
            fw_info.app.build_id = (utils.BUILD_ID[-7:]).decode("utf-8")

        if info_type.hash:
            fw_info.board.hash = utils.board_hash()
            fw_info.boot.hash = utils.boot_hash()
            fw_info.app.hash = utils.onekey_firmware_hash()

        # if info_type.specific:
        # N/A

        return fw_info

    def process_OneKeyBluetoothInfo(
        info_type: OneKeyInfoTypes,
    ) -> OneKeyBluetoothInfo:
        # Note:
        # we do not consider the "SE in boot, while device in main firmware" condition
        # because if so the device will stay in bootloader

        assert info_type is not None

        bt_info = OneKeyBluetoothInfo()

        # bt_info.boot = OneKeyFwImgInfo() # not implemented
        bt_info.app = OneKeyFwImgInfo()

        if info_type.version:
            bt_info.app.version = uart.get_ble_version()

        if info_type.build_id:
            bt_info.app.build_id = uart.get_ble_build_id()

        if info_type.hash:
            bt_info.app.hash = uart.get_ble_hash()

        if info_type.specific:
            bt_info.adv_name = uart.get_ble_name()
            bt_info.mac = uart.get_ble_mac()

        return bt_info

    def process_OneKeySEInfo(
        func_boot_get_version,
        func_boot_get_build_id,
        func_boot_get_hash,
        func_app_get_version,
        func_app_get_build_id,
        func_app_get_hash,
        info_type: OneKeyInfoTypes,
    ) -> OneKeySEInfo:
        assert func_boot_get_version is not None
        assert func_boot_get_build_id is not None
        assert func_boot_get_hash is not None
        assert func_app_get_version is not None
        assert func_app_get_build_id is not None
        assert func_app_get_hash is not None
        assert info_type is not None

        se_info = OneKeySEInfo()

        se_info.boot = OneKeyFwImgInfo()
        se_info.app = OneKeyFwImgInfo()

        if info_type.version:
            se_info.boot.version = func_boot_get_version()
            se_info.app.version = func_app_get_version()

        if info_type.build_id:
            se_info.boot.build_id = func_boot_get_build_id()
            se_info.app.build_id = func_app_get_build_id()

        if info_type.hash:
            se_info.boot.hash = func_boot_get_hash()
            se_info.app.hash = func_app_get_hash()

        if info_type.specific:
            se_info.state = OneKeySEState.APP
            se_info.type = OneKeySeType.THD89

        return se_info

    if (req.targets is not None) and (req.types is not None):
        resp = OneKeyInfoResp(protocol_version=1.0)

        if req.targets.status:
            resp.status = process_OneKeyStatus(
                info_type=req.types,
            )

        if req.targets.hw:
            resp.hw = process_OneKeyHardwareInfo(
                info_type=req.types,
            )

        if req.targets.fw:
            resp.fw = process_OneKeyMainMcuInfo(
                info_type=req.types,
            )

        if req.targets.bt:
            resp.bt = process_OneKeyBluetoothInfo(
                info_type=req.types,
            )

        if req.targets.se1:
            resp.se1 = process_OneKeySEInfo(
                func_boot_get_version=storage.device.get_se01_boot_version,
                func_boot_get_build_id=storage.device.get_se01_boot_build_id,
                func_boot_get_hash=storage.device.get_se01_boot_hash,
                func_app_get_version=storage.device.get_se01_version,
                func_app_get_build_id=storage.device.get_se01_build_id,
                func_app_get_hash=storage.device.get_se01_hash,
                info_type=req.types,
            )
        if req.targets.se2:
            resp.se2 = process_OneKeySEInfo(
                func_boot_get_version=storage.device.get_se02_boot_version,
                func_boot_get_build_id=storage.device.get_se02_boot_build_id,
                func_boot_get_hash=storage.device.get_se02_boot_hash,
                func_app_get_version=storage.device.get_se02_version,
                func_app_get_build_id=storage.device.get_se02_build_id,
                func_app_get_hash=storage.device.get_se02_hash,
                info_type=req.types,
            )
        if req.targets.se3:
            resp.se3 = process_OneKeySEInfo(
                func_boot_get_version=storage.device.get_se03_boot_version,
                func_boot_get_build_id=storage.device.get_se03_boot_build_id,
                func_boot_get_hash=storage.device.get_se03_boot_hash,
                func_app_get_version=storage.device.get_se03_version,
                func_app_get_build_id=storage.device.get_se03_build_id,
                func_app_get_hash=storage.device.get_se03_hash,
                info_type=req.types,
            )
        if req.targets.se4:
            resp.se4 = process_OneKeySEInfo(
                func_boot_get_version=storage.device.get_se04_boot_version,
                func_boot_get_build_id=storage.device.get_se04_boot_build_id,
                func_boot_get_hash=storage.device.get_se04_boot_hash,
                func_app_get_version=storage.device.get_se04_version,
                func_app_get_build_id=storage.device.get_se04_build_id,
                func_app_get_hash=storage.device.get_se04_hash,
                info_type=req.types,
            )

        return resp

    else:
        return None


def get_features(msg: "GetFeatures | StartSession | None") -> Features:
    import storage.recovery
    import storage.sd_salt
    import storage  # workaround for https://github.com/microsoft/pyright/issues/2685

    from trezor import sdcard
    from trezor.enums import (
        Capability,
        BackupAvailability,
        RecoveryStatus,
        DisplayRotation,
        OneKeyDeviceType,
    )
    from trezor.messages import Features
    from apps.common import mnemonic, safety_checks

    if (msg is not None) and (msg.ok_dev_info_req is not None):
        # onekey mode
        # Very frequently used status goes to Feature message
        # Less frequently used status goes to OneKeyStatus message, populate on request

        f = Features(
            busy=busy_expiry_ms() > 0,
            unlocked=device_is_unlocked(),
        )

        f.ok_dev_info_resp = process_OneKeyInfo(msg.ok_dev_info_req)

    else:
        # trezor mode
        # This mode is only for compatible with Trezor, all OneKey specific fields won't be here
        # Some data may not be actual value (e.g. versions), it's by design, not a bug

        f = Features(
            vendor=get_vendor(),
            major_version=2,  # fake version
            minor_version=99,  # fake version
            patch_version=99,  # fake version
            # bootloader_mode=False, # bootloader mode only, not needed under firmware
            device_id=storage.device.get_device_id(),
            pin_protection=config.has_pin(),
            # passphrase_protection -- private
            language=storage.device.get_language(),
            label=storage.device.get_label(),
            initialized=storage.device.is_initialized(),
            revision=utils.SCM_REVISION,
            # bootloader_hash # not used under bootloader and firmware
            # imported # not used under bootloader and firmware
            unlocked=config.is_unlocked(),
            # _passphrase_cached # not used under bootloader and firmware
            # firmware_present=True, # bootloader mode only
            # backup_availability -- private
            # flags -- private
            model=utils.MODEL,
            # fw_major=2, # bootloader mode only, fake version
            # fw_minor=99, # bootloader mode only, fake version
            # fw_patch=99, # bootloader mode only, fake version
            # fw_vendor=99, # bootloader mode only, fake version
            # unfinished_backup -- private
            # no_backup -- private
            # recovery_status -- private
            capabilities=[
                Capability.Bitcoin,
                Capability.Bitcoin_like,
                Capability.Binance,
                Capability.Cardano,
                # Capability.Crypto,
                Capability.EOS,
                Capability.Ethereum,
                # Capability.Monero,
                Capability.NEM,
                Capability.Ripple,
                Capability.Stellar,
                Capability.Tezos,
                Capability.PassphraseEntry,
                # Capability.U2F,
                # Capability.Shamir,
                # Capability.ShamirGroups,
                # Capability.Haptic,
            ],
            # backup_type -- private
            sd_card_present=sdcard.is_present(),
            # sd_protection -- private
            # wipe_code_protection -- private
            # session_id # only populate under StartSession message
            # passphrase_always_on_device -- private
            # safety_checks -- private
            # auto_lock_delay_ms -- private
            display_rotation=(
                DisplayRotation.North
            ),  # we don't support rotate, original function storage.device.get_rotation()
            # experimental_features -- private
            busy=busy_expiry_ms() > 0,
            # homescreen_format # used only by Trezor Suite
            # hide_passphrase_from_host -- private
            # internal_model # used only by Trezor Suite
            # unit_color # used only by Trezor Suite
            unit_btconly=False,
            # homescreen_width=480,
            # homescreen_height=800,
            bootloader_locked=True,
            language_version_matches=True,
            # unit_packaging # used only by Trezor Suite
            haptic_feedback=False,  # we have, but won't expose it to host
            # recovery_type # no impl.
            # optiga_sec # no impl.
        )

        # private fields:
        if device_is_unlocked():
            # passphrase_protection is private, see #1807
            f.passphrase_protection = storage.device.is_passphrase_enabled()
            f.backup_availability = (
                BackupAvailability.NotAvailable,
                BackupAvailability.Required,
            )[storage.device.needs_backup()]
            f.flags = storage.device.get_flags()
            f.unfinished_backup = storage.device.unfinished_backup()
            f.no_backup = storage.device.no_backup()
            f.recovery_status = (
                RecoveryStatus.Nothing
            )  # storage.recovery.is_in_progress()
            f.backup_type = mnemonic.get_type()
            f.sd_protection = storage.sd_salt.is_enabled()
            f.wipe_code_protection = config.has_wipe_code()
            f.passphrase_always_on_device = (
                storage.device.get_passphrase_always_on_device()
            )
            f.safety_checks = safety_checks.read_setting()
            f.auto_lock_delay_ms = storage.device.get_autolock_delay_ms()
            f.experimental_features = storage.device.get_experimental_features()
            # f.hide_passphrase_from_host # no impl.

        # legacy compatibility
        f.onekey_device_type = OneKeyDeviceType.PRO
        f.onekey_serial_no = storage.device.get_serial()

    return f


async def handle_StartSession(
    ctx: wire.Context | wire.QRContext, msg: StartSession
) -> Features:
    session_id = storage.cache.start_session(msg.session_id)

    if msg.derive_cardano is not None and msg.derive_cardano:
        # THD89 is not capable of Cardano
        from trezor.crypto import se_thd89

        state = se_thd89.get_session_state()
        if state[0] & 0x80 and not state[0] & 0x40:
            storage.cache.end_current_session()
            session_id = storage.cache.start_session()

        storage.cache.SESSION_DIRIVE_CARDANO = True
    else:
        storage.cache.SESSION_DIRIVE_CARDANO = False

    features = get_features(msg)
    features.session_id = session_id
    storage.cache.update_res_confirm_refresh()
    return features


async def handle_GetFeatures(ctx: wire.Context, msg: GetFeatures) -> Features:
    return get_features(msg)


async def handle_Cancel(ctx: wire.Context, msg: Cancel) -> Success:
    raise wire.ActionCancelled


async def handle_LockDevice(ctx: wire.Context, msg: LockDevice) -> Success:
    lock_device()
    return Success()


async def handle_SetBusy(ctx: wire.Context, msg: SetBusy) -> Success:
    if not storage.device.is_initialized():
        raise wire.NotInitialized("Device is not initialized")

    if msg.expiry_ms:
        import utime

        deadline = utime.ticks_add(utime.ticks_ms(), msg.expiry_ms)
        storage.cache.set_int(storage.cache.APP_COMMON_BUSY_DEADLINE_MS, deadline)
    else:
        storage.cache.delete(storage.cache.APP_COMMON_BUSY_DEADLINE_MS)
    set_homescreen()
    workflow.close_others()
    return Success()


async def handle_EndSession(ctx: wire.Context, msg: EndSession) -> Success:
    storage.cache.end_current_session()
    return Success()


async def handle_Ping(ctx: wire.Context, msg: Ping) -> Success:
    if msg.button_protection:
        from trezor.ui.layouts import confirm_action
        from trezor.enums import ButtonRequestType as B

        await confirm_action(ctx, "ping", "Confirm", "ping", br_code=B.ProtectCall)
    return Success(message=msg.message)


async def handle_DoPreauthorized(
    ctx: wire.Context, msg: DoPreauthorized
) -> protobuf.MessageType:
    from trezor.messages import PreauthorizedRequest
    from apps.common import authorization

    if not authorization.is_set():
        raise wire.ProcessError("No preauthorized operation")

    wire_types = authorization.get_wire_types()
    utils.ensure(bool(wire_types), "Unsupported preauthorization found")

    req = await ctx.call_any(PreauthorizedRequest(), *wire_types)

    assert req.MESSAGE_WIRE_TYPE is not None
    handler = workflow_handlers.find_registered_handler(
        ctx.iface, req.MESSAGE_WIRE_TYPE
    )
    if handler is None:
        return wire.unexpected_message()

    return await handler(ctx, req, authorization.get())  # type: ignore [Expected 2 positional arguments]


async def handle_UnlockPath(ctx: wire.Context, msg: UnlockPath) -> protobuf.MessageType:
    from trezor.crypto import hmac
    from trezor.messages import UnlockedPathRequest
    from trezor.ui.layouts import confirm_action
    from apps.common.paths import SLIP25_PURPOSE
    from apps.common.seed import Slip21Node, get_seed
    from apps.common.writers import write_uint32_le

    _KEYCHAIN_MAC_KEY_PATH = [b"TREZOR", b"Keychain MAC key"]

    # UnlockPath is relevant only for SLIP-25 paths.
    # Note: Currently we only allow unlocking the entire SLIP-25 purpose subtree instead of
    # per-coin or per-account unlocking in order to avoid UI complexity.
    if msg.address_n != [SLIP25_PURPOSE]:
        raise wire.DataError("Invalid path")

    seed = await get_seed(ctx)
    node = Slip21Node(seed)
    node.derive_path(_KEYCHAIN_MAC_KEY_PATH)
    mac = utils.HashWriter(hmac(hmac.SHA256, node.key()))
    for i in msg.address_n:
        write_uint32_le(mac, i)
    expected_mac = mac.get_digest()

    # Require confirmation to access SLIP25 paths unless already authorized.
    if msg.mac:
        if len(msg.mac) != len(expected_mac) or not utils.consteq(
            expected_mac, msg.mac
        ):
            raise wire.DataError("Invalid MAC")
    else:
        await confirm_action(
            ctx,
            "confirm_coinjoin_access",
            title="CoinJoin account",
            description="Do you want to allow access to your CoinJoin account?",
        )

    wire_types = (MessageType.GetAddress, MessageType.GetPublicKey, MessageType.SignTx)
    req = await ctx.call_any(UnlockedPathRequest(mac=expected_mac), *wire_types)

    assert req.MESSAGE_WIRE_TYPE in wire_types
    handler = workflow_handlers.find_registered_handler(
        ctx.iface, req.MESSAGE_WIRE_TYPE
    )
    assert handler is not None
    return await handler(ctx, req, msg)  # type: ignore [Expected 2 positional arguments]


async def handle_CancelAuthorization(
    ctx: wire.Context, msg: CancelAuthorization
) -> protobuf.MessageType:
    from apps.common import authorization

    authorization.clear()
    return Success(message="Authorization cancelled")


ALLOW_WHILE_LOCKED = (
    MessageType.StartSession,
    MessageType.EndSession,
    MessageType.GetFeatures,
    MessageType.Cancel,
    MessageType.LockDevice,
    MessageType.DoPreauthorized,
    MessageType.WipeDevice,
    MessageType.SetBusy,
)


def set_homescreen() -> None:
    import lvgl as lv  # type: ignore[Import "lvgl" could not be resolved]

    from trezor.lvglui.scrs import fingerprints

    ble_name = storage.device.get_ble_name()
    if storage.device.is_initialized():
        dev_state = get_state()
        device_name = storage.device.get_label()
        if not device_is_unlocked():
            if __debug__:
                print(
                    f"Device is locked by pin {not config.is_unlocked()} === fingerprint {not fingerprints.is_unlocked()}"
                )
            from trezor.lvglui.scrs.lockscreen import LockScreen

            screen = LockScreen(device_name, ble_name, dev_state)
        else:
            if __debug__:
                print(
                    f"Device is unlocked and has fingerprint {fingerprints.is_available() and not fingerprints.is_unlocked()}"
                )
            from trezor.lvglui.scrs.homescreen import MainScreen

            store_ble_name(ble_name)
            screen = MainScreen(device_name, ble_name, dev_state)
    else:
        from trezor.lvglui.scrs.initscreen import InitScreen

        InitScreen()
        return
    if not screen.is_visible():
        lv.scr_load(screen)
    lv.refr_now(None)


def store_ble_name(ble_name):
    from trezor import uart

    temp_ble_name = uart.get_ble_name()
    if not ble_name and temp_ble_name:
        storage.device.set_ble_name(temp_ble_name)


def get_state() -> str | None:
    from trezor.lvglui.i18n import gettext as _, keys as i18n_keys

    if storage.device.no_backup():
        dev_state = _(i18n_keys.MSG__SEEDLESS)
    elif storage.device.unfinished_backup():
        dev_state = _(i18n_keys.MSG__BACKUP_FAILED)
    elif storage.device.needs_backup():
        dev_state = _(i18n_keys.MSG__NEEDS_BACKUP)
    elif not config.has_pin():
        dev_state = _(i18n_keys.MSG__PIN_NOT_SET)
    elif storage.device.get_experimental_features():
        dev_state = _(i18n_keys.MSG__EXPERIMENTAL_MODE)
    else:
        dev_state = None
    return dev_state


def lock_device() -> None:
    if storage.device.is_initialized() and config.has_pin():
        from trezor.lvglui.scrs import fingerprints

        if fingerprints.is_available():
            fingerprints.lock()
        else:
            if __debug__:
                print(
                    f"pin locked,  finger is available: {fingerprints.is_available()} ===== finger is unlocked: {fingerprints.is_unlocked()} "
                )
            config.lock()
        wire.find_handler = get_pinlocked_handler
        set_homescreen()
        workflow.close_others()


def device_is_unlocked():
    from trezor.lvglui.scrs import fingerprints

    if fingerprints.is_available():
        return fingerprints.is_unlocked()
    else:
        return config.is_unlocked()


def lock_device_if_unlocked() -> None:
    if config.is_unlocked():
        lock_device()

    loop.schedule(utils.turn_off_lcd())


def screen_off_if_possible() -> None:
    if not ui.display.backlight():
        return

    if ui.display.backlight():
        from trezor import uart

        uart.flashled_close()

        if config.is_unlocked():
            ui.display.backlight(ui.style.BACKLIGHT_LOW)
        workflow.idle_timer.set(3 * 1000, lock_device_if_unlocked)


async def screen_off_delay():
    if not ui.display.backlight():
        return
    from trezor import uart

    uart.flashled_close()
    ui.display.backlight(ui.style.BACKLIGHT_LOW)
    workflow.idle_timer.set(3 * 1000, lock_device_if_unlocked)


def shutdown_device() -> None:
    from trezor import uart

    if storage.device.is_initialized():
        if not utils.CHARGING:
            uart.ctrl_power_off()


async def unlock_device(ctx: wire.GenericContext = wire.DUMMY_CONTEXT) -> None:
    """Ensure the device is in unlocked state.

    If the storage is locked, attempt to unlock it. Reset the homescreen and the wire
    handler.
    """
    from apps.common.request_pin import verify_user_pin, verify_user_fingerprint

    if not config.is_unlocked():
        if __debug__:
            print("pin is locked ")
        # verify_user_pin will raise if the PIN was invalid
        await verify_user_pin(ctx, allow_fingerprint=False)
    else:
        from trezor.lvglui.scrs import fingerprints

        if not fingerprints.is_unlocked():
            if __debug__:
                print("fingerprint is locked")
            verify_pin = verify_user_pin(ctx, close_others=False)
            verify_finger = verify_user_fingerprint(ctx)
            racer = loop.race(verify_pin, verify_finger)
            await racer
            if verify_finger in racer.finished:
                from trezor.lvglui.scrs.pinscreen import InputPin

                pin_wind = InputPin.get_window_if_visible()
                if pin_wind:
                    pin_wind.destroy()
    if storage.device.is_fingerprint_unlock_enabled():
        storage.device.finger_failed_count_reset()

    utils.mark_pin_verified()

    # reset the idle_timer
    reload_settings_from_storage()
    set_homescreen()
    wire.find_handler = workflow_handlers.find_registered_handler


# async def auth(ctx: wire.GenericContext = wire.DUMMY_CONTEXT) -> None:

#     from apps.common.request_pin import verify_user_pin, verify_user_fingerprint

#     verify_pin = verify_user_pin(ctx, close_others=False)
#     verify_finger = verify_user_fingerprint(ctx)
#     racer = loop.race(verify_pin, verify_finger)
#     await racer
#     if verify_finger in racer.finished:
#         from trezor.lvglui.scrs.pinscreen import InputPin

#         pin_wind = InputPin.get_window_if_visible()
#         if pin_wind:
#             pin_wind.destroy()


def get_pinlocked_handler(
    iface: wire.WireInterface, msg_type: int
) -> wire.Handler[wire.Msg] | None:
    orig_handler = workflow_handlers.find_registered_handler(iface, msg_type)
    if orig_handler is None:
        return None

    if __debug__:
        import usb

        if iface is usb.iface_debug:
            return orig_handler

    if msg_type in ALLOW_WHILE_LOCKED:
        return orig_handler

    async def wrapper(ctx: wire.Context, msg: wire.Msg) -> protobuf.MessageType:
        await unlock_device(ctx)
        return await orig_handler(ctx, msg)

    return wrapper


# this function is also called when handling ApplySettings
def reload_settings_from_storage(timeout_ms: int | None = None) -> None:
    workflow.idle_timer.remove(lock_device_if_unlocked)
    if not storage.device.is_initialized():
        return
    workflow.idle_timer.set(
        (
            timeout_ms
            if timeout_ms is not None
            else storage.device.get_autolock_delay_ms()
        ),
        screen_off_if_possible,
    )
    if utils.AUTO_POWER_OFF:
        workflow.idle_timer.set(
            storage.device.get_autoshutdown_delay_ms(), shutdown_device
        )
    else:
        workflow.idle_timer.remove(shutdown_device)
    wire.experimental_enabled = storage.device.get_experimental_features()
    ui.display.orientation(storage.device.get_rotation())


def boot() -> None:
    workflow_handlers.register(MessageType.OneKeyReboot, handle_OneKeyReboot)
    workflow_handlers.register(MessageType.StartSession, handle_StartSession)
    workflow_handlers.register(MessageType.GetFeatures, handle_GetFeatures)
    workflow_handlers.register(MessageType.Cancel, handle_Cancel)
    workflow_handlers.register(MessageType.LockDevice, handle_LockDevice)
    workflow_handlers.register(MessageType.EndSession, handle_EndSession)
    workflow_handlers.register(MessageType.Ping, handle_Ping)
    workflow_handlers.register(MessageType.DoPreauthorized, handle_DoPreauthorized)
    workflow_handlers.register(MessageType.UnlockPath, handle_UnlockPath)
    workflow_handlers.register(
        MessageType.CancelAuthorization, handle_CancelAuthorization
    )
    workflow_handlers.register(MessageType.SetBusy, handle_SetBusy)

    reload_settings_from_storage()
    from trezor.lvglui.scrs import fingerprints

    if __debug__:
        print(f"fingerprints.is_unlocked(): {fingerprints.is_unlocked()}")
        print(f"config.is_unlocked(): {config.is_unlocked()}")
    if config.is_unlocked() and fingerprints.is_unlocked():
        if __debug__:
            print("fingerprints is unlocked and config is unlocked")
        wire.find_handler = workflow_handlers.find_registered_handler
    else:
        if __debug__:
            print("fingerprints is locked or config is locked")
        wire.find_handler = get_pinlocked_handler
