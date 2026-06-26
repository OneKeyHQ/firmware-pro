from ..i18n import gettext as _, keys as i18n_keys
from .common import FullSizeWindow, lv
from .components.container import ContainerFlexCol
from .components.listitem import DisplayItem
from .components.radio import RadioTrigger


class ConfirmWebauthn(FullSizeWindow):
    def __init__(
        self, title: str, app_icon: str, app_name: str, account_name: str | None
    ):
        super().__init__(
            title,
            None,
            confirm_text=_(i18n_keys.BUTTON__CONFIRM),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            icon_path=app_icon,
            anim_dir=2,
        )
        self.container = ContainerFlexCol(self, self.title, pos=(0, 40), padding_row=0)
        self.container.add_dummy()
        self.item_app_name = DisplayItem(
            self.container, _(i18n_keys.LIST_KEY__APP_NAME__COLON), app_name
        )
        if account_name is not None:
            self.item_account_name = DisplayItem(
                self.container, _(i18n_keys.LIST_KEY__ACCOUNT_NAME__COLON), account_name
            )
        self.container.add_dummy()


class SelectWebauthnAccount(FullSizeWindow):
    def __init__(
        self, title: str, app_icon: str, app_name: str, account_names: list[str]
    ):
        super().__init__(
            title,
            app_name,
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            icon_path=app_icon,
            anim_dir=2,
        )
        self.content_area.set_style_max_height(632, 0)
        options = "\n".join(account_names)
        self.choices = RadioTrigger(self.content_area, options)
        anchor = getattr(self, "subtitle", None) or self.title
        self.choices.container.align_to(anchor, lv.ALIGN.OUT_BOTTOM_MID, 0, 24)
        for item in self.choices.items:
            arrow = lv.img(item)
            arrow.set_src("A:/res/arrow-right.png")
            arrow.set_align(lv.ALIGN.RIGHT_MID)
        self.content_area.add_event_cb(self.on_ready, lv.EVENT.READY, None)

    def on_ready(self, _event_obj):
        self.show_unload_anim()
        self.channel.publish(self.choices.get_selected_index())

    def eventhandler(self, event_obj):
        code = event_obj.code
        target = event_obj.get_target()
        if (
            code == lv.EVENT.CLICKED
            and hasattr(self, "btn_no")
            and target == self.btn_no
        ):
            self.show_dismiss_anim()
            self.channel.publish(None)
        else:
            super().eventhandler(event_obj)
