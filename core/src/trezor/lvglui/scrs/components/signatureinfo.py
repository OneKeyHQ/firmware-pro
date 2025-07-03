from trezor.lvglui.i18n import gettext as _, keys as i18n_keys

from .. import font_GeistMono28, font_GeistSemiBold26, lv, lv_colors
from ..widgets.style import StyleWrapper
from .button import NormalButton
from .container import ContainerFlexCol
from .listitem import CardHeader, CardItem, DisplayItem
from .pageable import PageAbleMessage


class DirectionComponent:
    """Component for displaying transaction direction information (from/to addresses)"""

    def __init__(
        self,
        parent,
        title: str | None = None,
        icon: str | None = None,
        # Direction fields
        approve_spender: str | None = None,
        to_address: str | None = None,
        from_address: str | None = None,
        signer: str | None = None,
        fee_payer: str | None = None,
        voter: str | None = None,
        delegator: str | None = None,
        validator: str | None = None,
    ):
        self.parent = parent
        self.title = title or _(i18n_keys.FORM__DIRECTIONS)
        self.icon = icon or "A:/res/group-icon-directions.png"

        self.container = ContainerFlexCol(parent, None, padding_row=0, no_align=True)

        self.header = CardHeader(
            self.container,
            self.title,
            self.icon,
        )

        if approve_spender:
            DisplayItem(
                self.container,
                _(i18n_keys.APPROVE_PROVIDER),
                approve_spender,
            )

        if to_address:
            DisplayItem(
                self.container,
                _(i18n_keys.LIST_KEY__TO__COLON),
                to_address,
            )

        if from_address:
            DisplayItem(
                self.container,
                _(i18n_keys.LIST_KEY__FROM__COLON),
                from_address,
            )

        if signer:
            DisplayItem(
                self.container,
                _(i18n_keys.LIST_KEY__SIGNER__COLON),
                signer,
            )

        if fee_payer:
            DisplayItem(
                self.container,
                _(i18n_keys.LIST_KEY__FEE_PAYER__COLON),
                fee_payer,
            )

        if voter:
            DisplayItem(
                self.container,
                _(i18n_keys.LIST_KEY__VOTER__COLON),
                voter,
            )

        if delegator:
            DisplayItem(
                self.container,
                _(i18n_keys.LIST_KEY__DELEGATOR__COLON),
                delegator,
            )

        if validator:
            DisplayItem(
                self.container,
                _(i18n_keys.LIST_KEY__VALIDATOR__COLON),
                validator,
            )

        self.container.add_dummy()


class AmountComponent:
    """Component for displaying amount information"""

    def __init__(
        self,
        parent,
        title: str | None = None,
        icon: str | None = None,
        # Amount fields
        amount: str | None = None,
        token_amount: str | None = None,
    ):
        self.parent = parent
        self.title = title or _(i18n_keys.LIST_KEY__AMOUNT__COLON)
        self.icon = icon or "A:/res/group-icon-amount.png"

        self.container = ContainerFlexCol(parent, None, padding_row=0, no_align=True)

        self.header = CardHeader(
            self.container,
            self.title,
            self.icon,
        )

        if amount:
            DisplayItem(self.container, None, amount)

        if token_amount:
            DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__TOKEN_AMOUNT__COLON), token_amount
            )

        self.container.add_dummy()


class FeeComponent:
    """Component for displaying fee information"""

    def __init__(
        self,
        parent,
        title: str | None = None,
        icon: str | None = None,
        # Fee fields
        fee: str | None = None,
        maximum_fee: str | None = None,
        gas_price: str | None = None,
        priority_fee_per_gas: str | None = None,
        max_fee_per_gas: str | None = None,
        gas_limit: str | None = None,
        gas_fee_cap: str | None = None,
        gas_premium: str | None = None,
        tip: str | None = None,
        total_amount: str | None = None,
    ):
        self.parent = parent
        self.title = title or _(i18n_keys.FORM__FEES)
        self.icon = icon or "A:/res/group-icon-fees.png"

        self.container = ContainerFlexCol(parent, None, padding_row=0, no_align=True)

        self.header = CardHeader(
            self.container,
            self.title,
            self.icon,
        )

        if fee:
            DisplayItem(self.container, _(i18n_keys.LIST_KEY__FEE__COLON), fee)

        if maximum_fee:
            DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__MAXIMUM_FEE__COLON), maximum_fee
            )

        if gas_price:
            DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__GAS_PRICE__COLON), gas_price
            )

        if priority_fee_per_gas:
            DisplayItem(
                self.container,
                _(i18n_keys.LIST_KEY__PRIORITY_FEE_PER_GAS__COLON),
                priority_fee_per_gas,
            )

        if max_fee_per_gas:
            DisplayItem(
                self.container,
                _(i18n_keys.LIST_KEY__MAXIMUM_FEE_PER_GAS__COLON),
                max_fee_per_gas,
            )

        if gas_limit:
            DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__GAS_LIMIT__COLON), gas_limit
            )

        if gas_fee_cap:
            DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__GAS_FEE_CAP__COLON), gas_fee_cap
            )

        if gas_premium:
            DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__GAS_PREMIUM__COLON), gas_premium
            )

        if tip:
            DisplayItem(self.container, _(i18n_keys.LIST_KEY__TIP__COLON), tip)

        if total_amount:
            DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__TOTAL_AMOUNT__COLON), total_amount
            )

        self.container.add_dummy()


class MoreInfoComponent:
    """Component for displaying additional information"""

    def __init__(
        self,
        parent,
        title: str | None = None,
        icon: str | None = None,
        # More info fields
        chain_id: str | int | None = None,
        chain_name: str | None = None,
        contract_address: str | None = None,
        token_address: str | None = None,
        token_id: str | int | None = None,
        network_magic: str | int | None = None,
        asset_id: str | None = None,
        operation: str | None = None,
        nonce: str | int | None = None,
        type: str | None = None,
        format: str | None = None,
        destination_tag: str | int | None = None,
    ):
        self.parent = parent
        self.title = title or _(i18n_keys.FORM__MORE)
        self.icon = icon or "A:/res/group-icon-more.png"

        self.container = ContainerFlexCol(parent, None, padding_row=0, no_align=True)

        self.header = CardHeader(
            self.container,
            self.title,
            self.icon,
        )

        if chain_id is not None:
            DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__CHAIN_ID__COLON), str(chain_id)
            )

        if chain_name:
            DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__CHAIN_NAME__COLON), chain_name
            )

        if contract_address:
            DisplayItem(
                self.container,
                _(i18n_keys.LIST_KEY__CONTRACT_ADDRESS__COLON),
                contract_address,
            )

        if token_address:
            DisplayItem(self.container, _(i18n_keys.TOKEN_ADDRESS), token_address)

        if token_id is not None:
            DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__TOKEN_ID__COLON), str(token_id)
            )

        if network_magic is not None:
            DisplayItem(
                self.container, _(i18n_keys.GLOBAL_TARGET_NETWORK), str(network_magic)
            )

        if asset_id:
            DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__ASSET_ID__COLON), asset_id
            )

        if operation:
            DisplayItem(self.container, _(i18n_keys.GLOBAL_OPERATION), operation)

        if nonce is not None:
            DisplayItem(self.container, "Nonce:", str(nonce))

        if type:
            DisplayItem(self.container, _(i18n_keys.LIST_KEY__TYPE__COLON), type)

        if format:
            DisplayItem(self.container, _(i18n_keys.LIST_KEY__FORMAT__COLON), format)

        if destination_tag is not None:
            DisplayItem(
                self.container,
                _(i18n_keys.LIST_KEY__DESTINATION_TAG__COLON),
                str(destination_tag),
            )

        self.container.add_dummy()


class DataComponent:
    """Component for displaying data with support for long content"""

    def __init__(
        self,
        parent,
        data: str,
        label: str | None = None,
        icon: str | None = None,
        max_length: int = 225,
        primary_color=None,
    ):
        self.parent = parent
        self.data = data
        self.label = label or _(i18n_keys.LIST_KEY__DATA__COLON)
        self.icon = icon or "A:/res/group-icon-data.png"
        self.max_length = max_length
        self.primary_color = primary_color or lv_colors.ONEKEY_GREEN
        self.long_data = False

        if len(data) > max_length:
            self.long_data = True
            self.display_data = data[: max_length - 3] + "..."
        else:
            self.display_data = data

        self.card_item = CardItem(
            parent,
            self.label,
            self.display_data,
            self.icon,
        )

        if self.long_data:
            self._add_view_button()

    def _add_view_button(self):
        """Add view full data button"""
        self.view_button = NormalButton(
            self.card_item.content, _(i18n_keys.BUTTON__VIEW_DATA)
        )
        self.view_button.set_size(185, 77)
        self.view_button.add_style(
            StyleWrapper().text_font(font_GeistSemiBold26).pad_hor(24), 0
        )
        self.view_button.align(lv.ALIGN.CENTER, 0, 0)
        self.view_button.add_event_cb(self._on_view_data, lv.EVENT.CLICKED, None)

    def _on_view_data(self, event_obj):
        """Handle view full data button click"""
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.view_button:
                PageAbleMessage(
                    _(i18n_keys.TITLE__VIEW_DATA),
                    self.data,
                    None,
                    primary_color=self.primary_color,
                    font=font_GeistMono28,
                    confirm_text=None,
                    cancel_text=None,
                )


class MemoComponent:
    """Component for displaying memo/note information"""

    def __init__(
        self, parent, memo: str, label: str | None = None, icon: str | None = None
    ):
        self.parent = parent
        self.memo = memo
        self.label = label or _(i18n_keys.LIST_KEY__MEMO__COLON)
        self.icon = icon or "A:/res/group-icon-more.png"

        self.card_item = CardItem(
            parent,
            self.label,
            memo,
            self.icon,
        )


class OverviewComponent:
    """Component for displaying overview information"""

    def __init__(
        self,
        parent,
        title: str | None = None,
        icon: str | None = None,
        # Overview fields
        to_address: str | None = None,
        approve_spender: str | None = None,
        max_fee: str | None = None,
        token_address: str | None = None,
    ):
        self.parent = parent
        self.title = title or _(i18n_keys.OVERVIEW)
        self.icon = icon or "A:/res/group-icon-more.png"

        has_content = any([to_address, approve_spender, token_address, max_fee])

        if not has_content:
            return

        self.group = ContainerFlexCol(parent, None, padding_row=0, no_align=True)
        self.group_header = CardHeader(self.group, self.title, self.icon)

        if to_address:
            DisplayItem(
                self.group,
                _(i18n_keys.LIST_KEY__TO__COLON),
                to_address,
            )

        if approve_spender:
            DisplayItem(
                self.group,
                _(i18n_keys.APPROVE_PROVIDER),
                approve_spender,
            )

        if max_fee:
            DisplayItem(self.group, _(i18n_keys.LIST_KEY__MAXIMUM_FEE__COLON), max_fee)

        if token_address:
            DisplayItem(
                self.group,
                _(i18n_keys.TOKEN_ADDRESS),
                token_address,
            )

        self.group.add_dummy()
