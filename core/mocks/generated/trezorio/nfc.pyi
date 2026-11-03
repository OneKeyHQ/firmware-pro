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


# extmod/modtrezorio/modtrezorio-nfc.h
def p2p_export_start(
    mnemonic: AnyBytes,
    listen_timeout_ms: int,
    turn_timeout_ms: int,
) -> int:
    """Start one exporter session. Returns 0 on start, otherwise an error.
    Copies verified user-entered BIP39 text for SE sealing with E2/09.
    Does not change the SE export switch.
    """


# extmod/modtrezorio/modtrezorio-nfc.h
def p2p_export_poll() -> tuple[int, int, int, int, int]:
    """Poll without waiting for RF. Returns (result, in, out, export_sw,
    state). Result -1 means pending, 0 means success, 10 means cancelled;
    other values are device_transfer_export_result_t.
    """


# extmod/modtrezorio/modtrezorio-nfc.h
def p2p_export_close() -> None:
    """Cancel and wipe the exporter session, then power NFC down.
    Idempotent."""
