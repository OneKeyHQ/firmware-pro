from micropython import const
from typing import TYPE_CHECKING

from trezor.enums import MessageType

if TYPE_CHECKING:
    from trezor import protobuf

    from trezor.messages import (
        StellarAccountMergeOp,
        StellarAllowTrustOp,
        StellarBumpSequenceOp,
        StellarChangeTrustOp,
        StellarCreateAccountOp,
        StellarCreatePassiveSellOfferOp,
        StellarManageDataOp,
        StellarManageBuyOfferOp,
        StellarManageSellOfferOp,
        StellarPathPaymentStrictReceiveOp,
        StellarPathPaymentStrictSendOp,
        StellarPaymentOp,
        StellarSetOptionsOp,
        StellarInvokeHostFunctionOp,
    )

    StellarMessageType = (
        StellarAccountMergeOp
        | StellarAllowTrustOp
        | StellarBumpSequenceOp
        | StellarChangeTrustOp
        | StellarCreateAccountOp
        | StellarCreatePassiveSellOfferOp
        | StellarManageDataOp
        | StellarManageBuyOfferOp
        | StellarManageSellOfferOp
        | StellarPathPaymentStrictReceiveOp
        | StellarPathPaymentStrictSendOp
        | StellarPaymentOp
        | StellarSetOptionsOp
        | StellarInvokeHostFunctionOp
    )


TX_TYPE = b"\x00\x00\x00\x02"

# source: https://github.com/stellar/go/blob/a1db2a6b1f/xdr/Stellar-transaction.x#L35
# Inflation not supported see https://github.com/trezor/trezor-core/issues/202#issuecomment-393342089
op_codes: dict[int, int] = {
    MessageType.StellarAccountMergeOp: 8,
    MessageType.StellarAllowTrustOp: 7,
    MessageType.StellarBumpSequenceOp: 11,
    MessageType.StellarChangeTrustOp: 6,
    MessageType.StellarCreateAccountOp: 0,
    MessageType.StellarCreatePassiveSellOfferOp: 4,
    MessageType.StellarManageDataOp: 10,
    MessageType.StellarManageBuyOfferOp: 12,
    MessageType.StellarManageSellOfferOp: 3,
    MessageType.StellarPathPaymentStrictReceiveOp: 2,
    MessageType.StellarPathPaymentStrictSendOp: 13,
    MessageType.StellarPaymentOp: 1,
    MessageType.StellarSetOptionsOp: 5,
    MessageType.StellarInvokeHostFunctionOp: 24,
}

op_wire_types = [
    MessageType.StellarAccountMergeOp,
    MessageType.StellarAllowTrustOp,
    MessageType.StellarBumpSequenceOp,
    MessageType.StellarChangeTrustOp,
    MessageType.StellarCreateAccountOp,
    MessageType.StellarCreatePassiveSellOfferOp,
    MessageType.StellarManageDataOp,
    MessageType.StellarManageBuyOfferOp,
    MessageType.StellarManageSellOfferOp,
    MessageType.StellarPathPaymentStrictReceiveOp,
    MessageType.StellarPathPaymentStrictSendOp,
    MessageType.StellarPaymentOp,
    MessageType.StellarSetOptionsOp,
    MessageType.StellarInvokeHostFunctionOp,
]

# https://github.com/stellar/go/blob/e0ffe19f58879d3c31e2976b97a5bf10e13a337b/xdr/xdr_generated.go#L584
ASSET_TYPE_NATIVE = const(0)
ASSET_TYPE_ALPHANUM4 = const(1)
ASSET_TYPE_ALPHANUM12 = const(2)

# https://www.stellar.org/developers/guides/concepts/accounts.html#balance
# https://github.com/stellar/go/blob/3d2c1defe73dbfed00146ebe0e8d7e07ce4bb1b6/amount/main.go#L23
AMOUNT_DECIMALS = const(7)

# https://github.com/stellar/go/blob/master/network/main.go
NETWORK_PASSPHRASE_PUBLIC = "Public Global Stellar Network ; September 2015"
NETWORK_PASSPHRASE_TESTNET = "Test SDF Network ; September 2015"

# https://www.stellar.org/developers/guides/concepts/accounts.html#flags
FLAG_AUTH_REQUIRED = const(1)
FLAG_AUTH_REVOCABLE = const(2)
FLAG_AUTH_IMMUTABLE = const(4)
FLAGS_MAX_SIZE = const(7)
STELLAR_KEY_TYPE_ED25519 = const(0)
STELLAR_KEY_TYPE_CONTRACT = const(1)
STELLAR_STRKEY_VERSION_CONTRACT = const(0x10)
STELLAR_STRKEY_VERSION_ED25519_PUBLIC_KEY = const(0x30)
STELLAR_HOST_FUNCTION_TYPE_INVOKE_CONTRACT = const(0)
STELLAR_TX_EXT_SOROBAN = const(1)


def get_op_code(msg: protobuf.MessageType) -> int:
    wire = msg.MESSAGE_WIRE_TYPE
    if wire not in op_codes:
        raise ValueError("Stellar: op code unknown")
    assert isinstance(wire, int)
    return op_codes[wire]
