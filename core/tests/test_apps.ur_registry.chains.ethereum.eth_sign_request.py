from common import *


if not utils.BITCOIN_ONLY:
    from apps.ur_registry.chains.ethereum.eth_sign_request import EthSignRequest, RequestType_Transaction
    from apps.ur_registry.crypto_key_path import PathComponent, CryptoKeyPath
    from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder

@unittest.skipUnless(not utils.BITCOIN_ONLY, "altcoin")
class TestEthSignReqEncode(unittest.TestCase):
    def test_encode(self):
        path1 = PathComponent.new(44, True)
        path2 = PathComponent.new(1, True)
        path3 = PathComponent.new(1, True)
        path4 = PathComponent.new(0, False)
        path5 = PathComponent.new(1, False)
        source_fingerprint = [18, 52, 86, 120]
        components = [path1, path2, path3, path4, path5]
        crypto_key_path = CryptoKeyPath.new(components, source_fingerprint, None)
        req_id = [155, 29, 235, 77, 59, 125, 75, 173, 155, 221, 43, 13, 123, 61, 203, 109,]
        sign_data = [
                248, 73, 128, 134, 9, 24, 78, 114, 160, 0, 130, 39, 16, 148, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 164, 127, 116, 101, 115, 116, 50, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 96, 0, 87, 128, 128,
                128,
        ]
        eth_sign_req = EthSignRequest.new(
                bytes(req_id),
                bytes(sign_data),
                RequestType_Transaction,
                1,
                crypto_key_path,
                None,
                "metamask".encode(),
        )
        ur = eth_sign_req.ur_encode()

        expected = "a601d825509b1deb4d3b7d4bad9bdd2b0d7b3dcb6d02584bf849808609184e72a00082271094000000000000000000000000000000000000000080a47f74657374320000000000000000000000000000000000000000000000000000006000578080800301040105d90130a2018a182cf501f501f500f401f4021a1234567807686d6574616d61736b"
        self.assertEqual(hexlify(ur.cbor).decode(), expected)


    def test_decode(self):
        req_id = [155, 29, 235, 77, 59, 125, 75, 173, 155, 221, 43, 13, 123, 61, 203, 109,]

        bytes = unhexlify("a601d825509b1deb4d3b7d4bad9bdd2b0d7b3dcb6d02584bf849808609184e72a00082271094000000000000000000000000000000000000000080a47f74657374320000000000000000000000000000000000000000000000000000006000578080800301040105d90130a2018a182cf501f501f500f401f4021a1234567807686d6574616d61736b")
        req = EthSignRequest.from_cbor(bytes)
        self.assertEqual(req.get_derivation_path().get_path(), "44'/1'/1'/0/1")
        self.assertEqual(req.get_data_type(), RequestType_Transaction)
        self.assertEqual(list(req.get_request_id()), req_id)


if __name__ == '__main__':
    unittest.main()
