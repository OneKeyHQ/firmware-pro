from ._bit_string import BitString
from ._cell import Cell


class Builder:
    SNAKE_DATA_CHUNK_BYTES = 127

    def __init__(self):
        self.bits = BitString(1023)
        self.refs = []
        self.is_exotic = False

    def __repr__(self):
        return f"<Builder refs_num: {len(self.refs)}, {repr(self.bits)}>"

    def store_cell(self, src: Cell):
        self.bits.write_bit_string(src.bits)
        self.refs += src.refs
        return self

    def store_ref(self, src: Cell):
        self.refs.append(src)
        return self

    def store_maybe_ref(self, src):
        if src:
            self.bits.write_bit(1)
            self.store_ref(src)
        else:
            self.bits.write_bit(0)

        return self

    def store_bit(self, value):
        self.bits.write_bit(value)
        return self

    def store_bit_array(self, value):
        self.bits.write_bit_array(value)
        return self

    def store_uint(self, value, bit_length):
        self.bits.write_uint(value, bit_length)
        return self

    def store_uint8(self, value):
        self.bits.write_uint8(value)
        return self

    def store_int(self, value, bit_length):
        self.bits.write_int(value, bit_length)
        return self

    def store_string(self, value):
        self.bits.write_string(value)
        return self

    def store_bytes(self, value):
        self.bits.write_bytes(value)
        return self

    def store_string_tail(self, value):
        if isinstance(value, str):
            value = value.encode("utf-8")
        elif isinstance(value, bytearray):
            value = bytes(value)
        elif not isinstance(value, bytes):
            raise TypeError("store_string_tail expects str or bytes-like input")
        if not value:
            return self

        chunk_size = self.SNAKE_DATA_CHUNK_BYTES
        self.store_bytes(value[:chunk_size])

        if len(value) <= chunk_size:
            return self

        tail = begin_cell().store_string_tail(value[chunk_size:]).end_cell()
        return self.store_ref(tail)

    def store_string_ref_tail(self, value):
        return self.store_ref(begin_cell().store_string_tail(value).end_cell())

    def store_bit_string(self, value):
        self.bits.write_bit_string(value)
        return self

    def store_address(self, value):
        self.bits.write_address(value)
        return self

    def store_grams(self, value):
        self.bits.write_grams(value)
        return self

    def store_coins(self, value):
        self.bits.write_coins(value)
        return self

    def end_cell(self):
        cell = Cell()
        cell.write_cell(self)
        return cell


def begin_cell():
    return Builder()
