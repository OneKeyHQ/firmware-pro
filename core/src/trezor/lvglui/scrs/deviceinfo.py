import binascii

from storage import device
from trezor import uart, utils


class DeviceInfoManager:
    _instance = None
    preloaded_info = {}

    def preload_device_info(self):
        version = device.get_firmware_version()
        build_id = utils.BUILD_ID[-7:].decode()
        firmware_hash = utils.onekey_firmware_hash()
        hex_hash = binascii.hexlify(firmware_hash).decode("ascii")
        short_hash = hex_hash[:7]
        version = f"{version} [{build_id}-{short_hash}]"

        ble_name = device.get_ble_name() or uart.get_ble_name()
        ble_version = uart.get_ble_version()
        ble_build = uart.get_ble_build_id()
        ble_hash = uart.get_ble_hash()
        ble_hex_hash = binascii.hexlify(ble_hash).decode("ascii")
        ble_short_hash = ble_hex_hash[:7]
        ble_version = f"{ble_version} [{ble_build}-{ble_short_hash}]"

        boot_version = utils.boot_version()
        boot_hash = utils.boot_hash()
        boot_hex_hash = binascii.hexlify(boot_hash).decode("ascii")
        boot_short_hash = boot_hex_hash[:7]
        boot_build_id = utils.boot_build_id()
        boot_version = f"{boot_version} [{boot_build_id}-{boot_short_hash}]"

        onekey_se01_version = device.get_se01_version()
        onekey_se01_hash = device.get_se01_hash()
        onekey_se01_hex_hash = binascii.hexlify(onekey_se01_hash).decode("ascii")
        onekey_se01_short_hash = onekey_se01_hex_hash[:7]
        onekey_se01_build_id = device.get_se01_build_id()
        onekey_se01_version = (
            f"{onekey_se01_version} [{onekey_se01_build_id}-{onekey_se01_short_hash}]"
        )

        onekey_se02_version = device.get_se02_version()
        onekey_se02_hash = device.get_se02_hash()
        onekey_se02_hex_hash = binascii.hexlify(onekey_se02_hash).decode("ascii")
        onekey_se02_short_hash = onekey_se02_hex_hash[:7]
        onekey_se02_build_id = device.get_se02_build_id()
        onekey_se02_version = (
            f"{onekey_se02_version} [{onekey_se02_build_id}-{onekey_se02_short_hash}]"
        )

        onekey_se03_version = device.get_se03_version()
        onekey_se03_hash = device.get_se03_hash()
        onekey_se03_hex_hash = binascii.hexlify(onekey_se03_hash).decode("ascii")
        onekey_se03_short_hash = onekey_se03_hex_hash[:7]
        onekey_se03_build_id = device.get_se03_build_id()
        onekey_se03_version = (
            f"{onekey_se03_version} [{onekey_se03_build_id}-{onekey_se03_short_hash}]"
        )

        onekey_se04_version = device.get_se04_version()
        onekey_se04_hash = device.get_se04_hash()
        onekey_se04_hex_hash = binascii.hexlify(onekey_se04_hash).decode("ascii")
        onekey_se04_short_hash = onekey_se04_hex_hash[:7]
        onekey_se04_build_id = device.get_se04_build_id()
        onekey_se04_version = (
            f"{onekey_se04_version} [{onekey_se04_build_id}-{onekey_se04_short_hash}]"
        )

        onekey_se01_boot_version = device.get_se01_boot_version()
        onekey_se01_boot_hash = device.get_se01_boot_hash()
        onekey_se01_boot_hex_hash = binascii.hexlify(onekey_se01_boot_hash).decode(
            "ascii"
        )
        onekey_se01_boot_short_hash = onekey_se01_boot_hex_hash[:7]
        onekey_se01_boot_build_id = device.get_se01_boot_build_id()
        onekey_se01_boot_version = f"{onekey_se01_boot_version} [{onekey_se01_boot_build_id}-{onekey_se01_boot_short_hash}]"

        onekey_se02_boot_version = device.get_se02_boot_version()
        onekey_se02_boot_hash = device.get_se02_boot_hash()
        onekey_se02_boot_hex_hash = binascii.hexlify(onekey_se02_boot_hash).decode(
            "ascii"
        )
        onekey_se02_boot_short_hash = onekey_se02_boot_hex_hash[:7]
        onekey_se02_boot_build_id = device.get_se02_boot_build_id()
        onekey_se02_boot_version = f"{onekey_se02_boot_version} [{onekey_se02_boot_build_id}-{onekey_se02_boot_short_hash}]"

        onekey_se03_boot_version = device.get_se03_boot_version()
        onekey_se03_boot_hash = device.get_se03_boot_hash()
        onekey_se03_boot_hex_hash = binascii.hexlify(onekey_se03_boot_hash).decode(
            "ascii"
        )
        onekey_se03_boot_short_hash = onekey_se03_boot_hex_hash[:7]
        onekey_se03_boot_build_id = device.get_se03_boot_build_id()
        onekey_se03_boot_version = f"{onekey_se03_boot_version} [{onekey_se03_boot_build_id}-{onekey_se03_boot_short_hash}]"

        onekey_se04_boot_version = device.get_se04_boot_version()
        onekey_se04_boot_hash = device.get_se04_boot_hash()
        onekey_se04_boot_hex_hash = binascii.hexlify(onekey_se04_boot_hash).decode(
            "ascii"
        )
        onekey_se04_boot_short_hash = onekey_se04_boot_hex_hash[:7]
        onekey_se04_boot_build_id = device.get_se04_boot_build_id()
        onekey_se04_boot_version = f"{onekey_se04_boot_version} [{onekey_se04_boot_build_id}-{onekey_se04_boot_short_hash}]"
        serial = device.get_serial()
        model = device.get_model()
        board_version = utils.board_version()

        self.preloaded_info = {
            "version": version,
            "ble_name": ble_name,
            "ble_version": ble_version,
            "boot_version": boot_version,
            "onekey_se01_version": onekey_se01_version,
            "onekey_se02_version": onekey_se02_version,
            "onekey_se03_version": onekey_se03_version,
            "onekey_se04_version": onekey_se04_version,
            "onekey_se01_boot_version": onekey_se01_boot_version,
            "onekey_se02_boot_version": onekey_se02_boot_version,
            "onekey_se03_boot_version": onekey_se03_boot_version,
            "onekey_se04_boot_version": onekey_se04_boot_version,
            "board_version": board_version,
            "serial": serial,
            "model": model,
        }

    @classmethod
    def instance(cls):
        if cls._instance is None:
            cls._instance = cls()
            cls._instance.preload_device_info()
        return cls._instance

    def get_info(self):

        return self.preloaded_info
