from dataclasses import dataclass
from typing import Collection, Optional, Tuple

from . import mapping

UsbId = Tuple[int, int]

VENDORS = ("bitcointrezor.com", "trezor.io", "onekey.so")


@dataclass(eq=True, frozen=True)
class TrezorModel:
    name: str
    minimum_version: Tuple[int, int, int]
    vendors: Collection[str]
    usb_ids: Collection[UsbId]
    default_mapping: mapping.ProtobufMapping


TREZOR_ONE = TrezorModel(
    name="1",
    minimum_version=(1, 8, 0),
    vendors=VENDORS,
    usb_ids=((0x534C, 0x0001),),
    default_mapping=mapping.DEFAULT_MAPPING,
)

TREZOR_T = TrezorModel(
    name="T",
    minimum_version=(2, 1, 0),
    vendors=VENDORS,
    usb_ids=((0x1209, 0x53C1), (0x1209, 0x53C0), (0x1209, 0x4F4B), (0x1209, 0x4F4A)),
    default_mapping=mapping.DEFAULT_MAPPING,
)

TREZOR_R = TrezorModel(
    name="R",
    minimum_version=(2, 1, 0),
    vendors=VENDORS,
    usb_ids=((0x1209, 0x53C1), (0x1209, 0x53C0)),
    default_mapping=mapping.DEFAULT_MAPPING,
)


@dataclass(eq=True, frozen=True)
class OneKeyModel:
    name: str
    minimum_version_boot: Tuple[int, int, int]
    minimum_version_fw: Tuple[int, int, int]
    vendors: Collection[str]
    usb_ids: Collection[UsbId]
    default_mapping: mapping.ProtobufMapping

ONEKEY_PRO = OneKeyModel(
    name="P",
    minimum_version_boot=(2, 6, 0),
    minimum_version_fw=(4, 10, 0),
    vendors=VENDORS,
    usb_ids=((0x1209, 0x53C1), (0x1209, 0x53C0), (0x1209, 0x4F4B), (0x1209, 0x4F4A)),
    default_mapping=mapping.DEFAULT_MAPPING,
)

TREZORS = {TREZOR_ONE, TREZOR_T, TREZOR_R}
ONEKEYS = {ONEKEY_PRO}


def by_name(name: str) -> Optional[TrezorModel] | Optional[OneKeyModel]:
    for model in TREZORS:
        if model.name == name:
            return model
    for model in ONEKEYS:
        if model.name == name:
            return model
    return None
