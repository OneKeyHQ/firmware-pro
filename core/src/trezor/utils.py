# pyright: off
import gc
import sys
from typing import TYPE_CHECKING

from trezorutils import (  # noqa: F401; FIRMWARE_SECTORS_COUNT,; firmware_sector_size,; get_firmware_chunk,
    BITCOIN_ONLY,
    BUILD_ID,
    EMULATOR,
    LVGL_UI,
    MODEL,
    ONEKEY_VERSION,
    SCM_REVISION,
    USE_THD89,
    VERSION_MAJOR,
    VERSION_MINOR,
    VERSION_PATCH,
    BW_STANDARD,
    BW_URL,
    BW_MINIMAL,
    consteq,
    firmware_hash,
    firmware_vendor,
    halt,
    memcpy,
    reboot2boardloader,
    reboot_to_bootloader,
    reset,
    usb_data_connected,
    board_hash,
    board_build_id,
    boot_hash,
    boot_build_id,
    se_version,
    se_hash,
    se_build_id,
    se_boot_version,
    se_boot_hash,
    se_boot_build_id,
    onekey_firmware_hash,
    get_tick,
    bytewords_decode,
    enter_lowpower,
)

if not EMULATOR:
    from trezorutils import boot_version, board_version
else:

    def boot_version() -> str:
        return "emulator-1.2.0"

    def board_version() -> str:
        return "emulator-1.0.0"


# pyright: on

BLE_NAME: str | None = None
BLE_BUILD_ID: str | None = None
BLE_HASH: bytes | None = None
DISABLE_ANIMATION = 0
BLE_CONNECTED: bool | None = None
BATTERY_CAP: int | None = None
SHORT_AUTO_LOCK_TIME_MS = 20 * 1000
DEFAULT_LABEL = "OneKey Pro"
AUTO_POWER_OFF = False
_SHOW_APP_GUIDE = False
_INITIALIZATION_PROCESSING = False
_COLLECTING_FINGERPRINT = False
_PIN_VERIFIED_SINCE_BOOT = False
FLASH_LED_BRIGHTNESS: int | None = None
_BACKUP_WITH_LITE_FIRST = False
_COLOR_FLAG: str | None = None

if __debug__:
    MAX_FP_ATTEMPTS = 50
else:
    MAX_FP_ATTEMPTS = 5

if __debug__:
    if EMULATOR:
        import uos

        DISABLE_ANIMATION = int(uos.getenv("TREZOR_DISABLE_ANIMATION") or "0")
        LOG_MEMORY = int(uos.getenv("TREZOR_LOG_MEMORY") or "0")
    else:
        LOG_MEMORY = 0

if TYPE_CHECKING:
    from trezor.protobuf import MessageType
    from typing import Any, Iterator, Protocol, Sequence, TypeVar

SCREENS = []

SPI_IFACE_NUM = 6


def set_up() -> None:
    from trezor import wire, io
    import usb

    # initialize the wire codec
    wire.setup(usb.iface_wire)
    if __debug__:
        wire.setup(usb.iface_debug, is_debug_session=True)
    # interface used for trezor wire protocol

    wire.setup(
        io.SPI(
            SPI_IFACE_NUM,
        )
    )


def clear_screens() -> None:
    for scr in SCREENS:
        try:
            scr.del_delayed(500)
            del scr.__class__._instance
            del scr
        except BaseException:
            pass
    SCREENS.clear()


def try_remove_scr(screen):
    try:
        SCREENS.remove(screen)
    except Exception:
        pass


def pin_verified_since_boot() -> bool:
    return _PIN_VERIFIED_SINCE_BOOT


def mark_pin_verified() -> None:
    global _PIN_VERIFIED_SINCE_BOOT
    if not _PIN_VERIFIED_SINCE_BOOT:
        _PIN_VERIFIED_SINCE_BOOT = True


def turn_on_lcd_if_possible() -> bool:
    resumed = lcd_resume()
    if resumed:
        from trezor import workflow, uart

        workflow.spawn(uart.handle_fingerprint())
    return resumed


def lcd_resume() -> bool:
    from trezor.ui import display
    from storage import device
    from apps import base
    from trezor import config
    from trezor.lvglui.scrs.charging import ChargingPromptScr

    if ChargingPromptScr.has_instance():
        ChargingPromptScr.get_instance().destroy()

    if display.backlight() != device.get_brightness():
        global AUTO_POWER_OFF
        display.backlight(device.get_brightness())
        AUTO_POWER_OFF = False
        base.reload_settings_from_storage(
            timeout_ms=SHORT_AUTO_LOCK_TIME_MS if not config.is_unlocked() else None
        )
        return True
    return False


async def internal_reloop():
    from trezor import loop

    loop.clear()


async def turn_off_lcd():
    from trezor.ui import display
    from trezor import loop, wire

    if display.backlight():
        global AUTO_POWER_OFF
        display.backlight(0)
        AUTO_POWER_OFF = True
    await wire.signal_ack()
    loop.clear()


def play_dead():
    from trezor import loop
    import usb

    loop.pop_tasks_on_iface(usb.iface_wire.iface_num())
    loop.pop_tasks_on_iface(SPI_IFACE_NUM)


def disable_airgap_mode():
    from storage import device
    from trezor import uart

    device.enable_airgap_mode(False)
    uart.ctrl_ble(enable=True)
    import usb

    if usb.bus.state() == 0:
        usb.bus = usb.init()
        for iface in usb.active_iface:
            usb.bus.add(iface)
        usb.bus.open(device.get_device_id())


def enable_airgap_mode():
    from storage import device
    from trezor import uart

    device.enable_airgap_mode(True)
    uart.ctrl_ble(enable=False)
    import usb

    if usb.bus.state() == 1:
        usb.bus.close()


def show_app_guide():
    global _SHOW_APP_GUIDE
    if _SHOW_APP_GUIDE:
        _SHOW_APP_GUIDE = False
        return True
    return False


def make_show_app_guide():
    global _SHOW_APP_GUIDE
    _SHOW_APP_GUIDE = True


def mark_initialization_processing():
    global _INITIALIZATION_PROCESSING
    _INITIALIZATION_PROCESSING = True


def is_initialization_processing():
    return _INITIALIZATION_PROCESSING


def mark_initialization_done():
    global _INITIALIZATION_PROCESSING
    _INITIALIZATION_PROCESSING = False


def mark_collecting_fingerprint():
    global _COLLECTING_FINGERPRINT
    _COLLECTING_FINGERPRINT = True


def is_collecting_fingerprint():
    return _COLLECTING_FINGERPRINT


def mark_collecting_fingerprint_done():
    global _COLLECTING_FINGERPRINT
    _COLLECTING_FINGERPRINT = False


def mark_backup_with_lite_1st():
    global _BACKUP_WITH_LITE_FIRST
    _BACKUP_WITH_LITE_FIRST = True


def mark_backup_with_keytag_1st():
    global _BACKUP_WITH_LITE_FIRST
    _BACKUP_WITH_LITE_FIRST = False


def is_backup_with_lite_1st():
    return _BACKUP_WITH_LITE_FIRST


def get_default_wallpaper():
    global _COLOR_FLAG
    if _COLOR_FLAG is None:
        import storage

        serial = storage.device.get_serial()
        color_flag = serial[-1]
        _COLOR_FLAG = color_flag
    if _COLOR_FLAG == "A":  # black shell
        return "A:/res/wallpaper-1.jpg"
    elif _COLOR_FLAG == "B":  # white shell
        return "A:/res/wallpaper-2.jpg"
    else:
        return "A:/res/wallpaper-1.jpg"


def unimport_begin() -> set[str]:
    return set(sys.modules)


def unimport_end(mods: set[str], collect: bool = True) -> None:
    # static check that the size of sys.modules never grows above value of
    # MICROPY_LOADED_MODULES_DICT_SIZE, so that the sys.modules dict is never
    # reallocated at run-time
    assert len(sys.modules) <= 160, "Please bump preallocated size in mpconfigport.h"
    for mod in sys.modules:  # pylint: disable=consider-using-dict-items
        if mod not in mods:
            # remove reference from sys.modules
            del sys.modules[mod]
            # remove reference from the parent module
            i = mod.rfind(".")
            if i < 0:
                continue
            path = mod[:i]
            name = mod[i + 1 :]
            try:
                delattr(sys.modules[path], name)
            except KeyError:
                # either path is not present in sys.modules, or module is not
                # referenced from the parent package. both is fine.
                pass

    # collect removed modules
    if collect:
        gc.collect()


class unimport:
    def __init__(self) -> None:
        self.mods: set[str] | None = None

    def __enter__(self) -> None:
        self.mods = unimport_begin()

    def __exit__(self, _exc_type: Any, _exc_value: Any, _tb: Any) -> None:
        assert self.mods is not None
        unimport_end(self.mods, collect=False)
        clear_screens()
        self.mods.clear()
        self.mods = None
        gc.collect()


def presize_module(modname: str, size: int) -> None:
    """Ensure the module's dict is preallocated to an expected size.

    This is used in modules like `trezor`, whose dict size depends not only on the
    symbols defined in the file itself, but also on the number of submodules that will
    be inserted into the module's namespace.
    """
    module = sys.modules[modname]
    for i in range(size):
        setattr(module, f"___PRESIZE_MODULE_{i}", None)
    for i in range(size):
        delattr(module, f"___PRESIZE_MODULE_{i}")


if __debug__:

    def mem_dump(filename: str) -> None:
        from micropython import mem_info

        print(f"### sysmodules ({len(sys.modules)}):")
        for mod in sys.modules:
            print("*", mod)
        if EMULATOR:
            from trezorutils import meminfo

            print("### dumping to", filename)
            meminfo(filename)
            mem_info()
        else:
            mem_info(True)


def ensure(cond: bool, msg: str | None = None) -> None:
    if not cond:
        if msg is None:
            raise AssertionError
        else:
            raise AssertionError(msg)


if TYPE_CHECKING:
    Chunkable = TypeVar("Chunkable", str, Sequence[Any])


def chunks(items: Chunkable, size: int) -> Iterator[Chunkable]:
    for i in range(0, len(items), size):
        yield items[i : i + size]


def chunks_intersperse(items: str, size: int, sep: str = "\n") -> Iterator[str]:
    first = True
    for i in range(0, len(items), size):
        if not first:
            yield sep
        else:
            first = False
        yield items[i : i + size]


if TYPE_CHECKING:

    class HashContext(Protocol):
        def update(self, __buf: bytes) -> None:
            ...

        def digest(self) -> bytes:
            ...

    class HashContextInitable(HashContext, Protocol):
        def __init__(  # pylint: disable=super-init-not-called
            self, __data: bytes | None = None
        ) -> None:
            ...

    class Writer(Protocol):
        def append(self, __b: int) -> None:
            ...

        def extend(self, __buf: bytes) -> None:
            ...


class HashWriter:
    def __init__(self, ctx: HashContext) -> None:
        self.ctx = ctx
        self.buf = bytearray(1)  # used in append()

    def append(self, b: int) -> None:
        self.buf[0] = b
        self.ctx.update(self.buf)

    def extend(self, buf: bytes) -> None:
        self.ctx.update(buf)

    def write(self, buf: bytes) -> None:  # alias for extend()
        self.ctx.update(buf)

    def get_digest(self) -> bytes:
        return self.ctx.digest()


if TYPE_CHECKING:
    BufferType = bytearray | memoryview


class BufferWriter:
    """Seekable and writeable view into a buffer."""

    def __init__(self, buffer: BufferType) -> None:
        self.buffer = buffer
        self.offset = 0

    def seek(self, offset: int) -> None:
        """Set current offset to `offset`.

        If negative, set to zero. If longer than the buffer, set to end of buffer.
        """
        offset = min(offset, len(self.buffer))
        offset = max(offset, 0)
        self.offset = offset

    def write(self, src: bytes) -> int:
        """Write exactly `len(src)` bytes into buffer, or raise EOFError.

        Returns number of bytes written.
        """
        buffer = self.buffer
        offset = self.offset
        if len(src) > len(buffer) - offset:
            raise EOFError
        nwrite = memcpy(buffer, offset, src, 0)
        self.offset += nwrite
        return nwrite


class BufferReader:
    """Seekable and readable view into a buffer."""

    def __init__(self, buffer: bytes | memoryview) -> None:
        if isinstance(buffer, memoryview):
            self.buffer = buffer
        else:
            self.buffer = memoryview(buffer)
        self.offset = 0

    def seek(self, offset: int) -> None:
        """Set current offset to `offset`.

        If negative, set to zero. If longer than the buffer, set to end of buffer.
        """
        offset = min(offset, len(self.buffer))
        offset = max(offset, 0)
        self.offset = offset

    def readinto(self, dst: BufferType) -> int:
        """Read exactly `len(dst)` bytes into `dst`, or raise EOFError.

        Returns number of bytes read.
        """
        buffer = self.buffer
        offset = self.offset
        if len(dst) > len(buffer) - offset:
            raise EOFError
        nread = memcpy(dst, 0, buffer, offset)
        self.offset += nread
        return nread

    def read(self, length: int | None = None) -> bytes:
        """Read and return exactly `length` bytes, or raise EOFError.

        If `length` is unspecified, reads all remaining data.

        Note that this method makes a copy of the data. To avoid allocation, use
        `readinto()`. To avoid copying use `read_memoryview()`.
        """
        return bytes(self.read_memoryview(length))

    def read_memoryview(self, length: int | None = None) -> memoryview:
        """Read and return a memoryview of exactly `length` bytes, or raise
        EOFError.

        If `length` is unspecified, reads all remaining data.
        """
        if length is None:
            ret = self.buffer[self.offset :]
            self.offset = len(self.buffer)
        elif length < 0:
            raise ValueError
        elif length <= self.remaining_count():
            ret = self.buffer[self.offset : self.offset + length]
            self.offset += length
        else:
            raise EOFError
        return ret

    def remaining_count(self) -> int:
        """Return the number of bytes remaining for reading."""
        return len(self.buffer) - self.offset

    def peek(self) -> int:
        """Peek the ordinal value of the next byte to be read."""
        if self.offset >= len(self.buffer):
            raise EOFError
        return self.buffer[self.offset]

    def get(self) -> int:
        """Read exactly one byte and return its ordinal value."""
        if self.offset >= len(self.buffer):
            raise EOFError
        byte = self.buffer[self.offset]
        self.offset += 1
        return byte


def obj_eq(self: Any, __o: Any) -> bool:
    """
    Compares object contents.
    """
    if self.__class__ is not __o.__class__:
        return False
    assert not hasattr(self, "__slots__")
    return self.__dict__ == __o.__dict__


def obj_repr(self: Any) -> str:
    """
    Returns a string representation of object.
    """
    assert not hasattr(self, "__slots__")
    return f"<{self.__class__.__name__}: {self.__dict__}>"


def truncate_utf8(string: str, max_bytes: int) -> str:
    """Truncate the codepoints of a string so that its UTF-8 encoding is at most `max_bytes` in length."""
    data = string.encode()
    if len(data) <= max_bytes:
        return string

    # Find the starting position of the last codepoint in data[0 : max_bytes + 1].
    i = max_bytes
    while i >= 0 and data[i] & 0xC0 == 0x80:
        i -= 1

    return data[:i].decode()


def is_empty_iterator(i: Iterator) -> bool:
    try:
        next(i)
    except StopIteration:
        return True
    else:
        return False


def empty_bytearray(preallocate: int) -> bytearray:
    """
    Returns bytearray that won't allocate for at least `preallocate` bytes.
    Useful in case you want to avoid allocating too often.
    """
    b = bytearray(preallocate)
    b[:] = bytes()
    return b


if __debug__:

    def dump_protobuf_lines(msg: MessageType, line_start: str = "") -> Iterator[str]:
        msg_dict = msg.__dict__
        if not msg_dict:
            yield line_start + msg.MESSAGE_NAME + " {}"
            return

        yield line_start + msg.MESSAGE_NAME + " {"
        for key, val in msg_dict.items():
            if type(val) == type(msg):
                sublines = dump_protobuf_lines(val, line_start=key + ": ")
                for subline in sublines:
                    yield "    " + subline
            elif val and isinstance(val, list) and type(val[0]) == type(msg):
                # non-empty list of protobuf messages
                yield f"    {key}: ["
                for subval in val:
                    sublines = dump_protobuf_lines(subval)
                    for subline in sublines:
                        yield "        " + subline
                yield "    ]"
            else:
                yield f"    {key}: {repr(val)}"

        yield "}"

    def dump_protobuf(msg: MessageType) -> str:
        return "\n".join(dump_protobuf_lines(msg))
