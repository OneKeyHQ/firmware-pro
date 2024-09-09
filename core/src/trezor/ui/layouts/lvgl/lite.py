from trezorio import nfc

from trezor import wire
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.lvglui.scrs.common import FullSizeWindow, lv
from trezor.lvglui.scrs.nfc import (
    LITE_CARD_CONNECT_FAILURE,
    LITE_CARD_FIND,
    LITE_CARD_HAS_BEEN_RESET,
    LITE_CARD_NO_BACKUP,
    LITE_CARD_NOT_SAME,
    LITE_CARD_OPERATE_SUCCESS,
    LITE_CARD_PIN_ERROR,
    LITE_CARD_UNSUPPORTED_WORD_COUNT,
    SearchDeviceScreen,
    TransferDataScreen,
)
from trezor.lvglui.scrs.pinscreen import InputLitePin, request_lite_pin_confirm

LITE_CARD_BUTTON_CONFIRM = 1
LITE_CARD_BUTTON_CANCLE = 0
LITE_CARD_BLANK = 2
LITE_CARD_WITH_DATA = 3


async def show_fullsize_window(
    ctx: wire.GenericContext,
    title,
    content,
    confirm_text,
    cancel_text=None,
    icon_path=None,
):
    screen = FullSizeWindow(
        title,
        content,
        confirm_text=confirm_text,
        cancel_text=cancel_text,
        icon_path=icon_path,
        anim_dir=0,
    )
    screen.btn_layout_ver()
    if hasattr(screen, "subtitle"):
        screen.subtitle.set_recolor(True)
    result = await ctx.wait(screen.request())
    return result


async def show_start_screen(ctx: wire.GenericContext):
    screen = FullSizeWindow(
        _(i18n_keys.TITLE__GET_STARTED),
        _(i18n_keys.CONTENT__PLACE_LITE_DEVICE_FIGURE_CLICK_CONTINUE),
        confirm_text=_(i18n_keys.BUTTON__CONTINUE),
        cancel_text=_(i18n_keys.BUTTON__BACK),
        anim_dir=0,
    )
    screen.img = lv.img(screen.content_area)
    screen.img.set_src("A:/res/nfc-start.png")
    screen.img.align_to(screen.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 52)
    return await ctx.wait(screen.request())


async def search_device(ctx: wire.GenericContext):
    search_scr = SearchDeviceScreen()
    return await ctx.wait(search_scr.request())


async def input_pin(ctx: wire.GenericContext):
    return await ctx.wait(InputLitePin().request())


async def backup_with_lite(
    ctx: wire.GenericContext, mnemonics: bytes, recovery_check: bool = False
):
    async def handle_pin_setup(card_num, mnemonics):
        pin = await request_lite_pin_confirm(ctx)
        if pin and pin != LITE_CARD_BUTTON_CANCLE:
            flag = await handle_second_placement(card_num, pin, mnemonics)
            return flag
        else:
            pass

    async def handle_second_placement(card_num, pin, mnemonics):
        while True:
            start_scr_againc = FullSizeWindow(
                _(i18n_keys.TITLE__CONNECT_AGAIN),
                _(i18n_keys.CONTENT__KEEP_LITE_DEVICE_TOGETHER_BACKUP_COMPLETE),
                confirm_text=_(i18n_keys.BUTTON__CONTINUE),
                cancel_text=_(i18n_keys.BUTTON__BACK),
                anim_dir=0,
            )
            start_scr_againc.img = lv.img(start_scr_againc.content_area)
            start_scr_againc.img.set_src("A:/res/nfc-start.png")
            start_scr_againc.img.align_to(
                start_scr_againc.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 52
            )
            again_flag = await ctx.wait(start_scr_againc.request())
            if again_flag == LITE_CARD_BUTTON_CONFIRM:
                while True:
                    status_code = await search_device(ctx)
                    if status_code == LITE_CARD_FIND:
                        trash_scr = TransferDataScreen()
                        trash_scr.set_pin_mnemonicmphrase(pin, card_num, mnemonics)
                        final_status_code = await ctx.wait(trash_scr.request())
                        if final_status_code == LITE_CARD_OPERATE_SUCCESS:

                            from trezor.ui.layouts import show_success

                            await show_success(
                                ctx,
                                _(i18n_keys.TITLE__BACK_UP_COMPLETE),
                                _(i18n_keys.TITLE__BACKUP_COMPLETED_DESC),
                                header=_(i18n_keys.TITLE__BACK_UP_COMPLETE),
                                button=_(i18n_keys.BUTTON__I_GOT_IT),
                            )
                            return LITE_CARD_OPERATE_SUCCESS
                        elif final_status_code == LITE_CARD_NOT_SAME:
                            await show_fullsize_window(
                                ctx,
                                _(i18n_keys.TITLE__CONNECT_FAILED),
                                _(
                                    i18n_keys.CONTENT__THE_TWO_ONEKEY_LITE_USED_FOR_CONNECTION_ARE_NOT_THE_SAME
                                ),
                                _(i18n_keys.BUTTON__I_GOT_IT),
                                icon_path="A:/res/danger.png",
                            )
                            return LITE_CARD_NOT_SAME

                        elif final_status_code == LITE_CARD_CONNECT_FAILURE:
                            flag = await show_fullsize_window(
                                ctx,
                                _(i18n_keys.TITLE__CONNECT_FAILED),
                                _(
                                    i18n_keys.CONTENT__MAKE_SURE_THE_CARD_IS_CLOSE_TO_THE_UPPER_LEFT
                                ),
                                _(i18n_keys.BUTTON__TRY_AGAIN),
                                _(i18n_keys.BUTTON__BACK),
                                icon_path="A:/res/danger.png",
                            )
                            if flag == LITE_CARD_BUTTON_CANCLE:
                                return LITE_CARD_CONNECT_FAILURE
                            elif flag == LITE_CARD_BUTTON_CONFIRM:
                                continue

                    else:
                        break
            else:
                break

    async def handle_existing_data(card_num, mnemonics):
        from trezor.lvglui.scrs.wipe_device import WipeLiteCardTips

        confirm_screen = WipeLiteCardTips()
        confirm_screen_flag = await ctx.wait(confirm_screen.request())
        if confirm_screen_flag:
            pin = await input_pin(ctx)
            place_again_data_card = True
            if pin:
                while place_again_data_card:
                    start_scr_againc = FullSizeWindow(
                        _(i18n_keys.TITLE__CONNECT_AGAIN),
                        _(i18n_keys.CONTENT__KEEP_LITE_DEVICE_TOGETHER_BACKUP_COMPLETE),
                        confirm_text=_(i18n_keys.BUTTON__CONTINUE),
                        cancel_text=_(i18n_keys.BUTTON__BACK),
                        anim_dir=0,
                    )
                    start_scr_againc.img = lv.img(start_scr_againc.content_area)
                    start_scr_againc.img.set_src("A:/res/nfc-start.png")
                    start_scr_againc.img.align_to(
                        start_scr_againc.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 52
                    )
                    again_flag = await ctx.wait(start_scr_againc.request())
                    if again_flag == LITE_CARD_BUTTON_CONFIRM:
                        place_again_data_card_next = True
                        while place_again_data_card_next:
                            status_code = await search_device(ctx)
                            if status_code == LITE_CARD_FIND:
                                trash_scr = TransferDataScreen()
                                trash_scr.check_pin_mnemonicmphrase(
                                    pin, card_num, mnemonics
                                )
                                final_status_code = await ctx.wait(trash_scr.request())
                                if final_status_code == LITE_CARD_OPERATE_SUCCESS:
                                    from trezor.ui.layouts import show_success

                                    await show_success(
                                        ctx,
                                        _(i18n_keys.TITLE__BACK_UP_COMPLETE),
                                        _(i18n_keys.TITLE__BACKUP_COMPLETED_DESC),
                                        header=_(i18n_keys.TITLE__BACK_UP_COMPLETE),
                                        button=_(i18n_keys.BUTTON__I_GOT_IT),
                                    )
                                    return LITE_CARD_OPERATE_SUCCESS
                                # lite card disconnect
                                elif final_status_code == LITE_CARD_CONNECT_FAILURE:
                                    flag = await show_fullsize_window(
                                        ctx,
                                        _(i18n_keys.TITLE__CONNECT_FAILED),
                                        _(
                                            i18n_keys.CONTENT__MAKE_SURE_THE_CARD_IS_CLOSE_TO_THE_UPPER_LEFT
                                        ),
                                        _(i18n_keys.BUTTON__TRY_AGAIN),
                                        _(i18n_keys.BUTTON__BACK),
                                        icon_path="A:/res/danger.png",
                                    )
                                    if flag == LITE_CARD_BUTTON_CANCLE:
                                        return LITE_CARD_CONNECT_FAILURE
                                    elif flag == LITE_CARD_BUTTON_CONFIRM:
                                        continue

                                elif final_status_code == LITE_CARD_NOT_SAME:
                                    await show_fullsize_window(
                                        ctx,
                                        _(i18n_keys.TITLE__CONNECT_FAILED),
                                        _(
                                            i18n_keys.CONTENT__THE_TWO_ONEKEY_LITE_USED_FOR_CONNECTION_ARE_NOT_THE_SAME
                                        ),
                                        _(i18n_keys.BUTTON__I_GOT_IT),
                                        icon_path="A:/res/danger.png",
                                    )
                                    return LITE_CARD_NOT_SAME

                                elif final_status_code == LITE_CARD_HAS_BEEN_RESET:
                                    await show_fullsize_window(
                                        ctx,
                                        _(i18n_keys.TITLE__LITE_HAS_BEEN_RESET),
                                        _(i18n_keys.TITLE__LITE_HAS_BEEN_RESET_DESC),
                                        _(i18n_keys.BUTTON__I_GOT_IT),
                                        icon_path="A:/res/danger.png",
                                    )
                                    return LITE_CARD_HAS_BEEN_RESET

                                elif str(final_status_code).startswith("63C"):
                                    retry_count = int(str(final_status_code)[-1], 16)
                                    await show_fullsize_window(
                                        ctx,
                                        _(i18n_keys.TITLE__LITE_PIN_ERROR),
                                        _(i18n_keys.TITLE__LITE_PIN_ERROR_DESC).format(
                                            f"#FF0000 {retry_count}#"
                                        ),
                                        _(i18n_keys.BUTTON__I_GOT_IT),
                                        icon_path="A:/res/danger.png",
                                    )
                                    return LITE_CARD_PIN_ERROR

                                else:
                                    break
                            elif status_code == LITE_CARD_BUTTON_CANCLE:
                                place_again_data_card_next = False
                    else:
                        break

    nfc.pwr_ctrl(True)
    first_placement = True
    while first_placement:
        start_flag = await show_start_screen(ctx)
        if start_flag == LITE_CARD_BUTTON_CONFIRM:
            status_code = await search_device(ctx)
            if status_code == LITE_CARD_FIND:
                trash_scr = TransferDataScreen()
                trash_scr.check_card_data()
                carddata = await ctx.wait(trash_scr.request())
                if carddata == LITE_CARD_CONNECT_FAILURE:
                    flag = await show_fullsize_window(
                        ctx,
                        _(i18n_keys.TITLE__CONNECT_FAILED),
                        _(
                            i18n_keys.CONTENT__MAKE_SURE_THE_CARD_IS_CLOSE_TO_THE_UPPER_LEFT
                        ),
                        _(i18n_keys.BUTTON__TRY_AGAIN),
                        _(i18n_keys.BUTTON__BACK),
                        icon_path="A:/res/danger.png",
                    )
                    if flag == LITE_CARD_BUTTON_CANCLE:
                        first_placement = False
                    elif flag == LITE_CARD_BUTTON_CONFIRM:
                        continue

                first_char = carddata[0]
                first_char_num = int(first_char)
                card_num = carddata[1:]
                if first_char_num == LITE_CARD_BLANK:
                    flag = await handle_pin_setup(card_num, mnemonics)
                elif first_char_num == LITE_CARD_WITH_DATA:
                    flag = await handle_existing_data(card_num, mnemonics)
                if flag in [
                    LITE_CARD_CONNECT_FAILURE,
                    LITE_CARD_NOT_SAME,
                    LITE_CARD_HAS_BEEN_RESET,
                    LITE_CARD_PIN_ERROR,
                ]:
                    continue
                elif flag == LITE_CARD_OPERATE_SUCCESS:
                    return LITE_CARD_OPERATE_SUCCESS
            elif status_code == LITE_CARD_BUTTON_CANCLE:
                continue
        elif start_flag == LITE_CARD_BUTTON_CANCLE:
            from trezor.ui.layouts import show_lite_card_exit

            try:
                await show_lite_card_exit(
                    ctx,
                    content=_(i18n_keys.TITLE__EXIT_BACKUP_PROCESS_DESC),
                    header=_(i18n_keys.TITLE__EXIT_BACKUP_PROCESS),
                    subheader=None,
                    button_confirm=_(i18n_keys.BUTTON__EXIT),
                    button_cancel=_(i18n_keys.BUTTON__CANCEL),
                )

                return
            except wire.ActionCancelled:
                continue


async def backup_with_lite_import(ctx: wire.GenericContext):
    async def handle_wallet_recovery(pin):
        trash_scr = TransferDataScreen()
        trash_scr.import_pin_mnemonicmphrase(pin)
        mnemonic_phrase = await ctx.wait(trash_scr.request())
        if mnemonic_phrase:
            return mnemonic_phrase

    nfc.pwr_ctrl(True)
    back_up_page = True

    while back_up_page:
        start_flag = await show_start_screen(ctx)
        if start_flag == LITE_CARD_BUTTON_CONFIRM:
            pin = await input_pin(ctx)
            if pin:
                first_placement = True
                while first_placement:
                    status_code = await search_device(ctx)
                    if status_code == LITE_CARD_FIND:
                        mnemonic_phrase = await handle_wallet_recovery(pin)
                        if mnemonic_phrase == LITE_CARD_CONNECT_FAILURE:
                            flag = await show_fullsize_window(
                                ctx,
                                _(i18n_keys.TITLE__CONNECT_FAILED),
                                _(
                                    i18n_keys.CONTENT__MAKE_SURE_THE_CARD_IS_CLOSE_TO_THE_UPPER_LEFT
                                ),
                                _(i18n_keys.BUTTON__TRY_AGAIN),
                                _(i18n_keys.BUTTON__BACK),
                                icon_path="A:/res/danger.png",
                            )
                            if flag == LITE_CARD_BUTTON_CANCLE:
                                first_placement = False
                            elif flag == LITE_CARD_BUTTON_CONFIRM:
                                continue
                        elif mnemonic_phrase == LITE_CARD_NO_BACKUP:
                            flag = await show_fullsize_window(
                                ctx,
                                _(i18n_keys.TITLE__NO_BACKUP_ON_THIS_CARD),
                                _(i18n_keys.TITLE__NO_BACKUP_ON_THIS_CARD_DESC),
                                _(i18n_keys.BUTTON__TRY_AGAIN),
                                _(i18n_keys.BUTTON__BACK),
                                icon_path="A:/res/danger.png",
                            )
                            if flag == LITE_CARD_BUTTON_CANCLE:
                                first_placement = False
                            elif flag == LITE_CARD_BUTTON_CONFIRM:
                                continue

                        elif mnemonic_phrase == LITE_CARD_HAS_BEEN_RESET:
                            await show_fullsize_window(
                                ctx,
                                _(i18n_keys.TITLE__LITE_HAS_BEEN_RESET),
                                _(i18n_keys.TITLE__LITE_HAS_BEEN_RESET_DESC),
                                _(i18n_keys.BUTTON__I_GOT_IT),
                                icon_path="A:/res/danger.png",
                            )
                            first_placement = False
                        elif mnemonic_phrase == LITE_CARD_UNSUPPORTED_WORD_COUNT:
                            await show_fullsize_window(
                                ctx,
                                _(i18n_keys.TITLE__UNSUPPORTED_RECOVERY_PHRASE),
                                _(i18n_keys.TITLE__UNSUPPORTED_RECOVERY_PHRASE_DESC),
                                _(i18n_keys.BUTTON__I_GOT_IT),
                                icon_path="A:/res/danger.png",
                            )
                            first_placement = False
                        elif str(mnemonic_phrase).startswith("63C"):
                            retry_count = int(str(mnemonic_phrase)[-1], 16)
                            flag = await show_fullsize_window(
                                ctx,
                                _(i18n_keys.TITLE__LITE_PIN_ERROR),
                                _(i18n_keys.TITLE__LITE_PIN_ERROR_DESC).format(
                                    f"#FF0000 {retry_count}#"
                                ),
                                _(i18n_keys.BUTTON__I_GOT_IT),
                                icon_path="A:/res/danger.png",
                            )
                            first_placement = False

                        elif mnemonic_phrase:
                            return mnemonic_phrase
                    elif status_code == LITE_CARD_BUTTON_CANCLE:
                        break

        if start_flag == LITE_CARD_BUTTON_CANCLE:
            return LITE_CARD_BUTTON_CANCLE

    return LITE_CARD_BUTTON_CANCLE
