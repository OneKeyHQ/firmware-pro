from common import *


if not utils.BITCOIN_ONLY:
    from apps.ur_registry import crypto_hd_key
    from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder
    from apps.ur_registry.crypto_key_path import PathComponent, CryptoKeyPath
    from apps.ur_registry.crypto_coin_info import CryptoCoinInfo, TestNet, Bitcoin


@unittest.skipUnless(not utils.BITCOIN_ONLY, "altcoin")
class TestCryptoHdPKeyEncode(unittest.TestCase):
    def test_encode(self):
        # master_key
        key="0x00e8f32e723decf4051aefac8e2c93c9c5b214313817cdb01a1494b917c8436b35"
        chain_code="0x873dff81c02f525623fd1fe5167eac3a55a049de3d314bb42ee227ffed37d508"
        key = unhexlify(key.lower().replace("0x", ""))
        chain_code = unhexlify(chain_code.lower().replace("0x", ""))
        hdkey = crypto_hd_key.CryptoHDKey()
        hdkey.new_master_key(key,chain_code)
        hdkey_cbor = hdkey.cbor_encode()

        expected = "A301F503582100E8F32E723DECF4051AEFAC8E2C93C9C5B214313817CDB01A1494B917C8436B35045820873DFF81C02F525623FD1FE5167EAC3A55A049DE3D314BB42EE227FFED37D508"
        self.assertEqual(hexlify(hdkey_cbor).decode().upper(), expected)


        # extended_key
        key="026fe2355745bb2db3630bbc80ef5d58951c963c841f54170ba6e5c12be7fc12a6"
        chain_code="ced155c72456255881793514edc5bd9447e7f74abb88c6d6b6480fd016ee8c85"
        key = unhexlify(key.lower().replace("0x", ""))
        chain_code = unhexlify(chain_code.lower().replace("0x", ""))
        hdkey = crypto_hd_key.CryptoHDKey()
        hdkey.new_extended_key(
            None,
            key,
            chain_code,
            CryptoCoinInfo(None, TestNet),
            CryptoKeyPath(
                [
                    PathComponent.new(44, True),
                    PathComponent.new(1, True),
                    PathComponent.new(1, True),
                    PathComponent.new(0, False),
                    PathComponent.new(1, False),
                ],
                None,
                None,
            ),
            None,
            [0xe9, 0x18, 0x1c, 0xf3],
            None,
            None
        )
        cbor = hdkey.cbor_encode()
        ur = hdkey.ur_encode()
        encoded = UREncoder.encode(ur)
        self.assertEqual(hexlify(cbor).decode().upper(), "A5035821026FE2355745BB2DB3630BBC80EF5D58951C963C841F54170BA6E5C12BE7FC12A6045820CED155C72456255881793514EDC5BD9447E7F74ABB88C6D6B6480FD016EE8C8505D90131A1020106D90130A1018A182CF501F501F500F401F4081AE9181CF3")
        self.assertEqual(encoded, "ur:crypto-hdkey/onaxhdclaojlvoechgferkdpqdiabdrflawshlhdmdcemtfnlrctghchbdolvwsednvdztbgolaahdcxtottgostdkhfdahdlykkecbbweskrymwflvdylgerkloswtbrpfdbsticmwylklpahtaadehoyaoadamtaaddyoyadlecsdwykadykadykaewkadwkaycywlcscewfihbdaehn")


    def test_decode(self):
        # master_key
        bytes = unhexlify("A301F503582100E8F32E723DECF4051AEFAC8E2C93C9C5B214313817CDB01A1494B917C8436B35045820873DFF81C02F525623FD1FE5167EAC3A55A049DE3D314BB42EE227FFED37D508")
        hdkey = crypto_hd_key.CryptoHDKey.from_cbor(bytes)
        self.assertEqual(hexlify(hdkey.get_key()).decode(), "00e8f32e723decf4051aefac8e2c93c9c5b214313817cdb01a1494b917c8436b35")
        self.assertEqual(hexlify(hdkey.get_chain_code()).decode(), "873dff81c02f525623fd1fe5167eac3a55a049de3d314bb42ee227ffed37d508")

        # extended_key
        bytes = unhexlify("A5035821026FE2355745BB2DB3630BBC80EF5D58951C963C841F54170BA6E5C12BE7FC12A6045820CED155C72456255881793514EDC5BD9447E7F74ABB88C6D6B6480FD016EE8C8505D90131A1020106D90130A1018A182CF501F501F500F401F4081AE9181CF3")
        hdkey = crypto_hd_key.CryptoHDKey.from_cbor(bytes)
        self.assertEqual(hexlify(hdkey.get_key()).decode(), "026fe2355745bb2db3630bbc80ef5d58951c963c841f54170ba6e5c12be7fc12a6")
        self.assertEqual(hexlify(hdkey.get_chain_code()).decode(), "ced155c72456255881793514edc5bd9447e7f74abb88c6d6b6480fd016ee8c85")
        self.assertEqual(hdkey.get_is_master(), False)
        self.assertEqual(hdkey.get_is_private_key(), False)
        self.assertEqual(hdkey.get_use_info().get_coin_type(), Bitcoin)
        self.assertEqual(hdkey.get_use_info().get_network(), TestNet)
        self.assertEqual(hdkey.get_origin().get_path(), "44'/1'/1'/0/1")
        self.assertEqual(hdkey.get_parent_fingerprint(), [0xe9, 0x18, 0x1c, 0xf3])
        self.assertEqual(hdkey.get_bip32_key(), "xpub6H8Qkexp9BdSgEwPAnhiEjp7NMXVEZWoAFWwon5mSwbuPZMfSUTpPwAP1Q2q2kYMRgRQ8udBpEj89wburY1vW7AWDuYpByteGogpB6pPprX")        


if __name__ == '__main__':
    unittest.main()
