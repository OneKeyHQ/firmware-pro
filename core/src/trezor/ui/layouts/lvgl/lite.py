from trezorio import nfc
from typing import TYPE_CHECKING

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
    SearchDeviceScreen,
    TransferDataScreen,
)
from trezor.lvglui.scrs.pinscreen import InputLitePin, request_lite_pin_confirm


async def backup_with_lite(
    ctx: wire.GenericContext, mnemonics: bytes, recovery_check: bool = False
):
    async def show_fullsize_window(
        title, content, confirm_text, cancel_text=None, icon_path=None
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
        return await ctx.wait(screen.request())

    async def show_start_screen():
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

    async def search_device():
        search_scr = SearchDeviceScreen()
        return await ctx.wait(search_scr.request())

    async def handle_pin_setup(card_num, mnemonics):
        pin = await request_lite_pin_confirm(ctx)
        if pin:
            await handle_second_placement(card_num, pin, mnemonics)
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
            if again_flag == 1:
                while True:
                    status_code = await search_device()
                    if status_code == LITE_CARD_FIND:
                        trash_scr = TransferDataScreen()
                        trash_scr.set_pin_mnemonicmphrase(pin, card_num, mnemonics)
                        final_status_code = await ctx.wait(trash_scr.request())
                        if final_status_code == LITE_CARD_OPERATE_SUCCESS:
                            await show_fullsize_window(
                                _(i18n_keys.TITLE__BACK_UP_COMPLETE),
                                _(i18n_keys.TITLE__BACKUP_COMPLETED_DESC),
                                _(i18n_keys.BUTTON__I_GOT_IT),
                                icon_path="A:/res/success.png",
                            )
                            return LITE_CARD_OPERATE_SUCCESS
                        elif final_status_code == LITE_CARD_NOT_SAME:
                            await show_fullsize_window(
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
                                _(i18n_keys.TITLE__CONNECT_FAILED),
                                _(
                                    i18n_keys.CONTENT__MAKE_SURE_THE_CARD_IS_CLOSE_TO_THE_UPPER_LEFT
                                ),
                                _(i18n_keys.BUTTON__TRY_AGAIN),
                                _(i18n_keys.BUTTON__BACK),
                                icon_path="A:/res/danger.png",
                            )
                            if flag == 0:
                                return LITE_CARD_CONNECT_FAILURE
                            elif flag == 1:
                                continue

                    else:
                        break
            else:
                break

    async def handle_existing_data(card_num, mnemonics):
        from trezor.lvglui.scrs.wipe_device import WipeDeviceTipsTmp

        confirm_screen = WipeDeviceTipsTmp()
        if await ctx.wait(confirm_screen.request()):
            pin = await ctx.wait(InputLitePin().request())
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
                    if again_flag == 1:
                        while True:
                            status_code = await search_device()
                            if status_code == LITE_CARD_FIND:
                                trash_scr = TransferDataScreen()
                                trash_scr.check_pin_mnemonicmphrase(
                                    pin, card_num, mnemonics
                                )
                                final_status_code = await ctx.wait(trash_scr.request())
                                if final_status_code == LITE_CARD_OPERATE_SUCCESS:
                                    await show_fullsize_window(
                                        _(i18n_keys.TITLE__BACK_UP_COMPLETE),
                                        _(i18n_keys.TITLE__BACKUP_COMPLETED_DESC),
                                        _(i18n_keys.BUTTON__I_GOT_IT),
                                        icon_path="A:/res/success.png",
                                    )
                                    return LITE_CARD_OPERATE_SUCCESS

                                elif final_status_code == LITE_CARD_CONNECT_FAILURE:
                                    flag = await show_fullsize_window(
                                        _(i18n_keys.TITLE__CONNECT_FAILED),
                                        _(
                                            i18n_keys.CONTENT__MAKE_SURE_THE_CARD_IS_CLOSE_TO_THE_UPPER_LEFT
                                        ),
                                        _(i18n_keys.BUTTON__TRY_AGAIN),
                                        _(i18n_keys.BUTTON__BACK),
                                        icon_path="A:/res/danger.png",
                                    )
                                    if flag == 0:
                                        return LITE_CARD_CONNECT_FAILURE
                                    elif flag == 1:
                                        continue

                                elif final_status_code == LITE_CARD_NOT_SAME:
                                    await show_fullsize_window(
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
                                        _(i18n_keys.TITLE__LITE_HAS_BEEN_RESET),
                                        _(i18n_keys.TITLE__LITE_HAS_BEEN_RESET_DESC),
                                        _(i18n_keys.BUTTON__I_GOT_IT),
                                        icon_path="A:/res/danger.png",
                                    )
                                    return LITE_CARD_HAS_BEEN_RESET

                                elif str(final_status_code).startswith("63C"):
                                    retry_count = int(str(final_status_code)[-1], 16)
                                    await show_fullsize_window(
                                        _(i18n_keys.TITLE__LITE_PIN_ERROR),
                                        _(i18n_keys.TITLE__LITE_PIN_ERROR_DESC).format(
                                            retry_count
                                        ),
                                        _(i18n_keys.BUTTON__I_GOT_IT),
                                        icon_path="A:/res/danger.png",
                                    )
                                    return LITE_CARD_PIN_ERROR

                                else:
                                    break
                    else:
                        break

    nfc.pwr_ctrl(True)
    first_placement = True
    while first_placement:
        start_flag = await show_start_screen()
        if start_flag == 1:
            status_code = await search_device()
            if status_code == 2:
                trash_scr = TransferDataScreen()
                trash_scr.check_card_data()
                carddata = await ctx.wait(trash_scr.request())
                if carddata == 3:
                    flag = await show_fullsize_window(
                        _(i18n_keys.TITLE__CONNECT_FAILED),
                        _(
                            i18n_keys.CONTENT__MAKE_SURE_THE_CARD_IS_CLOSE_TO_THE_UPPER_LEFT
                        ),
                        _(i18n_keys.BUTTON__TRY_AGAIN),
                        _(i18n_keys.BUTTON__BACK),
                        icon_path="A:/res/danger.png",
                    )
                    if flag == 0:
                        first_placement = False
                    elif flag == 1:
                        continue

                first_char = carddata[0]
                first_char_num = int(first_char)
                card_num = carddata[1:]
                if first_char_num == 2:
                    flag = await handle_pin_setup(card_num, mnemonics)

                elif first_char_num == 3:
                    flag = await handle_existing_data(card_num, mnemonics)
                    if flag in [3, 4, 5, 6]:
                        continue
                    elif flag == 2:
                        return 2
        elif start_flag == 0:
            back_up_page_flag = await show_fullsize_window(
                _(i18n_keys.TITLE__EXIT_BACKUP_PROCESS),
                _(i18n_keys.TITLE__EXIT_BACKUP_PROCESS_DESC),
                _(i18n_keys.BUTTON__EXIT),
                _(i18n_keys.BUTTON__CANCEL),
            )
            if back_up_page_flag == 1:
                return
            elif back_up_page_flag == 0:
                continue


async def backup_with_lite_import(ctx: wire.GenericContext):

    from trezor.lvglui.scrs.common import FullSizeWindow, lv
    from trezor.lvglui.scrs.pinscreen import request_lite_pin_confirm
    from trezor.lvglui.scrs.pinscreen import InputLitePin
    from trezor.lvglui.scrs.nfc import SearchDeviceScreen, TransferDataScreen

    async def show_fullsize_window(
        title, content, confirm_text, cancel_text=None, icon_path=None
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
        return await ctx.wait(screen.request())

    async def show_start_screen():

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

    async def search_for_device():
        search_scr = SearchDeviceScreen()
        return await ctx.wait(search_scr.request())

    async def handle_wallet_recovery(pin):
        trash_scr = TransferDataScreen()
        trash_scr.import_pin_mnemonicmphrase(pin)
        mnemonic_phrase = await ctx.wait(trash_scr.request())
        if mnemonic_phrase:
            return mnemonic_phrase

    async def input_pin():
        return await ctx.wait(InputLitePin().request())

    nfc.pwr_ctrl(True)
    back_up_page = True

    while back_up_page:
        start_flag = await show_start_screen()
        if start_flag == 1:
            pin = await input_pin()
            if pin:
                first_placement = True
                while first_placement:
                    status_code = await search_for_device()
                    if status_code == LITE_CARD_FIND:
                        mnemonic_phrase = await handle_wallet_recovery(pin)
                        if mnemonic_phrase == LITE_CARD_CONNECT_FAILURE:
                            flag = await show_fullsize_window(
                                _(i18n_keys.TITLE__CONNECT_FAILED),
                                _(
                                    i18n_keys.CONTENT__MAKE_SURE_THE_CARD_IS_CLOSE_TO_THE_UPPER_LEFT
                                ),
                                _(i18n_keys.BUTTON__TRY_AGAIN),
                                _(i18n_keys.BUTTON__BACK),
                                icon_path="A:/res/danger.png",
                            )
                            if flag == 0:
                                first_placement = False
                            elif flag == 1:
                                continue
                        elif mnemonic_phrase == LITE_CARD_NO_BACKUP:
                            flag = await show_fullsize_window(
                                _(i18n_keys.TITLE__NO_BACKUP_ON_THIS_CARD),
                                _(i18n_keys.TITLE__NO_BACKUP_ON_THIS_CARD_DESC),
                                _(i18n_keys.BUTTON__TRY_AGAIN),
                                _(i18n_keys.BUTTON__BACK),
                                icon_path="A:/res/danger.png",
                            )
                            if flag == 0:
                                first_placement = False
                            elif flag == 1:
                                continue

                        elif mnemonic_phrase == LITE_CARD_HAS_BEEN_RESET:
                            await show_fullsize_window(
                                _(i18n_keys.TITLE__LITE_HAS_BEEN_RESET),
                                _(i18n_keys.TITLE__LITE_HAS_BEEN_RESET_DESC),
                                _(i18n_keys.BUTTON__I_GOT_IT),
                                icon_path="A:/res/danger.png",
                            )
                            return LITE_CARD_HAS_BEEN_RESET

                        elif str(mnemonic_phrase).startswith("63C"):
                            retry_count = int(str(mnemonic_phrase)[-1], 16)
                            flag = await show_fullsize_window(
                                _(i18n_keys.TITLE__LITE_PIN_ERROR),
                                _(i18n_keys.TITLE__LITE_PIN_ERROR_DESC).format(
                                    retry_count
                                ),
                                _(i18n_keys.BUTTON__I_GOT_IT),
                                icon_path="A:/res/danger.png",
                            )
                            first_placement = False

                        elif mnemonic_phrase:
                            return mnemonic_phrase
                    elif status_code == 0:
                        break

        if start_flag == 0:
            return 0

    return 0
