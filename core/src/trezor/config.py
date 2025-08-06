import trezorconfig as config
from typing import Any


def unlock(
    pin: str, salt: bytes | None = None, pin_type: int = 0, auto_vibrate: bool = True
) -> tuple[bool, int]:
    print(f"Unlocking with pin: {pin}, salt: {salt}, pin_type: {pin_type}")
    result = config.unlock(pin, salt, pin_type)
    if auto_vibrate and not result[0]:
        from trezor import motor

        motor.vibrate(motor.ERROR)
    return result


def check_pin(
    pin: str, salt: bytes | None = None, pin_type: int = 0, auto_vibrate: bool = False
) -> tuple[bool, int]:
    print(f"Checking pin: {pin}, salt: {salt}, pin_type: {pin_type}")
    result = config.check_pin(pin, salt, pin_type)
    if auto_vibrate and not result[0]:
        from trezor import motor

        motor.vibrate(motor.ERROR)
    return result


def __getattr__(name: str) -> Any:
    return getattr(config, name)
