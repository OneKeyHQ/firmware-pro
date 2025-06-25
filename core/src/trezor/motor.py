from micropython import const
from typing import TYPE_CHECKING

import storage.device as storage_device
from trezor import io, utils

if TYPE_CHECKING:
    from typing import TypeVar

    VIBRATE_STYLE = TypeVar("VIBRATE_STYLE", bound=int)

WHISPER = const(0)
LIGHT = const(1)
MEDIUM = const(2)
HEAVY = const(3)
SUCCESS = const(4)
WARNING = const(5)
ERROR = const(6)

# pyright: off
if not utils.EMULATOR:
    MOTOR_CTL = io.MOTOR()

    def vibrate(
        style: VIBRATE_STYLE = LIGHT, force: bool = False
    ) -> VIBRATE_STYLE | None:
        if not storage_device.keyboard_haptic_enabled() and not force:
            return None
        if style == WHISPER:
            MOTOR_CTL.play_whisper()
        elif style == LIGHT:
            MOTOR_CTL.play_light()
        elif style == MEDIUM:
            MOTOR_CTL.play_medium()
        elif style == HEAVY:
            MOTOR_CTL.play_heavy()
        elif style == SUCCESS:
            MOTOR_CTL.play_success()
        elif style == WARNING:
            MOTOR_CTL.play_warning()
        elif style == ERROR:
            MOTOR_CTL.play_error()
        else:
            if __debug__:
                print(f"vibrate: unknown style {style}")
            pass
        return style

else:

    def vibrate(
        style: VIBRATE_STYLE = LIGHT, force: bool = False
    ) -> VIBRATE_STYLE | None:
        pass


# pyright: on
