from ubinascii import hexlify

from .chains.chains import gen_extra_data, get_coin_name
from .crypto_hd_key import CryptoHDKey

ACCOUNT_RESP = '{{"chain":"{chain}","chain_code":"{chain_code}","extended_public_key":"{extended_public_key}","extra":{extra},"name":"{name}","note":"{note}","path":"{path}","public_key":"{public_key}","xfp":"{xfp}"}}'


class Account:
    def __init__(
        self,
        chain,
        path,
        public_key,
        name,
        chain_code,
        extended_public_key,
        note,
        xfp,
        extra,
    ):
        self.chain = chain
        self.path = path
        self.public_key = public_key
        self.name = name
        self.chain_code = chain_code
        self.extended_public_key = extended_public_key
        self.note = note
        self.xfp = xfp
        self.extra = extra

    @staticmethod
    def from_crypto_hd_key(hdkey: CryptoHDKey):
        origin = hdkey.get_origin()
        coin_type = origin.get_components()[1].get_index() if origin is not None else 0
        chain = get_coin_name(coin_type)
        path = origin.get_path() if origin is not None else ""
        public_key = hexlify(hdkey.get_key()).decode()
        name = hdkey.get_name().decode()
        chain_code = hexlify(hdkey.get_chain_code()).decode()
        extended_public_key = ""
        if (
            chain_code is not None
            and hdkey.get_parent_fingerprint() is not None
            and hdkey.get_origin() is not None
        ):
            extended_public_key = hdkey.get_bip32_key()
        note = hdkey.get_note().decode()
        source_fingerprint = (
            origin.get_source_fingerprint() if origin is not None else None
        )
        xfp = (
            hexlify(bytes(source_fingerprint)).decode()
            if source_fingerprint is not None
            else None
        )
        extra = gen_extra_data(coin_type)

        return Account(
            chain if chain is not None else chain,
            f"m/{path}" if path is not None else path,
            public_key if public_key is not None else public_key,
            name if name is not None else name,
            chain_code if chain_code is not None else chain_code,
            extended_public_key
            if extended_public_key is not None
            else extended_public_key,
            note if note is not None else note,
            xfp if xfp is not None else xfp,
            extra,
        )

    @staticmethod
    def parse_crypto_hd_key(ur_type, cbor):
        if CryptoHDKey.get_registry_type() != ur_type:
            return '{"error": "type not match"}'

        crypto_hd_key = CryptoHDKey.from_cbor(cbor)
        account = Account.from_crypto_hd_key(crypto_hd_key)
        resp = ACCOUNT_RESP.format(
            chain=account.chain,
            chain_code=account.chain_code,
            extended_public_key=account.extended_public_key,
            extra=account.extra,
            name=account.name,
            note=account.note,
            path=account.path,
            public_key=account.public_key,
            xfp=account.xfp,
        )
        return resp
