from ubinascii import hexlify

from trezor.crypto.hashlib import blake2b, sha256
from trezor.strings import format_amount

INTENT_BYTES = b"\x00\x00\x00"
PERSONALMESSAGE_INTENT_BYTES = b"\x03\x00\x00"


def benfen_address_from_pubkey(pub_key_bytes: bytes) -> str:
    payloads = b"\x00" + pub_key_bytes
    h = blake2b(data=payloads, outlen=32).digest()
    return f"0x{hexlify(h).decode()}"


def try_convert_to_bfc_address(sui_addr: str) -> str | None:

    if len(sui_addr) < 3 or not (sui_addr[0] == "0" and (sui_addr[1] in "xX")):
        return None

    hex_part = sui_addr[2:]
    if len(hex_part) == 0 or len(hex_part) > 64:
        return None

    for c in hex_part:
        if not (c.isdigit() or c in "abcdefABCDEF"):
            return None

    padding = 64 - len(hex_part)
    if padding > 0:
        hex_part = "0" * padding + hex_part

    h = sha256()
    h.update(hex_part.encode("utf-8"))
    digest = h.digest()

    checksum = "".join([f"{b:02x}" for b in digest[:2]])

    return "BFC" + hex_part + checksum


def uleb_encode(num: int) -> bytes:
    arr = bytearray()
    while num > 0:
        val = num & 127
        num = num >> 7
        if num != 0:
            val |= 128
        arr.append(val)
    return arr


def format_benfen_amount(amount: int, currency_symbol: str = "BFC") -> str:

    decimals = 9
    formatted = format_amount(amount, decimals)
    return f"{formatted} {currency_symbol}"
