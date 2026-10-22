from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from apps.common.keychain import Keychain
    from apps.common.paths import Bip32Path

    from trezor.enums import MoneroNetworkType

    from .xmr.crypto import Scalar
    from .xmr.credentials import AccountCreds

_XMR_SE_INPUT_TOKEN_MAGIC = b"XMRSE01"
_XMR_SE_INPUT_TOKEN_LEN = len(_XMR_SE_INPUT_TOKEN_MAGIC) + 32 + 4 + 32 + 32


def encode_xmr_se_input_token(
    recv_deriv: bytes, real_idx: int, subaddr_sk: bytes, out_key: bytes
) -> bytearray:
    if len(recv_deriv) != 32 or len(subaddr_sk) != 32 or len(out_key) != 32:
        raise ValueError("Invalid XMR SE token key length")
    if real_idx < 0 or real_idx > 0xFFFFFFFF:
        raise ValueError("Invalid XMR output index")

    token = bytearray(_XMR_SE_INPUT_TOKEN_LEN)
    offset = 0
    token[offset : offset + len(_XMR_SE_INPUT_TOKEN_MAGIC)] = _XMR_SE_INPUT_TOKEN_MAGIC
    offset += len(_XMR_SE_INPUT_TOKEN_MAGIC)
    token[offset : offset + 32] = recv_deriv
    offset += 32
    token[offset : offset + 4] = bytes(
        (
            real_idx & 0xFF,
            (real_idx >> 8) & 0xFF,
            (real_idx >> 16) & 0xFF,
            (real_idx >> 24) & 0xFF,
        )
    )
    offset += 4
    token[offset : offset + 32] = subaddr_sk
    offset += 32
    token[offset : offset + 32] = out_key
    return token


def decode_xmr_se_input_token(token: bytes) -> tuple[bytes, int, bytes, bytes]:
    if len(token) != _XMR_SE_INPUT_TOKEN_LEN:
        raise ValueError("Invalid XMR SE token length")
    if token[: len(_XMR_SE_INPUT_TOKEN_MAGIC)] != _XMR_SE_INPUT_TOKEN_MAGIC:
        raise ValueError("Invalid XMR SE token magic")

    offset = len(_XMR_SE_INPUT_TOKEN_MAGIC)
    recv_deriv = token[offset : offset + 32]
    offset += 32
    real_idx = (
        token[offset]
        | (token[offset + 1] << 8)
        | (token[offset + 2] << 16)
        | (token[offset + 3] << 24)
    )
    offset += 4
    subaddr_sk = token[offset : offset + 32]
    offset += 32
    out_key = token[offset : offset + 32]
    return recv_deriv, real_idx, subaddr_sk, out_key


def xmr_subaddress_secret_key(
    view_key_private: Scalar, received_index: tuple[int, int]
) -> bytes:
    if received_index == (0, 0):
        return b"\x00" * 32

    from apps.monero.xmr import crypto_helpers, monero

    return crypto_helpers.encodeint(
        monero.get_subaddress_secret_key(
            view_key_private,
            major=received_index[0],
            minor=received_index[1],
        )
    )


def get_creds(
    keychain: Keychain, address_n: Bip32Path, network_type: MoneroNetworkType
) -> AccountCreds:
    from apps.monero.xmr import crypto_helpers, monero
    from apps.monero.xmr.credentials import AccountCreds
    from trezor import utils

    if utils.USE_THD89:
        from trezor.crypto import se_thd89

        fake_spend_sec = b"\x00" * 32
        pubkey, hash = se_thd89.derive_xmr(address_n)
        spend_sec = crypto_helpers.decodeint(fake_spend_sec)
        spend_pub = crypto_helpers.decodepoint(pubkey)

        view_sec, view_pub = monero.generate_keys(crypto_helpers.decodeint(hash))
        creds = AccountCreds.new_wallet_ex(
            view_sec, spend_sec, view_pub, spend_pub, network_type
        )
        return creds
    else:
        node = keychain.derive(address_n)

        key_seed = node.private_key()
        spend_sec, _, view_sec, _ = monero.generate_monero_keys(key_seed)

        creds = AccountCreds.new_wallet(view_sec, spend_sec, network_type)
        return creds


def compute_tx_key(
    spend_key_private: Scalar,
    tx_prefix_hash: bytes,
    salt: bytes,
    rand_mult_num: Scalar,
) -> bytes:
    from apps.monero.xmr import crypto, crypto_helpers
    from trezor import utils

    if utils.USE_THD89:
        from trezor.crypto import se_thd89

        passwd = se_thd89.xmr_get_tx_key(
            crypto_helpers.encodeint(rand_mult_num), tx_prefix_hash
        )
    else:
        rand_inp = crypto.sc_add_into(None, spend_key_private, rand_mult_num)
        passwd = crypto_helpers.keccak_2hash(
            crypto_helpers.encodeint(rand_inp) + tx_prefix_hash
        )
    tx_key = crypto_helpers.compute_hmac(salt, passwd)
    return tx_key


def compute_enc_key_host(
    view_key_private: Scalar, tx_prefix_hash: bytes
) -> tuple[bytes, bytes]:
    from trezor.crypto import random
    from apps.monero.xmr import crypto_helpers

    salt = random.bytes(32)
    passwd = crypto_helpers.keccak_2hash(
        crypto_helpers.encodeint(view_key_private) + tx_prefix_hash
    )
    tx_key = crypto_helpers.compute_hmac(salt, passwd)
    return tx_key, salt
