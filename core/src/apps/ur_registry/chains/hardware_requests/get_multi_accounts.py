from .hardware_call import HardwareCall


class GetMultiAccountsRequest:
    def __init__(self, req: HardwareCall):
        self.req = req
        self.qr = None
        self.encoder = None

    async def run(self):
        from trezor import wire

        params = self.req.get_params()
        if not params:
            raise ValueError("Invalid param")
        else:
            from apps.ur_registry.crypto_multi_accounts import CryptoMultiAccounts
            from trezor.messages import GetPublicKey
            from apps.bitcoin import get_public_key as bitcoin_get_public_key
            from apps.common import paths as paths_utils
            from storage import device
            from trezor import utils
            from apps.ur_registry import helpers, crypto_coin_info
            from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder

            keys = []
            root_fingerprint = None
            if any(key not in param for key in ("paths", "chain") for param in params):
                raise ValueError("Invalid param")
            for param in params:
                chain = param["chain"]
                paths = param["paths"]
                if chain in ("ETH", "BTC", "TBTC", "SBTC"):
                    coin_info = crypto_coin_info.CryptoCoinInfo(
                        crypto_coin_info.Bitcoin
                        if chain in ["BTC", "TBTC", "SBTC"]
                        else crypto_coin_info.Ethereum,
                        crypto_coin_info.MainNet
                        if chain in ["BTC", "ETH"]
                        else crypto_coin_info.TestNet,
                    )
                    for path in paths:
                        # pyright: off
                        pub_key = await bitcoin_get_public_key.get_public_key(
                            wire.QR_CONTEXT,
                            GetPublicKey(
                                address_n=paths_utils.parse_path(path),
                                coin_name="Bitcoin"
                                if chain in ["BTC", "ETH"]
                                else "Testnet",
                            ),
                        )
                        # pyright: on
                        if not root_fingerprint:
                            root_fingerprint = pub_key.root_fingerprint
                        hdkey = helpers.generate_HDKey(
                            pub_key,
                            coin_info,
                            path,
                            pub_key.root_fingerprint,
                            None,
                            None,
                        )
                        keys.append(hdkey)
            assert root_fingerprint is not None, "Root fingerprint should not be None"
            name = helpers.reveal_name(wire.QR_CONTEXT, root_fingerprint)
            cma = CryptoMultiAccounts(
                root_fingerprint,
                keys,
                device=name,
                device_id=device.get_device_id(),
                device_version=utils.ONEKEY_VERSION,
            )
            ur = cma.to_ur()
            encoder = UREncoder(ur)
            if encoder.is_single_part():
                self.qr = encoder.next_part()
                self.encoder = None
            else:
                self.encoder = encoder
                self.qr = None
