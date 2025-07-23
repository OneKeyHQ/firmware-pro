from typing import TYPE_CHECKING

from trezor import utils
from trezor.enums import BackupType
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.lvglui.scrs.components.container import (
    ContainerFlexCol,
    ContainerFlexRow,
    ContainerGrid,
)
from trezor.lvglui.scrs.components.listitem import ListItemWithLeadingCheckbox

from . import (
    font_GeistMono28,
    font_GeistRegular20,
    font_GeistRegular26,
    font_GeistSemiBold26,
    font_GeistSemiBold30,
    lv,
    lv_colors,
)
from .common import FullSizeWindow
from .components.radio import RadioTrigger
from .widgets.style import StyleWrapper

if TYPE_CHECKING:
    from typing import Sequence

    pass


class MnemonicDisplay(FullSizeWindow):
    def __init__(
        self,
        title: str,
        subtitle: str,
        mnemonics: Sequence[str],
        indicator_text: str | None = None,
    ):
        word_count = len(mnemonics)
        super().__init__(
            title,
            subtitle,
            _(i18n_keys.BUTTON__CONTINUE),
            anim_dir=0,
        )
        self.subtitle.set_recolor(True)
        if indicator_text:
            self.add_indicator(indicator_text)
        else:
            self.add_nav_back()
        # self.content_area.set_style_bg_color(
        #     lv_colors.WHITE_3, lv.PART.SCROLLBAR | lv.STATE.DEFAULT
        # )
        row_dsc = [58] * (int((word_count + 1) // 2))
        row_dsc.append(lv.GRID_TEMPLATE.LAST)
        # 2 columns
        col_dsc = [
            226,
            226,
            lv.GRID_TEMPLATE.LAST,
        ]
        self.container = ContainerGrid(
            self.content_area,
            row_dsc=row_dsc,
            col_dsc=col_dsc,
            align_base=self.subtitle,
            pos=(-12, 40),
            pad_gap=4,
        )
        word_style = (
            StyleWrapper()
            .pad_hor(8)
            .pad_ver(12)
            .radius(40)
            .bg_color(lv_colors.ONEKEY_GRAY_3)
            .bg_opa(lv.OPA.COVER)
            .text_align_left()
        )
        self.container.add_style(
            StyleWrapper()
            .text_font(font_GeistMono28)
            .text_color(lv_colors.WHITE)
            .pad_bottom(48),
            0,
        )
        self.container.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
        self.clear_flag(lv.obj.FLAG.SCROLLABLE)
        self.content_area.set_scroll_dir(lv.DIR.VER)
        self.content_area.clear_flag(lv.obj.FLAG.SCROLL_ELASTIC)
        self.content_area.clear_flag(lv.obj.FLAG.SCROLL_MOMENTUM)
        # self.set_grid_align(lv.GRID_ALIGN.STRETCH, lv.GRID_ALIGN.STRETCH)
        half = int((word_count + 1) // 2)
        for i in range(word_count):
            col = 0 if i < half else 1
            row = i % half
            word = lv.obj(self.container)
            word.remove_style_all()
            word.add_style(word_style, 0)
            word_label = lv.label(word)
            word_label.set_align(lv.ALIGN.LEFT_MID)
            word_label.set_text(f"{i+1:>2}. {mnemonics[i]}")
            word.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            word.set_grid_cell(
                lv.GRID_ALIGN.STRETCH, col, 1, lv.GRID_ALIGN.STRETCH, row, 1
            )
        self.content_area.add_event_cb(self.on_event, lv.EVENT.SCROLL_BEGIN, None)

    def add_indicator(self, indicator_text: str):
        self.word_count_indicator = lv.btn(self)
        self.word_count_indicator.set_size(lv.SIZE.CONTENT, 40)
        self.word_count_indicator.add_style(
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_GRAY_3)
            .bg_opa(lv.OPA.COVER)
            .text_font(font_GeistRegular20)
            .radius(100)
            .border_width(0)
            .pad_hor(12)
            .pad_ver(0)
            .text_align_left()
            .text_color(lv_colors.WHITE),
            0,
        )
        self.label = lv.label(self.word_count_indicator)
        self.label.set_long_mode(lv.label.LONG.WRAP)
        self.label.set_text(indicator_text)
        self.label.align(lv.ALIGN.LEFT_MID, 0, 0)
        self.word_count_indicator.align(lv.ALIGN.TOP_RIGHT, -12, 56)
        self.word_count_indicator.set_ext_click_area(50)
        self.click_indicator_img = lv.img(self.word_count_indicator)
        self.click_indicator_img.set_src("A:/res/s-arrow-down.png")
        self.click_indicator_img.align_to(self.label, lv.ALIGN.OUT_RIGHT_MID, 4, 0)
        self.click_indicator_img.set_size(13, 7)
        self.word_count_indicator.add_event_cb(self.on_event, lv.EVENT.CLICKED, None)

        self.content_area.align(lv.ALIGN.TOP_MID, 0, 96)

    def on_event(self, event_obj: lv.event_t):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.word_count_indicator:
                BackupTypeSelector(self)
        elif code == lv.EVENT.SCROLL_BEGIN:
            utils.lcd_resume()
        # region
        # self.panel = lv.obj(self.content_area)
        # self.panel.set_size(464, lv.SIZE.CONTENT)
        # self.panel.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 24)
        # self.panel.set_style_border_width(0, 0)
        # self.panel.set_style_bg_color(lv_colors.ONEKEY_GRAY_3, 0)
        # self.panel.set_style_bg_opa(255, 0)
        # self.panel.set_style_pad_ver(24, 0)
        # self.panel.set_style_pad_hor(5, 0)
        # self.panel.set_style_radius(2, 0)
        # self.panel.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
        # self.word_col1 = lv.label(self.panel)
        # self.word_col1.set_size(lv.pct(50), lv.SIZE.CONTENT)
        # self.word_col1.set_recolor(True)
        # self.word_col1.align(lv.ALIGN.TOP_LEFT, 0, 0)
        # self.word_col1.set_style_text_font(font_GeistMono28, 0)
        # self.word_col1.set_style_text_align(lv.TEXT_ALIGN.LEFT, 0)
        # self.word_col1.set_style_pad_hor(5, 0)
        # self.word_col1.set_style_text_line_space(18, 0)
        # # set_style_text_letter_space
        # self.word_col2 = lv.label(self.panel)
        # self.word_col2.set_size(lv.pct(50), lv.SIZE.CONTENT)
        # self.word_col2.set_recolor(True)
        # self.word_col2.align(lv.ALIGN.TOP_RIGHT, 0, 0)
        # self.word_col2.set_style_text_font(font_GeistMono28, 0)
        # self.word_col2.set_style_text_align(lv.TEXT_ALIGN.LEFT, 0)
        # self.word_col2.set_style_pad_hor(5, 0)
        # self.word_col2.set_style_text_line_space(18, 0)

        # text_col = ""
        # text_col2 = ""
        # for index in range(0, int(word_count / 2)):
        #     text_col += f"#999999 {index+1:>2}.#{mnemonics[index]}\n"
        #     text_col2 += f"#999999 {int(index+int(word_count/2)+1):>2}.#{mnemonics[int(index+int(word_count/2))]}\n"
        #     self.word_col1.set_text(text_col.rstrip())
        #     self.word_col2.set_text(text_col2.rstrip())
        # self.item = ListItemWithLeadingCheckbox(
        #     self.content_area,
        #     _(i18n_keys.CHECK__I_HAVE_WRITE_DOWN_THE_WORDS),
        # )
        # self.item.set_size(460, lv.SIZE.CONTENT)
        # self.item.align_to(self.panel, lv.ALIGN.OUT_BOTTOM_MID, 0, 10)

    #     self.btn_yes.disable()
    #     self.content_area.add_event_cb(self.value_changed, lv.EVENT.VALUE_CHANGED, None)

    # def value_changed(self, event_obj):
    #     code = event_obj.code
    #     target = event_obj.get_target()
    #     if code == lv.EVENT.VALUE_CHANGED:
    #         if target == self.item.checkbox:
    #             if target.get_state() & lv.STATE.CHECKED:
    #                 self.item.enable_bg_color()
    #                 self.btn_yes.enable(bg_color=lv_colors.ONEKEY_GREEN)
    #             else:
    #                 self.item.enable_bg_color(enable=False)
    #                 self.btn_yes.disable()
    # endregion


class CheckWordTips(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__SETUP_CREATE_ALMOST_DONE),
            _(i18n_keys.SUBTITLE__SETUP_CREATE_ALMOST_DOWN),
            confirm_text=_(i18n_keys.BUTTON__CONTINUE),
            cancel_text=_(i18n_keys.BUTTON__BACK),
        )


class BackupTips(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__READY_TO_BACK_UP),
            _(i18n_keys.SUBTITLE__DEVICE_BACKUP_BACK_UP_RECOVERY_PHRASE),
            _(i18n_keys.BUTTON__BACK_UP),
        )
        self.content_area.clear_flag(lv.obj.FLAG.SCROLL_ELASTIC)
        self.content_area.clear_flag(lv.obj.FLAG.SCROLL_MOMENTUM)
        self.container = ContainerFlexCol(
            self.content_area,
            self.subtitle,
            pos=(0, 40),
            padding_row=10,
            clip_corner=False,
        )
        self.item1 = ListItemWithLeadingCheckbox(
            self.container,
            _(i18n_keys.CHECK__DEVICE_BACK_UP_RECOVERY_PHRASE_1),
            radius=40,
        )
        self.item2 = ListItemWithLeadingCheckbox(
            self.container,
            _(i18n_keys.CHECK__DEVICE_BACK_UP_RECOVERY_PHRASE_2),
            radius=40,
        )
        self.item3 = ListItemWithLeadingCheckbox(
            self.container,
            _(i18n_keys.CHECK__DEVICE_BACK_UP_RECOVERY_PHRASE_3),
            radius=40,
        )
        self.btn_yes.disable()
        self.container.add_event_cb(self.on_event, lv.EVENT.VALUE_CHANGED, None)
        self.cb_cnt = 0

    def on_event(self, event_obj: lv.event_t):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.item1.checkbox:
                if target.get_state() & lv.STATE.CHECKED:
                    self.item1.enable_bg_color()
                    self.cb_cnt += 1
                else:
                    self.item1.enable_bg_color(False)
                    self.cb_cnt -= 1
            elif target == self.item2.checkbox:
                if target.get_state() & lv.STATE.CHECKED:
                    self.item2.enable_bg_color()
                    self.cb_cnt += 1
                else:
                    self.item2.enable_bg_color(False)
                    self.cb_cnt -= 1
            elif target == self.item3.checkbox:
                if target.get_state() & lv.STATE.CHECKED:
                    self.item3.enable_bg_color()
                    self.cb_cnt += 1
                else:
                    self.item3.enable_bg_color(False)
                    self.cb_cnt -= 1
            if self.cb_cnt == 3:
                self.btn_yes.enable(
                    bg_color=lv_colors.ONEKEY_GREEN, text_color=lv_colors.BLACK
                )
            elif self.cb_cnt < 3:
                self.btn_yes.disable()


class CheckWord(FullSizeWindow):
    def __init__(self, title: str, options: str):
        super().__init__(title, _(i18n_keys.SUBTITLE__DEVICE_BACKUP_CHECK_WORD))

        self.choices = RadioTrigger(self, options)

        self.tip_panel = lv.obj(self.content_area)
        self.tip_panel.remove_style_all()
        self.tip_panel.set_size(lv.pct(100), lv.SIZE.CONTENT)
        self.tip_img = lv.img(self.tip_panel)
        self.tip_img.set_align(lv.ALIGN.LEFT_MID)
        self.tip = lv.label(self.tip_panel)
        self.tip.set_recolor(True)
        self.tip.set_text("")
        self.tip_panel.add_style(
            StyleWrapper().text_font(font_GeistRegular26).text_align_center(),
            0,
        )
        self.tip_panel.align_to(self.choices.container, lv.ALIGN.OUT_TOP_MID, 12, -14)

        self.add_event_cb(self.on_ready, lv.EVENT.READY, None)

    def on_ready(self, event_obj):
        self.show_unload_anim()
        self.channel.publish(self.choices.get_selected_str())
        # self.destroy()

    def tip_correct(self):
        self.tip_img.set_src("A:/res/feedback-correct.png")
        self.tip.set_text(f"#00FF33 {_(i18n_keys.MSG__CORRECT__EXCLAMATION)}#")
        self.tip.align_to(self.tip_img, lv.ALIGN.OUT_RIGHT_MID, 4, 0)
        # self.tip.clear_flag(lv.obj.FLAG.HIDDEN)

    def tip_incorrect(self):
        self.tip_img.set_src("A:/res/feedback-incorrect.png")
        self.tip.set_text(f"#FF1100 {_(i18n_keys.MSG__INCORRECT__EXCLAMATION)}#")
        self.tip.align_to(self.tip_img, lv.ALIGN.OUT_RIGHT_MID, 4, 0)
        # self.tip.clear_flag(lv.obj.FLAG.HIDDEN)


class BackupTypeSelector(FullSizeWindow):
    def __init__(self, parent):
        super().__init__(
            _(i18n_keys.TITLE__RECOVERY_PHRASE_TYPES),
            _(i18n_keys.TITLE__RECOVERY_PHRASE_TYPES_DESC),
        )
        self.parent = parent
        self.add_nav_back()
        self.bkts_cls_style = (
            StyleWrapper()
            .text_font(font_GeistSemiBold26)
            .text_align_left()
            .text_color(lv_colors.WHITE_2)
        )
        self.content_area.clear_flag(lv.obj.FLAG.SCROLL_ELASTIC)
        self.content_area.clear_flag(lv.obj.FLAG.SCROLL_MOMENTUM)
        # BIP-39
        self.backup_types_bip39 = lv.label(self.content_area)
        self.backup_types_bip39.add_style(
            self.bkts_cls_style,
            0,
        )
        self.backup_types_bip39.set_text(_(i18n_keys.TITLE__LEGACY_BACKUP_TYPES_BIP39))
        self.backup_types_bip39.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 24)
        optional_str_bip39 = (
            _(i18n_keys.OPTION__STR_WRODS).format(12)
            + "\n"
            + _(i18n_keys.OPTION__STR_WRODS).format(18)
            + "\n"
            + _(i18n_keys.OPTION__STR_WRODS).format(24)
        )
        self.bip39_choice = RadioTrigger(self.content_area, optional_str_bip39)
        self.bip39_choice.container.align_to(
            self.backup_types_bip39, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 8
        )

        # SLIP-39
        self.backup_types_slip39 = lv.label(self.content_area)
        self.backup_types_slip39.add_style(
            self.bkts_cls_style,
            0,
        )
        self.backup_types_slip39.set_text(
            _(i18n_keys.TITLE__ADVANCED_BACKUP_TYPES_SLIP_39)
        )
        self.backup_types_slip39.align_to(
            self.bip39_choice.container, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 24
        )
        optional_str_slip39 = (
            _(i18n_keys.BUTTON__SINGLE_SHARE_BACKUP)
            + "\n"
            + _(i18n_keys.BUTTON__MULTI_SHARE_BACKUP)
        )
        self.slip39_choice = RadioTrigger(self.content_area, optional_str_slip39)
        self.slip39_choice.container.align_to(
            self.backup_types_slip39, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 8
        )

        self.slip39_tips = lv.label(self.content_area)
        self.slip39_tips.set_size(432, lv.SIZE.CONTENT)
        self.slip39_tips.set_long_mode(lv.label.LONG.WRAP)
        self.slip39_tips.add_style(
            StyleWrapper()
            .text_font(font_GeistRegular20)
            .text_align_left()
            .text_color(lv_colors.WHITE_2),
            0,
        )
        self.slip39_tips.set_text(
            _(
                i18n_keys.CONTENT__GENERATES_A_SINGLE_20_WORD_RECOVERY_PHRASE_OR_MULTIPLE_20_WORD_SHARES_WORDLISTS_TO_RECOVER_YOUR_WALLET
            )
        )
        self.slip39_tips.align_to(
            self.slip39_choice.container, lv.ALIGN.OUT_BOTTOM_LEFT, 12, 16
        )

        self.add_event_cb(self.on_ready, lv.EVENT.READY, None)
        self.add_event_cb(self.on_nav_back, lv.EVENT.CLICKED, None)

    def on_nav_back(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.nav_back.nav_btn:
                self.parent.channel.publish(None)
                self.destroy(200)

    def on_ready(self, event_obj):
        self.destroy(300)
        if self.bip39_choice.changed:
            selected_index = self.bip39_choice.get_selected_index()
            # bip-39 12, 18, 24
            strength = (128, 192, 256)
            self.parent.channel.publish((BackupType.Bip39, strength[selected_index]))
        elif self.slip39_choice.changed:
            selected_index = self.slip39_choice.get_selected_index()
            backup_type = (
                BackupType.Slip39_Single_Extendable
                if selected_index == 0
                else BackupType.Slip39_Basic_Extendable
            )
            self.parent.channel.publish((backup_type, 128))
        self.parent.destroy(100)


class Slip39BasicConfig(FullSizeWindow):
    def __init__(self, navigable: bool = True, min_num: int = 2):
        super().__init__(
            title=_(i18n_keys.TITLE__MULTI_SHARE_BACKUP),
            subtitle=None,
            confirm_text=_(i18n_keys.BUTTON__CONTINUE),
        )
        if navigable:
            self.add_nav_back()
        else:
            self.add_nav_back_right()
        self.slip39_config_style = (
            StyleWrapper()
            .text_font(font_GeistRegular26)
            .text_align_left()
            .text_color(lv_colors.WHITE_2)
        )
        self.member_count_label = lv.label(self.content_area)
        self.member_count_label.add_style(
            self.slip39_config_style,
            0,
        )
        self.member_count_label.set_text(_(i18n_keys.TITLE__NUMBER_OF_SHARE))
        self.member_count_label.align_to(self.title, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 24)

        self.member_count_groups = NumberGroup(
            self.content_area,
            self.member_count_label,
            max_visible_num=16,
            selected_num=5,
            min_num=min_num,
        )

        self.member_threshold_label = lv.label(self.content_area)
        self.member_threshold_label.add_style(
            self.slip39_config_style,
            0,
        )
        self.member_threshold_label.set_text(_(i18n_keys.TITLE__THRESHOLD))
        self.member_threshold_label.align_to(
            self.member_count_groups.container, lv.ALIGN.OUT_BOTTOM_LEFT, 0, 24
        )
        self.member_threshold_groups = NumberGroup(
            self.content_area,
            self.member_threshold_label,
            max_visible_num=5,
            selected_num=3,
            min_num=min_num,
            show_min_num=False,
        )
        self.member_threshold_groups.container.set_style_pad_bottom(48, 0)
        self.add_event_cb(self.eventhandler, lv.EVENT.CLICKED, None)

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if utils.lcd_resume():
                return
            if target == self.member_count_groups.selected_btn:
                self.member_count_groups.set_btn_selected(target)
                self.member_threshold_groups.set_max_visible_num(
                    self.member_count_groups.get_selected_num()
                )
            elif target == self.member_threshold_groups.selected_btn:
                self.member_threshold_groups.set_btn_selected(target)
            elif target == self.nav_back.nav_btn:
                self.destroy(50)
                self.channel.publish((0, 0))
            elif target == self.btn_yes:
                self.channel.publish(
                    (
                        self.member_count_groups.get_selected_num(),
                        self.member_threshold_groups.get_selected_num(),
                    )
                )
                self.destroy(50)


class NumberGroup:
    def __init__(
        self,
        parent,
        align_base,
        pos=(0, 8),
        max_visible_num=16,
        selected_num=5,
        min_num=2,
        show_min_num=True,
    ):
        self.parent = parent
        self.selected_num = selected_num
        self.max_visible_num = max_visible_num
        self.min_num = min_num
        self.num_count = 16 - min_num + 1

        self.container = ContainerFlexRow(
            self.parent,
            align_base=align_base,
            align=lv.ALIGN.OUT_BOTTOM_LEFT,
            pos=pos,
            padding_col=4,
        )
        self.container.set_size(456, lv.SIZE.CONTENT)
        self.container.set_style_pad_row(4, 0)
        self.container.set_flex_flow(lv.FLEX_FLOW.ROW_WRAP)
        self.container.set_flex_align(
            lv.FLEX_ALIGN.START, lv.FLEX_ALIGN.CENTER, lv.FLEX_ALIGN.CENTER
        )
        self.num_list = []
        self.selected_btn = None
        num_style = (
            StyleWrapper()
            .text_font(font_GeistSemiBold30)
            .text_align_center()
            .text_color(lv_colors.WHITE)
            .radius(40)
            .bg_color(lv_colors.ONEKEY_GRAY_3)
            .bg_opa(lv.OPA.COVER)
            .border_width(0)
        )
        num_checked_style = (
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_GREEN)
            .text_color(lv_colors.BLACK)
            .bg_opa(lv.OPA.COVER)
        )
        for i in range(0, self.num_count):
            num = i + min_num
            num_btn = lv.btn(self.container)
            num_btn.set_size(88, 94)
            num_btn.add_style(num_style, 0)
            num_btn.add_style(num_checked_style, lv.STATE.CHECKED)
            num_label = lv.label(num_btn)
            num_label.set_text(str(num))
            num_label.align(lv.ALIGN.CENTER, 0, 0)
            num_btn.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
            if num == selected_num:
                self.selected_btn = num_btn
                num_btn.add_state(lv.STATE.CHECKED)
            if num > max_visible_num or (num == 1 and not show_min_num):
                num_btn.add_flag(lv.obj.FLAG.HIDDEN)
            self.num_list.append(num_btn)

        self.container.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
        self.container.add_event_cb(self.on_select, lv.EVENT.CLICKED, None)

    def on_select(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target in self.num_list:
                self.set_btn_selected(target)

    def get_selected_num(self) -> int:
        return self.selected_num

    def get_selected_num_index(self) -> int:
        return self.selected_num - self.min_num

    def get_max_visible_num(self) -> int:
        return self.max_visible_num

    def set_max_visible_num(self, max_visible_num: int):
        self.max_visible_num = max_visible_num
        if self.selected_num > max_visible_num:
            assert self.selected_btn is not None
            self.selected_btn.clear_state(lv.STATE.CHECKED)
            self.selected_btn = self.num_list[max_visible_num - self.min_num]
            self.selected_btn.add_state(lv.STATE.CHECKED)
            self.selected_num = self.num_list.index(self.selected_btn) + self.min_num
        if self.selected_num == 1 and max_visible_num != 1:
            self.set_btn_selected(self.num_list[1])
        for i, num_btn in enumerate(self.num_list):
            current_num = i + self.min_num
            if current_num > max_visible_num or (
                current_num == 1 and max_visible_num != 1
            ):
                num_btn.add_flag(lv.obj.FLAG.HIDDEN)
            else:
                num_btn.clear_flag(lv.obj.FLAG.HIDDEN)
        self.parent.invalidate()

    def set_btn_selected(self, target_btn):
        if target_btn != self.selected_btn:
            assert self.selected_btn is not None
            self.selected_btn.clear_state(lv.STATE.CHECKED)
        self.selected_btn = target_btn
        self.selected_num = self.num_list.index(self.selected_btn) + self.min_num
        self.selected_btn.add_state(lv.STATE.CHECKED)


class CreateMultiShareBackup(FullSizeWindow):
    def __init__(self):
        super().__init__(
            title=_(i18n_keys.TITLE__CREATE_MULTI_SHARE_BACKUP),
            subtitle=_(i18n_keys.TITLE__CREATE_MULTI_SHARE_BACKUP_DESC),
            confirm_text=_(i18n_keys.BUTTON__CONTINUE),
        )
        self.add_nav_back_right()

        self.pic = lv.img(self.content_area)
        self.pic.set_src("A:/res/ss-mul-share.png")
        self.pic.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 55)


class CreateMultiShareBackupTips(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__CREATE_MULTI_SHARE_BACKUP),
            _(
                i18n_keys.CONTENT__NEXT_VERIFY_WALLET_OWNERSHIP_BY_ENTERING_YOUR_BACKUP_PHRASE_HERE_IS_WHAT_YOU_NEED_TO_KNOW
            ),
            _(i18n_keys.BUTTON__CONTINUE),
        )
        self.container = ContainerFlexCol(
            self.content_area,
            self.subtitle,
            pos=(0, 40),
            padding_row=10,
            clip_corner=False,
        )
        self.item1 = ListItemWithLeadingCheckbox(
            self.container,
            _(i18n_keys.CHECK__CREATE_MULTI_SHARE_BACKUP_PHRASE_1),
            radius=40,
        )
        self.item2 = ListItemWithLeadingCheckbox(
            self.container,
            _(i18n_keys.CHECK__CREATE_MULTI_SHARE_BACKUP_PHRASE_2),
            radius=40,
        )
        self.btn_yes.disable()
        self.container.add_event_cb(self.on_event, lv.EVENT.VALUE_CHANGED, None)
        self.cb_cnt = 0

    def on_event(self, event_obj: lv.event_t):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:
            if target == self.item1.checkbox:
                if target.get_state() & lv.STATE.CHECKED:
                    self.item1.enable_bg_color()
                    self.cb_cnt += 1
                else:
                    self.item1.enable_bg_color(False)
                    self.cb_cnt -= 1
            elif target == self.item2.checkbox:
                if target.get_state() & lv.STATE.CHECKED:
                    self.item2.enable_bg_color()
                    self.cb_cnt += 1
                else:
                    self.item2.enable_bg_color(False)
                    self.cb_cnt -= 1
            if self.cb_cnt == 2:
                self.btn_yes.enable(
                    bg_color=lv_colors.ONEKEY_GREEN, text_color=lv_colors.BLACK
                )
            elif self.cb_cnt < 2:
                self.btn_yes.disable()
