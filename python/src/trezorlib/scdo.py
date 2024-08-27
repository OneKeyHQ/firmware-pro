# This file is part of the Trezor project.
#
# Copyright (C) 2012-2022 Onekey and contributors
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License version 3
# as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the License along with this library.
# If not, see <https://www.gnu.org/licenses/lgpl-3.0.html>.

from typing import TYPE_CHECKING, AnyStr, Optional, Tuple

from . import messages
from .tools import expect, prepare_message_bytes, session

if TYPE_CHECKING:
    from .client import TrezorClient
    from .tools import Address
    from .protobuf import MessageType


def int_to_big_endian(value: int) -> bytes:
    return value.to_bytes((value.bit_length() + 7) // 8, "big")


def decode_hex(value: str) -> bytes:
    if value.startswith(("0x", "0X")):
        return bytes.fromhex(value[2:])
    else:
        return bytes.fromhex(value)


@expect(messages.ScdoAddress, field="address", ret_type=str)
def get_address(
    client: "TrezorClient",
    address_n: "Address",
    show_display: bool = False,
) -> "MessageType":
    
    return client.call(
        messages.ScdoGetAddress(
            address_n=address_n, show_display=show_display
        )
    )

@session
def sign_tx(
    client: "TrezorClient",
    n: "Address",
    nonce: int,
    gas_price: int,
    gas_limit: int,
    to: str,
    value: int,
    timestamp: int,
    data: Optional[bytes],
    tx_type: Optional[int] = None,
) -> Tuple[int, bytes, bytes]:

    msg = messages.ScdoSignTx(
        address_n=n,
        nonce=int_to_big_endian(nonce),
        gas_price=int_to_big_endian(gas_price),
        gas_limit=int_to_big_endian(gas_limit),
        to=to,
        value=int_to_big_endian(value),
        timestamp=int_to_big_endian(timestamp),
        tx_type=tx_type,
    )

    msg.data_length = len(data)
    data, chunk = data[1024:], data[:1024]
    msg.data_initial_chunk = chunk

    response = client.call(msg)
    assert isinstance(response, messages.ScdoSignedTx)

    while response.data_length is not None:
        data_length = response.data_length
        data, chunk = data[data_length:], data[:data_length]
        response = client.call(messages.ScdoTxAck(data_chunk=chunk))
        assert isinstance(response, messages.ScdoSignedTx)

    assert response.signature is not None

    # https://github.com/trezor/trezor-core/pull/311
    # only signature bit returned. recalculate signature_v

    return f"signature: {response.signature.hex()}"


@expect(messages.ScdoSignedMessage)
def sign_message(
    client: "TrezorClient", n: "Address", message: AnyStr
) -> "MessageType":
    return client.call(
        messages.ScdoSignMessage(address_n=n, message=prepare_message_bytes(message))
    )