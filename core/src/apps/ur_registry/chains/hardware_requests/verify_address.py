from .hardware_call import HardwareCall


class VerifyAddressRequest:
    def __init__(self, req: HardwareCall):
        self.req = req
        self.qr = None
        self.encoder = None

    async def run(self):
        from trezor import wire, messages
        from apps.common import paths

        params = self.req.get_params()[0]
        if any(key not in params for key in ("chain", "path", "address")):
            raise ValueError("Invalid param")
        if params["chain"] == "ETH":
            from apps.ethereum.onekey.get_address import get_address as eth_get_address

            if "chainId" not in params:
                raise ValueError("Invalid param")
            msg = messages.EthereumGetAddressOneKey(
                address_n=paths.parse_path(params["path"]),
                show_display=True,
                chain_id=int(params["chainId"]),
            )
            # pyright: off
            await eth_get_address(wire.QR_CONTEXT, msg)
            # pyright: on
        elif params["chain"] in ["BTC", "TBTC", "SBTC"]:
            from apps.bitcoin.get_address import get_address as btc_get_address

            if "scriptType" not in params:
                raise ValueError("Invalid param")
            # pyright: off
            msg = messages.GetAddress(
                address_n=paths.parse_path(params["path"]),
                show_display=True,
                script_type=int(params["scriptType"]),
                coin_name="Bitcoin" if params["chain"] == "BTC" else "Testnet",
            )
            await btc_get_address(wire.QR_CONTEXT, msg)
            # pyright: on
        else:
            raise ValueError("Invalid chain")
        # assert address.address is not None, "Address should not be None"
        # if address.address.lower() != params["address"].lower():
        #     if __debug__:
        #         print(f"Address mismatch: {address.address} != {params['address']}")
        #     else:
        #         raise ValueError("Address mismatch")
