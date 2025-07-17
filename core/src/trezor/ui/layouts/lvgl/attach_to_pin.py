from trezor import config, wire
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.lvglui.lv_colors import lv_colors
from trezor.lvglui.scrs import font_GeistRegular30, font_GeistSemiBold64
from trezor.lvglui.scrs.common import FullSizeWindow, lv
from trezor.lvglui.scrs.components.container import ContainerFlexCol
from trezor.lvglui.scrs.components.listitem import ListItemWithLeadingCheckbox

from apps.base import lock_device_if_unlocked
from apps.common.pin_constants import AttachCommon, PinResult, PinType

# UI Constants
USER_CANCELLED = 0
USER_CONFIRMED = 1
MAX_PASSPHRASE_LENGTH = 50


async def show_attach_to_pin_window(ctx):
    from trezor.lvglui.scrs.pinscreen import (
        request_passphrase_pin_confirm,
        request_passphrase_pin,
    )

    try:
        from trezor.crypto import se_thd89
        from apps.common.request_pin import (
            error_pin_invalid,
            request_pin_and_sd_salt,
            error_pin_used,
        )

        pin_screen_result = await show_pin_input_screen(ctx)
        if not pin_screen_result:
            return False
        curpin, _salt = await request_pin_and_sd_salt(
            ctx,
            _(i18n_keys.TITLE__ENTER_PIN),
            allow_fingerprint=False,
            standy_wall_only=True,
        )

        pinstatus, result = config.check_pin(curpin, None, PinType.USER_CHECK)
        if not pinstatus:
            return await error_pin_invalid(ctx)

        passphrase_pin = await request_passphrase_pin_confirm(ctx)
        if passphrase_pin == USER_CANCELLED:
            return False
        if curpin == passphrase_pin:
            return await error_pin_used(ctx)

        if len(passphrase_pin) >= AttachCommon.ATTACH_TO_PIN_MIN_LEN:
            passphrase_pin_str = (
                str(passphrase_pin)
                if not isinstance(passphrase_pin, str)
                else passphrase_pin
            )
            pinstatus, result = config.check_pin(
                passphrase_pin_str, None, PinType.PASSPHRASE_PIN_CHECK
            )
            if result == PinResult.PASSPHRASE_PIN_NO_MATCHED:
                current_space = se_thd89.get_pin_passphrase_space()
                if current_space < 1:
                    result = await show_hit_the_limit_window(ctx)
                    if result == USER_CONFIRMED:
                        while True:
                            passphrase_pin = await request_passphrase_pin(
                                ctx, _(i18n_keys.TITLE__ENTER_HIDDEN_WALLET_PIN)
                            )
                            if passphrase_pin == USER_CANCELLED:
                                return
                            passphrase_pin_str = (
                                str(passphrase_pin)
                                if not isinstance(passphrase_pin, str)
                                else passphrase_pin
                            )
                            pinstatus, result = config.check_pin(
                                passphrase_pin_str, None, PinType.PASSPHRASE_PIN_CHECK
                            )

                            if (
                                passphrase_pin_str != curpin
                                and result == PinResult.PASSPHRASE_PIN_ENTERED
                            ):
                                remove_status = await show_confirm_remove_pin_window(
                                    ctx
                                )
                                if remove_status == USER_CONFIRMED:
                                    passphrase_pin_str = (
                                        str(passphrase_pin)
                                        if not isinstance(passphrase_pin, str)
                                        else passphrase_pin
                                    )
                                    (
                                        remove_result,
                                        is_current,
                                    ) = se_thd89.delete_pin_passphrase(
                                        passphrase_pin_str
                                    )
                                    if remove_result:
                                        await showr_remove_pin_success_window(ctx)
                                        if is_current:
                                            return lock_device_if_unlocked()
                                        return True
                                    else:
                                        return False

                                elif remove_status == USER_CANCELLED:
                                    return
                            else:
                                try_again = await error_passphrase_pin_invalid(ctx)
                                if try_again == USER_CONFIRMED:
                                    continue
                                else:
                                    return
                    else:
                        return
                result = await show_not_attached_window(ctx)
                if result == USER_CANCELLED:
                    return False
                while True:
                    result = await show_attach_one_passphrase(ctx)
                    if result == USER_CANCELLED:
                        return False

                    from trezor.ui.layouts import request_passphrase_on_device

                    passphrase = await request_passphrase_on_device(
                        ctx, MAX_PASSPHRASE_LENGTH, min_len=1
                    )
                    if passphrase is None:
                        continue
                    if passphrase != USER_CANCELLED:
                        await show_save_your_passphrase_window(ctx)
                        curpin_str = (
                            str(curpin) if not isinstance(curpin, str) else curpin
                        )
                        passphrase_pin_str = (
                            str(passphrase_pin)
                            if not isinstance(passphrase_pin, str)
                            else passphrase_pin
                        )
                        passphrase_content_str = (
                            str(passphrase)
                            if not isinstance(passphrase, str)
                            else passphrase
                        )

                        save_result, save_status = se_thd89.save_pin_passphrase(
                            curpin_str, passphrase_pin_str, passphrase_content_str
                        )
                        if save_result:
                            await show_passphrase_set_and_attached_to_pin_window(ctx)
                            return True
                        else:
                            return False
            else:
                if passphrase_pin == curpin:
                    await show_pin_already_used_window(ctx)
                else:
                    next_status = await show_has_attached_window(ctx)
                    if next_status == USER_CONFIRMED:
                        while True:
                            passphrase_result = await show_attach_one_passphrase(ctx)
                            if passphrase_result == USER_CANCELLED:
                                return False
                            from trezor.ui.layouts import request_passphrase_on_device

                            passphrase = await request_passphrase_on_device(
                                ctx, MAX_PASSPHRASE_LENGTH, min_len=1
                            )
                            if passphrase is None:
                                continue
                            if passphrase != USER_CANCELLED:
                                await show_save_your_passphrase_window(ctx)
                                curpin_str = (
                                    str(curpin)
                                    if not isinstance(curpin, str)
                                    else curpin
                                )
                                passphrase_pin_str = (
                                    str(passphrase_pin)
                                    if not isinstance(passphrase_pin, str)
                                    else passphrase_pin
                                )
                                passphrase_content_str = (
                                    str(passphrase)
                                    if not isinstance(passphrase, str)
                                    else passphrase
                                )
                                save_result, save_status = se_thd89.save_pin_passphrase(
                                    curpin_str,
                                    passphrase_pin_str,
                                    passphrase_content_str,
                                )

                                # Only proceed if save was successful
                                if not save_result:
                                    return False

                                # Convert passphrase pin to string if needed
                                passphrase_pin_str = (
                                    str(passphrase_pin)
                                    if not isinstance(passphrase_pin, str)
                                    else passphrase_pin
                                )

                                # Verify the pin
                                pinstatus, result = config.check_pin(
                                    passphrase_pin_str,
                                    None,
                                    PinType.USER_AND_PASSPHRASE_PIN,
                                )

                                # Show success message
                                await show_passphrase_set_and_attached_to_pin_window(
                                    ctx
                                )

                                # Lock device if needed based on save status
                                if save_status:
                                    return lock_device_if_unlocked()
                                return True

                    elif next_status == USER_CANCELLED:
                        remove_status = await show_confirm_remove_pin_window(ctx)
                        if remove_status == USER_CONFIRMED:
                            passphrase_pin_str = (
                                str(passphrase_pin)
                                if not isinstance(passphrase_pin, str)
                                else passphrase_pin
                            )
                            remove_result, is_current = se_thd89.delete_pin_passphrase(
                                passphrase_pin_str
                            )
                            if remove_result:
                                await showr_remove_pin_success_window(ctx)
                                if is_current:
                                    return lock_device_if_unlocked()
                                return True
                            else:
                                return False
                    else:
                        return False

        return True
    except Exception:
        return False


async def error_passphrase_pin_invalid(ctx: wire.Context):
    screen = FullSizeWindow(
        _(i18n_keys.TITLE__WRONG_PIN),
        _(i18n_keys.SUBTITLE__SET_PIN_WRONG_PIN),
        confirm_text=_(i18n_keys.BUTTON__TRY_AGAIN),
        cancel_text=_(i18n_keys.BUTTON__CLOSE),
        icon_path="A:/res/danger.png",
        anim_dir=0,
    )
    return await ctx.wait(screen.request())


# PIN is not be attached
async def show_not_attached_window(ctx: wire.Context):
    screen = FullSizeWindow(
        None,
        None,
        confirm_text=_(i18n_keys.PASSPHRASE__PIN_ATTACHED_ONE),
        anim_dir=0,
    )

    close_btn = lv.btn(screen)
    close_btn.set_size(48, 48)
    close_btn.align(lv.ALIGN.TOP_RIGHT, -12, 56)
    close_btn.set_style_bg_color(lv_colors.BLACK, 0)
    close_btn.set_style_bg_opa(0, 0)
    close_btn.set_style_border_width(0, 0)
    close_btn.set_style_shadow_width(0, 0)
    close_btn.add_flag(lv.obj.FLAG.CLICKABLE)
    close_btn.set_ext_click_area(100)

    close_img = lv.img(close_btn)
    close_img.set_src("A:/res/nav-icon.png")
    close_img.center()

    title_label = lv.label(screen.content_area)
    title_label.set_text(_(i18n_keys.PASSPHRASE__PIN_NOT_ATTACHED))
    title_label.set_style_text_font(font_GeistSemiBold64, 0)
    title_label.set_style_text_color(lv_colors.WHITE, 0)
    title_label.set_style_text_letter_space(-6, 0)
    title_label.set_style_text_line_space(-6, 0)
    title_label.set_long_mode(lv.label.LONG.WRAP)
    title_label.set_size(456, lv.SIZE.CONTENT)
    title_label.align(lv.ALIGN.TOP_MID, 0, 72)

    if hasattr(screen, "subtitle"):
        screen.subtitle.delete()

    subtitle_label = lv.label(screen.content_area)
    subtitle_label.set_text(_(i18n_keys.PASSPHRASE__PIN_NOT_ATTACHED_DESC))
    subtitle_label.set_style_text_font(font_GeistRegular30, 0)
    subtitle_label.set_style_text_color(lv_colors.LIGHT_GRAY, 0)
    subtitle_label.set_style_text_letter_space(-2, 0)
    subtitle_label.set_style_text_line_space(5, 0)
    subtitle_label.set_long_mode(lv.label.LONG.WRAP)
    subtitle_label.set_size(456, lv.SIZE.CONTENT)
    subtitle_label.align_to(title_label, lv.ALIGN.OUT_BOTTOM_MID, 0, 16)

    processing = False

    def on_close_clicked(e):
        nonlocal processing
        if e.code == lv.EVENT.CLICKED and not processing:
            processing = True
            screen.show_dismiss_anim()
            screen.channel.publish(USER_CANCELLED)

    close_btn.add_event_cb(on_close_clicked, lv.EVENT.CLICKED, None)

    screen.btn_layout_ver()
    result = await ctx.wait(screen.request())
    return result


async def show_has_attached_window(ctx: wire.Context):
    screen = FullSizeWindow(
        None,
        None,
        confirm_text=_(i18n_keys.PASSPHRASE__PIN_UPDATE),
        cancel_text=_(i18n_keys.PASSPHRASE__PIN_REMOVE),
        anim_dir=0,
    )

    close_btn = lv.btn(screen)
    close_btn.set_size(48, 48)
    close_btn.align(lv.ALIGN.TOP_RIGHT, -12, 56)
    close_btn.set_style_bg_color(lv_colors.BLACK, 0)
    close_btn.set_style_bg_opa(0, 0)
    close_btn.set_style_border_width(0, 0)
    close_btn.set_style_shadow_width(0, 0)
    close_btn.add_flag(lv.obj.FLAG.CLICKABLE)
    close_btn.set_ext_click_area(100)

    close_img = lv.img(close_btn)
    close_img.set_src("A:/res/nav-icon.png")
    close_img.center()

    title_label = lv.label(screen.content_area)
    title_label.set_text(_(i18n_keys.PASSPHRASE__PIN_ATTACHED))
    title_label.set_style_text_font(font_GeistSemiBold64, 0)
    title_label.set_style_text_color(lv_colors.WHITE, 0)
    title_label.set_style_text_letter_space(-6, 0)
    title_label.set_style_text_line_space(-6, 0)
    title_label.set_long_mode(lv.label.LONG.WRAP)
    title_label.set_size(456, lv.SIZE.CONTENT)
    title_label.align(lv.ALIGN.TOP_MID, 0, 80)

    if hasattr(screen, "subtitle"):
        screen.subtitle.delete()

    subtitle_label = lv.label(screen.content_area)
    subtitle_label.set_text(_(i18n_keys.PASSPHRASE__PIN_ATTACHED_DESC))
    subtitle_label.set_style_text_font(font_GeistRegular30, 0)
    subtitle_label.set_style_text_color(lv_colors.LIGHT_GRAY, 0)
    subtitle_label.set_style_text_letter_space(-2, 0)
    subtitle_label.set_style_text_line_space(5, 0)
    subtitle_label.set_long_mode(lv.label.LONG.WRAP)
    subtitle_label.set_size(456, lv.SIZE.CONTENT)
    subtitle_label.align_to(title_label, lv.ALIGN.OUT_BOTTOM_MID, 0, 16)

    processing = False

    def on_close_clicked(e):
        nonlocal processing
        if e.code == lv.EVENT.CLICKED and not processing:
            processing = True
            screen.show_dismiss_anim()
            screen.channel.publish(-1)

    close_btn.add_event_cb(on_close_clicked, lv.EVENT.CLICKED, None)
    # screen.btn_no.enable(lv_colors.ONEKEY_RED_1, text_color=lv_colors.BLACK)
    result = await ctx.wait(screen.request())
    return result


async def show_pin_already_used_window(ctx: wire.Context):
    screen = FullSizeWindow(
        _(i18n_keys.PASSPHRASE__PIN_USED),
        _(i18n_keys.PASSPHRASE__PIN_USED_DESC),
        confirm_text=_(i18n_keys.BUTTON__CLOSE),
        icon_path="A:/res/danger.png",
        anim_dir=0,
    )
    result = await ctx.wait(screen.request())
    return result


# Hit the limit
async def show_hit_the_limit_window(ctx: wire.Context):
    screen = FullSizeWindow(
        _(i18n_keys.PASSPHRASE__PIN_HIT_LIMIT),
        _(i18n_keys.PASSPHRASE__PIN_HIT_LIMIT_DESC),
        confirm_text=_(i18n_keys.PASSPHRASE__PIN_REMOVE),
        cancel_text=_(i18n_keys.BUTTON__CLOSE),
        icon_path="A:/res/danger.png",
        anim_dir=0,
    )
    screen.btn_yes.enable(lv_colors.ONEKEY_RED_1, text_color=lv_colors.BLACK)
    result = await ctx.wait(screen.request())
    return result


# confirm remove pin
async def show_confirm_remove_pin_window(ctx: wire.Context):
    screen = FullSizeWindow(
        _(i18n_keys.PASSPHRASE__REMOVE),
        _(i18n_keys.PASSPHRASE__REMOVE_DESC),
        confirm_text=_(i18n_keys.BUTTON__REMOVE),
        cancel_text=_(i18n_keys.BUTTON__CANCEL),
        icon_path="A:/res/warning.png",
        anim_dir=0,
    )
    screen.btn_yes.enable(lv_colors.ONEKEY_RED_1, text_color=lv_colors.BLACK)
    result = await ctx.wait(screen.request())
    return result


async def showr_remove_pin_success_window(ctx: wire.Context):
    screen = FullSizeWindow(
        _(i18n_keys.PASSPHRASE__REMOVE_SUCCESSFUL),
        "",
        confirm_text=_(i18n_keys.BUTTON__DONE),
        icon_path="A:/res/success.png",
        anim_dir=0,
    )
    result = await ctx.wait(screen.request())
    return result


async def show_save_your_passphrase_window(ctx: wire.Context):
    screen = FullSizeWindow(
        _(i18n_keys.PASSPHRASE__SAVE),
        _(i18n_keys.PASSPHRASE__SAVE_DESC),
        confirm_text=_(i18n_keys.PASSPHRASE__UNDERSTAND),
        icon_path="A:/res/warning.png",
        anim_dir=0,
    )
    result = await ctx.wait(screen.request())
    return result


async def show_passphrase_set_and_attached_to_pin_window(ctx: wire.Context):
    screen = FullSizeWindow(
        _(i18n_keys.PASSPHRASE__SET),
        _(i18n_keys.PASSPHRASE__SET_DESC),
        confirm_text=_(i18n_keys.BUTTON__DONE),
        icon_path="A:/res/success.png",
        anim_dir=0,
    )
    screen.title.set_style_text_line_space(0, 0)
    result = await ctx.wait(screen.request())
    return result


async def show_attach_one_passphrase(ctx: wire.Context):
    class AttachOnePassphraseTips(FullSizeWindow):
        def __init__(self):
            title = _(i18n_keys.PASSPHRASE__ATTACH_ONE_PASSPHRASE)
            super().__init__(
                title,
                None,
                _(i18n_keys.BUTTON__SLIDE_TO_CONFIRM),
                _(i18n_keys.BUTTON__CANCEL),
                hold_confirm=True,
                primary_color=lv_colors.WHITE,
            )
            self.container = ContainerFlexCol(
                self.content_area,
                self.title,
                padding_row=8,
                clip_corner=False,
            )
            self.item1 = ListItemWithLeadingCheckbox(
                self.container,
                _(i18n_keys.PASSPHRASE__ATTACH_ONE_PASSPHRASE_DESC1),
                radius=40,
            )
            self.item2 = ListItemWithLeadingCheckbox(
                self.container,
                _(i18n_keys.PASSPHRASE__ATTACH_ONE_PASSPHRASE_DESC2),
                radius=40,
            )

            if hasattr(self, "slider"):
                self.slider.set_style_bg_color(
                    lv_colors.WHITE, lv.PART.KNOB | lv.STATE.DEFAULT
                )

                original_enable = self.slider.enable

                def new_enable(enable=True):
                    original_enable(enable)
                    if enable:
                        self.slider.set_style_bg_color(
                            lv_colors.WHITE, lv.PART.KNOB | lv.STATE.DEFAULT
                        )

                self.slider.enable = new_enable

            self.slider_enable(False)
            self.container.add_event_cb(
                self.on_value_changed, lv.EVENT.VALUE_CHANGED, None
            )
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

    screen = AttachOnePassphraseTips()
    result = await ctx.wait(screen.request())
    return result


async def show_pin_input_screen(ctx: wire.Context):
    """Display the PIN input screen for attaching passphrase to PIN"""
    screen = FullSizeWindow(
        None,
        _(i18n_keys.ITEM__ATTACH_TO_PIN_DESC),
        confirm_text=_(i18n_keys.BUTTON__CONTINUE),
        anim_dir=0,
    )

    close_btn = lv.btn(screen)
    close_btn.set_size(48, 48)
    close_btn.align(lv.ALIGN.TOP_RIGHT, -12, 56)
    close_btn.set_style_bg_color(lv_colors.BLACK, 0)
    close_btn.set_style_bg_opa(0, 0)
    close_btn.set_style_border_width(0, 0)
    close_btn.set_style_shadow_width(0, 0)
    close_btn.add_flag(lv.obj.FLAG.CLICKABLE)
    close_btn.set_ext_click_area(100)

    close_img = lv.img(close_btn)
    close_img.set_src("A:/res/nav-icon.png")
    close_img.center()

    title_label = lv.label(screen.content_area)
    title_label.set_text(_(i18n_keys.PASSPHRASE__ATTACH_TO_PIN))
    title_label.set_style_text_font(font_GeistSemiBold64, 0)
    title_label.set_style_text_color(lv_colors.WHITE, 0)
    title_label.set_style_text_letter_space(-6, 0)
    title_label.set_style_text_line_space(-6, 0)
    title_label.set_long_mode(lv.label.LONG.WRAP)
    title_label.set_size(456, lv.SIZE.CONTENT)
    title_label.align(lv.ALIGN.TOP_MID, 0, 72)

    if hasattr(screen, "subtitle"):
        screen.subtitle.delete()

    subtitle_label = lv.label(screen.content_area)
    subtitle_label.set_text(_(i18n_keys.ITEM__ATTACH_TO_PIN_DESC))
    subtitle_label.set_style_text_font(font_GeistRegular30, 0)
    subtitle_label.set_style_text_color(lv_colors.LIGHT_GRAY, 0)
    subtitle_label.set_style_text_letter_space(-2, 0)
    subtitle_label.set_style_text_line_space(5, 0)
    subtitle_label.set_long_mode(lv.label.LONG.WRAP)
    subtitle_label.set_size(456, lv.SIZE.CONTENT)
    subtitle_label.align_to(title_label, lv.ALIGN.OUT_BOTTOM_MID, 0, 16)

    pin_container = lv.obj(screen.content_area)
    pin_container.set_size(lv.pct(100), lv.SIZE.CONTENT)
    pin_container.align_to(subtitle_label, lv.ALIGN.OUT_BOTTOM_MID, 0, 24)
    pin_container.set_style_bg_opa(0, 0)
    pin_container.set_style_border_width(0, 0)
    pin_container.set_style_pad_all(0, 0)

    pin_img = lv.img(pin_container)
    pin_img.set_src("A:/res/attach_to_pin_display.png")
    pin_img.align_to(subtitle_label, lv.ALIGN.OUT_BOTTOM_MID, 0, 48)

    device_img = lv.img(screen.content_area)
    device_img.set_src("A:/res/attach_to_pin_dot_group.png")
    device_img.align_to(pin_img, lv.ALIGN.OUT_BOTTOM_MID, 0, 24)

    processing = False

    def on_close_clicked(e):
        nonlocal processing
        if e.code == lv.EVENT.CLICKED and not processing:
            processing = True
            screen.show_dismiss_anim()
            screen.channel.publish(False)

    close_btn.add_event_cb(on_close_clicked, lv.EVENT.CLICKED, None)
    result = await ctx.wait(screen.request())
    return result
