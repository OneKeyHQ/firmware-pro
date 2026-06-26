from typing import TYPE_CHECKING

from trezor.crypto.hashlib import sha256
from trezor.messages import EosTxActionAck, EosTxActionRequest
from trezor.utils import HashWriter

from .. import helpers, writers
from . import layout

if TYPE_CHECKING:
    from trezor import wire
    from trezor.utils import Writer


def _require_action_payload(payload):
    if payload is None:
        raise ValueError("Invalid action")
    return payload


async def process_action(
    ctx: wire.Context, sha: HashWriter, action: EosTxActionAck
) -> None:
    name = helpers.eos_name_to_string(action.common.name)
    account = helpers.eos_name_to_string(action.common.account)

    if not check_action(action, name, account):
        raise ValueError("Invalid action")

    w = bytearray()
    if account == "eosio":
        if name == "buyram":
            buy_ram = _require_action_payload(action.buy_ram)
            await layout.confirm_action_buyram(ctx, buy_ram)
            writers.write_action_buyram(w, buy_ram)
        elif name == "buyrambytes":
            buy_ram_bytes = _require_action_payload(action.buy_ram_bytes)
            await layout.confirm_action_buyrambytes(ctx, buy_ram_bytes)
            writers.write_action_buyrambytes(w, buy_ram_bytes)
        elif name == "sellram":
            sell_ram = _require_action_payload(action.sell_ram)
            await layout.confirm_action_sellram(ctx, sell_ram)
            writers.write_action_sellram(w, sell_ram)
        elif name == "delegatebw":
            delegate = _require_action_payload(action.delegate)
            await layout.confirm_action_delegate(ctx, delegate)
            writers.write_action_delegate(w, delegate)
        elif name == "undelegatebw":
            undelegate = _require_action_payload(action.undelegate)
            await layout.confirm_action_undelegate(ctx, undelegate)
            writers.write_action_undelegate(w, undelegate)
        elif name == "refund":
            refund = _require_action_payload(action.refund)
            await layout.confirm_action_refund(ctx, refund)
            writers.write_action_refund(w, refund)
        elif name == "voteproducer":
            vote_producer = _require_action_payload(action.vote_producer)
            await layout.confirm_action_voteproducer(ctx, vote_producer)
            writers.write_action_voteproducer(w, vote_producer)
        elif name == "updateauth":
            update_auth = _require_action_payload(action.update_auth)
            await layout.confirm_action_updateauth(ctx, update_auth)
            writers.write_action_updateauth(w, update_auth)
        elif name == "deleteauth":
            delete_auth = _require_action_payload(action.delete_auth)
            await layout.confirm_action_deleteauth(ctx, delete_auth)
            writers.write_action_deleteauth(w, delete_auth)
        elif name == "linkauth":
            link_auth = _require_action_payload(action.link_auth)
            await layout.confirm_action_linkauth(ctx, link_auth)
            writers.write_action_linkauth(w, link_auth)
        elif name == "unlinkauth":
            unlink_auth = _require_action_payload(action.unlink_auth)
            await layout.confirm_action_unlinkauth(ctx, unlink_auth)
            writers.write_action_unlinkauth(w, unlink_auth)
        elif name == "newaccount":
            new_account = _require_action_payload(action.new_account)
            await layout.confirm_action_newaccount(ctx, new_account)
            writers.write_action_newaccount(w, new_account)
        else:
            raise ValueError("Unrecognized action type for eosio")
    elif name == "transfer":
        transfer = _require_action_payload(action.transfer)
        await layout.confirm_action_transfer(ctx, transfer, account)
        writers.write_action_transfer(w, transfer)
    else:
        await process_unknown_action(ctx, w, action)

    writers.write_action_common(sha, action.common)
    writers.write_bytes_prefixed(sha, w)


async def process_unknown_action(
    ctx: wire.Context, w: Writer, action: EosTxActionAck
) -> None:
    if action.unknown is None:
        raise ValueError("Bad response. Unknown struct expected.")
    checksum = HashWriter(sha256())
    writers.write_uvarint(checksum, action.unknown.data_size)
    checksum.extend(action.unknown.data_chunk)

    writers.write_bytes_unchecked(w, action.unknown.data_chunk)
    bytes_left = action.unknown.data_size - len(action.unknown.data_chunk)

    while bytes_left != 0:
        action = await ctx.call(
            EosTxActionRequest(data_size=bytes_left), EosTxActionAck
        )

        if action.unknown is None:
            raise ValueError("Bad response. Unknown struct expected.")

        checksum.extend(action.unknown.data_chunk)
        writers.write_bytes_unchecked(w, action.unknown.data_chunk)

        bytes_left -= len(action.unknown.data_chunk)
        if bytes_left < 0:
            raise ValueError("Bad response. Buffer overflow.")

    await layout.confirm_action_unknown(ctx, action.common, checksum.get_digest())


def check_action(action: EosTxActionAck, name: str, account: str) -> bool:
    if account == "eosio":
        return (
            (name == "buyram" and action.buy_ram is not None)
            or (name == "buyrambytes" and action.buy_ram_bytes is not None)
            or (name == "sellram" and action.sell_ram is not None)
            or (name == "delegatebw" and action.delegate is not None)
            or (name == "undelegatebw" and action.undelegate is not None)
            or (name == "refund" and action.refund is not None)
            or (name == "voteproducer" and action.vote_producer is not None)
            or (name == "updateauth" and action.update_auth is not None)
            or (name == "deleteauth" and action.delete_auth is not None)
            or (name == "linkauth" and action.link_auth is not None)
            or (name == "unlinkauth" and action.unlink_auth is not None)
            or (name == "newaccount" and action.new_account is not None)
        )

    elif name == "transfer":
        return action.transfer is not None

    elif action.unknown is not None:
        return True

    return False
