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

import json
from typing import TYPE_CHECKING, Dict, TextIO, Tuple

import click
import time

from .. import scdo, tools
from . import with_client
from typing import (
    TYPE_CHECKING,
    Any,
    Dict,
    List,
    NoReturn,
    Optional,
    Sequence,
    TextIO,
    Tuple,
)

if TYPE_CHECKING:
    from ..client import TrezorClient


PATH_HELP = "BIP-32 path to key, e.g. m/44'/541'/0'/0/0"


@click.group(name="scdo")
def cli() -> None:
    """Scdo Chain commands."""


@cli.command()
@click.option("-n", "--address", required=True, help=PATH_HELP)
@click.option("-d", "--show-display", is_flag=True)

@with_client
def get_address(
    client: "TrezorClient", address: str, show_display: bool
) -> str:
    """Get Scdo address for specified path."""
    address_n = tools.parse_path(address)

    return scdo.get_address(client, address_n, show_display)


@cli.command()
@click.option("-n", "--address", required=True, help=PATH_HELP)
@click.option("-no", "--nonce", required=True, type=int)
@click.option("-g", "--gas-price", required=True, type=int)
@click.option("-l", "--gas-limit", required=True, type=int)
@click.option("-t", "--to", required=True, type=str)
@click.option("-v", "--value", required=True, type=int)
# @click.option("-s", "--timestamp", help="The initial data chunk (<= 1024 bytes)")
@click.option("-d", "--data", default="", type=str)
@click.option("-x", "--tx-type", default=0, help="Transaction type")

@with_client
def sign_tx(
    client: "TrezorClient",
    address: str,
    nonce: int,
    gas_price: int,
    gas_limit: int,
    to: str,
    value: int,
    data: str,
    tx_type: int
) -> str:
    """Sign Scdo transaction."""
    address_n = tools.parse_path(address)
    timestamp = int(0)
    # timestamp = int(time.time()) + 300
    print("timestamp: ", timestamp)

    if data:
        data_bytes = bytes.fromhex(data)
    else:
        data_bytes = b""

    return scdo.sign_tx(
        client,
        address_n,
        nonce,
        gas_price,
        gas_limit,
        to,
        value,
        timestamp,
        data_bytes,
        tx_type
    )

@cli.command()
@click.option("-n", "--address", required=True, help=PATH_HELP)
@click.argument("message")
@with_client
def sign_message(client: "TrezorClient", address: str, message: str) -> Dict[str, str]:
    """Sign message with Scdo address."""
    address_n = tools.parse_path(address)
    ret = scdo.sign_message(client, address_n, message)
    signature = ret.signature if ret.signature is not None else b""
    output = {
        "message": message,
        "address": ret.address,
        "signature": f"0x{signature.hex()}",
    }
    return output
