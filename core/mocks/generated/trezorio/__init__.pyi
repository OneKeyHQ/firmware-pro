from typing import *


# extmod/modtrezorio/modtrezorio-ble.h
class BLE:
    """
    """

    def __init__(
        self,
    ) -> None:
        """
        """

    def ctrl(self, cmd: byte, value: bytes) -> None:
        """
        Send command to the BLE.
        """


# extmod/modtrezorio/modtrezorio-display.h
class Display:
    """
    Provide access to device display.
    """
    WIDTH: int  # display width in pixels
    HEIGHT: int  # display height in pixels
    FONT_MONO: int  # id of monospace font
    FONT_NORMAL: int  # id of normal-width font
    FONT_BOLD: int  # id of bold-width font

    def __init__(self) -> None:
        """
        Initialize the display.
        """

    def clear(self) -> None:
        """
        Clear display with black color.
        """

    def refresh(self) -> None:
        """
        Refresh display (update screen).
        """

    def bar(self, x: int, y: int, w: int, h: int, color: int) -> None:
        """
        Renders a bar at position (x,y = upper left corner) with width w and
        height h of color color.
        """

    def bar_radius(
        self,
        x: int,
        y: int,
        w: int,
        h: int,
        fgcolor: int,
        bgcolor: int | None = None,
        radius: int | None = None,
    ) -> None:
        """
        Renders a rounded bar at position (x,y = upper left corner) with width w
        and height h of color fgcolor. Background is set to bgcolor and corners
        are drawn with radius radius.
        """

    def toif_info(self, image: bytes) -> tuple[int, int, bool]:
        """
        Returns tuple containing TOIF image dimensions: width, height, and
        whether it is grayscale.
        Raises an exception for corrupted images.
        """

    def image(self, x: int, y: int, image: bytes) -> None:
        """
        Renders an image at position (x,y).
        The image needs to be in Trezor Optimized Image Format (TOIF) -
        full-color mode.
        """

    def avatar(
        self, x: int, y: int, image: bytes, fgcolor: int, bgcolor: int
    ) -> None:
        """
        Renders an avatar at position (x,y).
        The image needs to be in Trezor Optimized Image Format (TOIF) -
        full-color mode. Image needs to be of exactly AVATAR_IMAGE_SIZE x
        AVATAR_IMAGE_SIZE pixels size.
        """

    def icon(
        self, x: int, y: int, icon: bytes, fgcolor: int, bgcolor: int
    ) -> None:
        """
        Renders an icon at position (x,y), fgcolor is used as foreground color,
        bgcolor as background. The icon needs to be in Trezor Optimized Image
        Format (TOIF) - gray-scale mode.
        """

    def loader(
        self,
        progress: int,
        indeterminate: bool,
        yoffset: int,
        fgcolor: int,
        bgcolor: int,
        icon: bytes | None = None,
        iconfgcolor: int | None = None,
    ) -> None:
        """
        Renders a rotating loader graphic.
        Progress determines its position (0-1000), fgcolor is used as foreground
        color, bgcolor as background. When icon and iconfgcolor are provided, an
        icon is drawn in the middle using the color specified in iconfgcolor.
        Icon needs to be of exactly LOADER_ICON_SIZE x LOADER_ICON_SIZE pixels
        size.
        """

    def print(self, text: str) -> None:
        """
        Renders text using 5x8 bitmap font (using special text mode).
        """

    def text(
        self,
        x: int,
        y: int,
        text: str,
        font: int,
        fgcolor: int,
        bgcolor: int,
        text_offset: int | None = None,
        text_len: int | None = None,
    ) -> None:
        """
        Renders left-aligned text at position (x,y) where x is left position and
        y is baseline. Font font is used for rendering, fgcolor is used as
        foreground color, bgcolor as background.
        Arguments text_offset and text_len can be used to render a substring of
        the text.
        """

    def text_center(
        self,
        x: int,
        y: int,
        text: str,
        font: int,
        fgcolor: int,
        bgcolor: int,
    ) -> None:
        """
        Renders text centered at position (x,y) where x is text center and y is
        baseline. Font font is used for rendering, fgcolor is used as foreground
        color, bgcolor as background.
        """

    def text_right(
        self,
        x: int,
        y: int,
        text: str,
        font: int,
        fgcolor: int,
        bgcolor: int,
    ) -> None:
        """
        Renders right-aligned text at position (x,y) where x is right position
        and y is baseline. Font font is used for rendering, fgcolor is used as
        foreground color, bgcolor as background.
        """

    def text_width(
        self,
        text: str,
        font: int,
        text_offset: int | None = None,
        text_len: int | None = None,
    ) -> int:
        """
        Returns a width of text in pixels. Font font is used for rendering.
        Arguments text_offset and text_len can be used to render a substring of
        the text.
        """

    def text_split(self, text: str, font: int, requested_width: int) -> int:
        """
        Returns how many characters of the string can be used before exceeding
        the requested width. Tries to avoid breaking words if possible. Font
        font is used for rendering.
        """

    def qrcode(self, x: int, y: int, data: bytes, scale: int) -> None:
        """
        Renders data encoded as a QR code centered at position (x,y).
        Scale determines a zoom factor.
        """

    def orientation(self, degrees: int | None = None) -> int:
        """
        Sets display orientation to 0, 90, 180 or 270 degrees.
        Everything needs to be redrawn again when this function is used.
        Call without the degrees parameter to just perform the read of the
        value.
        """

    def backlight(self, val: int | None = None) -> int:
        """
        Sets backlight intensity to the value specified in val.
        Call without the val parameter to just perform the read of the value.
        """

    def offset(self, xy: tuple[int, int] | None = None) -> tuple[int, int]:
        """
        Sets offset (x, y) for all subsequent drawing calls.
        Call without the xy parameter to just perform the read of the value.
        """

    def save(self, prefix: str) -> None:
        """
        Saves current display contents to PNG file with given prefix.
        """

    def clear_save(self) -> None:
        """
        Clears buffers in display saving.
        """

    def cover_background_show(self) -> None:
        """
        Show hardware CoverBackground layer.
        """

    def cover_background_hide(self) -> None:
        """
        Hide hardware CoverBackground layer.
        """

    def cover_background_set_visible(self, visible: bool) -> None:
        """
        Set hardware CoverBackground layer visibility state.
        """

    def cover_background_set_image(self, image_data: bytes) -> None:
        """
        Set hardware CoverBackground layer image from raw image data.
        """

    def cover_background_load_jpeg(self, jpeg_path: str) -> None:

    def cover_background_move_to_y(self, y_position: int) -> None:
        """
        Move hardware CoverBackground layer to Y position.
        """

    def cover_background_reload_statusbar_from_jpeg(self, jpeg_path: str) ->
    None:
        """
        Reload statusbar area from JPEG file.
        """

    def cover_background_set_statusbar_opacity(self, transparent: bool) -> None:
        """
        Set the opacity of the top 44px statusbar area in CoverBackground layer.
        """

    def cover_background_animate_to_y(self, target_y: int, duration_ms: int) ->
    None:
        """
        Animate hardware CoverBackground layer to Y position.
        """

    def cover_background_is_visible(self) -> bool:
        """
        Check if hardware CoverBackground layer is visible.
        """

    def homescreen_start_slide(self, target_y: int, duration_ms: int) -> None:
        """
        Start LVGL wallpaper slide animation to target Y position.
        """

    def homescreen_set_wallpaper_obj(self, img_obj: int) -> None:
        """
        Set LVGL wallpaper object pointer for animation.
        """

    def homescreen_is_sliding(self) -> bool:
        """
        Check if LVGL wallpaper slide animation is active.
        """

    def homescreen_stop_slide(self) -> None:
        """
        Stop LVGL wallpaper slide animation.
        """


# extmod/modtrezorio/modtrezorio-flash.h
class FlashOTP:
    """
    """

    def __init__(self) -> None:
        """
        """

    def write(self, block: int, offset: int, data: bytes) -> None:
        """
        Writes data to OTP flash
        """

    def read(self, block: int, offset: int, data: bytearray) -> None:
        """
        Reads data from OTP flash
        """

    def lock(self, block: int) -> None:
        """
        Lock OTP flash block
        """

    def is_locked(self, block: int) -> bool:
        """
        Is OTP flash block locked?
        """


# extmod/modtrezorio/modtrezorio-hid.h
class HID:
    """
    USB HID interface configuration.
    """

    def __init__(
        self,
        iface_num: int,
        ep_in: int,
        ep_out: int,
        emu_port: int,
        report_desc: bytes,
        subclass: int = 0,
        protocol: int = 0,
        polling_interval: int = 1,
        max_packet_len: int = 64,
    ) -> None:
        """
        """

    def iface_num(self) -> int:
        """
        Returns the configured number of this interface.
        """

    def write(self, msg: bytes) -> int:
        """
        Sends message using USB HID (device) or UDP (emulator).
        """

    def write_blocking(self, msg: bytes, timeout_ms: int) -> int:
        """
        Sends message using USB HID (device) or UDP (emulator).
        """


# extmod/modtrezorio/modtrezorio-local.h
class LOCAL_CTL:
    """
    """

    def __init__(
        self,
    ) -> None:
        """
        """

    def ctrl(self, ready: bool) -> None:
        """
        Send command to the LOCAL.
        """


# extmod/modtrezorio/modtrezorio-motor.h
class MOTOR:
    """
    """

    def __init__(
        self,
    ) -> None:
        """
        """

    def reset(self) -> None:
        """
        Reset motor and stop any on going vibrate
        """

    def play_whisper(self) -> None:
        """
        Play builtin whisper pattern
        """

    def play_light(self) -> None:
        """
        Play builtin light pattern
        """

    def play_medium(self) -> None:
        """
        Play builtin medium pattern
        """

    def play_heavy(self) -> None:
        """
        Play builtin heavy pattern
        """

    def play_success(self) -> None:
        """
        Play builtin success sequence
        """

    def play_warning(self) -> None:
        """
        Play builtin warning sequence
        """

    def play_error(self) -> None:
        """
        Play builtin error sequence
        """


# extmod/modtrezorio/modtrezorio-poll.h
def poll(ifaces: Iterable[int], list_ref: list, timeout_ms: int) -> bool:
    """
    Wait until one of `ifaces` is ready to read or write (using masks
    `list_ref`:
    `list_ref[0]` - the interface number, including the mask
    `list_ref[1]` - for touch event, tuple of:
                    (event_type, x_position, y_position)
                  - for button event (T1), tuple of:
                    (event type, button number)
                  - for USB read event, received bytes
    If timeout occurs, False is returned, True otherwise.
    """


# extmod/modtrezorio/modtrezorio-sbu.h
class SBU:
    """
    """

    def __init__(self) -> None:
        """
        """

    def set(self, sbu1: bool, sbu2: bool) -> None:
        """
        Sets SBU wires to sbu1 and sbu2 values respectively
        """


# extmod/modtrezorio/modtrezorio-spi.h
class SPI:
    """
    """

    def __init__(
        self,
        iface_num: int,
    ) -> None:
        """
        """

    def iface_num(self) -> int:
        """
        Returns the configured number of this interface.
        """

    def write(self, msg: bytes) -> int:
        """
        Sends message using SPI.
        """


# extmod/modtrezorio/modtrezorio-usb.h
class USB:
    """
    USB device configuration.
    """

    def __init__(
        self,
        vendor_id: int,
        product_id: int,
        release_num: int,
        device_class: int = 0,
        device_subclass: int = 0,
        device_protocol: int = 0,
        manufacturer: str = "",
        product: str = "",
        interface: str = "",
        usb21_enabled: bool = True,
        usb21_landing: bool = True,
    ) -> None:
        """
        """

    def add(self, iface: HID | VCP | WebUSB) -> None:
        """
        Registers passed interface into the USB stack.
        """

    def open(self, serial_number: str) -> None:
        """
        Initializes the USB stack.
        """

    def close(self) -> None:
        """
        Cleans up the USB stack.
        """

    def connected(self) -> bool:
        """
        Get USB connect state.
        """

    def connect_ctrl(self, state :bool) -> None:
        """
        Control usb connect.
        """

    def state(self) -> int:
      """
      Get USB state.
      """


# extmod/modtrezorio/modtrezorio-vcp.h
class VCP:
    """
    USB VCP interface configuration.
    """

    def __init__(
        self,
        iface_num: int,
        data_iface_num: int,
        ep_in: int,
        ep_out: int,
        ep_cmd: int,
        emu_port: int,
    ) -> None:
        """
        """

    def iface_num(self) -> int:
        """
        Returns the configured number of this interface.
        """


# extmod/modtrezorio/modtrezorio-webusb.h
class WebUSB:
    """
    USB WebUSB interface configuration.
    """

    def __init__(
        self,
        iface_num: int,
        ep_in: int,
        ep_out: int,
        emu_port: int,
        subclass: int = 0,
        protocol: int = 0,
        polling_interval: int = 1,
        max_packet_len: int = 64,
    ) -> None:
        """
        """

    def iface_num(self) -> int:
        """
        Returns the configured number of this interface.
        """

    def write(self, msg: bytes) -> int:
        """
        Sends message using USB WebUSB (device) or UDP (emulator).
        """
from . import fatfs, sdcard
POLL_READ: int  # wait until interface is readable and return read data
POLL_WRITE: int  # wait until interface is writable
TOUCH: int  # interface id of the touch events
TOUCH_START: int  # event id of touch start event
TOUCH_MOVE: int  # event id of touch move event
TOUCH_END: int  # event id of touch end event
UART: int  # interface id of the uart events
USB_STATE: int  # interface id of the usb state events
LOCAL: int  # interface local
BUTTON: int  # interface id of button events
BUTTON_PRESSED: int  # button down event
BUTTON_RELEASED: int  # button up event
BUTTON_LEFT: int  # button number of left button
BUTTON_RIGHT: int  # button number of right button
SPI_FACE: int  # interface id of the spi events
SPI_FIDO_FACE: int  # interface id of the spi fido events
WireInterface = Union[HID, WebUSB, SPI]
USB_CHECK: int # interface id for check of USB data connection
FINGERPRINT_STATE: int # interface id of the fingerprint state events
