import utime

from trezor import utils
from trezor.lvglui.scrs.components.button import NormalButton
from trezor.lvglui.scrs.components.pageable import PageAbleMessage

from ..i18n import gettext as _, keys as i18n_keys
from ..lv_colors import lv_colors
from ..lv_symbols import LV_SYMBOLS
from . import (
    font_GeistMono28,
    font_GeistRegular20,
    font_GeistRegular30,
    font_GeistSemiBold26,
    font_GeistSemiBold38,
    font_GeistSemiBold48,
)
from .common import FullSizeWindow, lv
from .components.banner import Banner
from .components.container import ContainerFlexCol
from .components.listitem import CardHeader, CardItem, DisplayItem
from .components.qrcode import QRCode
from .widgets.style import StyleWrapper


class Address(FullSizeWindow):
    class SHOW_TYPE:
        ADDRESS = 0
        QRCODE = 1

    def __init__(
        self,
        title,
        path,
        address,
        primary_color,
        icon_path: str,
        xpubs=None,
        address_qr=None,
        multisig_index: int | None = 0,
        addr_type=None,
        evm_chain_id: int | None = None,
        qr_first: bool = False,
    ):
        super().__init__(
            title,
            None,
            confirm_text=_(i18n_keys.BUTTON__DONE),
            cancel_text=_(i18n_keys.BUTTON__QRCODE)
            if not qr_first
            else _(i18n_keys.BUTTON__ADDRESS),
            anim_dir=0,
            primary_color=primary_color,
        )
        self.path = path
        self.xpubs = xpubs
        self.multisig_index = multisig_index
        self.address = address
        self.address_qr = address_qr
        self.icon = icon_path
        self.addr_type = addr_type
        self.evm_chain_id = evm_chain_id
        if primary_color:
            self.title.add_style(StyleWrapper().text_color(primary_color), 0)
        self.qr_first = qr_first
        if qr_first:
            self.show_qr_code(self.qr_first)
        else:
            self.show_address(evm_chain_id=evm_chain_id)

    def show_address(self, evm_chain_id: int | None = None):
        self.current = self.SHOW_TYPE.ADDRESS
        if hasattr(self, "qr"):
            self.qr.delete()
            del self.qr
        if hasattr(self, "subtitle"):
            self.subtitle.delete()
            del self.subtitle
        self.btn_no.label.set_text(_(i18n_keys.BUTTON__QRCODE))

        self.item_addr = DisplayItem(self.content_area, None, self.address, radius=40)
        self.item_addr.add_style(StyleWrapper().pad_ver(24), 0)
        self.item_addr.label.add_style(
            StyleWrapper()
            .text_font(font_GeistSemiBold48)
            .text_line_space(-2)
            .text_color(lv_colors.LIGHT_GRAY),
            0,
        )
        self.item_addr.align_to(self.title, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 40)

        self.container = ContainerFlexCol(
            self.content_area, self.item_addr, pos=(0, 8), padding_row=0
        )
        self.container.add_dummy()
        if self.addr_type:
            self.item_type = DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__TYPE__COLON), self.addr_type
            )
        if evm_chain_id:
            self.item_chin_id = DisplayItem(
                self.container,
                _(i18n_keys.LIST_KEY__CHAIN_ID__COLON),
                str(evm_chain_id),
            )
        self.item_path = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__PATH__COLON), self.path
        )
        self.container.add_dummy()
        self.xpub_group = ContainerFlexCol(
            self.content_area,
            self.container,
            pos=(0, 8),
            clip_corner=False,
        )
        for i, xpub in enumerate(self.xpubs or []):
            self.item3 = CardItem(
                self.xpub_group,
                _(i18n_keys.LIST_KEY__XPUB_STR_MINE__COLON).format(i + 1)
                if i == self.multisig_index
                else _(i18n_keys.LIST_KEY__XPUB_STR_COSIGNER__COLON).format(i + 1),
                xpub,
                "A:/res/group-icon-more.png",
            )

    def show_qr_code(self, has_tips: bool = False):
        self.current = self.SHOW_TYPE.QRCODE
        if hasattr(self, "container"):
            self.container.delete()
            del self.container
        if hasattr(self, "xpub_group"):
            self.xpub_group.delete()
            del self.xpub_group
        if hasattr(self, "item_addr"):
            self.item_addr.delete()
            del self.item_addr
        self.btn_no.label.set_text(_(i18n_keys.BUTTON__ADDRESS))
        if has_tips:
            from .components.label import SubTitle

            self.subtitle = SubTitle(
                self.content_area,
                self.title,
                (0, 16),
                _(
                    i18n_keys.CONTENT__RETUNRN_TO_THE_APP_AND_SCAN_THE_SIGNED_TX_QR_CODE_BELOW
                ),
            )
        self.qr = QRCode(
            self.content_area,
            self.address if self.address_qr is None else self.address_qr,
            self.icon,
        )
        self.qr.align_to(
            self.title if not has_tips else self.subtitle,
            lv.ALIGN.OUT_BOTTOM_LEFT,
            0,
            30,
        )

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            utils.lcd_resume()
            if target == self.btn_no:
                if self.current == self.SHOW_TYPE.ADDRESS:
                    self.show_qr_code(self.qr_first)
                else:
                    self.show_address(self.evm_chain_id)
            elif target == self.btn_yes:
                self.show_unload_anim()
                self.channel.publish(1)


class XpubOrPub(FullSizeWindow):
    def __init__(
        self, title, path, primary_color, icon_path: str, xpub=None, pubkey=None
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__EXPORT),
            _(i18n_keys.BUTTON__CANCEL),
            anim_dir=2,
            icon_path=icon_path,
            primary_color=primary_color,
        )
        self.title.add_style(StyleWrapper().text_color(primary_color), 0)
        self.item_xpub_or_pub = CardItem(
            self.content_area,
            _(i18n_keys.LIST_KEY__XPUB__COLON)
            if xpub
            else _(i18n_keys.LIST_KEY__PUBLIC_KEY__COLON),
            xpub or pubkey,
            "A:/res/group-icon-more.png",
        )
        self.item_xpub_or_pub.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, 40)
        self.container = ContainerFlexCol(
            self.content_area, self.item_xpub_or_pub, pos=(0, 16), padding_row=0
        )
        self.container.add_dummy()
        self.item_path = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__PATH__COLON), path
        )
        self.container.add_dummy()


class Message(FullSizeWindow):
    def __init__(
        self,
        title,
        address,
        message,
        primary_color,
        icon_path,
        verify: bool = False,
        evm_chain_id: int | None = None,
        item_addr_title: str | None = None,
        is_standard: bool = True,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__VERIFY) if verify else _(i18n_keys.BUTTON__SIGN),
            _(i18n_keys.BUTTON__CANCEL),
            anim_dir=2,
            primary_color=primary_color,
            icon_path=icon_path,
        )
        self.primary_color = primary_color
        self.long_message = False
        self.full_message = message
        if len(message) > 150:
            self.message = message[:147] + "..."
            self.long_message = True
        else:
            self.message = message
        if not is_standard:
            self.warning_banner = Banner(
                self.content_area,
                2,
                _(i18n_keys.CONTENT__NON_STANDARD_MESSAGE_SIGNATURE),
            )
            self.warning_banner.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, 40)
        self.item_message = CardItem(
            self.content_area,
            _(i18n_keys.LIST_KEY__MESSAGE__COLON),
            self.message,
            "A:/res/group-icon-data.png",
        )
        self.item_message.align_to(
            self.title if is_standard else self.warning_banner,
            lv.ALIGN.OUT_BOTTOM_LEFT,
            0,
            40 if is_standard else 8,
        )
        if self.long_message:
            self.show_full_message = NormalButton(
                self.item_message.content, _(i18n_keys.BUTTON__VIEW_DATA)
            )
            self.show_full_message.set_size(lv.SIZE.CONTENT, 77)
            self.show_full_message.add_style(
                StyleWrapper().text_font(font_GeistSemiBold26), 0
            )
            self.show_full_message.align(lv.ALIGN.CENTER, 0, 0)
            self.show_full_message.remove_style(None, lv.PART.MAIN | lv.STATE.PRESSED)
            self.show_full_message.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.container = ContainerFlexCol(
            self.content_area, self.item_message, pos=(0, 8), padding_row=0
        )
        self.container.add_dummy()
        if evm_chain_id:
            self.item_chain_id = DisplayItem(
                self.container,
                _(i18n_keys.LIST_KEY__CHAIN_ID__COLON),
                str(evm_chain_id),
            )
        self.item_addr = DisplayItem(
            self.container,
            _(i18n_keys.LIST_KEY__ADDRESS__COLON)
            if not item_addr_title
            else item_addr_title,
            address,
        )
        self.container.add_dummy()

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.show_full_message:
                PageAbleMessage(
                    _(i18n_keys.TITLE__MESSAGE),
                    self.full_message,
                    None,
                    primary_color=self.primary_color,
                    font=font_GeistMono28,
                    confirm_text=None,
                    cancel_text=None,
                )


class TransactionOverview(FullSizeWindow):
    def __init__(self, title, address, primary_color, icon_path, has_details=None):
        if __debug__:
            self.layout_address = address

        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            anim_dir=2,
            primary_color=primary_color,
            icon_path="A:/res/icon-send.png",
            sub_icon_path=icon_path or "A:/res/evm-eth.png",
        )
        self.group_directions = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=0
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_to = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__TO__COLON),
            address,
        )
        self.group_directions.add_dummy()

        if has_details:
            self.view_btn = NormalButton(
                self.content_area,
                f"{LV_SYMBOLS.LV_SYMBOL_ANGLE_DOUBLE_DOWN}  {_(i18n_keys.BUTTON__DETAILS)}",
            )
            self.view_btn.set_size(456, 82)
            self.view_btn.add_style(StyleWrapper().text_font(font_GeistSemiBold26), 0)
            self.view_btn.enable()
            self.view_btn.align_to(self.group_directions, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)
            self.view_btn.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.view_btn:
                self.destroy(400)
                self.channel.publish(2)

    if __debug__:

        def read_content(self) -> list[str]:
            return (
                [self.layout_title or ""]
                + [self.layout_subtitle or ""]
                + [self.layout_address or ""]
            )


class TransactionDetailsETH(FullSizeWindow):
    def __init__(
        self,
        title,
        address_from,
        address_to,
        amount,
        fee_max,
        is_eip1559=False,
        gas_price=None,
        max_priority_fee_per_gas=None,
        max_fee_per_gas=None,
        total_amount=None,
        primary_color=lv_colors.ONEKEY_GREEN,
        contract_addr=None,
        token_id=None,
        evm_chain_id=None,
        raw_data=None,
        sub_icon_path=None,
        striped=False,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path="A:/res/icon-send.png",
            sub_icon_path=sub_icon_path,
        )
        self.primary_color = primary_color
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        if striped:
            self.group_amounts = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_amounts,
                _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                "A:/res/group-icon-amount.png",
            )
            self.group_body_amount = DisplayItem(
                self.group_amounts,
                None,
                amount,
            )
            self.group_amounts.add_dummy()

        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_to_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__TO__COLON),
            address_to,
        )
        self.item_group_body_from_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FROM__COLON),
            address_from,
        )
        self.group_directions.add_dummy()

        self.group_fees = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
        )
        self.item_group_body_fee_max = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__MAXIMUM_FEE__COLON),
            fee_max,
        )
        if not is_eip1559:
            if gas_price:
                self.item_group_body_gas_price = DisplayItem(
                    self.group_fees,
                    _(i18n_keys.LIST_KEY__GAS_PRICE__COLON),
                    gas_price,
                )
        else:
            self.item_group_body_priority_fee_per_gas = DisplayItem(
                self.group_fees,
                _(i18n_keys.LIST_KEY__PRIORITY_FEE_PER_GAS__COLON),
                max_priority_fee_per_gas,
            )
            self.item_group_body_max_fee_per_gas = DisplayItem(
                self.group_fees,
                _(i18n_keys.LIST_KEY__MAXIMUM_FEE_PER_GAS__COLON),
                max_fee_per_gas,
            )
        if total_amount is None:
            if not contract_addr:  # token transfer
                total_amount = f"{amount}\n{fee_max}"
            else:  # nft transfer
                total_amount = f"{fee_max}"
        self.item_group_body_total_amount = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__TOTAL_AMOUNT__COLON),
            total_amount,
        )
        self.group_fees.add_dummy()

        if contract_addr or evm_chain_id:
            self.group_more = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
            )
            if evm_chain_id:
                self.item_group_body_chain_id = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__CHAIN_ID__COLON),
                    str(evm_chain_id),
                )
            if contract_addr:
                self.item_group_body_contract_addr = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__CONTRACT_ADDRESS__COLON),
                    contract_addr,
                )
                self.item_group_body_token_id = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__TOKEN_ID__COLON),
                    token_id,
                )
            self.group_more.add_dummy()

        if raw_data:
            from trezor import strings

            self.data_str = strings.format_customer_data(raw_data)
            if not self.data_str:
                return
            self.long_data = False
            if len(self.data_str) > 225:
                self.long_data = True
                self.data = self.data_str[:222] + "..."
            else:
                self.data = self.data_str
            self.item_data = CardItem(
                self.container,
                _(i18n_keys.LIST_KEY__DATA__COLON),
                self.data,
                "A:/res/group-icon-data.png",
            )
            if self.long_data:
                self.show_full_data = NormalButton(
                    self.item_data.content, _(i18n_keys.BUTTON__VIEW_DATA)
                )
                self.show_full_data.set_size(lv.SIZE.CONTENT, 77)
                self.show_full_data.add_style(
                    StyleWrapper().text_font(font_GeistSemiBold26).pad_hor(24), 0
                )
                self.show_full_data.align(lv.ALIGN.CENTER, 0, 0)
                self.show_full_data.remove_style(None, lv.PART.MAIN | lv.STATE.PRESSED)
                self.show_full_data.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.show_full_data:
                PageAbleMessage(
                    _(i18n_keys.TITLE__VIEW_DATA),
                    self.data_str,
                    None,
                    primary_color=self.primary_color,
                    font=font_GeistMono28,
                    confirm_text=None,
                    cancel_text=None,
                )


class TransactionDetailsAlepHium(FullSizeWindow):
    def __init__(
        self,
        title,
        address_from,
        address_to,
        subtitle=None,
        amount=None,
        gas_amount=None,
        primary_color=lv_colors.ONEKEY_GREEN,
        token_id=None,
        raw_data=None,
        icon_path=None,
        sub_icon_path=None,
        token_amount=None,
    ):
        super().__init__(
            title,
            subtitle,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path=icon_path,
            sub_icon_path=sub_icon_path,
        )
        self.primary_color = primary_color
        if raw_data:
            self.container = ContainerFlexCol(
                self.content_area, self.subtitle, pos=(0, 40)
            )
        else:
            self.container = ContainerFlexCol(
                self.content_area, self.title, pos=(0, 40)
            )
        if amount:
            self.group_directions = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_directions,
                _(i18n_keys.FORM__DIRECTIONS),
                "A:/res/group-icon-directions.png",
            )
            self.item_group_body_to_addr = DisplayItem(
                self.group_directions,
                _(i18n_keys.LIST_KEY__TO__COLON),
                address_to,
            )
            self.item_group_body_from_addr = DisplayItem(
                self.group_directions,
                _(i18n_keys.LIST_KEY__FROM__COLON),
                address_from,
            )
            self.group_directions.add_dummy()

        if token_amount:
            self.group_amounts = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_amounts,
                _(i18n_keys.SUBTITLE__ADA_TX_CONTAINS_TOKEN),
                "A:/res/notice.png",
            )
            self.group_amounts.add_dummy()
            self.group_directions = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_directions,
                _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                "A:/res/group-icon-directions.png",
            )
            self.item_group_body_to_addr = DisplayItem(
                self.group_directions,
                _(i18n_keys.LIST_KEY__TOKEN_ID__COLON),
                token_id,
            )
            self.item_group_body_from_addr = DisplayItem(
                self.group_directions,
                _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                str(token_amount),
            )
            self.group_directions.add_dummy()

            self.group_directions = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_directions,
                _(i18n_keys.FORM__DIRECTIONS),
                "A:/res/group-icon-directions.png",
            )
            self.item_group_body_to_addr = DisplayItem(
                self.group_directions,
                _(i18n_keys.LIST_KEY__TO__COLON),
                address_to,
            )
            self.item_group_body_from_addr = DisplayItem(
                self.group_directions,
                _(i18n_keys.LIST_KEY__FROM__COLON),
                address_from,
            )
            self.group_directions.add_dummy()

        if gas_amount:
            self.group_fees = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
            )
            self.item_group_body_gas_price = DisplayItem(
                self.group_fees,
                _(i18n_keys.LIST_KEY__TRANSACTION_FEE__COLON),
                gas_amount,
            )

        if raw_data:
            from trezor import strings

            self.data_str = strings.format_customer_data(raw_data)
            if not self.data_str:
                return
            self.long_data = False
            if len(self.data_str) > 225:
                self.long_data = True
                self.data = self.data_str[:222] + "..."
            else:
                self.data = self.data_str

            self.item_data = CardItem(
                self.container,
                _(i18n_keys.LIST_KEY__DATA__COLON),
                self.data,
                "A:/res/group-icon-data.png",
            )

            if self.long_data:
                self.show_full_data = NormalButton(
                    self.item_data.content, _(i18n_keys.BUTTON__VIEW_DATA)
                )
                self.show_full_data.set_size(lv.SIZE.CONTENT, 77)
                self.show_full_data.add_style(
                    StyleWrapper().text_font(font_GeistSemiBold26).pad_hor(24), 0
                )
                self.show_full_data.align(lv.ALIGN.CENTER, 0, 0)
                self.show_full_data.remove_style(None, lv.PART.MAIN | lv.STATE.PRESSED)
                self.show_full_data.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.show_full_data:
                PageAbleMessage(
                    _(i18n_keys.TITLE__VIEW_DATA),
                    self.data_str,
                    None,
                    primary_color=self.primary_color,
                    font=font_GeistMono28,
                    confirm_text=None,
                    cancel_text=None,
                )


class ContractDataOverview(FullSizeWindow):
    def __init__(self, title, description, data, primary_color):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            anim_dir=0,
        )
        self.primary_color = primary_color
        self.container = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=0
        )
        self.container.add_dummy()
        self.item_size = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__SIZE__COLON), description
        )
        self.container.add_dummy()

        self.data_str = data
        self.long_data = False
        if len(self.data_str) > 225:
            self.long_data = True
            self.data = self.data_str[:222] + "..."
        else:
            self.data = self.data_str
        self.item_data = CardItem(
            self.content_area,
            _(i18n_keys.LIST_KEY__DATA__COLON),
            self.data,
            "A:/res/group-icon-data.png",
        )
        self.item_data.align_to(self.container, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)
        if self.long_data:
            self.show_full_data = NormalButton(
                self.item_data.content, _(i18n_keys.BUTTON__VIEW_DATA)
            )
            self.show_full_data.set_size(lv.SIZE.CONTENT, 77)
            self.show_full_data.add_style(
                StyleWrapper().text_font(font_GeistSemiBold26).pad_hor(24), 0
            )
            self.show_full_data.align(lv.ALIGN.CENTER, 0, 0)
            self.show_full_data.remove_style(None, lv.PART.MAIN | lv.STATE.PRESSED)
            self.show_full_data.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.show_full_data:
                PageAbleMessage(
                    _(i18n_keys.TITLE__VIEW_DATA),
                    self.data_str,
                    None,
                    primary_color=self.primary_color,
                    font=font_GeistMono28,
                    confirm_text=None,
                    cancel_text=None,
                )


class BlobDisPlay(FullSizeWindow):
    def __init__(
        self,
        title,
        description: str,
        content: str,
        icon_path: str = "A:/res/warning.png",
        anim_dir: int = 1,
        primary_color=lv_colors.ONEKEY_GREEN,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__CANCEL),
            icon_path=icon_path,
            anim_dir=anim_dir,
            primary_color=primary_color or lv_colors.ONEKEY_GREEN,
        )
        self.primary_color = primary_color
        self.container = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=0
        )
        self.container.add_dummy()
        self.item_data = DisplayItem(self.container, description, content)
        self.container.add_dummy()
        self.long_message = False
        if len(content) > 240:
            self.long_message = True
            self.btn_yes.label.set_text(_(i18n_keys.BUTTON__VIEW))
            self.data = content

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.btn_yes:
                if self.long_message:
                    PageAbleMessage(
                        _(i18n_keys.TITLE__MESSAGE),
                        self.data,
                        self.channel,
                        primary_color=self.primary_color,
                    )
                    self.destroy()
                else:
                    self.show_unload_anim()
                    self.channel.publish(1)
            elif target == self.btn_no:
                self.show_dismiss_anim()
                self.channel.publish(0)


class ConfirmMetaData(FullSizeWindow):
    def __init__(self, title, subtitle, description, data, primary_color, icon_path):
        if __debug__:
            self.layout_data = data

        super().__init__(
            title,
            subtitle,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path=icon_path,
        )

        if description:
            self.container = ContainerFlexCol(
                self.content_area, self.subtitle, pos=(0, 40), padding_row=0
            )
            self.container.add_dummy()
            self.item1 = DisplayItem(self.container, description, data)
            self.container.add_dummy()

    if __debug__:

        def read_content(self) -> list[str]:
            return (
                [self.layout_title or ""]
                + [self.layout_subtitle or ""]
                + [self.layout_data or ""]
            )


class TransactionDetailsBTC(FullSizeWindow):
    def __init__(
        self,
        title: str,
        amount: str,
        fee: str,
        total: str,
        primary_color,
        icon_path: str,
        striped=False,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path="A:/res/icon-send.png",
            sub_icon_path=icon_path,
            anim_dir=0,
        )
        self.container = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=8
        )
        if striped:
            self.group_amounts = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_amounts,
                _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                "A:/res/group-icon-amount.png",
            )
            self.group_body_amount = DisplayItem(
                self.group_amounts,
                None,
                amount,
            )
            self.group_amounts.add_dummy()
        self.group_fees = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
        )
        self.item_group_body_fee = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__FEE__COLON),
            fee,
        )
        self.item_group_body_total_amount = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__TOTAL_AMOUNT__COLON),
            total,
        )
        self.group_fees.add_dummy()


class JointTransactionDetailsBTC(FullSizeWindow):
    def __init__(self, title: str, amount: str, total: str, primary_color):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=0
        )
        self.container.add_dummy()
        self.item_spend = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__AMOUNT_YOU_SPEND__COLON), amount
        )
        self.item_total = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__TOTAL_AMOUNT__COLON), total
        )
        self.container.add_dummy()


class ModifyFee(FullSizeWindow):
    def __init__(self, description: str, fee_change: str, fee_new: str, primary_color):
        super().__init__(
            _(i18n_keys.TITLE__MODIFY_FEE),
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=0
        )
        self.container.add_dummy()
        self.item_fee_change = DisplayItem(self.container, description, fee_change)
        self.item_fee_new = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__NEW_FEE__COLON), fee_new
        )
        self.container.add_dummy()


class ModifyOutput(FullSizeWindow):
    def __init__(
        self,
        address: str,
        description: str,
        amount_change: str,
        amount_new: str,
        primary_Color,
    ):
        super().__init__(
            _(i18n_keys.TITLE__MODIFY_AMOUNT),
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_Color,
        )
        self.container = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=0
        )
        self.container.add_dummy()
        self.item_addr = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__ADDRESS__COLON), address
        )
        self.item_amount_change = DisplayItem(
            self.container, description, amount_change
        )
        self.item_amount_new = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__NEW_AMOUNT__COLON), amount_new
        )
        self.container.add_dummy()


class ConfirmReplacement(FullSizeWindow):
    def __init__(self, title: str, txids: list[str], primary_color):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        for txid in txids:
            self.item_body_tx_id = DisplayItem(
                self.group_directions,
                _(i18n_keys.LIST_KEY__TRANSACTION_ID__COLON),
                txid,
            )
        self.group_directions.add_dummy()


class ConfirmPaymentRequest(FullSizeWindow):
    def __init__(self, title: str, subtitle, amount: str, to_addr: str, primary_color):
        super().__init__(
            title,
            subtitle,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=0
        )
        self.container.add_dummy()
        self.item_to_addr = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__TO__COLON), to_addr
        )
        self.item_amount = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__AMOUNT__COLON), amount
        )
        self.container.add_dummy()


class ConfirmDecredSstxSubmission(FullSizeWindow):
    def __init__(
        self, title: str, subtitle: str, amount: str, to_addr: str, primary_color
    ):
        super().__init__(
            title,
            subtitle,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=0
        )
        self.container.add_dummy()
        self.item_amount = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__AMOUNT__COLON), amount
        )
        self.item_to_addr = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__TO__COLON), to_addr
        )
        self.container.add_dummy()


class ConfirmCoinJoin(FullSizeWindow):
    def __init__(
        self,
        title: str,
        coin_name: str,
        max_rounds: str,
        max_fee_per_vbyte: str,
        primary_color,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=0
        )
        self.container.add_dummy()
        self.item_coin_name = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__COIN_NAME__COLON), coin_name
        )
        self.item_mrc = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__MAXIMUM_ROUNDS__COLON), max_rounds
        )
        self.item_fee_rate = DisplayItem(
            self.container,
            _(i18n_keys.LIST_KEY__MAXIMUM_MINING_FEE__COLON),
            max_fee_per_vbyte,
        )
        self.container.add_dummy()


class ConfirmSignIdentity(FullSizeWindow):
    def __init__(self, title: str, identity: str, subtitle: str | None, primary_color):
        super().__init__(
            title,
            subtitle,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
        )
        align_base = self.title if subtitle is None else self.subtitle
        self.container = ContainerFlexCol(
            self.content_area, align_base, pos=(0, 40), padding_row=0
        )
        self.container.add_dummy()
        self.item_id = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__IDENTITY__COLON), identity
        )
        self.container.add_dummy()


class ConfirmProperties(FullSizeWindow):
    def __init__(self, title: str, properties: list[tuple[str, str]], primary_color):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__CANCEL),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=0
        )
        self.container.add_dummy()
        for key, value in properties:
            self.item = DisplayItem(self.container, f"{key.upper()}", value)
        self.container.add_dummy()


class ConfirmTransferBinance(FullSizeWindow):
    def __init__(self, items: list[tuple[str, str, str]], primary_color, icon_path):
        super().__init__(
            _(i18n_keys.TITLE__CONFIRM_TRANSFER),
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path=icon_path,
        )
        self.container = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=0
        )
        self.container.add_dummy()
        for key, value, address in items:
            self.item_key = DisplayItem(self.container, key, "")
            self.item_amount = DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__AMOUNT__COLON), value
            )
            self.item_to_addr = DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__TO__COLON), address
            )
        self.container.add_dummy()


class ShouldShowMore(FullSizeWindow):
    def __init__(
        self, title: str, key: str, value: str, button_text: str, primary_color
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__CANCEL),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=0
        )
        self.container.add_dummy()
        self.item = DisplayItem(self.container, f"{key}:", value)
        self.container.add_dummy()
        self.show_more = NormalButton(self.content_area, button_text)
        self.show_more.align_to(self.container, lv.ALIGN.OUT_BOTTOM_MID, 0, 32)
        self.show_more.add_event_cb(self.on_show_more, lv.EVENT.CLICKED, None)

    def on_show_more(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.show_more:
                # 2 means show more
                self.channel.publish(2)
                self.destroy()


class EIP712DOMAIN(FullSizeWindow):
    def __init__(self, title: str, primary_color, icon_path, **kwargs):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__CANCEL),
            anim_dir=2,
            primary_color=primary_color,
            icon_path=icon_path,
        )
        self.container = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=0
        )
        self.container.add_dummy()
        if kwargs.get("name"):
            self.item_name = DisplayItem(
                self.container, "name (string):", kwargs.get("name")
            )
        if kwargs.get("version"):
            self.item_version = DisplayItem(
                self.container, "version (string):", kwargs.get("version")
            )
        if kwargs.get("chainId"):
            self.item_chain_id = DisplayItem(
                self.container, "chainId (uint256):", kwargs.get("chainId")
            )
        if kwargs.get("verifyingContract"):
            self.item_vfc = DisplayItem(
                self.container,
                "verifyingContract (address):",
                kwargs.get("verifyingContract"),
            )
        if kwargs.get("salt"):
            self.item_salt = DisplayItem(
                self.container, "salt (bytes32):", kwargs.get("salt")
            )
        self.container.add_dummy()


class EIP712Warning(FullSizeWindow):
    def __init__(
        self, title: str, warning_level, text, primary_type, primary_color, icon_path
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            anim_dir=2,
            primary_color=primary_color,
            icon_path=icon_path,
        )
        self.warning_banner = Banner(self.content_area, warning_level, text)
        self.warning_banner.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, 40)
        self.container = ContainerFlexCol(
            self.content_area, self.warning_banner, pos=(0, 24), padding_row=0
        )
        self.container.add_dummy(bg_color=lv_colors.ONEKEY_BLACK_3)
        self.primary_type = DisplayItem(
            self.container,
            "PrimaryType:",
            primary_type,
            bg_color=lv_colors.ONEKEY_BLACK_3,
        )
        self.container.add_dummy(bg_color=lv_colors.ONEKEY_BLACK_3)


class TonTransfer(FullSizeWindow):
    def __init__(
        self,
        address_from,
        address_to,
        amount,
        memo,
        primary_color=None,
    ):
        super().__init__(
            _(i18n_keys.TITLE__SIGN_STR_TRANSACTION).format("TON"),
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        self.item1 = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__AMOUNT__COLON), amount
        )
        self.item2 = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__TO__COLON), address_to
        )
        self.item3 = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__FROM__COLON), address_from
        )
        if memo:
            self.item4 = DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__MEMO__COLON), memo
            )


class TonTransaction(FullSizeWindow):
    def __init__(
        self,
        title,
        address_from,
        address_to,
        amount,
        fee_max,
        is_eip1559=False,
        gas_price=None,
        max_priority_fee_per_gas=None,
        max_fee_per_gas=None,
        total_amount=None,
        primary_color=lv_colors.ONEKEY_GREEN,
        contract_addr=None,
        token_id=None,
        evm_chain_id=None,
        raw_data=None,
        sub_icon_path=None,
        striped=False,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path="A:/res/icon-send.png",
            sub_icon_path=sub_icon_path,
        )
        self.primary_color = primary_color
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        if striped:
            self.group_amounts = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_amounts,
                _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                "A:/res/group-icon-amount.png",
            )
            self.group_body_amount = DisplayItem(
                self.group_amounts,
                None,
                amount,
            )
            self.group_amounts.add_dummy()

        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_to_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__TO__COLON),
            address_to,
        )
        self.item_group_body_from_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FROM__COLON),
            address_from,
        )
        self.group_directions.add_dummy()

        self.group_fees = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
        )
        self.item_group_body_fee_max = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__MAXIMUM_FEE__COLON),
            fee_max,
        )
        if not is_eip1559:
            if gas_price:
                self.item_group_body_gas_price = DisplayItem(
                    self.group_fees,
                    _(i18n_keys.LIST_KEY__GAS_PRICE__COLON),
                    gas_price,
                )
        else:
            self.item_group_body_priority_fee_per_gas = DisplayItem(
                self.group_fees,
                _(i18n_keys.LIST_KEY__PRIORITY_FEE_PER_GAS__COLON),
                max_priority_fee_per_gas,
            )
            self.item_group_body_max_fee_per_gas = DisplayItem(
                self.group_fees,
                _(i18n_keys.LIST_KEY__MAXIMUM_FEE_PER_GAS__COLON),
                max_fee_per_gas,
            )
        if total_amount is None:
            if not contract_addr:  # token transfer
                total_amount = f"{amount}\n{fee_max}"
            else:  # nft transfer
                total_amount = f"{fee_max}"
        self.item_group_body_total_amount = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__TOTAL_AMOUNT__COLON),
            total_amount,
        )
        self.group_fees.add_dummy()

        if contract_addr or evm_chain_id:
            self.group_more = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
            )
            if evm_chain_id:
                self.item_group_body_chain_id = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__CHAIN_ID__COLON),
                    str(evm_chain_id),
                )
            if contract_addr:
                self.item_group_body_contract_addr = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__CONTRACT_ADDRESS__COLON),
                    contract_addr,
                )
                self.item_group_body_token_id = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__TOKEN_ID__COLON),
                    token_id,
                )
            self.group_more.add_dummy()

        if raw_data:
            from trezor import strings

            self.data_str = strings.format_customer_data(raw_data)
            if not self.data_str:
                return
            self.long_data = False
            if len(self.data_str) > 225:
                self.long_data = True
                self.data = self.data_str[:222] + "..."
            else:
                self.data = self.data_str
            self.item_data = CardItem(
                self.container,
                _(i18n_keys.LIST_KEY__DATA__COLON),
                self.data,
                "A:/res/group-icon-data.png",
            )
            if self.long_data:
                self.show_full_data = NormalButton(
                    self.item_data.content, _(i18n_keys.BUTTON__VIEW_DATA)
                )
                self.show_full_data.set_size(lv.SIZE.CONTENT, 77)
                self.show_full_data.add_style(
                    StyleWrapper().text_font(font_GeistSemiBold26).pad_hor(24), 0
                )
                self.show_full_data.align(lv.ALIGN.CENTER, 0, 0)
                self.show_full_data.remove_style(None, lv.PART.MAIN | lv.STATE.PRESSED)
                self.show_full_data.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.show_full_data:
                PageAbleMessage(
                    _(i18n_keys.TITLE__VIEW_DATA),
                    self.data_str,
                    None,
                    primary_color=self.primary_color,
                    font=font_GeistMono28,
                    confirm_text=None,
                    cancel_text=None,
                )


class TonConnect(FullSizeWindow):
    def __init__(
        self,
        doamin,
        address,
        payload,
        primary_color=None,
    ):
        super().__init__(
            _(i18n_keys.TITLE__SIGN_STR_MESSAGE).format("TON"),
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        self.item1 = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__DOMAIN__COLON), doamin
        )
        self.item2 = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__ADDRESS__COLON), address
        )
        # self.item3 = DisplayItem(
        #     self.container, _(i18n_keys.LIST_KEY__FROM__COLON), address_from
        # )
        if payload:
            self.item3 = DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__MEMO__COLON), payload
            )


class TonMessage(FullSizeWindow):
    def __init__(
        self,
        title,
        address,
        message,
        domain,
        primary_color,
        icon_path,
        verify: bool = False,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__VERIFY) if verify else _(i18n_keys.BUTTON__SIGN),
            _(i18n_keys.BUTTON__CANCEL),
            anim_dir=2,
            primary_color=primary_color,
            icon_path=icon_path,
        )
        self.primary_color = primary_color
        self.long_message = False
        self.full_message = message
        if len(message) > 150:
            self.message = message[:147] + "..."
            self.long_message = True
        else:
            self.message = message
        self.item_message = CardItem(
            self.content_area,
            _(i18n_keys.LIST_KEY__MESSAGE__COLON),
            self.message,
            "A:/res/group-icon-data.png",
        )
        self.item_message.align_to(self.title, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 40)
        if self.long_message:
            self.show_full_message = NormalButton(
                self.item_message.content, _(i18n_keys.BUTTON__VIEW_DATA)
            )
            self.show_full_message.set_size(185, 77)
            self.show_full_message.add_style(
                StyleWrapper().text_font(font_GeistSemiBold26), 0
            )
            self.show_full_message.align(lv.ALIGN.CENTER, 0, 0)
            self.show_full_message.remove_style(None, lv.PART.MAIN | lv.STATE.PRESSED)
            self.show_full_message.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)
        self.container = ContainerFlexCol(
            self.content_area, self.item_message, pos=(0, 8), padding_row=0
        )
        self.container.add_dummy()

        self.item_addr = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__ADDRESS__COLON), address
        )
        self.item_domain = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__DOMAIN__COLON), domain
        )

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.show_full_message:
                PageAbleMessage(
                    _(i18n_keys.TITLE__MESSAGE),
                    self.full_message,
                    None,
                    primary_color=self.primary_color,
                    font=font_GeistMono28,
                    confirm_text=None,
                    cancel_text=None,
                )


class TransactionDetailsTON(FullSizeWindow):
    def __init__(
        self,
        title,
        address_from,
        address_to,
        amount,
        fee_max,
        is_eip1559=False,
        gas_price=None,
        max_priority_fee_per_gas=None,
        max_fee_per_gas=None,
        total_amount=None,
        primary_color=lv_colors.ONEKEY_GREEN,
        contract_addr=None,
        token_id=None,
        evm_chain_id=None,
        raw_data=None,
        is_raw_data=False,
        sub_icon_path=None,
        striped=False,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path="A:/res/icon-send.png",
            sub_icon_path=sub_icon_path,
        )
        self.primary_color = primary_color
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        if striped:
            self.group_amounts = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_amounts,
                _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                "A:/res/group-icon-amount.png",
            )
            self.group_body_amount = DisplayItem(
                self.group_amounts,
                None,
                amount,
            )
            self.group_amounts.add_dummy()

        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_to_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__TO__COLON),
            address_to,
        )
        self.item_group_body_from_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FROM__COLON),
            address_from,
        )
        self.group_directions.add_dummy()

        if contract_addr or evm_chain_id:
            self.group_more = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
            )
            if evm_chain_id:
                self.item_group_body_chain_id = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__CHAIN_ID__COLON),
                    str(evm_chain_id),
                )
            if contract_addr:
                self.item_group_body_contract_addr = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__CONTRACT_ADDRESS__COLON),
                    contract_addr,
                )
                self.item_group_body_token_id = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__TOKEN_ID__COLON),
                    token_id,
                )
            self.group_more.add_dummy()

        if raw_data:
            from trezor import strings

            self.data_str = strings.format_customer_data(raw_data)
            if not self.data_str:
                return
            self.long_data = False
            if len(self.data_str) > 225:
                self.long_data = True
                self.data = self.data_str[:222] + "..."
            else:
                self.data = self.data_str
            self.item_data = CardItem(
                self.container,
                _(i18n_keys.LIST_KEY__DATA__COLON)
                if self.data.startswith("b5ee9c72")
                else _(i18n_keys.LIST_KEY__MEMO__COLON),
                self.data,
                "A:/res/group-icon-data.png"
                if self.data.startswith("b5ee9c72")
                else "A:/res/group-icon-more.png",
            )

            if self.long_data:
                self.show_full_data = NormalButton(
                    self.item_data.content, _(i18n_keys.BUTTON__VIEW_DATA)
                )
                self.show_full_data.set_size(lv.SIZE.CONTENT, 77)
                self.show_full_data.add_style(
                    StyleWrapper().text_font(font_GeistSemiBold26).pad_hor(24), 0
                )
                self.show_full_data.align(lv.ALIGN.CENTER, 0, 0)
                self.show_full_data.remove_style(None, lv.PART.MAIN | lv.STATE.PRESSED)
                self.show_full_data.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.show_full_data:
                PageAbleMessage(
                    _(i18n_keys.TITLE__VIEW_DATA),
                    self.data_str,
                    None,
                    primary_color=self.primary_color,
                    font=font_GeistMono28,
                    confirm_text=None,
                    cancel_text=None,
                )


class TransactionDetailsTRON(FullSizeWindow):
    def __init__(
        self,
        title,
        address_from,
        address_to,
        amount,
        fee_max,
        primary_color,
        icon_path,
        total_amount=None,
        striped=False,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path="A:/res/icon-send.png",
            sub_icon_path=icon_path,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))

        if striped:
            self.group_amounts = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_amounts,
                _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                "A:/res/group-icon-amount.png",
            )
            self.group_body_amount = DisplayItem(
                self.group_amounts,
                None,
                amount,
            )
            self.group_amounts.add_dummy()
        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_to_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__TO__COLON),
            address_to,
        )
        self.item_group_body_from_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FROM__COLON),
            address_from,
        )
        self.group_directions.add_dummy()

        self.group_fees = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
        )
        self.item_group_body_fee_max = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__MAXIMUM_FEE__COLON),
            fee_max,
        )
        if total_amount is None:
            total_amount = f"{amount}\n{fee_max}"
        self.item_group_body_total = DisplayItem(
            self.group_fees, _(i18n_keys.LIST_KEY__TOTAL_AMOUNT__COLON), total_amount
        )
        self.group_fees.add_dummy()


class TransactionDetailsNear(FullSizeWindow):
    def __init__(
        self,
        title,
        address_from,
        address_to,
        amount,
        primary_color,
        icon_path,
        striped=False,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path="A:/res/icon-send.png",
            sub_icon_path=icon_path,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))

        if striped:
            self.group_amounts = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_amounts,
                _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                "A:/res/group-icon-amount.png",
            )
            self.group_body_amount = DisplayItem(
                self.group_amounts,
                None,
                amount,
            )
            self.group_amounts.add_dummy()
        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_to_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__TO__COLON),
            address_to,
        )
        self.item_group_body_from_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FROM__COLON),
            address_from,
        )
        self.group_directions.add_dummy()


class SecurityCheck(FullSizeWindow):
    def __init__(self):
        super().__init__(
            title=_(i18n_keys.TITLE__SECURITY_CHECK),
            subtitle=_(i18n_keys.SUBTITLE__SECURITY_CHECK),
            confirm_text=_(i18n_keys.BUTTON__CONFIRM),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            icon_path="A:/res/security-check.png",
            anim_dir=2,
        )


class PassphraseDisplayConfirm(FullSizeWindow):
    def __init__(self, passphrase: str):
        super().__init__(
            title=_(i18n_keys.TITLE__USE_THIS_PASSPHRASE),
            subtitle=_(i18n_keys.SUBTITLE__USE_THIS_PASSPHRASE),
            confirm_text=_(i18n_keys.BUTTON__CONFIRM),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            anim_dir=0,
        )

        self.panel = lv.obj(self.content_area)
        self.panel.remove_style_all()
        self.panel.set_size(456, 272)
        self.panel.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 40)

        self.panel.add_style(
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_BLACK_3)
            .bg_opa()
            .border_width(0)
            .text_font(font_GeistSemiBold38)
            .text_color(lv_colors.LIGHT_GRAY)
            .text_align_left()
            .pad_ver(16)
            .pad_hor(24)
            .radius(40),
            0,
        )
        self.content = lv.label(self.panel)
        self.content.set_size(lv.pct(100), lv.pct(100))
        self.content.set_text(passphrase)
        self.content.set_long_mode(lv.label.LONG.WRAP)
        self.input_count_tips = lv.label(self.content_area)
        self.input_count_tips.align_to(self.panel, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)
        self.input_count_tips.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular20)
            .text_letter_space(-1)
            .text_align_left()
            .text_color(lv_colors.LIGHT_GRAY),
            0,
        )
        self.input_count_tips.set_text(f"{len(passphrase)}/50")

    def show_unload_anim(self):
        self.clean()
        self.destroy(10)


class SolBlindingSign(FullSizeWindow):
    def __init__(self, fee_payer: str, message_hex: str, primary_color, icon_path):
        super().__init__(
            _(i18n_keys.TITLE__VIEW_TRANSACTION),
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path=icon_path,
        )
        self.warning_banner = Banner(
            self.content_area, 2, _(i18n_keys.TITLE__UNKNOWN_TRANSACTION)
        )
        self.warning_banner.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, 40)
        self.container = ContainerFlexCol(self.content_area, self.title)
        self.container.align_to(self.warning_banner, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)

        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_fee_payer = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FEE_PAYER__COLON),
            fee_payer,
        )
        self.group_directions.add_dummy()

        self.group_more = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
        )
        self.item_group_body_format = DisplayItem(
            self.group_more,
            _(i18n_keys.LIST_KEY__FORMAT__COLON),
            _(i18n_keys.LIST_VALUE__UNKNOWN__COLON),
        )
        self.item_group_body_message_hex = DisplayItem(
            self.group_more,
            _(i18n_keys.LIST_KEY__MESSAGE_HASH__COLON),
            message_hex,
        )
        self.group_more.add_dummy()


class SolTransfer(FullSizeWindow):
    def __init__(
        self,
        title,
        from_addr: str,
        fee_payer: str,
        to_addr: str,
        amount: str,
        primary_color,
        icon_path,
        striped: bool = False,
    ):
        super().__init__(
            title=title,
            subtitle=None,
            confirm_text=_(i18n_keys.BUTTON__CONTINUE),
            cancel_text=_(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path="A:/res/icon-send.png",
            sub_icon_path=icon_path,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        if striped:
            self.group_amounts = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_amounts,
                _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                "A:/res/group-icon-amount.png",
            )
            self.group_body_amount = DisplayItem(
                self.group_amounts,
                None,
                amount,
            )
            self.group_amounts.add_dummy()

        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_to_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__TO__COLON),
            to_addr,
        )
        self.item_group_body_from_addr = DisplayItem(
            self.group_directions, _(i18n_keys.LIST_KEY__FROM__COLON), from_addr
        )
        self.group_directions.add_dummy()

        self.group_fees = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
        )
        self.item_group_body_fee_payer = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__FEE_PAYER__COLON),
            fee_payer,
        )
        self.group_fees.add_dummy()


class SolCreateAssociatedTokenAccount(FullSizeWindow):
    def __init__(
        self,
        fee_payer: str,
        funding_account: str,
        associated_token_account: str,
        wallet_address: str,
        token_mint: str,
        primary_color,
    ):
        super().__init__(
            title=_(i18n_keys.TITLE__CREATE_TOKEN_ACCOUNT),
            subtitle=None,
            confirm_text=_(i18n_keys.BUTTON__CONTINUE),
            cancel_text=_(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_body_ata = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__NEW_TOKEN_ACCOUNT),
            associated_token_account,
        )
        self.item_body_owner = DisplayItem(
            self.group_directions, _(i18n_keys.LIST_KEY__OWNER), wallet_address
        )
        self.item_body_mint_addr = DisplayItem(
            self.group_directions, _(i18n_keys.LIST_KEY__MINT_ADDRESS), token_mint
        )
        self.item_body_founder = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FUNDED_BY__COLON),
            funding_account,
        )
        self.group_directions.add_dummy()

        self.group_fees = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
        )
        self.item_group_body_fee_payer = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__FEE_PAYER__COLON),
            fee_payer,
        )
        self.group_fees.add_dummy()


class SolTokenTransfer(FullSizeWindow):
    def __init__(
        self,
        title,
        from_addr: str,
        to: str,
        amount: str,
        source_owner: str,
        fee_payer: str,
        primary_color,
        icon_path,
        token_mint: str | None = None,
        striped: bool = False,
    ):
        super().__init__(
            title,
            subtitle=None,
            confirm_text=_(i18n_keys.BUTTON__CONTINUE),
            cancel_text=_(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path="A:/res/icon-send.png",
            sub_icon_path=icon_path,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        if striped:
            self.group_amounts = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_amounts,
                _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                "A:/res/group-icon-amount.png",
            )
            self.group_body_amount = DisplayItem(
                self.group_amounts,
                None,
                amount,
            )
            self.group_amounts.add_dummy()

        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_to_ata_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__TO_TOKEN_ACCOUNT__COLON),
            to,
        )
        self.item_group_body_from_ata_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FROM_TOKEN_ACCOUNT__COLON),
            from_addr,
        )
        self.group_directions.add_dummy()

        self.group_fees = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
        )
        self.item_group_body_fee_payer = DisplayItem(
            self.group_fees, _(i18n_keys.LIST_KEY__FEE_PAYER__COLON), fee_payer
        )
        self.group_fees.add_dummy()

        self.group_more = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
        )
        self.item_group_body_signer = DisplayItem(
            self.group_more, _(i18n_keys.LIST_KEY__SIGNER__COLON), source_owner
        )
        if token_mint:
            self.item_group_body_mint_addr = DisplayItem(
                self.group_more, _(i18n_keys.LIST_KEY__MINT_ADDRESS), token_mint
            )
        self.group_more.add_dummy()


class BlindingSignCommon(FullSizeWindow):
    def __init__(self, signer: str, primary_color, icon_path):
        super().__init__(
            _(i18n_keys.TITLE__VIEW_TRANSACTION),
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path=icon_path,
        )
        self.warning_banner = Banner(
            self.content_area, 2, _(i18n_keys.TITLE__UNKNOWN_TRANSACTION)
        )
        self.warning_banner.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, 40)
        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=0)
        self.container.align_to(self.warning_banner, lv.ALIGN.OUT_BOTTOM_MID, 0, 24)
        self.container.add_dummy()
        self.item_format = DisplayItem(
            self.container,
            _(i18n_keys.LIST_KEY__FORMAT__COLON),
            _(i18n_keys.LIST_VALUE__UNKNOWN__COLON),
        )
        self.item_signer = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__SIGNER__COLON), signer
        )
        self.container.add_dummy()


class Modal(FullSizeWindow):
    def __init__(
        self,
        title: str | None,
        subtitle: str | None,
        confirm_text: str = "",
        cancel_text: str = "",
        icon_path: str | None = None,
        anim_dir: int = 1,
    ):
        super().__init__(
            title, subtitle, confirm_text, cancel_text, icon_path, anim_dir=anim_dir
        )

    def show_unload_anim(self):
        self.destroy(200)


class AlgoCommon(FullSizeWindow):
    def __init__(self, type: str, primary_color, icon_path):
        super().__init__(
            _(i18n_keys.TITLE__VIEW_TRANSACTION),
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path=icon_path,
        )
        self.container = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=0
        )
        self.container.add_dummy()
        self.item_type = DisplayItem(
            self.container,
            _(i18n_keys.LIST_KEY__TYPE__COLON),
            type,
        )
        self.container.add_dummy()


class AlgoPayment(FullSizeWindow):
    def __init__(
        self,
        title,
        sender,
        receiver,
        close_to,
        rekey_to,
        genesis_id,
        note,
        fee,
        amount,
        primary_color,
        icon_path,
        striped=False,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path="A:/res/icon-send.png",
            sub_icon_path=icon_path,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        if striped:
            self.group_amounts = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_amounts,
                _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                "A:/res/group-icon-amount.png",
            )
            self.item_group_body_amount = DisplayItem(self.group_amounts, None, amount)
            self.group_amounts.add_dummy()

        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_to_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__TO__COLON),
            receiver,
        )
        self.item_group_body_from_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FROM__COLON),
            sender,
        )
        self.group_directions.add_dummy()

        self.group_fees = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
        )
        self.item_group_body_fee = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__MAXIMUM_FEE__COLON),
            fee,
        )
        self.group_fees.add_dummy()

        if note:
            self.item_note = CardItem(
                self.container,
                _(i18n_keys.LIST_KEY__NOTE__COLON),
                note,
                "A:/res/group-icon-data.png",
            )

        if any([close_to, rekey_to, genesis_id]):
            self.group_more = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
            )
            if close_to is not None:
                self.item_close_reminder_to = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__CLOSE_REMAINDER_TO__COLON),
                    close_to,
                )
            if rekey_to is not None:
                self.item_rekey_to = DisplayItem(
                    self.group_more, _(i18n_keys.LIST_KEY__REKEY_TO__COLON), rekey_to
                )
            if genesis_id is not None:
                self.item_genesis_id = DisplayItem(
                    self.group_more, "GENESIS ID:", genesis_id
                )
            self.group_more.add_dummy()


class AlgoAssetFreeze(FullSizeWindow):
    def __init__(
        self,
        sender,
        rekey_to,
        fee,
        index,
        target,
        new_freeze_state,
        genesis_id,
        note,
        primary_color,
    ):
        super().__init__(
            _(i18n_keys.TITLE__SIGN_STR_TRANSACTION).format("ALGO"),
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))

        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_from_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FROM__COLON),
            sender,
        )
        self.group_directions.add_dummy()

        self.group_fees = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
        )
        self.item_group_body_fee = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__MAXIMUM_FEE__COLON),
            fee,
        )
        self.group_fees.add_dummy()

        if note:
            self.item_note = CardItem(
                self.container,
                _(i18n_keys.LIST_KEY__NOTE__COLON),
                note,
                "A:/res/group-icon-data.png",
            )

        self.group_more = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
        )
        self.item_group_body_asset_freeze_state = DisplayItem(
            self.group_more,
            _(i18n_keys.LIST_KEY__FREEZE_ASSET_ID__COLON),
            _(i18n_keys.LIST_VALUE__TRUE)
            if new_freeze_state is True
            else _(i18n_keys.LIST_VALUE__FALSE),
        )
        self.item_group_freeze_account = DisplayItem(
            self.group_more, _(i18n_keys.LIST_KEY__FREEZE_ACCOUNT__COLON), target
        )
        self.item_group_body_asset_id = DisplayItem(
            self.group_more, _(i18n_keys.LIST_KEY__ASSET_ID__COLON), index
        )
        if rekey_to is not None:
            self.item_rekey_to = DisplayItem(
                self.group_more, _(i18n_keys.LIST_KEY__REKEY_TO__COLON), rekey_to
            )
        if genesis_id is not None:
            self.item_genesis_id = DisplayItem(
                self.group_more, "GENESIS ID:", genesis_id
            )
        self.group_more.add_dummy()


class AlgoAssetXfer(FullSizeWindow):
    def __init__(
        self,
        title,
        sender,
        receiver,
        index,
        fee,
        amount,
        close_assets_to,
        revocation_target,
        rekey_to,
        genesis_id,
        note,
        primary_color,
        icon_path,
        striped=False,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path="A:/res/icon-send.png",
            sub_icon_path=icon_path,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        if striped:
            self.group_amounts = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_amounts,
                _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                "A:/res/group-icon-amount.png",
            )
            self.item_group_body_amount = DisplayItem(self.group_amounts, None, amount)
            self.group_amounts.add_dummy()

        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_to_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__TO__COLON),
            receiver,
        )
        self.item_group_body_from_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FROM__COLON),
            sender,
        )
        self.group_directions.add_dummy()

        self.group_fees = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
        )
        self.item_group_body_fee = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__MAXIMUM_FEE__COLON),
            fee,
        )
        self.group_fees.add_dummy()

        if note:
            self.item_note = CardItem(
                self.container,
                _(i18n_keys.LIST_KEY__NOTE__COLON),
                note,
                "A:/res/group-icon-data.png",
            )

        self.group_more = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
        )
        if revocation_target is not None:
            self.item_group_body_revocation_addr = DisplayItem(
                self.group_more,
                _(i18n_keys.LIST_KEY__REVOCATION_ADDRESS__COLON),
                revocation_target,
            )
        self.item_group_body_asset_id = DisplayItem(
            self.group_more, _(i18n_keys.LIST_KEY__ASSET_ID__COLON), index
        )
        if rekey_to is not None:
            self.item_rekey_to = DisplayItem(
                self.group_more, _(i18n_keys.LIST_KEY__REKEY_TO__COLON), rekey_to
            )
        if close_assets_to is not None:
            self.item_close_assets_to = DisplayItem(
                self.group_more,
                _(i18n_keys.LIST_KEY__CLOSE_ASSET_TO__COLON),
                close_assets_to,
            )
        if genesis_id is not None:
            self.item_genesis_id = DisplayItem(
                self.group_more, "GENESIS ID:", genesis_id
            )
        self.group_more.add_dummy()


class AlgoAssetCfg(FullSizeWindow):
    def __init__(
        self,
        fee,
        sender,
        index,
        total,
        default_frozen,
        unit_name,
        asset_name,
        decimals,
        manager,
        reserve,
        freeze,
        clawback,
        url,
        metadata_hash,
        rekey_to,
        genesis_id,
        note,
        primary_color,
    ):
        super().__init__(
            _(i18n_keys.TITLE__SIGN_STR_TRANSACTION).format("ALGO"),
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        if url is not None:
            self.banner = Banner(
                self.content_area, 0, _(i18n_keys.LIST_KEY__INTERACT_WITH).format(url)
            )
            self.banner.align(self.lv.ALIGN.TOP_MID, 0, 40)
            self.container.align_to(self.banner, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)

        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_from_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FROM__COLON),
            sender,
        )
        self.group_directions.add_dummy()

        self.group_fees = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
        )
        self.item_group_body_fee = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__MAXIMUM_FEE__COLON),
            fee,
        )
        self.group_fees.add_dummy()

        if note:
            self.item_note = CardItem(
                self.container,
                _(i18n_keys.LIST_KEY__NOTE__COLON),
                note,
                "A:/res/group-icon-data.png",
            )

        if any(
            [
                asset_name,
                index,
                manager,
                reserve,
                freeze,
                clawback,
                default_frozen,
                freeze,
                total,
                decimals,
                unit_name,
                metadata_hash,
                rekey_to,
                genesis_id,
            ]
        ):
            self.group_more = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
            )
            if asset_name is not None:
                self.item_group_body_asset_name = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__ASSET_NAME__COLON),
                    asset_name,
                )
            if unit_name is not None:
                self.item_group_body_unit_name = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__UNIT_NAME__COLON),
                    unit_name,
                )
            if index is not None and index != "0":
                self.item_group_body_asset_id = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__ASSET_ID__COLON),
                    index,
                )
            if clawback is not None:
                self.item_group_body_clawback_addr = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__CLAW_BACK_ADDRESS__COLON),
                    clawback,
                )
            if manager is not None:
                self.item_group_body_manager_addr = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__MANAGER_ADDRESS__COLON),
                    manager,
                )
            if reserve is not None:
                self.item_group_body_reserve_addr = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__RESERVE_ADDRESS__COLON),
                    reserve,
                )
            if default_frozen is not None:
                self.item_group_body_default_frozen = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__FREEZE_ADDRESS__QUESTION),
                    _(i18n_keys.LIST_VALUE__TRUE)
                    if default_frozen is True
                    else _(i18n_keys.LIST_VALUE__FALSE),
                )
            if freeze is not None:
                self.item_group_body_freeze_addr = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__FREEZE_ADDRESS__COLON),
                    freeze,
                )
            if decimals is not None and decimals != "0":
                self.item_group_body_decimals = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__DECIMALS__COLON),
                    decimals,
                )
            if total is not None:
                self.item_group_body_total = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__TOTAL__COLON),
                    total,
                )
            if metadata_hash is not None:
                self.item_group_body_metadata_hash = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__METADATA_HASH__COLON),
                    metadata_hash,
                )
            if rekey_to is not None:
                self.item_rekey_to = DisplayItem(
                    self.group_more, _(i18n_keys.LIST_KEY__REKEY_TO__COLON), rekey_to
                )
            if genesis_id is not None:
                self.item_genesis_id = DisplayItem(
                    self.group_more, "GENESIS ID:", genesis_id
                )
            self.group_more.add_dummy()


class AlgoKeyregOnline(FullSizeWindow):
    def __init__(
        self,
        sender,
        fee,
        votekey,
        selkey,
        sprfkey,
        rekey_to,
        genesis_id,
        note,
        primary_color,
    ):
        super().__init__(
            _(i18n_keys.TITLE__SIGN_STR_TRANSACTION).format("ALGO"),
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))

        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_from_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FROM__COLON),
            sender,
        )
        self.group_directions.add_dummy()

        self.group_fees = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
        )
        self.item_group_body_fee = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__MAXIMUM_FEE__COLON),
            fee,
        )
        self.group_fees.add_dummy()

        if note:
            self.item_note = CardItem(
                self.container,
                _(i18n_keys.LIST_KEY__NOTE__COLON),
                note,
                "A:/res/group-icon-data.png",
            )

        self.group_more = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
        )
        self.item_group_body_vrf_public_key = DisplayItem(
            self.group_more,
            _(i18n_keys.LIST_KEY__VRF_PUBLIC_KEY__COLON),
            selkey,
        )
        self.item_group_body_vote_public_key = DisplayItem(
            self.group_more,
            _(i18n_keys.LIST_KEY__VOTE_PUBLIC_KEY__COLON),
            votekey,
        )
        if sprfkey is not None:
            self.item_group_body_sprf_public_key = DisplayItem(
                self.group_more,
                _(i18n_keys.LIST_KEY__STATE_PROOF_PUBLIC_KEY__COLON),
                sprfkey,
            )
        if rekey_to is not None:
            self.item_rekey_to = DisplayItem(
                self.group_more, _(i18n_keys.LIST_KEY__REKEY_TO__COLON), rekey_to
            )
        if genesis_id is not None:
            self.item_genesis_id = DisplayItem(
                self.group_more, "GENESIS ID:", genesis_id
            )
        self.group_more.add_dummy()


class AlgoKeyregNonp(FullSizeWindow):
    def __init__(self, sender, fee, nonpart, rekey_to, genesis_id, note, primary_color):
        super().__init__(
            _(i18n_keys.TITLE__SIGN_STR_TRANSACTION).format("ALGO"),
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_from_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FROM__COLON),
            sender,
        )
        self.group_directions.add_dummy()

        self.group_fees = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
        )
        self.item_group_body_fee = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__MAXIMUM_FEE__COLON),
            fee,
        )
        self.group_fees.add_dummy()

        if note:
            self.item_note = CardItem(
                self.container,
                _(i18n_keys.LIST_KEY__NOTE__COLON),
                note,
                "A:/res/group-icon-data.png",
            )

        self.group_more = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
        )
        self.item_group_body_nonpart = DisplayItem(
            self.group_more,
            _(i18n_keys.LIST_KEY__NONPARTICIPATION__COLON),
            _(i18n_keys.LIST_VALUE__FALSE)
            if nonpart is True
            else _(i18n_keys.LIST_VALUE__TRUE),
        )
        if rekey_to is not None:
            self.item_rekey_to = DisplayItem(
                self.group_more, _(i18n_keys.LIST_KEY__REKEY_TO__COLON), rekey_to
            )
        self.group_more.add_dummy()


class AlgoApplication(FullSizeWindow):
    def __init__(self, signer: str, primary_color, icon_path):
        super().__init__(
            _(i18n_keys.TITLE__VIEW_TRANSACTION),
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path=icon_path,
        )
        self.container = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=0
        )
        self.container.add_dummy()
        self.item1 = DisplayItem(
            self.container,
            _(i18n_keys.LIST_KEY__FORMAT__COLON),
            "Application",
        )
        self.item2 = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__SIGNER__COLON), signer
        )
        self.container.add_dummy()


class RipplePayment(FullSizeWindow):
    def __init__(
        self,
        title,
        address_from,
        address_to,
        amount,
        fee_max,
        total_amount=None,
        tag=None,
        primary_color=None,
        icon_path=None,
        striped=False,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path="A:/res/icon-send.png",
            sub_icon_path=icon_path,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        if striped:
            self.group_amounts = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_amounts,
                _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                "A:/res/group-icon-amount.png",
            )
            self.item_group_body_amount = DisplayItem(
                self.group_amounts,
                None,
                amount,
            )
            self.group_amounts.add_dummy()

        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_to_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__TO__COLON),
            address_to,
        )
        self.item_group_body_from_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FROM__COLON),
            address_from,
        )
        self.group_directions.add_dummy()

        self.group_fees = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
        )
        self.item_group_body_fee = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__MAXIMUM_FEE__COLON),
            fee_max,
        )
        if total_amount is None:
            total_amount = f"{amount}\n{fee_max}"
        self.item_group_body_total_amount = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__TOTAL_AMOUNT__COLON),
            total_amount,
        )
        self.group_fees.add_dummy()

        if tag:
            self.group_more = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
            )
            self.item_group_body_tag = DisplayItem(
                self.group_more,
                _(i18n_keys.LIST_KEY__DESTINATION_TAG__COLON),
                tag,
            )
            self.group_more.add_dummy()


class NftRemoveConfirm(FullSizeWindow):
    def __init__(self, icon_path):
        super().__init__(
            title=_(i18n_keys.TITLE__REMOVE_NFT),
            subtitle=_(i18n_keys.SUBTITLE__REMOVE_NFT),
            confirm_text=_(i18n_keys.BUTTON__REMOVE),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            icon_path=icon_path,
            anim_dir=0,
        )
        self.btn_yes.enable(bg_color=lv_colors.ONEKEY_RED_1, text_color=lv_colors.BLACK)

    def destroy(self, _delay_ms=400):
        self.del_delayed(200)


class FilecoinPayment(FullSizeWindow):
    def __init__(
        self,
        title,
        address_from,
        address_to,
        amount,
        gaslimit,
        gasfeecap=None,
        gaspremium=None,
        total_amount=None,
        primary_color=None,
        icon_path=None,
        striped=False,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path="A:/res/icon-send.png",
            sub_icon_path=icon_path,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        if striped:
            self.group_amounts = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_amounts,
                _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                "A:/res/group-icon-amount.png",
            )
            self.item_group_body_amount = DisplayItem(
                self.group_amounts,
                None,
                amount,
            )
            self.group_amounts.add_dummy()

        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_to_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__TO__COLON),
            address_to,
        )
        self.item_group_body_from_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FROM__COLON),
            address_from,
        )
        self.group_directions.add_dummy()

        self.group_fees = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
        )
        self.item_group_body_gas_limit = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__GAS_LIMIT__COLON),
            gaslimit,
        )
        self.item_group_body_gas_cap = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__GAS_FEE_CAP__COLON),
            gasfeecap,
        )
        self.item_group_body_gas_premium = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__GAS_PREMIUM__COLON),
            gaspremium,
        )
        self.item_group_body_total_amount = DisplayItem(
            self.group_fees,
            _(i18n_keys.LIST_KEY__TOTAL_AMOUNT__COLON),
            total_amount,
        )
        self.group_fees.add_dummy()


class CosmosTransactionOverview(FullSizeWindow):
    def __init__(
        self,
        title,
        types,
        value,
        amount,
        address,
        primary_color,
        icon_path,
        striped=False,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            anim_dir=2,
            primary_color=primary_color,
            icon_path="A:/res/icon-send.png" if types is None else icon_path,
            sub_icon_path=icon_path if types is None else None,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))

        if types is None:
            if striped:
                self.group_amounts = ContainerFlexCol(
                    self.container, None, padding_row=0, no_align=True
                )
                self.item_group_header = CardHeader(
                    self.group_amounts,
                    _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                    "A:/res/group-icon-amount.png",
                )
                self.item_group_body_amount = DisplayItem(
                    self.group_amounts,
                    None,
                    amount,
                )
                self.group_amounts.add_dummy()

            self.group_directions = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_directions,
                _(i18n_keys.FORM__DIRECTIONS),
                "A:/res/group-icon-directions.png",
            )
            self.item_group_body_to_addr = DisplayItem(
                self.group_directions,
                _(i18n_keys.LIST_KEY__TO__COLON),
                address,
            )
            self.group_directions.add_dummy()
        else:
            self.container.add_style(StyleWrapper().pad_row(0), 0)
            self.container.add_dummy()
            self.item_type = DisplayItem(
                self.container,
                _(i18n_keys.LIST_KEY__TYPE__COLON),
                value,
            )
            self.container.add_dummy()

        self.view_btn = NormalButton(
            self.content_area,
            f"{LV_SYMBOLS.LV_SYMBOL_ANGLE_DOUBLE_DOWN}  {_(i18n_keys.BUTTON__DETAILS)}",
        )
        self.view_btn.set_size(456, 82)
        self.view_btn.add_style(StyleWrapper().text_font(font_GeistSemiBold26), 0)
        self.view_btn.enable()
        self.view_btn.align_to(self.container, lv.ALIGN.OUT_BOTTOM_MID, 0, 16)
        self.view_btn.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.view_btn:
                self.destroy(400)
                self.channel.publish(2)


class CosmosSend(FullSizeWindow):
    def __init__(
        self,
        title,
        chain_id,
        chain_name,
        address_from,
        address_to,
        amount,
        fee,
        memo,
        primary_color=None,
        icon_path=None,
        striped=False,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
            icon_path="A:/res/icon-send.png",
            sub_icon_path=icon_path,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        if striped:
            self.group_amounts = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_amounts,
                _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                "A:/res/group-icon-amount.png",
            )
            self.item_group_body_amount = DisplayItem(
                self.group_amounts,
                None,
                amount,
            )
            self.group_amounts.add_dummy()

        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_to_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__TO__COLON),
            address_to,
        )
        self.item_group_body_from_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FROM__COLON),
            address_from,
        )
        self.group_directions.add_dummy()

        if fee:
            self.group_fees = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
            )
            self.item_group_body_fee = DisplayItem(
                self.group_fees,
                _(i18n_keys.LIST_KEY__FEE__COLON),
                fee,
            )
            self.group_fees.add_dummy()

        self.group_more = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
        )
        if chain_name:
            self.item_group_body_chain_name = DisplayItem(
                self.group_more,
                _(i18n_keys.LIST_KEY__CHAIN_NAME__COLON),
                chain_name,
            )
        else:
            self.item_group_body_chain_id = DisplayItem(
                self.group_more, _(i18n_keys.LIST_KEY__CHAIN_ID__COLON), chain_id
            )
        if memo:
            self.item_group_body_memo = DisplayItem(
                self.group_more,
                _(i18n_keys.LIST_KEY__MEMO__COLON),
                memo,
            )
        self.group_more.add_dummy()


class CosmosDelegate(FullSizeWindow):
    def __init__(
        self,
        title,
        chain_id,
        chain_name,
        delegator,
        validator,
        amount,
        fee,
        memo,
        primary_color=None,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        self.group_amounts = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_amounts,
            _(i18n_keys.LIST_KEY__AMOUNT__COLON),
            "A:/res/group-icon-amount.png",
        )
        self.item_group_body_amount = DisplayItem(
            self.group_amounts,
            None,
            amount,
        )
        self.group_amounts.add_dummy()

        if fee:
            self.group_fees = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
            )
            self.item_group_body_fee = DisplayItem(
                self.group_fees,
                _(i18n_keys.LIST_KEY__FEE__COLON),
                fee,
            )
            self.group_fees.add_dummy()

        self.group_more = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
        )
        self.item_group_body_delegator = DisplayItem(
            self.group_more,
            _(i18n_keys.LIST_KEY__DELEGATOR__COLON),
            delegator,
        )
        self.item_group_body_validator = DisplayItem(
            self.group_more,
            _(i18n_keys.LIST_KEY__VALIDATOR__COLON),
            validator,
        )

        if chain_name:
            self.item_group_body_chain_name = DisplayItem(
                self.group_more,
                _(i18n_keys.LIST_KEY__CHAIN_NAME__COLON),
                chain_name,
            )
        else:
            self.item_group_body_chain_id = DisplayItem(
                self.group_more, _(i18n_keys.LIST_KEY__CHAIN_ID__COLON), chain_id
            )
        if memo:
            self.item_group_body_memo = DisplayItem(
                self.group_more,
                _(i18n_keys.LIST_KEY__MEMO__COLON),
                memo,
            )
        self.group_more.add_dummy()


class CosmosSignCommon(FullSizeWindow):
    def __init__(
        self,
        chain_id: str,
        chain_name: str,
        signer: str,
        fee: str,
        title: str,
        value: str,
        memo,
        primary_color,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__CANCEL),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        if signer:
            self.group_directions = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_directions,
                _(i18n_keys.FORM__DIRECTIONS),
                "A:/res/group-icon-directions.png",
            )
            self.item_group_body_signer = DisplayItem(
                self.group_directions,
                _(i18n_keys.LIST_KEY__SIGNER__COLON),
                signer,
            )
            self.group_directions.add_dummy()

        if fee:
            self.group_fees = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
            )
            self.item_group_body_fee = DisplayItem(
                self.group_fees,
                _(i18n_keys.LIST_KEY__FEE__COLON),
                fee,
            )
            self.group_fees.add_dummy()

        self.group_more = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
        )
        if chain_name:
            self.item_group_body_chain_name = DisplayItem(
                self.group_more,
                _(i18n_keys.LIST_KEY__CHAIN_NAME__COLON),
                chain_name,
            )
        else:
            self.item_group_body_chain_id = DisplayItem(
                self.group_more, _(i18n_keys.LIST_KEY__CHAIN_ID__COLON), chain_id
            )
        self.item_group_body_type = DisplayItem(
            self.group_more,
            _(i18n_keys.LIST_KEY__TYPE__COLON),
            value,
        )
        if memo:
            self.item_group_body_memo = DisplayItem(
                self.group_more,
                _(i18n_keys.LIST_KEY__MEMO__COLON),
                memo,
            )
        self.group_more.add_dummy()


class CosmosSignContent(FullSizeWindow):
    def __init__(
        self,
        msgs_item: dict,
        primary_color,
    ):
        super().__init__(
            _(i18n_keys.TITLE__CONTENT),
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__CANCEL),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(
            self.content_area, self.title, pos=(0, 40), padding_row=0
        )
        self.container.add_dummy()
        for key, value in msgs_item.items():
            if len(value) <= 80:
                self.item = DisplayItem(self.container, key, value)
        self.container.add_dummy()


class CosmosLongValue(FullSizeWindow):
    def __init__(
        self,
        title,
        content: str,
        primary_color,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__CANCEL),
            primary_color=primary_color or lv_colors.ONEKEY_GREEN,
        )
        self.primary_color = primary_color
        PageAbleMessage(
            title,
            content,
            self.channel,
            primary_color=self.primary_color,
        )
        self.destroy()


class CosmosSignCombined(FullSizeWindow):
    def __init__(self, chain_id: str, signer: str, fee: str, data: str, primary_color):
        super().__init__(
            _(i18n_keys.TITLE__VIEW_TRANSACTION),
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__CANCEL),
            primary_color=primary_color,
        )
        self.primary_color = primary_color
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))
        if signer:
            self.group_directions = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_directions,
                _(i18n_keys.FORM__DIRECTIONS),
                "A:/res/group-icon-directions.png",
            )
            self.item_group_body_signer = DisplayItem(
                self.group_directions,
                _(i18n_keys.LIST_KEY__SIGNER__COLON),
                signer,
            )
            self.group_directions.add_dummy()

        if fee:
            self.group_fees = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
            )
            self.item_group_body_fee = DisplayItem(
                self.group_fees,
                _(i18n_keys.LIST_KEY__FEE__COLON),
                fee,
            )
            self.group_fees.add_dummy()

        self.group_more = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
        )
        self.item_group_body_chain_id = DisplayItem(
            self.group_more, _(i18n_keys.LIST_KEY__CHAIN_ID__COLON), chain_id
        )
        self.group_more.add_dummy()

        self.long_data = False
        self.data_str = data
        if len(data) > 225:
            self.long_data = True
            self.data = data[:222] + "..."
        else:
            self.data = data
        self.item_data = CardItem(
            self.container,
            _(i18n_keys.LIST_KEY__DATA__COLON),
            self.data,
            "A:/res/group-icon-data.png",
        )
        if self.long_data:
            self.show_full_data = NormalButton(
                self.item_data.content, _(i18n_keys.BUTTON__VIEW_DATA)
            )
            self.show_full_data.set_size(lv.SIZE.CONTENT, 82)
            self.show_full_data.add_style(
                StyleWrapper().text_font(font_GeistSemiBold26).pad_hor(24), 0
            )
            self.show_full_data.align(lv.ALIGN.CENTER, 0, 0)
            self.show_full_data.remove_style(None, lv.PART.MAIN | lv.STATE.PRESSED)
            self.show_full_data.add_event_cb(self.on_click, lv.EVENT.CLICKED, None)

    def on_click(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.show_full_data:
                PageAbleMessage(
                    _(i18n_keys.TITLE__VIEW_DATA),
                    self.data_str,
                    None,
                    primary_color=self.primary_color,
                    font=font_GeistMono28,
                    confirm_text=None,
                    cancel_text=None,
                )


class ConfirmTypedHash(FullSizeWindow):
    def __init__(self, title, icon, domain_hash, message_hash, primary_color):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            icon,
            primary_color=primary_color,
            anim_dir=0,
        )
        self.banner = Banner(
            self.content_area, 2, _(i18n_keys.MSG__SIGNING_MSG_MAY_HAVE_RISK)
        )
        self.banner.align_to(self.title, lv.ALIGN.OUT_BOTTOM_MID, 0, 40)
        self.container = ContainerFlexCol(self.content_area, self.title, padding_row=0)
        self.container.align_to(self.banner, lv.ALIGN.OUT_BOTTOM_MID, 0, 24)
        self.container.add_dummy()
        self.item_separator_hash = DisplayItem(
            self.container,
            _(i18n_keys.LIST_KEY__DOMAIN_SEPARATOR_HASH__COLON),
            domain_hash,
        )
        self.item_message_hash = DisplayItem(
            self.container,
            _(i18n_keys.LIST_KEY__MESSAGE_HASH__COLON),
            message_hash,
        )
        self.container.add_dummy()


class PolkadotBalances(FullSizeWindow):
    def __init__(
        self,
        title,
        chain_name,
        module,
        method,
        sender,
        dest,
        source,
        balance,
        tip,
        keep_alive,
        primary_color,
        icon_path,
        striped=False,
    ):
        super().__init__(
            title,
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__CANCEL),
            primary_color=primary_color,
            icon_path="A:/res/icon-send.png",
            sub_icon_path=icon_path,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 48))
        if striped:
            self.group_amounts = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_amounts,
                _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                "A:/res/group-icon-amount.png",
            )
            self.item_group_body_amount = DisplayItem(
                self.group_amounts,
                None,
                balance,
            )
            self.group_amounts.add_dummy()

        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_to_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__TO__COLON),
            dest,
        )

        self.item_group_body_signer = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__SIGNER__COLON),
            sender,
        )
        self.group_directions.add_dummy()

        if tip:
            self.group_fees = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group_fees, _(i18n_keys.FORM__FEES), "A:/res/group-icon-fees.png"
            )
            self.item_group_body_fee = DisplayItem(
                self.group_fees,
                _(i18n_keys.LIST_KEY__TIP__COLON),
                tip,
            )
            self.group_fees.add_dummy()

        self.group_more = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
        )
        self.item_group_body_chain_name = DisplayItem(
            self.group_more,
            _(i18n_keys.LIST_KEY__CHAIN_NAME__COLON),
            chain_name,
        )
        if source:
            self.item_group_body_source = DisplayItem(
                self.group_more,
                _(i18n_keys.LIST_KEY__SOURCE_COLON),
                source,
            )
        # if module:
        #     self.item_group_body_module = DisplayItem(
        #         self.group_more,
        #         _(i18n_keys.LIST_KEY__MODULE_COLON),
        #         module,
        #     )
        # if method:
        #     self.item_group_body_method = DisplayItem(
        #         self.group_more,
        #         _(i18n_keys.LIST_KEY__METHOD_COLON),
        #         method,
        #     )
        if keep_alive is not None:
            self.item_group_body_keep_alive = DisplayItem(
                self.group_more,
                _(i18n_keys.LIST_KEY__KEEP_ALIVE_COLON),
                keep_alive,
            )
        self.group_more.add_dummy()


class TronAssetFreeze(FullSizeWindow):
    def __init__(
        self,
        is_freeze,
        sender,
        resource,
        balance,
        duration,
        receiver,
        lock,
        primary_color,
    ):
        super().__init__(
            _(i18n_keys.TITLE__SIGN_STR_TRANSACTION).format("Tron"),
            None,
            _(i18n_keys.BUTTON__CONTINUE),
            _(i18n_keys.BUTTON__REJECT),
            primary_color=primary_color,
        )
        self.container = ContainerFlexCol(self.content_area, self.title, pos=(0, 40))

        self.group_directions = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_directions,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
        )
        self.item_group_body_from_addr = DisplayItem(
            self.group_directions,
            _(i18n_keys.LIST_KEY__FROM__COLON),
            sender,
        )
        self.group_directions.add_dummy()

        self.group_more = ContainerFlexCol(
            self.container, None, padding_row=0, no_align=True
        )
        self.item_group_header = CardHeader(
            self.group_more, _(i18n_keys.FORM__MORE), "A:/res/group-icon-more.png"
        )
        if resource is not None:
            self.item_body_resource = DisplayItem(
                self.group_more, _(i18n_keys.LIST_KEY__RESOURCE_COLON), resource
            )
        if balance:
            if is_freeze:
                self.item_body_freeze_balance = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__FROZEN_BALANCE_COLON),
                    balance,
                )
            else:
                self.item_body_balance = DisplayItem(
                    self.group_more,
                    _(i18n_keys.LIST_KEY__AMOUNT__COLON),
                    balance,
                )
        if duration:
            self.item_body_duration = DisplayItem(
                self.group_more,
                _(i18n_keys.LIST_KEY__FROZEN_DURATION_COLON),
                duration,
            )
        if receiver is not None:
            self.item_body_receiver = DisplayItem(
                self.group_more, _(i18n_keys.LIST_KEY__RECEIVER_ADDRESS_COLON), receiver
            )
        if lock is not None:
            self.item_body_lock = DisplayItem(
                self.group_more, _(i18n_keys.LIST_KEY__LOCK_COLON), lock
            )
        self.group_more.add_dummy()


class UrResponse(FullSizeWindow):
    def __init__(
        self,
        title,
        subtitle,
        qr_code,
        encoder=None,
    ):
        super().__init__(
            title,
            subtitle,
            confirm_text=_(i18n_keys.BUTTON__DONE),
            anim_dir=0,
        )
        self.btn_yes.enable(lv_colors.ONEKEY_GRAY_3, text_color=lv_colors.WHITE)
        import gc

        gc.collect()
        gc.threshold(int(18248 * 1.5))  # type: ignore["threshold" is not a known member of module]
        self.qr_code = qr_code
        self.encoder = encoder
        self.qr = QRCode(
            self.content_area,
            self.qr_code if self.qr_code else encoder.next_part(),  # type: ignore["next_part" is not a known member of "None"]
        )
        self.qr.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 30)
        self.content_area.clear_flag(lv.obj.FLAG.SCROLL_ELASTIC)
        self.content_area.clear_flag(lv.obj.FLAG.SCROLL_MOMENTUM)
        self.content_area.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
        if encoder is not None:
            from trezor import workflow

            workflow.spawn(self.update_qr())

    def destroy(self, delay_ms=400):
        return self.delete()

    async def update_qr(self):
        from trezor import loop

        while True:
            stop_single = self.request()
            racer = loop.race(stop_single, loop.sleep(100))
            await racer
            if stop_single in racer.finished:
                self.destroy()
                return
            # if self.scrolling:
            #     await loop.sleep(5000)
            #     continue
            qr_data = self.encoder.next_part()  # type: ignore["next_part" is not a known member of "None"]
            self.qr.update(qr_data, len(qr_data))


class ErrorFeedback(FullSizeWindow):
    def __init__(self, title, subtitle, btn_text: str = ""):
        super().__init__(
            title,
            subtitle,
            cancel_text=_(i18n_keys.BUTTON__BACK) if not btn_text else btn_text,
            icon_path="A:/res/danger.png",
        )

    def destroy(self):
        return super().destroy(0)


##################
# misc functions #
##################
class AirgapMode(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__AIR_GAP_MODE),
            _(i18n_keys.CONTENT__WHAT_DOES_AIR_GAP_MEANS),
            _(i18n_keys.BUTTON__SKIP),
            _(i18n_keys.BUTTON__GO_SETTING),
        )
        self.btn_yes.enable()
        self.btn_no.enable(bg_color=lv_colors.ONEKEY_GREEN, text_color=lv_colors.BLACK)


class AirGapToggleTips(FullSizeWindow):
    def __init__(self, enable, callback_obj=None):
        super().__init__(
            title=_(i18n_keys.TITLE__ENABLE_AIR_GAP)
            if enable
            else _(i18n_keys.TITLE__DISABLE_AIR_GAP),
            subtitle=_(i18n_keys.CONTENT__ARE_YOU_SURE_TO_ENABLE_AIRGAP_MODE)
            if enable
            else _(i18n_keys.CONTENT__ARE_YOU_SURE_TO_DISABLE_AIRGAP_MODE),
            confirm_text=_(i18n_keys.BUTTON__ENABLE)
            if enable
            else _(i18n_keys.BUTTON__DISABLE),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            anim_dir=0,
        )
        self.last_click_time = 0
        self.click_interval = 1000
        self.callback_obj = callback_obj

    def eventhandler(self, event_obj):
        current_time = utime.ticks_ms()
        if utime.ticks_diff(current_time, self.last_click_time) < self.click_interval:
            return
        self.last_click_time = current_time
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            elif target == self.btn_no:
                if self.callback_obj:
                    lv.event_send(self.callback_obj, lv.EVENT.CANCEL, None)
                else:
                    self.channel.publish(0)
            elif target == self.btn_yes:
                if self.callback_obj:
                    lv.event_send(self.callback_obj, lv.EVENT.READY, None)
                else:
                    self.channel.publish(1)
            else:
                return
            self.show_dismiss_anim()

    def destroy(self, delay_ms=100):
        return self.del_delayed(100)


class ConnectWalletTutorial(FullSizeWindow):
    def __init__(
        self,
        title: str,
        sub_title,
        steps: list[tuple[str, str]],
        website_url,
        logo_path,
    ):
        super().__init__(
            title,
            sub_title,
            confirm_text=_(i18n_keys.BUTTON__DONE),
            cancel_text=_(i18n_keys.ACTION__LEARN_MORE),
            anim_dir=0,
        )
        self.website_url = website_url
        self.logo_path = logo_path
        self.container = ContainerFlexCol(self.content_area, self.subtitle, pos=(0, 40))
        for i, step in enumerate(steps):
            self.group = ContainerFlexCol(
                self.container, None, padding_row=0, no_align=True
            )
            self.item_group_header = CardHeader(
                self.group,
                step[0],
                f"A:/res/group-icon-num-{i+1}.png",
            )
            self.item_group_body = DisplayItem(
                self.group,
                None,
                step[1],
            )
            self.item_group_body.add_style(
                StyleWrapper().text_color(lv_colors.ONEKEY_GRAY_4), 0
            )
            self.group.add_dummy()

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.btn_yes:
                self.destroy(10)
            elif target == self.btn_no:
                ConnectWalletTutorial.ShowOnlineWebsiteQR(
                    self.website_url, self.logo_path
                )

    class ShowOnlineWebsiteQR(FullSizeWindow):
        def __init__(self, qr_content: str, logo_path):
            super().__init__(
                None,
                None,
                anim_dir=0,
                cancel_text=_(i18n_keys.BUTTON__CLOSE),
            )
            import gc

            gc.collect()
            gc.threshold(int(18248 * 1.5))  # type: ignore["threshold" is not a known member of module]
            self.qr = QRCode(self.content_area, qr_content, logo_path)
            self.qr.align_to(self.content_area, lv.ALIGN.TOP_MID, 0, 16)

            self.desc = lv.label(self.content_area)
            self.desc.set_size(456, lv.SIZE.CONTENT)
            self.desc.set_long_mode(lv.label.LONG.WRAP)
            self.desc.add_style(
                StyleWrapper()
                .text_font(font_GeistRegular30)
                .text_color(lv_colors.LIGHT_GRAY)
                .pad_hor(12)
                .pad_ver(16),
                0,
            )
            self.desc.set_text(
                _(i18n_keys.CONTENT__SCAN_THE_QR_CODE_TO_VIEW_THE_DETAILED_TUTORIAL)
            )
            self.desc.align_to(self.qr, lv.ALIGN.OUT_BOTTOM_MID, 0, 8)

        def eventhandler(self, event_obj):
            code = event_obj.code
            target = event_obj.get_target()
            if code == lv.EVENT.CLICKED:
                if utils.lcd_resume():
                    return
                if target == self.btn_no:
                    self.destroy(10)
