from typing import TYPE_CHECKING
from ubinascii import hexlify

from trezor import ui
from trezor.enums import ButtonRequestType, EthereumDataType, EthereumDataTypeOneKey
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.messages import (
    EthereumFieldType,
    EthereumFieldTypeOneKey,
    EthereumStructMember,
    EthereumStructMemberOneKey,
)
from trezor.strings import format_amount, strip_amount
from trezor.ui.layouts import (
    confirm_action,
    confirm_address,
    confirm_blob,
    confirm_output,
    confirm_sign_typed_hash,
    confirm_text,
    should_show_approve_details,
    should_show_more,
)
from trezor.ui.layouts.lvgl.altcoin import (
    confirm_approve,
    confirm_approve_eip1559,
    confirm_total_ethereum,
    confirm_total_ethereum_eip1559,
)

from . import networks, tokens
from .helpers import (
    address_from_bytes,
    decode_typed_data,
    get_type_name,
    get_type_name_onekey,
)

if TYPE_CHECKING:
    from typing import Awaitable, Iterable

    from trezor.wire import Context
    from trezor.messages import EthereumGnosisSafeTxAck


def require_confirm_tx(
    ctx: Context,
    to_bytes: bytes,
    value: int,
    chain_id: int,
    token: tokens.EthereumTokenInfo | None = None,
    is_nft: bool = False,
) -> Awaitable[None]:
    if to_bytes:
        to_str = address_from_bytes(to_bytes, networks.by_chain_id(chain_id))
    else:
        to_str = _(i18n_keys.LIST_VALUE__NEW_CONTRACT)
    return confirm_output(
        ctx,
        address=to_str,
        amount=format_ethereum_amount(value, token, chain_id, is_nft),
        font_amount=ui.BOLD,
        color_to=ui.GREY,
        br_code=ButtonRequestType.SignTx,
    )


def require_show_overview(
    ctx: Context,
    to_bytes: bytes,
    value: int,
    gas_price: int,
    gas_limit: int,
    chain_id: int,
    token: tokens.EthereumTokenInfo | None = None,
    token_address: str | None = None,
    is_nft: bool = False,
    has_raw_data: bool = True,
) -> Awaitable[bool]:
    if to_bytes:
        to_str = address_from_bytes(to_bytes, networks.by_chain_id(chain_id))
    else:
        to_str = _(i18n_keys.LIST_VALUE__NEW_CONTRACT)
    fee_max = gas_price * gas_limit

    if value == 0 and has_raw_data:
        title = _(i18n_keys.TITLE_REQUEST_CONFIRMATION)
    else:
        title = _(i18n_keys.TITLE__SEND_MULTILINE).format(
            strip_amount(format_ethereum_amount(value, token, chain_id, is_nft))[0]
        )
    from trezor.ui.layouts.lvgl import should_show_details_new

    return should_show_details_new(
        ctx,
        title=title,
        br_code=ButtonRequestType.SignTx,
        to_address=to_str,
        max_fee=format_ethereum_amount(fee_max, None, chain_id),
        token_address=token_address,
        banner_key=_(i18n_keys.WARNING_UNRECOGNIZED_TOKEN)
        if token is tokens.UNKNOWN_TOKEN
        else None,
    )

    # return should_show_details(
    #     ctx,
    #     title=_(i18n_keys.TITLE__SEND_MULTILINE).format(
    #         strip_amount(format_ethereum_amount(value, token, chain_id, is_nft))[0]
    #     ),
    #     address=to_str,
    #     br_code=ButtonRequestType.SignTx,
    # )


def require_confirm_fee(
    ctx: Context,
    spending: int,
    gas_price: int,
    gas_limit: int,
    chain_id: int,
    token: tokens.EthereumTokenInfo | None = None,
    from_address: str | None = None,
    to_address: str | None = None,
    contract_addr: str | None = None,
    token_id: int | None = None,
    evm_chain_id: int | None = None,
    raw_data: bytes | None = None,
    token_address: str | None = None,
) -> Awaitable[None]:
    fee_max = gas_price * gas_limit
    return confirm_total_ethereum(
        ctx,
        format_ethereum_amount(
            spending, token, chain_id, is_nft=True if token_id else False
        ),
        format_ethereum_amount(gas_price, None, chain_id),
        format_ethereum_amount(fee_max, None, chain_id),
        from_address,
        to_address,
        format_ethereum_amount(spending + fee_max, None, chain_id)
        if (token is None and contract_addr is None)
        else None,
        contract_addr,
        token_id,
        evm_chain_id=evm_chain_id,
        raw_data=raw_data,
        token_address=token_address,
    )


async def require_confirm_eip1559_fee(
    ctx: Context,
    spending: int,
    max_priority_fee: int,
    max_gas_fee: int,
    gas_limit: int,
    chain_id: int,
    token: tokens.EthereumTokenInfo | None = None,
    from_address: str | None = None,
    to_address: str | None = None,
    contract_addr: str | None = None,
    token_id: int | None = None,
    evm_chain_id: int | None = None,
    raw_data: bytes | None = None,
    token_address: str | None = None,
) -> None:

    fee_max = max_gas_fee * gas_limit
    await confirm_total_ethereum_eip1559(
        ctx,
        format_ethereum_amount(
            spending, token, chain_id, is_nft=True if token_id else False
        ),
        format_ethereum_amount(max_priority_fee, None, chain_id),
        format_ethereum_amount(max_gas_fee, None, chain_id),
        format_ethereum_amount(fee_max, None, chain_id),
        from_address,
        to_address,
        format_ethereum_amount(spending + fee_max, None, chain_id)
        if (token is None and contract_addr is None)
        else None,
        contract_addr,
        token_id,
        evm_chain_id=evm_chain_id,
        raw_data=raw_data,
        token_address=token_address,
    )


def require_show_approve_overview(
    ctx: Context,
    approve_spender: bytes,
    approve_value: int,
    approve_token: tokens.EthereumTokenInfo,
    approve_token_address: bytes,
    max_gas_fee: int,
    gas_limit: int,
    chain_id: int,
    provider_name: str | None = None,
    provider_icon_path: str | None = None,
) -> Awaitable[bool]:

    title = format_approve_title(approve_token, approve_value, chain_id, provider_name)

    fee_max = max_gas_fee * gas_limit
    is_unlimited = approve_value == 2**256 - 1

    approve_spender_str = address_from_bytes(
        approve_spender, networks.by_chain_id(chain_id)
    )
    approve_token_address_str = address_from_bytes(
        approve_token_address, networks.by_chain_id(chain_id)
    )

    return should_show_approve_details(
        ctx,
        approve_spender=approve_spender_str,
        max_fee=format_ethereum_amount(fee_max, None, chain_id),
        token_address=approve_token_address_str,
        provider_icon_path=provider_icon_path or "A:/res/provider-default.png",
        title=title,
        is_unlimited=is_unlimited,
        br_code=ButtonRequestType.SignTx,
    )


async def require_confirm_eip1559_erc20_approve(
    ctx: Context,
    approve_value: int,
    max_priority_fee: int,
    max_gas_fee: int,
    gas_limit: int,
    chain_id: int,
    token: tokens.EthereumTokenInfo,
    from_address: str | None = None,
    to_address: str | None = None,
    token_address: str | None = None,
    token_id: int | None = None,
    evm_chain_id: int | None = None,
    raw_data: bytes | None = None,
    provider_name: str | None = None,
    provider_icon: str | None = None,
    is_nft: bool = False,
) -> None:
    fee_max = max_gas_fee * gas_limit
    title = format_approve_title(token, approve_value, chain_id, provider_name)
    is_unlimited = approve_value == 2**256 - 1

    await confirm_approve_eip1559(
        ctx,
        title,
        format_ethereum_amount(
            approve_value, token, chain_id, is_nft=True if token_id else False
        ),
        format_ethereum_amount(max_priority_fee, None, chain_id),
        format_ethereum_amount(max_gas_fee, None, chain_id),
        format_ethereum_amount(fee_max, None, chain_id),
        from_address,
        to_address,
        format_ethereum_amount(approve_value + fee_max, None, chain_id)
        if (token is None and token_address is None)
        else None,
        token_address,
        token_id,
        evm_chain_id=evm_chain_id,
        raw_data=raw_data,
        provider_name=provider_name,
        provider_icon=provider_icon,
        is_unlimited=is_unlimited,
    )


async def require_confirm_legacy_erc20_approve(
    ctx: Context,
    approve_value: int,
    gas_price: int,
    gas_limit: int,
    chain_id: int,
    token: tokens.EthereumTokenInfo,
    from_address: str | None = None,
    to_address: str | None = None,
    token_address: str | None = None,
    token_id: int | None = None,
    evm_chain_id: int | None = None,
    raw_data: bytes | None = None,
    provider_name: str | None = None,
    provider_icon: str | None = None,
    is_nft: bool = False,
) -> None:
    fee_max = gas_price * gas_limit
    title = format_approve_title(token, approve_value, chain_id, provider_name)
    is_unlimited = approve_value == 2**256 - 1

    await confirm_approve(
        ctx,
        title,
        format_ethereum_amount(
            approve_value, token, chain_id, is_nft=True if token_id else False
        ),
        format_ethereum_amount(gas_price, None, chain_id),
        format_ethereum_amount(fee_max, None, chain_id),
        from_address,
        to_address,
        format_ethereum_amount(approve_value + fee_max, None, chain_id)
        if (token is None and token_address is None)
        else None,
        token_address,
        token_id,
        evm_chain_id=evm_chain_id,
        raw_data=raw_data,
        provider_name=provider_name,
        provider_icon=provider_icon,
        is_unlimited=is_unlimited,
    )


def require_confirm_unknown_token(
    ctx: Context, address_bytes: bytes
) -> Awaitable[None]:
    contract_address_hex = "0x" + hexlify(address_bytes).decode()
    return confirm_address(
        ctx,
        _(i18n_keys.TITLE__UNKNOWN_TOKEN),
        contract_address_hex,
        description=_(i18n_keys.LIST_KEY__CONTRACT__COLON),
        br_type="unknown_token",
        icon="A:/res/warning.png",
        icon_color=ui.ORANGE,
        br_code=ButtonRequestType.SignTx,
    )


def require_confirm_data(ctx: Context, data: bytes, data_total: int) -> Awaitable[None]:
    from trezor.ui.layouts import confirm_data

    return confirm_data(
        ctx,
        "confirm_data",
        title=_(i18n_keys.TITLE__VIEW_DATA),
        description=_(i18n_keys.SUBTITLE__STR_BYTES).format(data_total),
        data=data,
        br_code=ButtonRequestType.SignTx,
    )


async def confirm_typed_data_final(ctx: Context) -> None:
    await confirm_action(
        ctx,
        "confirm_typed_data_final",
        title=_(i18n_keys.TITLE__SIGN_STR_TYPED_DATA).format(ctx.name),
        action=_(i18n_keys.SUBTITLE__SIGN_712_TYPED_DATA),
        verb=_(i18n_keys.BUTTON__SLIDE_TO_CONFIRM),
        icon=None,
        hold=True,
        anim_dir=0,
    )


async def confirm_typed_hash_final(ctx: Context) -> None:
    await confirm_action(
        ctx,
        "confirm_typed_hash_final",
        title=_(i18n_keys.TITLE__SIGN_STR_TYPED_HASH).format(ctx.name),
        action=_(i18n_keys.SUBTITLE__SIGN_STR_TYPED_HASH),
        verb=_(i18n_keys.BUTTON__SLIDE_TO_CONFIRM),
        icon=None,
        hold=True,
        anim_dir=0,
    )


async def confirm_typed_hash(ctx: Context, domain_hash, message_hash) -> None:
    await confirm_sign_typed_hash(ctx, domain_hash, message_hash)


def confirm_empty_typed_message(ctx: Context) -> Awaitable[None]:
    return confirm_text(
        ctx,
        "confirm_empty_typed_message",
        title=_(i18n_keys.TITLE__CONFIRM_MESSAGE),
        data="",
        description=_(i18n_keys.SUBTITLE__NO_MESSAGE_FIELD),
    )


async def confirm_domain(ctx: Context, domain: dict[str, bytes]) -> None:
    domain_name = (
        decode_typed_data(domain["name"], "string") if domain.get("name") else None
    )
    domain_version = (
        decode_typed_data(domain["version"], "string")
        if domain.get("version")
        else None
    )
    chain_id = (
        decode_typed_data(domain["chainId"], "uint256")
        if domain.get("chainId")
        else None
    )
    verifying_contract = (
        decode_typed_data(domain["verifyingContract"], "address")
        if domain.get("verifyingContract")
        else None
    )
    salt = decode_typed_data(domain["salt"], "bytes32") if domain.get("salt") else None
    from trezor.ui.layouts import confirm_domain

    await confirm_domain(
        ctx,
        **{
            "name": domain_name,
            "version": domain_version,
            "chainId": chain_id,
            "verifyingContract": verifying_contract,
            "salt": salt,
        },
    )


async def should_show_domain(ctx: Context, name: bytes, version: bytes) -> bool:
    domain_name = decode_typed_data(name, "string")
    domain_version = decode_typed_data(version, "string")

    para = (
        (ui.NORMAL, "Name and version"),
        (ui.BOLD, domain_name),
        (ui.BOLD, domain_version),
    )
    return await should_show_more(
        ctx,
        title="Confirm domain",
        para=para,
        button_text="Show full domain",
        br_type="should_show_domain",
    )


async def should_show_struct(
    ctx: Context,
    description: str,
    data_members: list[EthereumStructMember] | list[EthereumStructMemberOneKey],
    title: str = "Confirm struct",
    button_text: str = "Show full struct",
) -> bool:
    para = (
        (ui.BOLD, description),
        (
            ui.NORMAL,
            _(i18n_keys.LIST_KEY__CONTAINS_STR_KEY).format(len(data_members))
            # format_plural("Contains {count} {plural}", len(data_members), "key"),
        ),
        (ui.NORMAL, ", ".join(field.name for field in data_members)),
    )
    return await should_show_more(
        ctx,
        title=title,
        para=para,
        button_text=button_text,
        br_type="should_show_struct",
    )


async def should_show_array(
    ctx: Context,
    parent_objects: Iterable[str],
    data_type: str,
    size: int,
) -> bool:
    para = (
        (ui.NORMAL, _(i18n_keys.INSERT__ARRAY_OF_STR_STR).format(size, data_type)),
        # format_plural("Array of {count} {plural}", size, data_type)),
    )
    return await should_show_more(
        ctx,
        title=".".join(parent_objects),
        para=para,
        button_text=_(i18n_keys.BUTTON__VIEW_FULL_ARRAY),
        br_type="should_show_array",
    )


async def confirm_typed_value(
    ctx: Context,
    name: str,
    value: bytes,
    parent_objects: list[str],
    field: EthereumFieldType,
    array_index: int | None = None,
) -> None:
    type_name = get_type_name(field)

    if array_index is not None:
        title = ".".join(parent_objects + [name])
        description = f"[{array_index}] ({type_name}):"
    else:
        title = ".".join(parent_objects)
        description = f"{name} ({type_name}):"

    data = decode_typed_data(value, type_name)

    if field.data_type in (EthereumDataType.ADDRESS, EthereumDataType.BYTES):
        await confirm_blob(
            ctx,
            "confirm_typed_value",
            title=title,
            data=data,
            description=description,
            ask_pagination=True,
            icon=None,
        )
    else:
        await confirm_text(
            ctx,
            "confirm_typed_value",
            title=title,
            data=data,
            description=description,
            icon=None,
        )


async def confirm_typed_value_onekey(
    ctx: Context,
    name: str,
    value: bytes,
    parent_objects: list[str],
    field: EthereumFieldTypeOneKey,
    array_index: int | None = None,
) -> None:
    type_name = get_type_name_onekey(field)

    if array_index is not None:
        title = ".".join(parent_objects + [name])
        description = f"[{array_index}] ({type_name}):"
    else:
        title = ".".join(parent_objects)
        description = f"{name} ({type_name}):"

    data = decode_typed_data(value, type_name)

    if field.data_type in (
        EthereumDataTypeOneKey.ADDRESS,
        EthereumDataTypeOneKey.BYTES,
    ):
        await confirm_blob(
            ctx,
            "confirm_typed_value",
            title=title,
            data=data,
            description=description,
            ask_pagination=True,
            icon=None,
        )
    else:
        await confirm_text(
            ctx,
            "confirm_typed_value",
            title=title,
            data=data,
            description=description,
            icon=None,
        )


def format_ethereum_amount(
    value: int,
    token: tokens.EthereumTokenInfo | None,
    chain_id: int,
    is_nft: bool = False,
) -> str:
    if is_nft:
        return f"{value} NFT"
    if token:
        suffix = token.symbol
        decimals = token.decimals
    else:
        suffix = networks.shortcut_by_chain_id(chain_id)
        decimals = 18

    # Don't want to display wei values for tokens with small decimal numbers
    # if decimals > 9 and value < 10 ** (decimals - 9):
    #     suffix = "Wei " + suffix
    #     decimals = 0

    return f"{format_amount(value, decimals)} {suffix}"


def limit_str(s: str, limit: int = 16) -> str:
    """Shortens string to show the last <limit> characters."""
    if len(s) <= limit + 2:
        return s

    return ".." + s[-limit:]


async def require_confirm_safe_tx(
    ctx: Context,
    from_address: str,
    msg: EthereumGnosisSafeTxAck,
    domain_hash: bytes,
    message_hash: bytes,
    safe_tx_hash: bytes,
) -> None:

    from trezor.ui.layouts import confirm_safe_tx

    await confirm_safe_tx(
        ctx,
        from_address,
        msg.to,
        format_ethereum_amount(int.from_bytes(msg.value, "big"), None, msg.chain_id),
        msg.data,
        int(msg.operation),
        int.from_bytes(msg.safeTxGas, "big"),
        int.from_bytes(msg.baseGas, "big"),
        format_ethereum_amount(int.from_bytes(msg.gasPrice, "big"), None, msg.chain_id),
        msg.gasToken,
        msg.refundReceiver,
        int.from_bytes(msg.nonce, "big"),
        msg.verifyingContract,
        f"0x{hexlify(domain_hash).decode()}",
        f"0x{hexlify(message_hash).decode()}",
        f"0x{hexlify(safe_tx_hash).decode()}",
    )


def format_approve_title(
    approve_token: tokens.EthereumTokenInfo,
    value: int,
    chain_id: int,
    provider_name: str | None = None,
) -> str:

    if value == 0:
        action_type = "REVOKE"
    elif value == 2**256 - 1:
        action_type = "APPROVE_UNLIMITED"
    else:
        action_type = "APPROVE_LIMITED"

    token_status = "UNKNOWN" if approve_token == tokens.UNKNOWN_TOKEN else "KNOWN"

    provider_status = "KNOWN" if provider_name is not None else "UNKNOWN"

    combination_key = f"{action_type}_{token_status}_{provider_status}"

    if token_status == "UNKNOWN":
        token_name = "UNKN"
    else:
        token_name = approve_token.symbol

    amount_display = ""
    if action_type == "APPROVE_LIMITED":
        amount_display = strip_amount(
            format_ethereum_amount(value, approve_token, chain_id)
        )[0]

    title_map = {
        # Example: "Revoke UNKN for 1inch"
        "REVOKE_UNKNOWN_KNOWN": _(i18n_keys.REVOKE_TOKEN).format(
            token=token_name, name=provider_name
        ),
        # Example: "Revoke UNKN"
        "REVOKE_UNKNOWN_UNKNOWN": _(i18n_keys.TITLE_REVOKE).format(name=token_name),
        # Example: "Revoke USDT for 1inch"
        "REVOKE_KNOWN_KNOWN": _(i18n_keys.REVOKE_TOKEN).format(
            token=token_name, name=provider_name
        ),
        # Example: "Revoke USDT"
        "REVOKE_KNOWN_UNKNOWN": _(i18n_keys.TITLE_REVOKE).format(name=token_name),
        # Example: "Approve Unlimited UNKN for 1inch"
        "APPROVE_UNLIMITED_UNKNOWN_KNOWN": _(i18n_keys.APPROVE_UNLIMITED_TOKEN).format(
            token=token_name, name=provider_name
        ),
        # Example: "Approve Unlimited UNKN"
        "APPROVE_UNLIMITED_UNKNOWN_UNKNOWN": _(i18n_keys.TITLE_UNLIMITED).format(
            name=token_name
        ),
        # Example: "Approve unlimited USDT for 1inch"
        "APPROVE_UNLIMITED_KNOWN_KNOWN": _(i18n_keys.APPROVE_UNLIMITED_TOKEN).format(
            token=token_name, name=provider_name
        ),
        # Example: "Approve unlimited USDT"
        "APPROVE_UNLIMITED_KNOWN_UNKNOWN": _(i18n_keys.TITLE_UNLIMITED).format(
            name=token_name
        ),
        # Example: "Approve 10.678 UNKN for 1inch"
        "APPROVE_LIMITED_UNKNOWN_KNOWN": _(i18n_keys.APPROVE_TOKEN_AMOUNT).format(
            token=amount_display, name=provider_name
        ),
        # Example: "Approve 10.678 UNKN"
        "APPROVE_LIMITED_UNKNOWN_UNKNOWN": _(i18n_keys.TITLE_APPROVE).format(
            name=amount_display
        ),
        # Example: "Approve 10.678 USDT for 1inch"
        "APPROVE_LIMITED_KNOWN_KNOWN": _(i18n_keys.APPROVE_TOKEN_AMOUNT).format(
            token=amount_display, name=provider_name
        ),
        # Example: "Approve 10.678 USDT"
        "APPROVE_LIMITED_KNOWN_UNKNOWN": _(i18n_keys.TITLE_APPROVE).format(
            name=amount_display
        ),
    }

    return title_map[combination_key]


async def require_confirm_safe_approve_hash(
    ctx: Context,
    to_addr: str,
    from_addr: str,
    hash_to_approve: str,
    nonce: int,
    gas_price: int,
    gas_limit: int,
    chain_id: int,
    is_unknown_network: bool = False,
) -> None:
    from trezor.ui.layouts import confirm_safe_approve_hash

    fee_max = gas_price * gas_limit
    await confirm_safe_approve_hash(
        ctx,
        "Safe transaction",
        from_addr,
        to_addr,
        hash_to_approve,
        str(nonce),
        format_ethereum_amount(fee_max, None, chain_id),
        is_eip1559=False,
        gas_price=format_ethereum_amount(gas_price, None, chain_id),
        chain_id=chain_id if is_unknown_network else None,
    )


async def require_confirm_safe_approve_hash_eip1559(
    ctx: Context,
    to_addr: str,
    from_addr: str,
    hash_to_approve: str,
    nonce: int,
    max_priority_fee: int,
    max_gas_fee: int,
    gas_limit: int,
    chain_id: int,
    is_unknown_network: bool = False,
) -> None:
    from trezor.ui.layouts import confirm_safe_approve_hash

    fee_max = max_gas_fee * gas_limit
    await confirm_safe_approve_hash(
        ctx,
        "Safe transaction",
        from_addr,
        to_addr,
        hash_to_approve,
        str(nonce),
        format_ethereum_amount(fee_max, None, chain_id),
        is_eip1559=True,
        max_priority_fee_per_gas=format_ethereum_amount(
            max_priority_fee, None, chain_id
        ),
        max_fee_per_gas=format_ethereum_amount(max_gas_fee, None, chain_id),
        chain_id=chain_id if is_unknown_network else None,
    )


async def require_confirm_safe_exec_transaction(
    ctx: Context,
    from_addr: str,
    to_addr: str,
    to_address_safe: str,
    value_safe: int,
    operation: int,
    safe_tx_gas: int,
    base_gas: int,
    gas_price_safe: int,
    gas_token: str,
    refund_receiver: str,
    signatures: str,
    gas_price: int,
    gas_limit: int,
    nonce: int,
    chain_id: int,
    call_data: str | dict[str, str] | None = None,
    call_method: str | None = None,
    is_unknown_network: bool = False,
) -> None:
    from trezor.ui.layouts import confirm_safe_exec_transaction

    fee_max = gas_price * gas_limit
    await confirm_safe_exec_transaction(
        ctx,
        from_addr,
        to_addr,
        to_address_safe,
        format_ethereum_amount(value_safe, None, chain_id),
        operation,
        str(safe_tx_gas),
        str(base_gas),
        format_ethereum_amount(gas_price_safe, None, chain_id),
        gas_token,
        refund_receiver,
        signatures,
        format_ethereum_amount(fee_max, None, chain_id),
        nonce,
        is_eip1559=False,
        chain_id=chain_id if is_unknown_network else None,
        call_data=call_data,
        call_method=call_method,
        gas_price=format_ethereum_amount(gas_price, None, chain_id),
    )


async def require_confirm_safe_exec_transaction_eip1559(
    ctx: Context,
    from_addr: str,
    to_addr: str,
    to_address_safe: str,
    value_safe: int,
    operation: int,
    safe_tx_gas: int,
    base_gas: int,
    gas_price_safe: int,
    gas_token: str,
    refund_receiver: str,
    signatures: str,
    nonce: int,
    chain_id: int,
    gas_limit: int,
    max_priority_fee_per_gas: int,
    max_fee_per_gas: int,
    call_data: str | dict[str, str] | None = None,
    call_method: str | None = None,
    is_unknown_network: bool = False,
) -> None:
    from trezor.ui.layouts import confirm_safe_exec_transaction

    fee_max = max_fee_per_gas * gas_limit
    await confirm_safe_exec_transaction(
        ctx,
        from_addr,
        to_addr,
        to_address_safe,
        format_ethereum_amount(value_safe, None, chain_id),
        operation,
        str(safe_tx_gas),
        str(base_gas),
        format_ethereum_amount(gas_price_safe, None, chain_id),
        gas_token,
        refund_receiver,
        signatures,
        format_ethereum_amount(fee_max, None, chain_id),
        nonce,
        is_eip1559=True,
        chain_id=chain_id if is_unknown_network else None,
        call_data=call_data,
        call_method=call_method,
        max_priority_fee_per_gas=format_ethereum_amount(
            max_priority_fee_per_gas, None, chain_id
        ),
        max_fee_per_gas=format_ethereum_amount(max_fee_per_gas, None, chain_id),
    )
