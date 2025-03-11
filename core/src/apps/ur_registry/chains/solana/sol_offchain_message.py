from trezor.messages import SolanaSignOffChainMessage

from .sol_sign_request import SolSignRequest


class SolOffChainMessage:
    def __init__(self, req: SolSignRequest, standard: bool):
        self.req = req
        self.qr = None
        self.encoder = None
        self.standard = standard

    def gen_request(self):
        from apps.solana.sign_offchain_message import decode_offchain_message

        sign_data = self.req.get_sign_data()
        address_n = self.req.get_address_n()
        (
            message_version,
            message_format,
            message,
            application_domain,
        ) = decode_offchain_message(sign_data, standard=self.standard)
        # pyright: off
        return SolanaSignOffChainMessage(
            message=message,
            address_n=address_n,
            message_version=message_version,
            message_format=message_format,
            application_domain=application_domain,
        )
        # pyright: on

    async def run(self):
        from trezor import wire
        from apps.solana.sign_offchain_message import sign_offchain_message
        from apps.ur_registry.chains.solana.sol_signature import SolSignature
        from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder

        # pyright: off
        req = self.gen_request()
        resp = await sign_offchain_message(wire.QR_CONTEXT, req)
        sol_signature = SolSignature(
            request_id=self.req.get_request_id(),
            signature=resp.signature,
        )
        # pyright: on
        ur = sol_signature.ur_encode()
        encoded = UREncoder.encode(ur).upper()
        self.qr = encoded
