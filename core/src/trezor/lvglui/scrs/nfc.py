from trezorio import nfc

from trezor import loop
from trezor.crypto import bip39

from ..i18n import gettext as _, keys as i18n_keys
from . import lv
from .common import FullSizeWindow

LITE_CARD_ERROR_REPONSE = -1
LITE_CARD_SUCCESS_REPONSE = 0
LITE_CARD_ERROR = 1
LITE_CARD_FIND = 2
LITE_CARD_CONNECT_FAILURE = 3
LITE_CARD_NOT_SAME = 4
LITE_CARD_HAS_BEEN_RESET = 5
LITE_CARD_PIN_ERROR = 6
LITE_CARD_NO_BACKUP = 8
LITE_CARD_UNSUPPORTED_WORD_COUNT = 9
LITE_CARD_DISCONNECT = 99
LITE_CARD_OPERATE_SUCCESS = 2
LITE_CARD_SUCCESS_STATUS = b"\x90\x00"
LITE_CARD_DISCONECT_STATUS = b"\x99\x99"

CMD_GET_SERIAL_NUMBER = b"\x80\xcb\x80\x00\x05\xdf\xff\x02\x81\x01"
CMD_OLD_APPLET = b"\x00\xa4\x04\x00\x08\xD1\x56\x00\x01\x32\x83\x40\x01"
CMD_NEW_APPLET = (
    b"\x00\xa4\x04\x00\x0E\x6F\x6E\x65\x6B\x65\x79\x2E\x62\x61\x63\x6B\x75\x70\x01"
)
CMD_GET_PIN_RETRY_COUNT = b"\x80\xcb\x80\x00\x05\xdf\xff\x02\x81\x02"
CMD_RESET_CARD = b"\x80\xcb\x80\x00\x05\xdf\xfe\x02\x82\x05"
CMD_GET_PIN_STATUS = b"\x80\xcb\x80\x00\x05\xdf\xff\x02\x81\x05"
CMD_SELECT_PRIMARY_SAFETY = b"\x00\xa4\x04\x00"
CMD_EXPORT_DATA = b"\x80\x4b\x00\x00"
CMD_BACKUP_DATA = b"\x80\x3b\x00\x00"
CMD_SETUP_NEW_PIN = b"\x80\xcb\x80\x00\x0e\xdf\xfe\x0b\x82\x04\x08\x00\x06"
CMD_GET_BACKUP_STATUS = b"\x80\x6A\x00\x00"
CMD_VERIFY_PIN = b"\x80\x20\x00\x00\x07\x06"


async def handle_sw1sw2_connect_error(self):
    self.channel.publish(LITE_CARD_CONNECT_FAILURE)
    await loop.sleep(180)
    self.clean()
    self.destroy()


async def handle_cleanup(self, data):
    self.channel.publish(data)
    await loop.sleep(180)
    self.clean()
    self.destroy()


async def run_card_search(self):
    from trezor import motor

    while self.searching:
        await loop.sleep(1000)
        if nfc.poll_card():
            motor.vibrate(motor.HEAVY)
            self.searching = False
            self.channel.publish(LITE_CARD_FIND)
            self.clean()
            self.destroy()
            return
        else:
            pass


async def get_card_num(self):
    card_num = None
    card_num, sw1sw2 = nfc.send_recv(CMD_GET_SERIAL_NUMBER)
    if sw1sw2 == LITE_CARD_DISCONECT_STATUS:
        await handle_sw1sw2_connect_error(self)
        return card_num, LITE_CARD_ERROR_REPONSE
    elif sw1sw2 == LITE_CARD_SUCCESS_STATUS:
        pass
    else:
        _, sw1sw2 = nfc.send_recv(CMD_SELECT_PRIMARY_SAFETY)
        if sw1sw2 == LITE_CARD_DISCONECT_STATUS:
            await handle_sw1sw2_connect_error(self)
            return card_num, LITE_CARD_ERROR_REPONSE
        card_num, sw1sw2 = nfc.send_recv(CMD_GET_SERIAL_NUMBER)
        if sw1sw2 == LITE_CARD_DISCONECT_STATUS:
            await handle_sw1sw2_connect_error(self)
            return card_num, LITE_CARD_ERROR_REPONSE

    if isinstance(card_num, bytes):
        card_num_str = card_num.decode("utf-8")
    else:
        card_num_str = str(card_num)

    return card_num_str, LITE_CARD_SUCCESS_REPONSE


def get_card_type(card_num_str):
    card_type = None
    if len(card_num_str) >= 5:
        if card_num_str[4] == "T" and card_num_str[5] == "2":
            card_type = "OLD"
        elif card_num_str[4] == "B" and card_num_str[5] == "A":
            card_type = "NEW"
    return card_type


async def check_card_data(self):

    card_num, status = await get_card_num(self)
    if status == LITE_CARD_ERROR_REPONSE:
        return
    pinresp, pinsw1sw2 = nfc.send_recv(CMD_GET_PIN_STATUS)
    if pinsw1sw2 == LITE_CARD_DISCONECT_STATUS:
        await handle_sw1sw2_connect_error(self)
        return
    if pinsw1sw2 == LITE_CARD_SUCCESS_STATUS and pinresp == b"\x02":
        if card_num is not None:
            data = "2" + str(card_num)
            await handle_cleanup(self, data)
            return
    elif pinsw1sw2 == LITE_CARD_SUCCESS_STATUS:
        if card_num is not None:
            data = "3" + str(card_num)
            await handle_cleanup(self, data)
            return
    else:
        await handle_cleanup(self, LITE_CARD_ERROR_REPONSE)
        return


async def check_best_try_restcard(self):
    numresp, numsw1sw2 = nfc.send_recv(CMD_GET_PIN_RETRY_COUNT)
    if numsw1sw2 == LITE_CARD_DISCONECT_STATUS:
        await handle_sw1sw2_connect_error(self)
        return LITE_CARD_ERROR_REPONSE

    if numresp in [b"\x01", b"\x00"] and numsw1sw2 == LITE_CARD_SUCCESS_STATUS:

        _, restsw1sw2 = nfc.send_recv(CMD_RESET_CARD, True)
        if restsw1sw2 == LITE_CARD_DISCONECT_STATUS:
            await handle_sw1sw2_connect_error(self)
            return LITE_CARD_ERROR_REPONSE
        await handle_cleanup(self, LITE_CARD_HAS_BEEN_RESET)
        return LITE_CARD_ERROR_REPONSE
    return LITE_CARD_SUCCESS_REPONSE


async def start_import_pin_mnemonicmphrase(self, pin):

    card_num, status = await get_card_num(self)
    if status == LITE_CARD_ERROR_REPONSE:
        return
    card_type = get_card_type(card_num)

    if card_type == "OLD":
        _, sw1sw2 = nfc.send_recv(CMD_SELECT_PRIMARY_SAFETY)
        if sw1sw2 == LITE_CARD_DISCONECT_STATUS:
            await handle_sw1sw2_connect_error(self)
            return
    elif card_type == "NEW":
        pass
    else:
        return LITE_CARD_CONNECT_FAILURE

    if card_type == "OLD":
        status = await check_best_try_restcard(self)
        if status == LITE_CARD_ERROR_REPONSE:
            return

    if card_type == "NEW":
        resp, sw1sw2 = nfc.send_recv(CMD_NEW_APPLET)
        if sw1sw2 == LITE_CARD_DISCONECT_STATUS:
            await handle_sw1sw2_connect_error(self)
            return

    if card_type == "NEW":
        status = await check_best_try_restcard(self)
        if status == LITE_CARD_ERROR_REPONSE:
            return

    pinresp, pinsw1sw2 = nfc.send_recv(CMD_GET_PIN_STATUS)
    if pinsw1sw2 == LITE_CARD_DISCONECT_STATUS:
        await handle_sw1sw2_connect_error(self)
        return
    if pinsw1sw2 == LITE_CARD_SUCCESS_STATUS and pinresp == b"\x02":
        await handle_cleanup(self, LITE_CARD_NO_BACKUP)
        return
    if card_type == "NEW":
        resp, sw1sw2 = nfc.send_recv(CMD_GET_BACKUP_STATUS)
        state = resp[0]
        if sw1sw2 == LITE_CARD_DISCONECT_STATUS:
            await handle_sw1sw2_connect_error(self)
            return
        elif (state & 0x02) != 0x02:
            await handle_cleanup(self, LITE_CARD_NO_BACKUP)
            return
    if card_type == "OLD":
        resp, sw1sw2 = nfc.send_recv(CMD_OLD_APPLET)
        if sw1sw2 == LITE_CARD_DISCONECT_STATUS:
            await handle_sw1sw2_connect_error(self)
            return

    pin_bytes = "".join(pin).encode()
    command_data = CMD_VERIFY_PIN + pin_bytes
    resp, sw1sw2 = nfc.send_recv(command_data, True)
    if sw1sw2 == LITE_CARD_DISCONECT_STATUS:
        await handle_sw1sw2_connect_error(self)
        return

    if sw1sw2[0] == 0x63 and (sw1sw2[1] & 0xF0) == 0xC0:
        retry_count = sw1sw2[1] & 0x0F
        print(f"PIN verification failed. Remaining retries: {retry_count}")
        pin_status = f"63C{retry_count:X}"
        await handle_cleanup(self, pin_status)
        return

    if card_type == "OLD":
        exportresp, exportsw1sw2 = nfc.send_recv(CMD_EXPORT_DATA)
    else:
        exportresp, exportsw1sw2 = nfc.send_recv(CMD_EXPORT_DATA, True)
    if exportsw1sw2 == LITE_CARD_DISCONECT_STATUS or len(exportresp) < 8:
        await handle_sw1sw2_connect_error(self)
        return
    decoder = MnemonicEncoder()
    encoded_mnemonic_str, _, _ = decoder.parse_card_data(exportresp)
    decoded_mnemonics = decoder.decode_mnemonics(encoded_mnemonic_str)
    word_count = len(decoded_mnemonics.split())
    if word_count in [15, 21]:
        await handle_cleanup(self, LITE_CARD_UNSUPPORTED_WORD_COUNT)
        return
    await handle_cleanup(self, decoded_mnemonics)


async def start_check_pin_mnemonicmphrase(self, pin, mnemonic, card_num):

    card_num_again, status = await get_card_num(self)
    if status == LITE_CARD_ERROR_REPONSE:
        return
    card_type = get_card_type(card_num)

    if card_type == "OLD":
        _, sw1sw2 = nfc.send_recv(CMD_SELECT_PRIMARY_SAFETY)
        if sw1sw2 == LITE_CARD_DISCONECT_STATUS:
            await handle_sw1sw2_connect_error(self)
            return
    elif card_type == "NEW":
        pass
    else:
        return LITE_CARD_CONNECT_FAILURE

    if card_num != card_num_again:
        await handle_cleanup(self, LITE_CARD_NOT_SAME)
        return

    if card_type == "OLD":
        status = await check_best_try_restcard(self)
        if status == LITE_CARD_ERROR_REPONSE:
            return

    if card_type == "OLD":
        _, sw1sw2 = nfc.send_recv(CMD_OLD_APPLET)
    else:
        _, sw1sw2 = nfc.send_recv(CMD_NEW_APPLET)

    if card_type == "NEW":
        status = await check_best_try_restcard(self)
        if status == LITE_CARD_ERROR_REPONSE:
            return

    # verify pin
    pin_bytes = "".join(pin).encode()
    command_data = CMD_VERIFY_PIN + pin_bytes
    _, sw1sw2 = nfc.send_recv(command_data, True)
    if sw1sw2 == LITE_CARD_DISCONECT_STATUS:
        await handle_sw1sw2_connect_error(self)
        return
    if sw1sw2[0] == 0x63 and (sw1sw2[1] & 0xF0) == 0xC0:
        retry_count = sw1sw2[1] & 0x0F
        print(f"PIN verification failed. Remaining retries: {retry_count}")
        pin_status = f"63C{retry_count:X}"
        self.stop_animation()
        await handle_cleanup(self, pin_status)
        return retry_count

    if card_type == "NEW":
        command_data = CMD_SETUP_NEW_PIN + pin_bytes
        _, sw1sw2 = nfc.send_recv(command_data, True)
        if sw1sw2 == LITE_CARD_DISCONECT_STATUS:
            await handle_sw1sw2_connect_error(self)
            return

    # verify pin
    pin_bytes = "".join(pin).encode()
    command_data = CMD_VERIFY_PIN + pin_bytes
    _, sw1sw2 = nfc.send_recv(command_data, True)

    if sw1sw2 == LITE_CARD_DISCONECT_STATUS:
        await handle_sw1sw2_connect_error(self)
        return

    encoder = MnemonicEncoder()
    encoded_mnemonic_str = encoder.encode_mnemonics(mnemonic)
    version = "01"
    lang = "00"
    meta = "ffff" + version + lang
    payload = encoded_mnemonic_str + meta
    payload_bytes = bytes(
        int(payload[i : i + 2], 16) for i in range(0, len(payload), 2)
    )

    seed_length = len(payload_bytes)
    lc = seed_length.to_bytes(1, "big")
    apdu_command = CMD_BACKUP_DATA + lc + payload_bytes
    if card_type == "OLD":
        _, importsw1sw2 = nfc.send_recv(apdu_command)
    else:
        _, importsw1sw2 = nfc.send_recv(apdu_command, True)
    if importsw1sw2 == LITE_CARD_DISCONECT_STATUS:
        await handle_sw1sw2_connect_error(self)
        return
    if importsw1sw2 == LITE_CARD_SUCCESS_STATUS:
        await handle_cleanup(self, LITE_CARD_OPERATE_SUCCESS)
        return
    self.channel.publish(LITE_CARD_CONNECT_FAILURE)
    await loop.sleep(180)
    self.clean()
    self.destroy()
    return


async def start_set_pin_mnemonicmphrase(self, pin, mnemonic, card_num):

    card_num_again, status = await get_card_num(self)
    if status == LITE_CARD_ERROR_REPONSE:
        return
    card_type = get_card_type(card_num_again)
    if card_type == "OLD":
        _, restsw1sw2 = nfc.send_recv(CMD_RESET_CARD, True)
        if restsw1sw2 == LITE_CARD_DISCONECT_STATUS:
            await handle_sw1sw2_connect_error(self)
            return
    if card_num != card_num_again:
        await handle_cleanup(self, LITE_CARD_NOT_SAME)
        return

    # reset card
    if card_type == "OLD":
        _, sw1sw2 = nfc.send_recv(CMD_RESET_CARD, True)
        if sw1sw2 == LITE_CARD_DISCONECT_STATUS:
            await handle_sw1sw2_connect_error(self)
            return
    # select app
    if card_type == "NEW":
        _, sw1sw2 = nfc.send_recv(CMD_NEW_APPLET)
        if sw1sw2 == LITE_CARD_DISCONECT_STATUS:
            await handle_sw1sw2_connect_error(self)
            return
    # set pin
    pin_bytes = "".join(pin).encode()
    command_data = CMD_SETUP_NEW_PIN + pin_bytes

    _, sw1sw2 = nfc.send_recv(command_data, True)
    if sw1sw2 == LITE_CARD_DISCONECT_STATUS:
        await handle_sw1sw2_connect_error(self)
        return

    if card_type == "OLD":
        _, sw1sw2 = nfc.send_recv(CMD_OLD_APPLET)

    # verify pin
    pin_bytes = "".join(pin).encode()
    command_data = CMD_VERIFY_PIN + pin_bytes

    restsw1sw2, sw1sw2 = nfc.send_recv(command_data, True)

    encoder = MnemonicEncoder()
    encoded_mnemonic_str = encoder.encode_mnemonics(mnemonic)
    version = "01"
    lang = "00"
    meta = "ffff" + version + lang
    payload = encoded_mnemonic_str + meta
    payload_bytes = bytes(
        int(payload[i : i + 2], 16) for i in range(0, len(payload), 2)
    )
    seed_length = len(payload_bytes)
    lc = seed_length.to_bytes(1, "big")
    apdu_command = CMD_BACKUP_DATA + lc + payload_bytes
    if card_type == "OLD":
        _, importsw1sw2 = nfc.send_recv(apdu_command)
    else:
        _, importsw1sw2 = nfc.send_recv(apdu_command, True)

    if importsw1sw2 == LITE_CARD_DISCONECT_STATUS:
        self.stop_animation()
        await handle_sw1sw2_connect_error(self)
        return
    if importsw1sw2 == LITE_CARD_SUCCESS_STATUS:
        await handle_cleanup(self, LITE_CARD_OPERATE_SUCCESS)
        return
    await handle_cleanup(self, LITE_CARD_CONNECT_FAILURE)
    return


class MnemonicEncoder:
    def encode_mnemonics(self, seed):
        n = 2048
        words = seed.split()
        i = 0
        while words:
            w = words.pop()
            k = bip39.find(w)
            i = i * n + k
        result_str = str(i)
        if len(result_str) % 2 != 0:
            result_str = "0" + result_str
        return result_str

    def int_to_hex_str(self, num):
        """Convert an integer to a hexadecimal string."""
        hex_str = hex(num)[2:]  # Convert to hex and remove the '0x' prefix
        if len(hex_str) % 2:  # Make sure the length is even
            hex_str = "0" + hex_str
        return hex_str

    def fromhex(self, hex_str):
        """Convert a hex string to a byte array."""
        return bytes(int(hex_str[i : i + 2], 16) for i in range(0, len(hex_str), 2))

    def bytes_to_hex_str(self, byte_data):
        return "".join(f"{byte:02x}" for byte in byte_data)

    def decode_mnemonics(self, encoded_mnemonic_str):
        n = 2048
        encoded_int = int(encoded_mnemonic_str, 10)
        words = []

        while encoded_int > 0:
            index = int(encoded_int % n)
            encoded_int = encoded_int // n
            words.append(bip39.get_word(index))
        # v1 fix
        fix_fill_count = 0
        supported_mnemonic_length = [12, 15, 18, 21, 24]
        for length in supported_mnemonic_length:
            if len(words) == length:
                break
            if len(words) < length:
                fix_fill_count = length - len(words)
                break
        words.extend([bip39.get_word(0)] * fix_fill_count)

        return " ".join(words)

    def parse_card_data(self, data):

        encoded_mnemonic_bytes = data[:-4]
        version_bytes = data[-4:-3]
        lang_bytes = data[-3:-2]
        encoded_mnemonic_str = self.bytes_to_hex_str(encoded_mnemonic_bytes)
        version = self.bytes_to_hex_str(version_bytes)
        lang = self.bytes_to_hex_str(lang_bytes)

        return encoded_mnemonic_str, version, lang


class SearchDeviceScreen(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__CONNECTING),
            _(i18n_keys.CONTENT__KEEP_LITE_DEVICE_TOGETHER_BACKUP_COMPLETE),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            anim_dir=0,
        )
        self.img_bg = lv.img(self.content_area)
        self.img_bg.set_src("A:/res/nfc-bg.png")
        self.img_bg.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 125)
        self.img_searching = lv.img(self.content_area)
        self.img_searching.set_src("A:/res/nfc-icon-searching.png")
        self.img_searching.align_to(self.img_bg, lv.ALIGN.CENTER, 0, 0)
        self.anim = lv.anim_t()
        self.anim.init()
        self.anim.set_var(self.img_bg)
        self.anim.set_values(0, 3600)
        self.anim.set_time(1000)
        self.anim.set_repeat_count(0xFFFF)  # infinite
        self.anim.set_path_cb(lv.anim_t.path_linear)
        self.anim.set_custom_exec_cb(lambda _a, val: self.set_angle(val))
        self.anim_r = lv.anim_t.start(self.anim)
        self.searching = True
        loop.schedule(run_card_search(self))

    def set_angle(self, angle):
        try:
            self.img_bg.set_angle(angle)
        except Exception:
            lv.anim_del(self.anim_r.var, None)


class TransferDataScreen(FullSizeWindow):
    def __init__(self):
        super().__init__(
            _(i18n_keys.TITLE__TRANSFERRING),
            _(i18n_keys.TITLE__TRANSFERRING_DESC),
            cancel_text=_(i18n_keys.BUTTON__CANCEL),
            anim_dir=0,
        )
        self.img_searching = lv.img(self.content_area)
        self.img_searching.set_src("A:/res/nfc-icon-transfering.png")
        self.img_searching.align_to(self.subtitle, lv.ALIGN.OUT_BOTTOM_MID, 0, 237)
        self.searching = True

    def set_angle(self, angle):
        try:
            self.img_bg.set_angle(angle)
        except Exception:
            pass

    def check_card_data(self):
        loop.schedule(check_card_data(self))

    def stop_animation(self):
        self.searching = False

    def set_pin_mnemonicmphrase(self, pin, card_num, mnemonics):

        loop.schedule(start_set_pin_mnemonicmphrase(self, pin, mnemonics, card_num))

    def check_pin_mnemonicmphrase(self, pin, card_num, mnemonics):

        loop.schedule(start_check_pin_mnemonicmphrase(self, pin, mnemonics, card_num))

    def import_pin_mnemonicmphrase(self, pin):
        loop.schedule(start_import_pin_mnemonicmphrase(self, pin))
