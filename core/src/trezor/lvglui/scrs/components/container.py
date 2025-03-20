from trezor.lvglui.lv_colors import lv_colors

import lvgl as lv  # type: ignore[Import "lvgl" could not be resolved]

from ..widgets.style import StyleWrapper


class ContainerFlexCol(lv.obj):
    def __init__(
        self,
        parent,
        align_base,
        align=lv.ALIGN.OUT_BOTTOM_MID,
        pos: tuple = (0, 40),
        padding_row: int = 8,
        clip_corner: bool = True,
        no_align: bool = False,
    ) -> None:
        super().__init__(parent)
        self.remove_style_all()
        self.set_size(456, lv.SIZE.CONTENT)
        if not no_align:
            if align_base is None:
                self.align(lv.ALIGN.BOTTOM_MID, 0, -8)
            else:
                self.align_to(align_base, align, 0, pos[1])
        self.add_style(
            StyleWrapper()
            .bg_opa(lv.OPA.TRANSP)
            .radius(40)
            .border_width(0)
            .pad_hor(0)
            .clip_corner(True if clip_corner else False)
            .pad_row(padding_row),
            0,
        )
        self.clear_flag(lv.obj.FLAG.CLICKABLE)
        self.set_flex_flow(lv.FLEX_FLOW.COLUMN)
        self.set_flex_align(
            lv.FLEX_ALIGN.CENTER, lv.FLEX_ALIGN.CENTER, lv.FLEX_ALIGN.CENTER
        )
        self.add_flag(lv.obj.FLAG.EVENT_BUBBLE)

    def add_dummy(self, bg_color=lv_colors.ONEKEY_GRAY_3):
        dummy = lv.obj(self)
        dummy.remove_style_all()
        dummy.set_size(lv.pct(100), 12)
        dummy.add_style(StyleWrapper().bg_color(bg_color).bg_opa(), 0)


class ContainerFlexRow(lv.obj):
    def __init__(
        self,
        parent,
        align_base,
        align=lv.ALIGN.OUT_TOP_MID,
        pos: tuple = (0, -48),
        padding_col: int = 8,
    ) -> None:
        super().__init__(parent)
        self.remove_style_all()
        self.set_size(lv.pct(100), lv.SIZE.CONTENT)
        if align_base:
            self.align_to(align_base, align, pos[0], pos[1])
        self.add_style(
            StyleWrapper()
            .bg_color(lv_colors.BLACK)
            .bg_opa(lv.OPA.TRANSP)
            .radius(0)
            .border_width(0)
            .pad_column(padding_col),
            0,
        )
        self.set_flex_flow(lv.FLEX_FLOW.ROW)
        # align style of the items in the container
        self.set_flex_align(
            lv.FLEX_ALIGN.CENTER, lv.FLEX_ALIGN.CENTER, lv.FLEX_ALIGN.CENTER
        )


class ContainerFlex(lv.obj):
    def __init__(
        self,
        parent,
        align_base,
        align=lv.ALIGN.OUT_TOP_MID,
        pos: tuple = (0, -48),
        padding_col: int = 8,
        flex_flow: lv.FLEX_FLOW = lv.FLEX_FLOW.ROW,
        main_align: lv.FLEX_ALIGN = lv.FLEX_ALIGN.CENTER,
        cross_align: lv.FLEX_ALIGN = lv.FLEX_ALIGN.CENTER,
        track_align: lv.FLEX_ALIGN = lv.FLEX_ALIGN.CENTER,
    ) -> None:
        super().__init__(parent)
        self.remove_style_all()
        self.set_size(lv.pct(100), lv.SIZE.CONTENT)
        if align_base:
            self.align_to(align_base, align, pos[0], pos[1])
        self.add_style(
            StyleWrapper()
            .bg_color(lv_colors.BLACK)
            .bg_opa(lv.OPA.TRANSP)
            .radius(0)
            .border_width(0)
            .pad_column(padding_col),
            0,
        )
        self.set_flex_flow(flex_flow)
        # align style of the items in the container
        self.set_flex_align(main_align, cross_align, track_align)
        self.set_layout(lv.LAYOUT_FLEX.value)


class ContainerGrid(lv.obj):
    def __init__(
        self,
        parent,
        row_dsc,
        col_dsc,
        align_base=None,
        align_type=lv.ALIGN.OUT_BOTTOM_LEFT,
        pos: tuple = (0, 40),
        pad_gap=16,
    ) -> None:
        super().__init__(parent)
        self.set_size(lv.pct(100), lv.SIZE.CONTENT)
        if align_base:
            self.align_to(align_base, align_type, pos[0], pos[1])
        else:
            self.align(lv.ALIGN.TOP_MID, 0, 48)

        self.add_style(
            StyleWrapper()
            .bg_color(lv_colors.ONEKEY_GREEN_1)
            .radius(0)
            .bg_opa(lv.OPA.TRANSP)
            .pad_hor(10)
            .pad_ver(0)
            .pad_gap(pad_gap)
            .border_width(0)
            .grid_column_dsc_array(col_dsc)
            .grid_row_dsc_array(row_dsc),
            0,
        )
        self.set_grid_align(lv.GRID_ALIGN.SPACE_AROUND, lv.GRID_ALIGN.END)
        self.set_layout(lv.LAYOUT_GRID.value)
