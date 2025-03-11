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
            from trezor.messages import HDNodeType, BatchGetPublickeys, Path, PublicKey
            from apps.common import paths as paths_utils
            from apps.common.keychain import get_keychain
            from apps.misc.batch_get_pubkeys import validate
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
                if chain.lower() in ("eth", "btc", "tbtc", "sbtc", "sol"):
                    if chain.lower() in ("btc", "tbtc", "sbtc"):
                        coin_type = crypto_coin_info.Bitcoin
                    elif chain.lower() in ("sol",):
                        coin_type = crypto_coin_info.Solana
                    else:
                        coin_type = crypto_coin_info.Ethereum
                    network = (
                        crypto_coin_info.TestNet
                        if chain.lower() in ("tbtc", "sbtc")
                        else crypto_coin_info.MainNet
                    )
                    coin_info = crypto_coin_info.CryptoCoinInfo(
                        coin_type,
                        network,
                    )
                    if coin_type == crypto_coin_info.Solana:
                        curve_name = "ed25519"
                    else:
                        curve_name = "secp256k1"
                    validate(
                        BatchGetPublickeys(
                            ecdsa_curve_name=curve_name,
                            paths=[
                                Path(address_n=paths_utils.parse_path(path))
                                for path in paths
                            ],
                        )
                    )
                    # pyright: off
                    if curve_name == "ed25519":
                        root_fingerprint = (
                            await get_keychain(
                                wire.QR_CONTEXT,
                                "secp256k1",
                                [paths_utils.AlwaysMatchingSchema],
                            )
                        ).root_fingerprint()
                    else:
                        root_fingerprint = None
                    keychain = await get_keychain(
                        wire.QR_CONTEXT,
                        curve_name,
                        [paths_utils.AlwaysMatchingSchema],
                    )
                    if root_fingerprint is None:
                        root_fingerprint = keychain.root_fingerprint()
                    # pyright: on
                    for path in paths:
                        node = keychain.derive(paths_utils.parse_path(path))
                        pub_key = PublicKey(
                            node=HDNodeType(
                                depth=node.depth(),
                                child_num=node.child_num(),
                                fingerprint=node.fingerprint(),
                                chain_code=node.chain_code(),
                                public_key=node.public_key(),
                            ),
                            xpub="",
                            root_fingerprint=root_fingerprint,
                        )
                        if coin_type == crypto_coin_info.Solana:
                            hdkey = helpers.generate_HDKey_ED25519(
                                pub_key.node.public_key[1:],
                                path,
                            )
                        else:
                            hdkey = helpers.generate_HDKey(
                                pub_key,
                                coin_info,
                                path,
                                root_fingerprint,
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
