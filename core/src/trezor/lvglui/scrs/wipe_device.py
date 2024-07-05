from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.lvglui.lv_colors import lv_colors

from .common import FullSizeWindow, lv
from .components.container import ContainerFlexCol
from .components.listitem import ListItemWithLeadingCheckbox
from .widgets.style import StyleWrapper


class WipeDevice(FullSizeWindow):
    def __init__(self):
        title = _(i18n_keys.TITLE__RESET_DEVICE)
        subtitle = _(i18n_keys.SUBTITLE__RESET_DEVICE)
        confirm_text = _(i18n_keys.BUTTON__CONTINUE)
        cancel_text = _(i18n_keys.BUTTON__CANCEL)
        icon_path = "A:/res/warning.png"
        super().__init__(
            title, subtitle, confirm_text, cancel_text, icon_path, None, anim_dir=2
        )
        self.btn_yes.set_style_bg_color(
            lv_colors.ONEKEY_YELLOW, lv.PART.MAIN | lv.STATE.DEFAULT
        )
        self.status_bar = lv.obj(self)
        self.status_bar.remove_style_all()
        self.status_bar.set_size(lv.pct(100), 44)
        self.status_bar.add_style(
            StyleWrapper()
            .bg_opa()
            .align(lv.ALIGN.TOP_LEFT)
            .bg_img_src("A:/res/warning-bar.png"),
            0,
        )

    def show_unload_anim(self):
        self.clean()
        self.destroy(100)


class WipeDeviceTips(FullSizeWindow):
    def __init__(self):
        title = _(i18n_keys.TITLE__ERASE_THIS_DEVICE)
        subtitle = _(i18n_keys.SUBTITLE__DEVICE_WIPE_DEVICE_FACTORY_RESET)
        # icon_path = "A:/res/danger.png"
        super().__init__(
            title,
            subtitle,
            _(i18n_keys.BUTTON__SLIDE_TO_RESET),
            _(i18n_keys.BUTTON__CANCEL),
            # icon_path=icon_path,
            hold_confirm=True,
        )
        self.container = ContainerFlexCol(
            self.content_area,
            self.subtitle,
            padding_row=8,
            clip_corner=False,
        )
        self.item1 = ListItemWithLeadingCheckbox(
            self.container,
            _(i18n_keys.CHECK__DEVICE_WIPE_DEVICE_FACTORY_RESET_1),
            radius=40,
        )
        self.item2 = ListItemWithLeadingCheckbox(
            self.container,
            _(i18n_keys.CHECK__DEVICE_WIPE_DEVICE_FACTORY_RESET_2),
            radius=40,
        )
        self.slider_enable(False)
        self.container.add_event_cb(self.on_value_changed, lv.EVENT.VALUE_CHANGED, None)
        self.cb_cnt = 0

    def slider_enable(self, enable: bool = True):
        if enable:
            self.slider.add_flag(lv.obj.FLAG.CLICKABLE)
            self.slider.enable()
        else:
            self.slider.clear_flag(lv.obj.FLAG.CLICKABLE)
            self.slider.enable(False)

    def on_value_changed(self, event_obj):
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
                self.slider_enable()
            elif self.cb_cnt < 2:
                self.slider_enable(False)


class WipeDeviceSuccess(FullSizeWindow):
    def __init__(self):
        title = _(i18n_keys.TITLE__RESET_COMPLETE)
        subtitle = _(i18n_keys.SUBTITLE__THE_DEVICE_IS_RESET)
        icon_path = "A:/res/success.png"
        confirm_text = _(i18n_keys.BUTTON__RESTART)
        super().__init__(
            title, subtitle, confirm_text=confirm_text, icon_path=icon_path, anim_dir=0
        )

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.CLICKED:
            if target == self.btn_yes:
                self.channel.publish(1)
            self.destroy()
            from apps.base import set_homescreen

            set_homescreen()


class WipeDeviceTipsTmp(FullSizeWindow):
    def __init__(self):
        title = "This Card Contains Backup"  # 标题
        subtitle = "if you continue,your previous backup will be fully overwritten and will be lost forever"  # 副标题
        icon_path = "A:/res/warning.png"
        super().__init__(
            title,
            subtitle,
            "Overwrite",  # 滑动重置按钮文本
            _(i18n_keys.BUTTON__CANCEL),  # 取消按钮文本
            icon_path=icon_path,
            # hold_confirm=True,  # 保持确认
        )
        self.container = ContainerFlexCol(
            self.content_area,
            self.subtitle,
            padding_row=8,  # 行内边距
            clip_corner=False,
        )
        self.item1 = ListItemWithLeadingCheckbox(
            self.container,
            _(i18n_keys.CHECK__DEVICE_WIPE_DEVICE_FACTORY_RESET_1),  # 复选框项1文本
            radius=40,  # 圆角半径
        )
        print("slider_enable start")
        self.slider_enable(False)  # 禁用滑动条
        self.container.add_event_cb(
            self.on_value_changed, lv.EVENT.VALUE_CHANGED, None
        )  # 添加事件回调
        self.cb_cnt = 0  # 初始化复选框计数器

    def slider_enable(self, enable: bool = True):
        if enable:
            print("slider_enable true")  # 打印调试信息：开始备份
            self.btn_yes.add_flag(lv.obj.FLAG.CLICKABLE)  # 添加可点击标志
            self.btn_yes.enable(
                bg_color=lv_colors.ONEKEY_GREEN, text_color=lv_colors.BLACK
            )  # 启用确认按钮并设置为绿色

        else:
            print("slider_enable false")
            self.btn_yes.clear_flag(lv.obj.FLAG.CLICKABLE)  # 清除可点击标志
            self.btn_yes.enable(
                bg_color=lv_colors.ONEKEY_BLACK_1, text_color=lv_colors.ONEKEY_GRAY
            )  # 禁用确认按钮并设置为灰色

    def on_value_changed(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if code == lv.EVENT.VALUE_CHANGED:  # 值改变事件
            if target == self.item1.checkbox:
                if target.get_state() & lv.STATE.CHECKED:  # 复选框选中状态
                    self.item1.enable_bg_color()  # 启用背景颜色
                    self.cb_cnt += 1  # 复选框计数器加1
                else:
                    self.item1.enable_bg_color(False)  # 禁用背景颜色
                    self.cb_cnt -= 1  # 复选框计数器减1
            if self.cb_cnt == 1:
                self.slider_enable()  # 启用滑动条
            elif self.cb_cnt < 1:
                self.slider_enable(False)  # 禁用滑动条
