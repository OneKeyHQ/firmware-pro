from micropython import const

from trezor import wire

from . import helpers
from .crypto_hd_key import CryptoHDKey
from .ur_py.ur import cbor_lite
from .ur_py.ur.ur import UR
from .ur_py.ur.ur_encoder import UREncoder

MASTER_FINGERPRINT = const(1)
KEYS = const(2)
DEVICE = const(3)
DEVICE_ID = const(4)
DEVICE_VERSION = const(5)


class CryptoMultiAccounts:
    def __init__(
        self,
        master_fingerprint: int,
        keys: list[CryptoHDKey],
        device: str | None = None,
        device_id: str | None = None,
        device_version: str | None = None,
    ):
        self.master_fingerprint = master_fingerprint
        self.keys = keys
        self.device = device
        self.device_id = device_id
        self.device_version = device_version

    @staticmethod
    def get_registry_type() -> str:
        return "crypto-multi-accounts"

    @staticmethod
    def get_tag() -> int:
        return 1103

    def set_master_fingerprint(self, master_fingerprint: int):
        self.master_fingerprint = master_fingerprint

    def set_keys(self, keys: list[CryptoHDKey]):
        self.keys = keys

    def add_key(self, key: CryptoHDKey):
        self.keys.append(key)

    def set_device(self, device: str):
        self.device = device

    def set_device_id(self, device_id: str):
        self.device_id = device_id

    def set_device_version(self, device_version: str):
        self.device_version = device_version

    def get_master_fingerprint(self) -> int:
        return self.master_fingerprint

    def get_keys(self) -> list[CryptoHDKey]:
        return self.keys

    def get_device(self) -> str | None:
        return self.device

    def get_device_id(self) -> str | None:
        return self.device_id

    def get_device_version(self) -> str | None:
        return self.device_version

    def to_ur(self) -> UR:
        data = self.cbor_encode()
        return UR(CryptoMultiAccounts.get_registry_type(), data)

    def cbor_encode(self) -> bytes:
        encoder = cbor_lite.CBOREncoder()
        size = 2
        if self.device is not None:
            size += 1
        if self.device_id is not None:
            size += 1
        if self.device_version is not None:
            size += 1
        # encode map size
        encoder.encodeMapSize(size)
        # encode master fingerprint
        encoder.encodeInteger(MASTER_FINGERPRINT)
        encoder.encodeInteger(self.master_fingerprint)
        # encode keys
        encoder.encodeInteger(KEYS)
        encoder.encodeArraySize(len(self.keys))
        for key in self.keys:
            encoder.encodeTag(CryptoHDKey.get_tag())
            encoder.cborExtend(key.cbor_encode())
        # encode device
        if self.device is not None:
            encoder.encodeInteger(DEVICE)
            encoder.encodeText(self.device)
        # encode device id
        if self.device_id is not None:
            encoder.encodeInteger(DEVICE_ID)
            encoder.encodeText(self.device_id)
        # encode device version
        if self.device_version is not None:
            encoder.encodeInteger(DEVICE_VERSION)
            encoder.encodeText(self.device_version)

        return encoder.get_bytes()

    @staticmethod
    def cbor_decode(decoder: cbor_lite.CBORDecoder) -> "CryptoMultiAccounts":
        cma = CryptoMultiAccounts(0, [])
        size, _ = decoder.decodeMapSize()
        for _ in range(size):
            key, _ = decoder.decodeInteger()
            if key == MASTER_FINGERPRINT:
                value, length = decoder.decodeInteger()
                if length != 4:
                    raise ValueError("Invalid master fingerprint")
                cma.set_master_fingerprint(value)
            elif key == KEYS:
                keys = []
                array_size, _ = decoder.decodeArraySize()
                for _ in range(array_size):
                    tag, _ = decoder.decodeTag()
                    if tag != CryptoHDKey.get_tag():
                        raise ValueError(f"Invalid crypto hd key tag {tag}")
                    keys.append(CryptoHDKey.decode(decoder))
                cma.set_keys(keys)
            elif key == DEVICE:
                cma.set_device(decoder.decodeText()[0])
            elif key == DEVICE_ID:
                cma.set_device_id(decoder.decodeText()[0])
            elif key == DEVICE_VERSION:
                cma.set_device_version(decoder.decodeText()[0])
            else:
                raise ValueError(f"Invalid key {key}")
        return cma


async def generate_crypto_multi_accounts(ctx: wire.Context) -> UREncoder:
    from trezor.messages import GetPublicKey
    from apps.bitcoin import get_public_key as bitcoin_get_public_key
    from apps.common import paths
    from storage import device
    from trezor import utils

    eth_pub = await bitcoin_get_public_key.get_public_key(
        ctx, GetPublicKey(address_n=paths.parse_path(helpers.ETH_STANDARD_PREFIX))
    )
    btc_legacy_pub = await bitcoin_get_public_key.get_public_key(
        ctx, GetPublicKey(address_n=paths.parse_path(helpers.BTC_LEGACY_PREFIX))
    )
    btc_segwit_pub = await bitcoin_get_public_key.get_public_key(
        ctx, GetPublicKey(address_n=paths.parse_path(helpers.BTC_SEGWIT_PREFIX))
    )
    btc_native_segwit_pub = await bitcoin_get_public_key.get_public_key(
        ctx, GetPublicKey(address_n=paths.parse_path(helpers.BTC_NATIVE_SEGWIT_PREFIX))
    )
    btc_taproot_pub = await bitcoin_get_public_key.get_public_key(
        ctx, GetPublicKey(address_n=paths.parse_path(helpers.BTC_TAPROOT_PREFIX))
    )
    assert eth_pub.root_fingerprint is not None, "Root fingerprint should not be None"
    name = helpers.reveal_name(ctx, eth_pub.root_fingerprint)
    cma = CryptoMultiAccounts(
        eth_pub.root_fingerprint,
        [
            helpers.generate_hdkey_ETHStandard(ctx, eth_pub, False),
            helpers.generate_hdkey_BTCLegacy(btc_legacy_pub),
            helpers.generate_hdkey_BTCSegWit(btc_segwit_pub),
            helpers.generate_hdkey_BTCNativeSegWit(btc_native_segwit_pub),
            helpers.generate_hdkey_BTCTaproot(btc_taproot_pub),
        ],
        device=name,
        device_id=device.get_device_id(),
        device_version=utils.ONEKEY_VERSION,
    )
    ur = cma.to_ur()
    encoder = UREncoder(ur)
    return encoder
