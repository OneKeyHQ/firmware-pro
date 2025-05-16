from typing import *


# extmod/modtrezorio/modtrezorio-fingerprint.h
class FpError(OSError):
    pass


# extmod/modtrezorio/modtrezorio-fingerprint.h
class EnrollDuplicate(FpError):
    pass


# extmod/modtrezorio/modtrezorio-fingerprint.h
class NoFp(FpError):
    pass


# extmod/modtrezorio/modtrezorio-fingerprint.h
class GetImageFail(FpError):
    pass


# extmod/modtrezorio/modtrezorio-fingerprint.h
class ExtractFeatureFail(FpError):
    pass


# extmod/modtrezorio/modtrezorio-fingerprint.h
class NotMatch(FpError):
    pass


# extmod/modtrezorio/modtrezorio-fingerprint.h
def detect() -> bool:
    """
    Detect fingerprint.
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def enroll(seq: int) -> bool:
    """
    Enroll fingerprint.
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def register_template(id: int) -> bool:
    """
    Register fingerprint template.
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def save(id: int) -> bool:
    """
    Save fingerprint data.
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def get_group() -> bytes:
    """
    Get fingerprint group.
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def match() -> int:
    """
    Verify fingerprint.
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def remove(id: int) -> bool:
    """
    Remove fingerprint.
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def remove_group(group_id: bytes) -> bool:
    """
    Remove fingerprint group.
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def clear() -> bool:
    """
    Remove all fingerprints.
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def get_template_count() -> int:
    """
    Get number of stored fingerprints.
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def list_template() -> tuple[int | None] | None:
    """
    List fingerprints.
    returns: tuple of fingerprint ids
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def sleep() -> bool:
     """
     make fingerprint sensor to sleep mode.
     """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def set_sensitivity_and_area(sensitivity: int, area: int) -> bool:
    """
    Set fingerprint sensor sensitivity and area.
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def get_sensitivity_and_area() -> tuple[int, int]:
    """
    Get fingerprint sensor sensitivity and area.
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def clear_template_cache(clear_data: bool = False) -> None  :
    """
    Clear fingerprint template cache.
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def get_max_template_count() -> int:
    """
    Get maximum number of stored fingerprints.
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def data_version_is_new() -> bool:
    """
    Check if fingerprint data version is new.
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def data_upgrade_prompted():
    """
    Set fingerprint data upgrade prompted.
    """


# extmod/modtrezorio/modtrezorio-fingerprint.h
def data_upgrade_is_prompted() -> bool:
    """
    Check if fingerprint data upgrade is prompted.
    """
