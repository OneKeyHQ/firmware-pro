import storage.cache
from trezor import messages, wire
from trezor.enums import InputScriptType, MessageType
from trezor.lvglui.scrs import lv
from trezor.lvglui.scrs.template import ADDRESS_OFFLINE_RETURN_TYPE
from trezor.ui.layouts import show_address_offline

from apps.workflow_handlers import find_registered_handler

chain_list = [
    (
        MessageType.GetAddress,
        {
            "symbol": " BTC",
            "name": "Bitcoin",
            "msg_class": "GetAddress",
            "index_pos": -3,
            "legacy_path": [0x80000000 + 44, 0x80000000 + 0, 0x80000000 + 0, 0, 0],
            "native_segwit_path": [
                0x80000000 + 84,
                0x80000000 + 0,
                0x80000000 + 0,
                0,
                0,
            ],
            "nested_segwit_path": [
                0x80000000 + 49,
                0x80000000 + 0,
                0x80000000 + 0,
                0,
                0,
            ],
            "taproot_path": [0x80000000 + 86, 0x80000000 + 0, 0x80000000 + 0, 0, 0],
            "icon_96": "A:/res/btc-btc.png",
            "icon_48": "A:/assets/addr/btc-btc-48.png",
            "primary_color": 0xFF9C00,
        },
    ),
    (
        MessageType.EthereumGetAddressOneKey,
        {
            "symbol": " ETH",
            "name": "Ethereum",
            "msg_class": "EthereumGetAddressOneKey",
            "index_pos": -1,
            "ledger_pos": -3,
            "base_path": [0x80000000 + 44, 0x80000000 + 60, 0x80000000 + 0, 0, 0],
            "icon_96": "A:/res/evm-eth.png",
            "icon_48": "A:/assets/addr/evm-eth-48.png",
            "primary_color": 0x637FFF,
        },
    ),
    (
        MessageType.SolanaGetAddress,
        {
            "symbol": " SOL",
            "name": "Solana",
            "msg_class": "SolanaGetAddress",
            "index_pos": -2,
            "base_path": [
                0x80000000 + 44,
                0x80000000 + 501,
                0x80000000 + 0,
                0x80000000 + 0,
            ],
            "icon_96": "A:/res/chain-sol.png",
            "icon_48": "A:/assets/addr/chain-sol-48.png",
            "primary_color": 0xC74AE3,
        },
    ),
    (
        MessageType.CardanoGetAddress,
        {
            "symbol": " ADA",
            "name": "Cardano",
            "msg_class": "CardanoGetAddress",
            "index_pos": -3,
            "base_path": [0x80000000 + 1852, 0x80000000 + 1815, 0x80000000 + 0, 0, 0],
            "staking_path": [
                0x80000000 + 1852,
                0x80000000 + 1815,
                0x80000000 + 0,
                2,
                0,
            ],
            "protocol_magic": 764824073,
            "network_id": 1,
            "icon_96": "A:/res/chain-ada.png",
            "icon_48": "A:/assets/addr/chain-ada-48.png",
            "primary_color": 0x2970FF,
        },
    ),
    (
        MessageType.TronGetAddress,
        {
            "symbol": " TRX",
            "name": "Tron",
            "msg_class": "TronGetAddress",
            "index_pos": -1,
            "base_path": [0x80000000 + 44, 0x80000000 + 195, 0x80000000 + 0, 0, 0],
            "icon_96": "A:/res/chain-tron.png",
            "icon_48": "A:/assets/addr/chain-tron-48.png",
            "primary_color": 0xFF0013,
        },
    ),
    (
        MessageType.TonGetAddress,
        {
            "symbol": " TON",
            "name": "TON",
            "msg_class": "TonGetAddress",
            "index_pos": -1,
            "base_path": [0x80000000 + 44, 0x80000000 + 607, 0x80000000 + 0],
            "icon_96": "A:/res/chain-ton.png",
            "icon_48": "A:/assets/addr/chain-ton-48.png",
            "primary_color": 0x0098EA,
        },
    ),
    (
        MessageType.SuiGetAddress,
        {
            "symbol": " SUI",
            "name": "Sui",
            "msg_class": "SuiGetAddress",
            "index_pos": -3,
            "base_path": [
                0x80000000 + 44,
                0x80000000 + 784,
                0x80000000 + 0,
                0x80000000 + 0,
                0x80000000 + 0,
            ],
            "icon_96": "A:/res/chain-sui.png",
            "icon_48": "A:/assets/addr/chain-sui-48.png",
            "primary_color": 0x6FBCF0,
        },
    ),
    (
        MessageType.PolkadotGetAddress,
        {
            "symbol": " DOT",
            "name": "Polkadot",
            "msg_class": "PolkadotGetAddress",
            "index_pos": -3,
            "base_path": [
                0x80000000 + 44,
                0x80000000 + 354,
                0x80000000 + 0,
                0x80000000 + 0,
                0x80000000 + 0,
            ],
            "icon_96": "A:/res/chain-dot.png",
            "icon_48": "A:/assets/addr/chain-dot-48.png",
            "primary_color": 0xE6007A,
        },
    ),
    (
        MessageType.NearGetAddress,
        {
            "symbol": " NEAR",
            "name": "NEAR",
            "msg_class": "NearGetAddress",
            "index_pos": -1,
            "base_path": [0x80000000 + 44, 0x80000000 + 397, 0x80000000 + 0],
            "icon_96": "A:/res/chain-near.png",
            "icon_48": "A:/assets/addr/chain-near-48.png",
            "primary_color": 0xFFFFFF,
        },
    ),
    (
        MessageType.AptosGetAddress,
        {
            "symbol": " APT",
            "name": "Aptos",
            "msg_class": "AptosGetAddress",
            "index_pos": -3,
            "base_path": [
                0x80000000 + 44,
                0x80000000 + 637,
                0x80000000 + 0,
                0x80000000 + 0,
                0x80000000 + 0,
            ],
            "icon_96": "A:/res/chain-apt.png",
            "icon_48": "A:/assets/addr/chain-apt-48.png",
            "primary_color": 0xFFFFFF,
        },
    ),
    (
        MessageType.KaspaGetAddress,
        {
            "symbol": " KAS",
            "name": "Kaspa",
            "msg_class": "KaspaGetAddress",
            "index_pos": -1,
            "base_path": [0x80000000 + 44, 0x80000000 + 111111, 0x80000000 + 0, 0, 0],
            "icon_96": "A:/res/chain-kaspa.png",
            "icon_48": "A:/assets/addr/chain-kaspa-48.png",
            "primary_color": 0x55ECC7,
        },
    ),
    (
        MessageType.FilecoinGetAddress,
        {
            "symbol": " FIL",
            "name": "Filecoin",
            "msg_class": "FilecoinGetAddress",
            "index_pos": -1,
            "base_path": [0x80000000 + 44, 0x80000000 + 461, 0x80000000 + 0, 0, 0],
            "icon_96": "A:/res/chain-fil.png",
            "icon_48": "A:/assets/addr/chain-fil-48.png",
            "primary_color": 0x0090FF,
        },
    ),
    (
        MessageType.AlephiumGetAddress,
        {
            "symbol": " ALPH",
            "name": "Alephium",
            "msg_class": "AlephiumGetAddress",
            "index_pos": -3,
            "base_path": [0x80000000 + 44, 0x80000000 + 1234, 0x80000000 + 0, 0, 0],
            "icon_96": "A:/res/chain-alephium.png",
            "icon_48": "A:/assets/addr/chain-alephium-48.png",
            "primary_color": 0xFFFFFF,
        },
    ),
    (
        MessageType.AlgorandGetAddress,
        {
            "symbol": " ALGO",
            "name": "Algorand",
            "msg_class": "AlgorandGetAddress",
            "index_pos": -1,
            "base_path": [
                0x80000000 + 44,
                0x80000000 + 283,
                0x80000000 + 0,
                0x80000000 + 0,
                0x80000000 + 0,
            ],
            "icon_96": "A:/res/chain-algo.png",
            "icon_48": "A:/assets/addr/chain-algo-48.png",
            "primary_color": 0xFFFFFF,
        },
    ),
    (
        MessageType.ConfluxGetAddress,
        {
            "symbol": " CFX",
            "name": "Conflux",
            "msg_class": "ConfluxGetAddress",
            "index_pos": -1,
            "base_path": [0x80000000 + 44, 0x80000000 + 503, 0x80000000 + 0, 0, 0],
            "icon_96": "A:/res/chain-cfx.png",
            "icon_48": "A:/assets/addr/chain-cfx-48.png",
            "primary_color": 0xFFFFFF,
        },
    ),
    (
        MessageType.CosmosGetAddress,
        {
            "symbol": " ATOM",
            "name": "Cosmos",
            "msg_class": "CosmosGetAddress",
            "index_pos": -1,
            "base_path": [0x80000000 + 44, 0x80000000 + 118, 0x80000000 + 0, 0, 0],
            "icon_96": "A:/res/chain-atom.png",
            "icon_48": "A:/assets/addr/chain-atom-48.png",
            "primary_color": 0xE0E0E0,
        },
    ),
    (
        MessageType.NervosGetAddress,
        {
            "symbol": " CKB",
            "name": "Nervos",
            "msg_class": "NervosGetAddress",
            "index_pos": -1,
            "base_path": [0x80000000 + 44, 0x80000000 + 309, 0x80000000 + 0, 0, 0],
            "icon_96": "A:/res/chain-nervos.png",
            "icon_48": "A:/assets/addr/chain-nervos-48.png",
            "primary_color": 0xFFFFFF,
        },
    ),
    (
        MessageType.NexaGetAddress,
        {
            "symbol": " NEXA",
            "name": "Nexa",
            "msg_class": "NexaGetAddress",
            "index_pos": -3,
            "base_path": [0x80000000 + 44, 0x80000000 + 29223, 0x80000000 + 0, 0, 0],
            "icon_96": "A:/res/chain-nexa.png",
            "icon_48": "A:/assets/addr/chain-nexa-48.png",
            "primary_color": 0xFFE144,
        },
    ),
    (
        MessageType.NostrGetPublicKey,
        {
            "symbol": " NOSTR",
            "name": "Nostr",
            "msg_class": "NostrGetPublicKey",
            "index_pos": -3,
            "base_path": [0x80000000 + 44, 0x80000000 + 1237, 0x80000000 + 0, 0, 0],
            "icon_96": "A:/res/chain-nostr.png",
            "icon_48": "A:/assets/addr/chain-nostr-48.png",
            "primary_color": 0x8D45DD,
        },
    ),
    (
        MessageType.RippleGetAddress,
        {
            "symbol": " XRP",
            "name": "Ripple",
            "msg_class": "RippleGetAddress",
            "index_pos": -3,
            "base_path": [0x80000000 + 44, 0x80000000 + 144, 0x80000000 + 0, 0, 0],
            "icon_96": "A:/res/chain-xrp.png",
            "icon_48": "A:/assets/addr/chain-xrp-48.png",
            "primary_color": 0xFFFFFF,
        },
    ),
    (
        MessageType.ScdoGetAddress,
        {
            "symbol": " SCDO",
            "name": "SCDO",
            "msg_class": "ScdoGetAddress",
            "index_pos": -3,
            "base_path": [0x80000000 + 44, 0x80000000 + 541, 0x80000000 + 0, 0, 0],
            "icon_96": "A:/res/chain-scdo.png",
            "icon_48": "A:/assets/addr/chain-scdo-48.png",
            "primary_color": 0xFFFFFF,
        },
    ),
]


class AddressManager:
    """Core logic for handling address generation and display"""

    class STATE:
        INIT = 0
        SHOW_ADDRESS = 1
        HANDLE_RESPONSE = 2
        UPDATE_ADDRESS = 3
        FINISH = 4
        ERROR = 5

    def __init__(self):
        self.current_handler = None
        self.current_chain_info = None
        self.current_index = 0
        self.btc_script_type = None
        self.use_ledger_path = None
        self.user_interaction = None
        self.addr_type = None
        self.prev_session_id = None

    def get_chain_info(self, msg_type) -> dict:
        """Get chain configuration information"""
        for mt, info in chain_list:
            if mt == msg_type:
                return info
        raise wire.DataError("Chain info not found")

    def _prepare_btc_message(self):
        """Prepare Bitcoin message"""
        if self.current_chain_info is None:
            raise wire.DataError("Chain info is None")

        if self.btc_script_type == InputScriptType.SPENDP2SHWITNESS:
            path = self.current_chain_info["nested_segwit_path"][:]
            self.addr_type = "Nested Segwit"
        elif self.btc_script_type == InputScriptType.SPENDTAPROOT:
            path = self.current_chain_info["taproot_path"][:]
            self.addr_type = "Taproot"
        elif self.btc_script_type == InputScriptType.SPENDWITNESS:
            path = self.current_chain_info["native_segwit_path"][:]
            self.addr_type = "Native Segwit"
        elif self.btc_script_type == InputScriptType.SPENDADDRESS:
            path = self.current_chain_info["legacy_path"][:]
            self.addr_type = "Legacy"
        else:
            self.btc_script_type = InputScriptType.SPENDTAPROOT
            path = self.current_chain_info["taproot_path"][:]
            self.addr_type = "Taproot"
        pos = self.current_chain_info["index_pos"]
        path[pos] += self.current_index

        msg_class = getattr(messages, self.current_chain_info["msg_class"])
        return msg_class(
            address_n=path, show_display=False, script_type=self.btc_script_type
        )

    def _prepare_eth_message(self):
        """Prepare Ethereum message"""
        if self.current_chain_info is None:
            raise wire.DataError("Chain info is None")

        path = self.current_chain_info["base_path"][:]
        if self.use_ledger_path is True:
            pos = self.current_chain_info["ledger_pos"]
            self.addr_type = "Ledger Live"
        else:
            pos = self.current_chain_info["index_pos"]
            self.addr_type = "BIP44 Standard"
        path[pos] += self.current_index
        msg_class = getattr(messages, self.current_chain_info["msg_class"])
        return msg_class(address_n=path, show_display=False)

    def _prepare_ada_message(self):
        """Prepare Cardano message"""
        if self.current_chain_info is None:
            raise wire.DataError("Chain info is None")

        path = self.current_chain_info["base_path"][:]
        staking_path = self.current_chain_info["staking_path"][:]
        pos = self.current_chain_info["index_pos"]
        path[pos] += self.current_index
        staking_path[pos] += self.current_index
        self.addr_type = "BIP44 Standard"

        from trezor.messages import CardanoAddressParametersType
        from trezor.enums import CardanoDerivationType, CardanoAddressType

        address_parameters = CardanoAddressParametersType(
            address_type=CardanoAddressType.BASE,
            address_n=path,
            address_n_staking=staking_path,
        )
        derivation_type = CardanoDerivationType.ICARUS

        storage.cache.end_current_session()
        self.curr_session_id = storage.cache.start_session()
        storage.cache.SESSION_DIRIVE_CARDANO = True

        msg_class = getattr(messages, self.current_chain_info["msg_class"])
        return msg_class(
            show_display=False,
            protocol_magic=764824073,
            network_id=1,
            address_parameters=address_parameters,
            derivation_type=derivation_type,
        )

    def _prepare_dot_message(self):
        """Prepare Polkadot message"""
        if self.current_chain_info is None:
            raise wire.DataError("Chain info is None")

        path = self.current_chain_info["base_path"][:]
        pos = self.current_chain_info["index_pos"]
        path[pos] += self.current_index
        self.addr_type = "BIP44 Standard"

        msg_class = getattr(messages, self.current_chain_info["msg_class"])
        return msg_class(
            address_n=path, show_display=False, network="polkadot", prefix=0
        )

    def _prepare_ckb_message(self):
        """Prepare Nervos message"""
        if self.current_chain_info is None:
            raise wire.DataError("Chain info is None")

        path = self.current_chain_info["base_path"][:]
        pos = self.current_chain_info["index_pos"]
        path[pos] += self.current_index
        self.addr_type = "BIP44 Standard"

        msg_class = getattr(messages, self.current_chain_info["msg_class"])
        return msg_class(address_n=path, show_display=False, network="ckb")

    def _prepare_alephium_message(self):
        """Prepare Alephium message"""
        if self.current_chain_info is None:
            raise wire.DataError("Chain info is None")

        path = self.current_chain_info["base_path"][:]
        pos = self.current_chain_info["index_pos"]
        path[pos] += self.current_index
        self.addr_type = "BIP44 Standard"

        msg_class = getattr(messages, self.current_chain_info["msg_class"])
        return msg_class(address_n=path, show_display=False, target_group=0)

    def _prepare_nexa_message(self):
        """Prepare Nexa message"""
        if self.current_chain_info is None:
            raise wire.DataError("Chain info is None")

        path = self.current_chain_info["base_path"][:]
        pos = self.current_chain_info["index_pos"]
        path[pos] += self.current_index
        self.addr_type = "BIP44 Standard"

        msg_class = getattr(messages, self.current_chain_info["msg_class"])
        return msg_class(address_n=path, show_display=False, prefix="nexa")

    def _prepare_default_message(self):
        """Prepare default chain message"""
        if self.current_chain_info is None:
            raise wire.DataError("Chain info is None")

        path = self.current_chain_info["base_path"][:]
        pos = self.current_chain_info["index_pos"]
        path[pos] += self.current_index
        self.addr_type = "BIP44 Standard"

        msg_class = getattr(messages, self.current_chain_info["msg_class"])
        return msg_class(address_n=path, show_display=False)

    def _prepare_chain_message(self, msg_type):
        """Prepare message object based on chain type"""

        if msg_type == MessageType.GetAddress:
            return self._prepare_btc_message()
        elif msg_type == MessageType.EthereumGetAddressOneKey:
            return self._prepare_eth_message()
        elif msg_type == MessageType.CardanoGetAddress:
            return self._prepare_ada_message()
        elif msg_type == MessageType.PolkadotGetAddress:
            return self._prepare_dot_message()
        elif msg_type == MessageType.NervosGetAddress:
            return self._prepare_ckb_message()
        elif msg_type == MessageType.AlephiumGetAddress:
            return self._prepare_alephium_message()
        else:
            return self._prepare_default_message()

    async def generate_address(self, msg_type, index=0, ctx=None):
        """Main logic for generating addresses"""

        ctx = wire.DummyContext()

        self.current_index = index
        state = self.STATE.INIT

        while state not in (self.STATE.FINISH, self.STATE.ERROR):

            # pyright: off
            if state == self.STATE.INIT:
                handler = find_registered_handler(None, msg_type)
                chain_info = self.get_chain_info(msg_type)

                self.current_handler = handler
                self.current_chain_info = chain_info

                state = self.STATE.SHOW_ADDRESS

            elif state == self.STATE.SHOW_ADDRESS:
                # Generate message
                msg = self._prepare_chain_message(msg_type)

                address_resp = await self.current_handler(ctx, msg)

                ctx.primary_color = lv.color_hex(
                    self.current_chain_info["primary_color"]
                )
                ctx.icon_path = self.current_chain_info["icon_96"]

                self.user_interaction = await show_address_offline(
                    ctx,
                    address=address_resp.address
                    if msg_type != MessageType.NostrGetPublicKey
                    else address_resp.npub,
                    network=self.current_chain_info["name"],
                    addr_type=self.addr_type,
                    account_name=f" Account #{self.current_index + 1}",
                )
                state = self.STATE.HANDLE_RESPONSE

            elif state == self.STATE.HANDLE_RESPONSE:
                # Handle user interaction results
                if isinstance(self.user_interaction, tuple):
                    return_type, value = self.user_interaction
                    if return_type == ADDRESS_OFFLINE_RETURN_TYPE.ETH_LEDGER_PATH:
                        self.use_ledger_path = value
                        state = self.STATE.SHOW_ADDRESS
                    elif return_type == ADDRESS_OFFLINE_RETURN_TYPE.BTC_DERIVE_SCRIPTS:
                        self.btc_script_type = value
                        state = self.STATE.SHOW_ADDRESS
                elif self.user_interaction == ADDRESS_OFFLINE_RETURN_TYPE.DONE:
                    state = self.STATE.FINISH
                else:
                    state = self.STATE.ERROR
            # pyright: on

        # Clean up
        self.cleanup()

    def cleanup(self):
        """Clean up state"""
        self.current_handler = None
        self.current_chain_info = None
        self.user_interaction = None
        self.addr_type = None
