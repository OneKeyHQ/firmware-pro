from typing import TYPE_CHECKING

from . import messages
from .tools import expect

if TYPE_CHECKING:
    from .client import TrezorClient
    from .tools import Address


@expect(messages.TonAddress)
def get_address(client: "TrezorClient",
                n: "Address",
                version: messages.TonWalletVersion=messages.TonWalletVersion.V4R2,
                workchain: messages.TonWorkChain=messages.TonWorkChain.BASECHAIN,
                bounceable: bool = False,
                test_only: bool = False,
                wallet_id: int = 698983191,
                show_display: bool = False):
    return client.call(
        messages.TonGetAddress(
            address_n=n,
            wallet_version=version,
            workchain=workchain,
            is_bounceable=bounceable,
            is_testnet_only=test_only,
            wallet_id=wallet_id,
            show_display=show_display
        )
    )

@expect(messages.TonSignedMessage)
def sign_message(client: "TrezorClient",
                n: "Address",
                destination: str,
                jetton_master_address: str,
                jetton_wallet_address: str,
                ton_amount: int,
                jetton_amount: int,
                jetton_amount_bytes: str,
                fwd_fee: int,
                mode: int,
                seqno: int,
                expire_at: int,
                comment: str="",
                is_raw_data: bool = False,
                version: messages.TonWalletVersion=messages.TonWalletVersion.V4R2,
                wallet_id: int = 698983191,
                workchain: messages.TonWorkChain=messages.TonWorkChain.BASECHAIN,
                bounceable: bool = False,
                test_only: bool = False,
                init_data: str = None,
                ext_destination: list[str] = None,
                ext_ton_amount: list[int] = None,
                ext_payload: list[str] = None,
                signing_message_repr: str = None
                ):
    if jetton_amount_bytes is not None:
        jetton_amount_bytes = int(jetton_amount_bytes).to_bytes((int(jetton_amount_bytes).bit_length() + 7) // 8, byteorder='big')
    if signing_message_repr is not None:
        signing_message_repr = bytes.fromhex(signing_message_repr[2:] if signing_message_repr.startswith("0x") else signing_message_repr)
    if init_data is not None:
        init_data_bytes = bytes.fromhex(init_data[2:] if init_data.startswith("0x") else init_data)
    else:
        init_data_bytes = None
    msg = messages.TonSignMessage(
            address_n=n,
            destination=destination,
            jetton_master_address=jetton_master_address,
            jetton_wallet_address=jetton_wallet_address,
            ton_amount=ton_amount,
            jetton_amount=jetton_amount,
            jetton_amount_bytes=jetton_amount_bytes,
            fwd_fee=fwd_fee,
            comment=comment,
            mode=mode,
            seqno=seqno,
            expire_at=expire_at,
            wallet_version=version,
            is_raw_data=is_raw_data,
            wallet_id=wallet_id,
            workchain=workchain,
            is_bounceable=bounceable,
            is_testnet_only=test_only,
            ext_destination=ext_destination,
            ext_ton_amount=ext_ton_amount,
            ext_payload=ext_payload,
            signing_message_repr=signing_message_repr
        )
    if init_data_bytes is not None:
        msg.init_data_length = len(init_data_bytes)
        init_data_bytes, chunk = init_data_bytes[1024:], init_data_bytes[:1024]
        msg.init_data_initial_chunk = chunk
    else:
        msg.init_data_length = 0
    response = client.call(msg)
    assert isinstance(response, messages.TonSignedMessage)

    while response.init_data_length is not None:
        init_data_length = response.init_data_length
        init_data_bytes, chunk = init_data_bytes[init_data_length:], init_data_bytes[:init_data_length]
        response = client.call(messages.TonTxAck(init_data_chunk=chunk))
        assert isinstance(response, messages.TonSignedMessage)
    return response

@expect(messages.TonSignedProof)
def sign_proof(client: "TrezorClient",
                n: "Address",
                expire_at: int,
                appdomain: str,
                comment: str,
                version: messages.TonWalletVersion=messages.TonWalletVersion.V4R2,
                wallet_id: int = 698983191,
                workchain: messages.TonWorkChain=messages.TonWorkChain.BASECHAIN,
                bounceable: bool = False,
                test_only: bool = False):
    appdomain = appdomain.encode("utf-8")
    if comment is not None:
        comment = comment.encode("utf-8")
    return client.call(
        messages.TonSignProof(
            address_n=n,
            appdomain=appdomain,
            comment=comment,
            expire_at=expire_at,
            version=version,
            wallet_id=wallet_id,
            workchain=workchain,
            bounceable=bounceable,
            is_test_only=test_only,
        )
    )