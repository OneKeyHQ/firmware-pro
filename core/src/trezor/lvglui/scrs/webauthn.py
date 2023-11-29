from ..i18n import gettext as _, keys as i18n_keys
from .common import FullSizeWindow
from .components.container import ContainerFlexCol
from .components.listitem import DisplayItem


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
