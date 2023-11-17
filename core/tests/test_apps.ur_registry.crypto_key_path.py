from common import *


if not utils.BITCOIN_ONLY:
    from apps.ur_registry import crypto_key_path
    from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder


@unittest.skipUnless(not utils.BITCOIN_ONLY, "altcoin")
class TestCryptoKeyPath(unittest.TestCase):
    def test_encode(self):
        paths = []
        paths.append(crypto_key_path.PathComponent.new(44, True))
        paths.append(crypto_key_path.PathComponent.new(118, True))
        paths.append(crypto_key_path.PathComponent.new(0, True))
        paths.append(crypto_key_path.PathComponent.new(0, False))
        paths.append(crypto_key_path.PathComponent.new(None, False))
        source_fingerprint = [120, 35, 8, 4]
        key_path = crypto_key_path.CryptoKeyPath(paths,source_fingerprint,5)
        cbor = key_path.cbor_encode()
        ur = key_path.ur_encode()
        encoded = UREncoder.encode(ur)
        self.assertEqual(hexlify(cbor).decode().upper(), "A3018A182CF51876F500F500F480F4021A782308040305")
        self.assertEqual(encoded, "ur:crypto-keypath/otadlecsdwykcskoykaeykaewklawkaocykscnayaaaxahrybsckoe")

        paths = []
        paths.append(crypto_key_path.PathComponent.new(44, True))
        paths.append(crypto_key_path.PathComponent.new(118, True))
        paths.append(crypto_key_path.PathComponent.new(0, True))
        paths.append(crypto_key_path.PathComponent.new(0, False))
        paths.append(crypto_key_path.PathComponent.new(0, False))
        source_fingerprint = [120, 35, 8, 4]
        key_path = crypto_key_path.CryptoKeyPath(paths,source_fingerprint,5)
        cbor = key_path.cbor_encode()
        ur = key_path.ur_encode()
        encoded = UREncoder.encode(ur)
        self.assertEqual(hexlify(cbor).decode().upper(), "A3018A182CF51876F500F500F400F4021A782308040305")
        self.assertEqual(encoded, "ur:crypto-keypath/otadlecsdwykcskoykaeykaewkaewkaocykscnayaaaxahhpbkchot")


    def test_decode(self):
        bytes = unhexlify("a3018a182cf51876f500f500f480f4021a782308040305")
        keypath = crypto_key_path.CryptoKeyPath.from_cbor(bytes)
        self.assertEqual(keypath.get_depth(), 5)
        self.assertEqual(keypath.get_source_fingerprint(), [120, 35, 8, 4])
        self.assertEqual(keypath.get_path(), "44'/118'/0'/0/*")

        bytes = unhexlify("a3018a182cf51876f500f500f400f4021a782308040305")
        keypath = crypto_key_path.CryptoKeyPath.from_cbor(bytes)
        keypath.from_cbor(bytes)
        self.assertEqual(keypath.get_path(), "44'/118'/0'/0/0")


if __name__ == '__main__':
    unittest.main()
