import struct


# pyright: off
class UInteger:
    byteorder = "big"

    def convert(self, bytes_value):
        return int.from_bytes(bytes_value, self.byteorder)


class SInteger:
    byteorder = "big"

    def convert(self, bytes_value):
        return int.from_bytes(bytes_value, self.byteorder, signed=True)


class Bool:
    @staticmethod
    def convert(bytes_value):
        if len(bytes_value) > 1:
            raise ValueError("value to long use 1 byte for Bool type")
        elif len(bytes_value) == 0:
            raise ValueError("value to short use 1 byte for Bool type")
        else:
            if ord(bytes_value) == 0:
                return False
            else:
                return True


class String:
    encoding = "utf-8"

    def convert(self, bytes_value):
        return bytes_value.decode(encoding=self.encoding)


class Float:
    def convert(self, bytes_value):
        return struct.unpack("f", bytes_value)[0]


# pyright: on
