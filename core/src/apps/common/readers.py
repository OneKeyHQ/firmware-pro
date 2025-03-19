from trezor.utils import BufferReader


def read_compact_size(r: BufferReader) -> int:
    prefix = r.get()
    if prefix < 253:
        n = prefix
    elif prefix == 253:
        n = r.get()
        n += r.get() << 8
    elif prefix == 254:
        n = r.get()
        n += r.get() << 8
        n += r.get() << 16
        n += r.get() << 24
    elif prefix == 255:
        n = r.get()
        n += r.get() << 8
        n += r.get() << 16
        n += r.get() << 24
        n += r.get() << 32
        n += r.get() << 40
        n += r.get() << 48
        n += r.get() << 56
    else:
        raise ValueError
    return n


def read_uint16_be(r: BufferReader) -> int:
    n = r.get()
    return (n << 8) + r.get()


def read_uint32_be(r: BufferReader) -> int:
    n = r.get()
    for _ in range(3):
        n = (n << 8) + r.get()
    return n


def read_uint64_be(r: BufferReader) -> int:
    n = r.get()
    for _ in range(7):
        n = (n << 8) + r.get()
    return n


def _from_bytes_to_signed(bs: bytes, byteorder) -> int:
    if len(bs) == 0:
        raise ValueError("Empty bytes")
    if byteorder not in ["big", "little"]:
        raise ValueError("Invalid byteorder")
    negative = bs[0] & 0x80 if byteorder == "big" else bs[-1] & 0x80
    if not negative:
        return int.from_bytes(bs, byteorder)
    neg_b = bytes(~b & 0xFF for b in bs)
    return -1 - int.from_bytes(neg_b, byteorder)


def read_uint16_le(r: BufferReader) -> int:
    data = r.read_memoryview(2)
    return int.from_bytes(data, "little")


def read_uint32_le(r: BufferReader) -> int:
    data = r.read_memoryview(4)
    return int.from_bytes(data, "little")


def read_uint64_le(r: BufferReader) -> int:
    data = r.read_memoryview(8)
    return int.from_bytes(data, "little")


def read_int8_le(r: BufferReader) -> int:
    data = r.read_memoryview(1)
    return _from_bytes_to_signed(data, "little")


def read_int16_le(r: BufferReader) -> int:
    data = r.read_memoryview(2)
    return _from_bytes_to_signed(data, "little")


def read_int32_le(r: BufferReader) -> int:
    data = r.read_memoryview(4)
    return _from_bytes_to_signed(data, "little")


def read_int64_le(r: BufferReader) -> int:
    data = r.read_memoryview(8)
    return _from_bytes_to_signed(data, "little")
