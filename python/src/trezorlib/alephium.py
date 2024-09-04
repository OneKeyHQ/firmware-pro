#!/usr/bin/env python3

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
from typing import TYPE_CHECKING

from . import messages
from .tools import expect

if TYPE_CHECKING:
    from .client import TrezorClient
    from .tools import Address
    from .protobuf import MessageType


@expect(messages.AlephiumAddress)
def get_address(
    client: "TrezorClient",
    address_n: "Address",
    show_display: bool = False,
    include_public_key: bool = False,
) -> "MessageType":
    res = client.call(
        messages.AlephiumGetAddress(
            address_n=address_n, 
            show_display=show_display,
            include_public_key=include_public_key
        )
    )
    return res


@expect(messages.AlephiumSignedTx)
def sign_tx(client: "TrezorClient", address_n: "Address", rawtx: str, data_length:int):
   resp = client.call(messages.AlephiumSignTx(address_n=address_n,  data_initial_chunk=rawtx,data_length = data_length))
   while isinstance(resp, messages.AlephiumTxRequest):
        print("AlephiumTxRequest error"+str(resp.data_length))
        data_chunk = bytes.fromhex("00000000000000000000")
        resp = client.call(messages.AlephiumTxAck(data_chunk=data_chunk))
   while isinstance(resp, messages.AlephiumBytecodeRequest):
        # print("AlephiumTxRequest error"+str(resp.data_length))
        bytecode_data = bytes.fromhex("01010300000007b413c40de0b6b3a7640000a20c0c1440206c3b1f6262ffad9a4cb1e78f03f17f3593837505a69edbc18a59cf23c1f1c4020100")
        resp = client.call(messages.AlephiumBytecodeAck(bytecode_data=bytecode_data))
   return resp


@expect(messages.AlephiumMessageSignature)
def sign_message(client: "TrezorClient", address_n: "Address", message: str,message_type: str):
   message_bytes = message.encode('utf-8')
   message_type_bytes = message_type.encode('utf-8')
   resp = client.call(messages.AlephiumSignMessage(address_n=address_n, message = message_bytes , message_type= message_type_bytes))
   return resp
