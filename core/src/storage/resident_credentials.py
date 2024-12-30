from storage import common, device
from trezor import utils
from trezor.crypto import se_thd89

if utils.USE_THD89:
    MAX_RESIDENT_CREDENTIALS = se_thd89.FIDO2_CRED_COUNT_MAX
    _RESIDENT_CREDENTIAL_START_KEY = 0
else:
    _RESIDENT_CREDENTIAL_START_KEY = 1
    MAX_RESIDENT_CREDENTIALS = 100


def get(index: int) -> bytes | None:
    if not 0 <= index < MAX_RESIDENT_CREDENTIALS:
        raise ValueError  # invalid credential index

    return common.get(common.APP_WEBAUTHN, index + _RESIDENT_CREDENTIAL_START_KEY)


def set(index: int, data: bytes, is_overwritten: bool = False) -> None:
    if not 0 <= index < MAX_RESIDENT_CREDENTIALS:
        raise ValueError  # invalid credential index

    common.set(common.APP_WEBAUTHN, index + _RESIDENT_CREDENTIAL_START_KEY, data)
    if not is_overwritten:
        _increase_fido2_counter()


def delete(index: int) -> None:
    if not 0 <= index < MAX_RESIDENT_CREDENTIALS:
        raise ValueError  # invalid credential index

    common.delete(common.APP_WEBAUTHN, index + _RESIDENT_CREDENTIAL_START_KEY)
    _decrement_fido2_counter()


def delete_all() -> None:
    if device.get_fido2_counter() == 0:
        return
    if utils.USE_THD89:
        se_thd89.fido_delete_all_credentials()
    else:
        for i in range(MAX_RESIDENT_CREDENTIALS):
            common.delete(common.APP_WEBAUTHN, i + _RESIDENT_CREDENTIAL_START_KEY)
    _reset_fido2_counter()


def get_fido2_counter() -> int:
    return device.get_fido2_counter()


def _increase_fido2_counter() -> None:
    value = device.get_fido2_counter()
    if value >= MAX_RESIDENT_CREDENTIALS:
        raise ValueError(
            f"FIDO2 counter cannot be greater than {MAX_RESIDENT_CREDENTIALS}"
        )
    device.set_fido2_counter(value + 1)


def _decrement_fido2_counter() -> None:
    value = device.get_fido2_counter()
    if value == 0:
        raise ValueError("FIDO2 counter cannot be decremented below 0")
    device.set_fido2_counter(value - 1)


def _reset_fido2_counter() -> None:
    device.set_fido2_counter(0)
