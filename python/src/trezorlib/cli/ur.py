# This file is part of the OneKey project, https://onekey.so/
#
# Copyright (C) 2021 OneKey Team <core@onekey.so>
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.

from typing import TYPE_CHECKING

import click

from . import with_client
from .. import ur, tools

if TYPE_CHECKING:
    from ..client import TrezorClient


PATH_HELP = "BIP-32 path, e.g. m/44'/0'/0'/0'/0'"


@click.group(name="ur")
def cli():
    """UR commands."""


@cli.command()
@click.option("-n", "--address", required=True, help=PATH_HELP)
@click.option("-d", "--show-display", is_flag=True)
@with_client
def crypto_hd_key(client: "TrezorClient", address: str, show_display: bool) -> str:
    """Get HD KEY in ur encoding."""
    address_n = tools.parse_path(address)
    return ur.crypto_hd_key(client, address_n, show_display)

