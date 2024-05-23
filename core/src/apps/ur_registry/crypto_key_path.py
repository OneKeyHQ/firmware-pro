from .ur_py.ur import cbor_lite
from .ur_py.ur.cbor_lite import CBORDecoder, CBOREncoder
from .ur_py.ur.ur import UR

COMPONENTS = 1
SOURCE_FINGERPRINT = 2
DEPTH = 3


class PathComponent:
    HARDEN_BIT = 0x80000000

    def __init__(self, index=None, wildcard=None, hardened=None):
        self.index = index
        self.wildcard = wildcard
        self.hardened = hardened

    @staticmethod
    def new(index, hardened):
        p = PathComponent()
        if index is not None:
            if index & PathComponent.HARDEN_BIT != 0:
                raise ValueError("Invalid index - most significant bit cannot be set")
            p.index = index
            p.wildcard = False
            p.hardened = hardened
        else:
            p.index = index
            p.wildcard = True
            p.hardened = hardened
        return p

    def get_index(self):
        return self.index

    def get_canonical_index(self):
        if self.is_hardened() is True:
            return self.get_index() + PathComponent.HARDEN_BIT
        else:
            return self.get_index()

    def is_wildcard(self):
        return self.wildcard

    def is_hardened(self):
        return self.hardened


class CryptoKeyPath:
    def __init__(
        self,
        components: list[PathComponent],
        source_fingerprint: int = None,
        depth: int | None = None,
    ):
        self.components = components
        self.source_fingerprint = source_fingerprint
        self.depth = depth

    @staticmethod
    def get_registry_type():
        return "crypto-keypath"

    @staticmethod
    def get_tag():
        return 304

    @staticmethod
    def new(components, source_fingerprint, depth):
        return CryptoKeyPath(components, source_fingerprint, depth)

    def get_components(self) -> list[PathComponent]:
        return self.components

    def get_source_fingerprint(self) -> int | None:
        return self.source_fingerprint

    def get_depth(self):
        return self.depth

    def get_path(self) -> str | None:
        if self.components is None:
            return None

        path = ""
        for component in self.components:
            if component.wildcard is True and component.hardened is True:
                path += "*'"
            if component.wildcard is True and component.hardened is False:
                path += "*"
            if component.wildcard is False and component.hardened is True:
                path += f"{component.index}'"
            if component.wildcard is False and component.hardened is False:
                path += f"{component.index}"
            path += "/"
        return path[:-1]

    def from_path(self, path, fingerprint):
        n = path.split("/")
        if n[0] == "m" or n[0] == "M":
            n = n[1:]

        components = []
        for x in n:
            if x.endswith(("h", "'")):
                p = PathComponent(True, int(x[:-1]), False)
            else:
                p = PathComponent(False, int(x[:-1]), False)
            components.append(p)

        self.components = components
        self.source_fingerprint = fingerprint
        self.depth = None

    def cbor_encode(self):
        encoder = CBOREncoder()
        size = 1
        if self.source_fingerprint is not None:
            size += 1
        if self.depth is not None:
            size += 1
        encoder.encodeMapSize(size)

        encoder.encodeInteger(COMPONENTS)
        encoder.encodeArraySize(2 * len(self.components))
        for component in self.components:
            if component.is_wildcard():
                encoder.encodeArraySize(0)
            else:
                if component.index is not None:
                    encoder.encodeInteger(component.index)
                else:
                    encoder.encodeInteger(0)
            encoder.encodeBool(component.is_hardened())

        if self.source_fingerprint is not None:
            encoder.encodeInteger(SOURCE_FINGERPRINT)
            encoder.encodeInteger(self.source_fingerprint)
        if self.depth is not None:
            encoder.encodeInteger(DEPTH)
            encoder.encodeInteger(self.depth)

        return encoder.get_bytes()

    def ur_encode(self):
        data = self.cbor_encode()
        return UR(self.get_registry_type(), data)

    @staticmethod
    def from_cbor(cbor):
        decoder = CBORDecoder(cbor)
        return CryptoKeyPath.decode(decoder)

    @staticmethod
    def decode(decoder):
        size, _ = decoder.decodeMapSize()
        components = []
        source_fingerprint = None
        depth = None
        for _ in range(size):
            value, _ = decoder.decodeInteger()
            if value == COMPONENTS:
                array_size, _ = decoder.decodeArraySize()
                previous_type = None
                path_index = None
                hardened = False
                for _ in range(array_size):
                    tag, value, _ = decoder.decodeTagAndValue(cbor_lite.Flag_None)
                    if tag is cbor_lite.Tag_Major_array:
                        previous_type = cbor_lite.Tag_Major_array
                    if tag is cbor_lite.Tag_Major_unsignedInteger:
                        previous_type = cbor_lite.Tag_Major_unsignedInteger
                        path_index = value
                    if tag is cbor_lite.Tag_Major_simple:
                        # if value == cbor_lite.Tag_Minor_true:
                        #     hardened = True
                        # else:
                        #     hardened = False
                        hardened = value == cbor_lite.Tag_Minor_true
                        if previous_type is cbor_lite.Tag_Major_array:
                            p = PathComponent.new(None, hardened)
                            components.append(p)
                        if previous_type is cbor_lite.Tag_Major_unsignedInteger:
                            p = PathComponent.new(path_index, hardened)
                            components.append(p)

            if value == SOURCE_FINGERPRINT:
                value, _ = decoder.decodeInteger()
                source_fingerprint = value
            if value == DEPTH:
                value, _ = decoder.decodeInteger()
                depth = value
        return CryptoKeyPath(components, source_fingerprint, depth)
