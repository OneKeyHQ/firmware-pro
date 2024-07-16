from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from apps.common.keychain import Keychain
    from apps.common.paths import Bip32Path

    from trezor.enums import MoneroNetworkType

    from .xmr.crypto import Scalar
    from .xmr.credentials import AccountCreds


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
