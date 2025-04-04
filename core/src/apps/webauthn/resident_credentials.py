from micropython import const
from typing import Iterator

import storage
import storage.resident_credentials
from storage.resident_credentials import MAX_RESIDENT_CREDENTIALS

from .credential import Fido2Credential
from .fido_seed import ensure_fido_seed

RP_ID_HASH_LENGTH = const(32)
_ALLOW_RESIDENT_CREDENTIALS = storage.device.get_se01_version() >= "1.1.5"


def _credential_from_data(index: int, data: bytes) -> Fido2Credential:
    rp_id_hash = data[:RP_ID_HASH_LENGTH]
    cred_id = data[RP_ID_HASH_LENGTH:]
    cred = Fido2Credential.from_cred_id(cred_id, rp_id_hash)
    cred.index = index
    return cred


@ensure_fido_seed
def find_all() -> Iterator[Fido2Credential]:
    if not _ALLOW_RESIDENT_CREDENTIALS:
        return
    registered_count = storage.resident_credentials.get_fido2_counter()
    if registered_count == 0:
        return
    find = 0
    for index in range(MAX_RESIDENT_CREDENTIALS):
        if find >= registered_count:
            return
        data = storage.resident_credentials.get(index)
        if data is not None:
            yield _credential_from_data(index, data)
            find += 1


@ensure_fido_seed
def find_by_rp_id_hash(rp_id_hash: bytes) -> Iterator[Fido2Credential]:
    if not _ALLOW_RESIDENT_CREDENTIALS:
        return
    for index in range(MAX_RESIDENT_CREDENTIALS):
        data = storage.resident_credentials.get(index)

        if data is None:
            # empty slot
            continue

        if data[:RP_ID_HASH_LENGTH] != rp_id_hash:
            # rp_id_hash mismatch
            continue

        yield _credential_from_data(index, data)


@ensure_fido_seed
def get_resident_credential(index: int) -> Fido2Credential | None:
    if not _ALLOW_RESIDENT_CREDENTIALS:
        return None
    if not 0 <= index < MAX_RESIDENT_CREDENTIALS:
        return None

    data = storage.resident_credentials.get(index)
    if data is None:
        return None

    return _credential_from_data(index, data)


@ensure_fido_seed
def store_resident_credential(cred: Fido2Credential) -> bool:
    if not _ALLOW_RESIDENT_CREDENTIALS:
        return False
    if storage.resident_credentials.get_fido2_counter() >= MAX_RESIDENT_CREDENTIALS:
        return False

    slot = None
    is_overwritten = False
    for index in range(MAX_RESIDENT_CREDENTIALS):
        stored_data = storage.resident_credentials.get(index)
        if stored_data is None:
            # found candidate empty slot
            if slot is None:
                slot = index
            continue

        if cred.rp_id_hash != stored_data[:RP_ID_HASH_LENGTH]:
            # slot is occupied by a different rp_id_hash
            continue

        stored_cred = _credential_from_data(index, stored_data)
        # If a credential for the same RP ID and user ID already exists, then overwrite it.
        if stored_cred.user_id == cred.user_id:
            slot = index
            is_overwritten = True
            break

    if slot is None:
        return False

    cred_data = cred.rp_id_hash + cred.id
    try:
        storage.resident_credentials.set(slot, cred_data, is_overwritten)
    except ValueError:
        return False
    return True
