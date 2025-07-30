from trezor.lvglui.i18n import gettext as _, keys as i18n_keys

from ...lv_symbols import LV_SYMBOLS
from .. import font_GeistMono28, font_GeistSemiBold26, lv, lv_colors
from ..widgets.style import StyleWrapper
from .button import NormalButton
from .container import ContainerFlexCol
from .listitem import CardHeader, CardItem, DisplayItem
from .pageable import PageAbleMessage


class CardGroup:
    def __init__(
        self,
        parent,
        align_base,
        card_title: str,
        card_icon: str,
        items: tuple[tuple[str, str], ...],
        relative_pos=(0, 40),
        no_align=True,
    ):
        self.pannel = ContainerFlexCol(
            parent, align_base, pos=relative_pos, padding_row=0, no_align=no_align
        )
        self.item_group_header = CardHeader(
            self.pannel,
            card_title,
            card_icon,
        )
        for key, value in items:
            if key and value:
                DisplayItem(self.pannel, key, value)
        self.pannel.add_dummy()


class AmountGroup(CardGroup):
    def __init__(self, parent, items: tuple[tuple[str, str], ...]):
        super().__init__(
            parent,
            None,
            _(i18n_keys.LIST_KEY__AMOUNT__COLON),
            "A:/res/group-icon-amount.png",
            items,
        )


class DirectionGroup(CardGroup):
    def __init__(self, parent, items: tuple[tuple[str, str], ...]):
        super().__init__(
            parent,
            None,
            _(i18n_keys.FORM__DIRECTIONS),
            "A:/res/group-icon-directions.png",
            items,
        )


class FeeGroup(CardGroup):
    def __init__(self, parent, items: tuple[tuple[str, str], ...]):
        super().__init__(
            parent,
            None,
            _(i18n_keys.FORM__FEES),
            "A:/res/group-icon-fees.png",
            items,
        )


class MoreGroup(CardGroup):
    def __init__(self, parent, items: tuple[tuple[str, str], ...]):
        super().__init__(
            parent,
            None,
            _(i18n_keys.FORM__MORE),
            "A:/res/group-icon-more.png",
            items,
        )


class RawDataItem:
    def __init__(
        self, parent, raw_data, max_length=225, primary_color=lv_colors.ONEKEY_GREEN
    ):
        from trezor import strings

        self.data_str = strings.format_customer_data(raw_data)
        if not self.data_str:
            return
        self.primary_color = primary_color
        self.long_data = False
        if len(self.data_str) > max_length:
            self.long_data = True
            self.data = self.data_str[: max_length - 3] + "..."
        else:
            self.data = self.data_str
        self.item_data = CardItem(
            parent,
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


class ShowMore:
    def __init__(self, parent, align_base, relative_pos=(0, 8)):
        self.view_btn = NormalButton(
            parent,
            f"{LV_SYMBOLS.LV_SYMBOL_ANGLE_DOUBLE_DOWN}  {_(i18n_keys.BUTTON__DETAILS)}",
        )
        self.view_btn.set_size(456, 82)
        self.view_btn.add_style(StyleWrapper().text_font(font_GeistSemiBold26), 0)
        self.view_btn.enable()
        self.view_btn.align_to(align_base, lv.ALIGN.OUT_BOTTOM_MID, *relative_pos)
