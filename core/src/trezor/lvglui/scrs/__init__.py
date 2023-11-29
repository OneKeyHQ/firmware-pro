from trezor import utils

import lvgl as lv  # type: ignore[Import "lvgl" could not be resolved]

from ..lv_colors import lv_colors  # noqa: F401
from ..lv_symbols import LV_SYMBOLS  # noqa: F401

if utils.EMULATOR:
    font_GeistSemiBold64 = lv.font_load("A:/res/Geist-SemiBold-64-emu.bin")
    font_GeistSemiBold48 = lv.font_load(
        "A:/res/Geist-SemiBold-48-emu.bin"
    )  # only used for number keyboard
    font_GeistSemiBold38 = lv.font_load("A:/res/Geist-SemiBold-38-emu.bin")
    font_GeistSemiBold26 = lv.font_load("A:/res/Geist-SemiBold-26-emu.bin")
    font_GeistSemiBold30 = lv.font_load("A:/res/Geist-SemiBold-30-emu.bin")
    font_GeistRegular30 = lv.font_load("A:/res/Geist-Regular-30-emu.bin")
    font_GeistRegular26 = lv.font_load("A:/res/Geist-Regular-26-emu.bin")
    font_GeistRegular20 = lv.font_load("A:/res/Geist-Regular-20-emu.bin")
    font_GeistMono28 = lv.font_load("A:/res/GeistMono-Regular-28-emu.bin")

else:
    font_GeistSemiBold64 = lv.font_geist_semibold_64
    font_GeistSemiBold48 = lv.font_geist_semibold_48
    font_GeistSemiBold38 = lv.font_geist_semibold_38
    font_GeistSemiBold26 = lv.font_geist_semibold_26
    font_GeistSemiBold30 = lv.font_geist_semibold_30
    font_GeistRegular30 = lv.font_geist_regular_30
    font_GeistRegular26 = lv.font_geist_regular_26
    font_GeistRegular20 = lv.font_geist_regular_20
    font_GeistMono28 = lv.font_geist_mono_28
