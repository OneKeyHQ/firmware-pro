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
def save(id: int) -> bool:
    """
    Save fingerprint.
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
 def sleep() -> bool:
     """
     make fingerprint sensor to sleep mode.
     """
