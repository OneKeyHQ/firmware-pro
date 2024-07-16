from typing import *


# extmod/modtrezorio/modtrezorio-nfc.h
def pwr_ctrl(on_off: bool) -> bool:
    """
    Control NFC power.
    """


# extmod/modtrezorio/modtrezorio-nfc.h
def poll_card() -> bool:
    """
    Poll card.
    """


# extmod/modtrezorio/modtrezorio-nfc.h
def send_recv(apdu: bytes, safe: bool = False) -> tuple[bytes, bytes]:
    """
    Send receive data through NFC.
    """
