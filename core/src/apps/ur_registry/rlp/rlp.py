import struct


def convert_to_bytes(input_object):
    """Convert python objects to bytes can be deserialize with converters"""
    if isinstance(input_object, str):
        return input_object.encode(encoding="utf-8")

    elif isinstance(input_object, bool):
        if input_object:
            return b"\x01"
        else:
            return b"\x00"

    elif isinstance(input_object, int):
        bytes_needed = round((input_object.bit_length() + 7) / 8)
        if input_object >= 0:
            return input_object.to_bytes(bytes_needed, "big")
        else:
            return input_object.to_bytes(bytes_needed, "big", signed=True)

    elif isinstance(input_object, float):
        return struct.pack("f", input_object)

    else:
        try:
            return bytes(input_object)
        except TypeError:
            raise TypeError(f'cannot serialize "{type(input_object).__name__}" object')


def encode(input_decoded_object):
    """Encode lists or objects to bytes."""
    if isinstance(input_decoded_object, bytes):
        bytes_string = input_decoded_object
        length = len(bytes_string)
        if length == 1 and ord(bytes_string) <= 0x7F:
            return bytes_string
        elif length <= 55:
            return (0x80 + length).to_bytes(1, "big") + bytes_string
        else:
            bytes_needed = round((length.bit_length() + 7) / 8)
            return (
                (0xB7 + bytes_needed).to_bytes(1, "big")
                + length.to_bytes(bytes_needed, "big")
                + bytes_string
            )

    elif isinstance(input_decoded_object, list):
        encoded_list_values = b""
        for item in input_decoded_object:
            encoded_list_values += encode(item)
        values_length = len(encoded_list_values)

        if values_length <= 55:
            return (0xC0 + values_length).to_bytes(1, "big") + encoded_list_values
        else:
            bytes_needed = round((values_length.bit_length() + 7) / 8)
            return (
                (0xF7 + bytes_needed).to_bytes(1, "big")
                + values_length.to_bytes(bytes_needed, "big")
                + encoded_list_values
            )
    else:
        return encode(convert_to_bytes(input_decoded_object))


def _decode(value: bytes):
    # Short value
    if value[0] <= 0x7F:
        decoded_data = value[0].to_bytes(1, "big")
        offset = value[1:]

        if offset != b"":
            decoded_offset = _decode(offset)
            if isinstance(decoded_offset, tuple):
                output = [decoded_data]
                output.extend(decoded_offset)
                output = tuple(output)
                return output
            else:
                return (decoded_data, decoded_offset)
        else:
            return decoded_data

    # Short string
    elif value[0] <= 0xB7:
        length = value[0] - 0x80
        decoded_data = value[1 : length + 1]
        offset = value[1 + length :]

        if offset != b"":
            decoded_offset = _decode(offset)
            if isinstance(decoded_offset, tuple):
                output = [decoded_data]
                output.extend(decoded_offset)
                output = tuple(output)
                return output
            else:
                return (decoded_data, decoded_offset)
        else:
            return decoded_data

    # Long string
    elif value[0] <= 0xBF:
        length_bytes = value[0] - 0xB7
        length = int.from_bytes(value[1 : length_bytes + 1], "big")
        decoded_data = value[1 + length_bytes : 1 + length + length_bytes]
        offset = value[1 + length_bytes + length :]

        if offset != b"":
            decoded_offset = _decode(offset)
            if isinstance(decoded_offset, tuple):
                output = [decoded_data]
                output.extend(decoded_offset)
                output = tuple(output)
                return output
            else:
                return (decoded_data, decoded_offset)
        else:
            return decoded_data

    # Empty list
    elif value[0] <= 0xC0:
        data = []
        offset = value[1:]
        decoded_data = data
        if offset != b"":
            decoded_offset = _decode(offset)
            if isinstance(decoded_offset, tuple):
                decoded_data = [[]]
                decoded_data.extend(decoded_offset)
                decoded_data = tuple(decoded_data)
                return decoded_data
            else:
                return (data, decoded_offset)
        else:
            return decoded_data

    # Short list
    elif value[0] <= 0xF7:
        length = value[0] - 0xC0
        data = value[1 : length + 1]
        offset = value[1 + length :]
        decoded_data = _decode(data)
        if offset != b"":
            decoded_offset = _decode(offset)
            if isinstance(decoded_data, tuple):
                list_output = []
                list_output.extend(decoded_data)
                if isinstance(decoded_offset, tuple):
                    output = []
                    output.append(list_output)
                    output.extend(decoded_offset)
                    output = tuple(output)
                    return output
                else:
                    return (list_output, decoded_offset)
            else:
                return ([decoded_data], decoded_offset)
        else:
            if isinstance(decoded_data, tuple):
                list_output = []
                list_output.extend(decoded_data)
                return list_output
            else:
                return [decoded_data]
    # Long list
    elif value[0] > 0xF7:
        length_bytes = value[0] - 0xF7
        length = int.from_bytes(value[1 : length_bytes + 1], "big")
        data = value[1 + length_bytes : 1 + length_bytes + length]
        offset = value[1 + length_bytes + length :]
        decoded_data = _decode(data)

        if offset != b"":
            decoded_offset = _decode(offset)
            if isinstance(decoded_data, tuple):
                list_output = []
                list_output.extend(decoded_data)
                if isinstance(decoded_offset, tuple):
                    output = []
                    output.append(list_output)
                    output.extend(decoded_offset)
                    output = tuple(output)
                    return output
                else:
                    return (list_output, decoded_offset)
            else:
                return ([decoded_data], decoded_offset)
        else:
            if isinstance(decoded_data, tuple):
                list_output = []
                list_output.extend(decoded_data)
                return list_output
            else:
                return [decoded_data]


def convert_to_types(decoded_object, template):
    """Decode list of bytes to list of python objects. Use a template"""
    if isinstance(decoded_object, bytes):
        if template:
            return template().convert(decoded_object)
        else:
            return decoded_object

    elif isinstance(decoded_object, list):
        output = []
        i = 0
        for item in decoded_object:
            converted = convert_to_types(item, template[i])
            output.append(converted)
            i += 1
        return output


def decode(byte_string: bytes, template=None):
    """Decode bytes strings to python objects. Use a template if you need convert the bytes to python objects"""
    decoded_bytes = _decode(byte_string)
    if template:
        return convert_to_types(decoded_bytes, template=template)
    else:
        return decoded_bytes
