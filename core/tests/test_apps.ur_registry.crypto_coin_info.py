from common import *


if not utils.BITCOIN_ONLY:
    from apps.ur_registry import crypto_coin_info
    from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder


@unittest.skipUnless(not utils.BITCOIN_ONLY, "altcoin")
class TestCryptoCoinInfo(unittest.TestCase):
    def test_encode(self):
        info = crypto_coin_info.CryptoCoinInfo(0,1)
        info_cbor = info.cbor_encode()
        result = hexlify(bytes(info_cbor)).decode()
        self.assertEqual(result, "a201000201")

        ur = info.ur_encode()
        encoded = UREncoder.encode(ur)
        self.assertEqual(encoded, "ur:crypto-coin-info/oeadaeaoadehfdbany")


    def test_decode(self):
        bytes = unhexlify("a201000201")
        coin_info = crypto_coin_info.CryptoCoinInfo.from_cbor(bytes)
        self.assertEqual(coin_info.get_coin_type(), crypto_coin_info.Bitcoin)
        self.assertEqual(coin_info.get_network(), crypto_coin_info.TestNet)


if __name__ == '__main__':
    unittest.main()
