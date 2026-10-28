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


# extmod/modtrezorio/modtrezorio-nfc.h
def close_secure_channel() -> None:
    """
    Tear down the SCP11/SCP03 session. The next APDU sent with safe=True
    negotiates a fresh one. Needed where the card drops the session on its
    own side without the applet being re-selected.
    """
