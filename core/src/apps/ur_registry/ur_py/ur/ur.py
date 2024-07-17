#
# ur.py
#
# Copyright © 2020 Foundation Devices, Inc.
# Licensed under the "BSD-2-Clause Plus Patent License"
#

from .utils import is_ur_type


class InvalidType(Exception):
    pass


class UR:
    def __init__(self, registry_type, cbor):
        if not is_ur_type(registry_type):
            raise InvalidType()

        self.registry_type = registry_type
        self.cbor = cbor

    def __eq__(self, obj):
        if obj is None:
            return False
        return self.registry_type == obj.registry_type and self.cbor == obj.cbor
