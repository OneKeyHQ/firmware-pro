from storage import common
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


def set(index: int, data: bytes) -> None:
    if not 0 <= index < MAX_RESIDENT_CREDENTIALS:
        raise ValueError  # invalid credential index

    common.set(common.APP_WEBAUTHN, index + _RESIDENT_CREDENTIAL_START_KEY, data)


def delete(index: int) -> None:
    if not 0 <= index < MAX_RESIDENT_CREDENTIALS:
        raise ValueError  # invalid credential index

    common.delete(common.APP_WEBAUTHN, index + _RESIDENT_CREDENTIAL_START_KEY)


def delete_all() -> None:
    if utils.USE_THD89:
        se_thd89.fido_delete_all_credentials()
    else:
        for i in range(MAX_RESIDENT_CREDENTIALS):
            common.delete(common.APP_WEBAUTHN, i + _RESIDENT_CREDENTIAL_START_KEY)
