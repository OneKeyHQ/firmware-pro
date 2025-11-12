from typing import TYPE_CHECKING

from trezor import loop, wire
from trezor.crypto import random
from trezor.enums import ButtonRequestType
from trezor.lvglui.i18n import gettext as _, keys as i18n_keys
from trezor.lvglui.scrs.reset_device import CheckWord

from .common import interact, raise_if_cancelled

if TYPE_CHECKING:
    from typing import Sequence

    NumberedWords = Sequence[tuple[int, str]]

if __debug__:
    pass


async def show_share_words(
    ctx: wire.GenericContext,
    share_words: Sequence[str],
    share_index: int | None = None,
    share_count: int | None = None,
    group_index: int | None = None,
) -> None:

    word_cnt = len(share_words)
    header_title = _(i18n_keys.TITLE__RECOVERY_PHRASE)
    if share_index is None:
        subtitle = _(i18n_keys.SUBTITLE__DEVICE_BACKUP_MANUAL_BACKUP).format(word_cnt)

    elif group_index is None:
        # header_title = f"Recovery share #{share_index + 1}"
        assert share_count is not None
        subtitle = f"{_(i18n_keys.SUBTITLE__DEVICE_BACKUP_MANUAL_BACKUP).format(word_cnt)}\n#00FF33 {_(i18n_keys.SUBTITLE__THIS_IS_SHARE_STR_OF_STR).format(num1=share_index + 1, num2=share_count)}#"
    else:
        # header_title = f"Group {group_index + 1} - Share {share_index + 1}"
        subtitle = f"Write down the following {word_cnt} words in order for Group {group_index + 1} - Share {share_index + 1} of {share_count}."

    if __debug__:
        from apps import debug

        def export_displayed_words() -> None:
            # export currently displayed mnemonic words into debuglink
            debug.reset_current_words.publish(share_words)

        export_displayed_words()

    # shares_words_check = []  # check we display correct data
    from trezor.lvglui.scrs.reset_device import MnemonicDisplay

    screen = MnemonicDisplay(
        header_title,
        subtitle,
        share_words,
    )
    # make sure we display correct data
    # utils.ensure(share_words == shares_words_check)

    await raise_if_cancelled(
        interact(
            ctx,
            screen,
            "backup_words",
            ButtonRequestType.ResetDevice,
        )
    )


async def confirm_word(
    ctx: wire.GenericContext,
    share_index: int | None,
    share_words: Sequence[str],
    offset: int,
    count: int,
    group_index: int | None = None,
) -> bool:
    # remove duplicates
    non_duplicates = list(set(share_words))
    # # remove current checked words
    # non_duplicates.remove(share_words[offset])
    # shuffle list
    random.shuffle(non_duplicates)
    checked_word = non_duplicates[0]
    # take 3 words as choices
    choices = [non_duplicates[-1], non_duplicates[1], checked_word]
    # shuffle again so the confirmed word is not always the first choice
    random.shuffle(choices)

    if __debug__:
        from apps import debug

        debug.reset_word_index.publish(offset)

    # let the user pick a word
    checked_index = share_words.index(checked_word) + offset
    title = _(i18n_keys.TITLE__WORD_STR).format(checked_index + 1)
    options = f"{choices[0]}\n{choices[1]}\n{choices[2]}"
    selector = CheckWord(title, options=options)
    selected_word: str = await ctx.wait(selector.request())
    # confirm it is the correct one
    is_correct = selected_word == checked_word
    await loop.sleep(240)
    if is_correct:
        selector.tip_correct()
    else:
        selector.tip_incorrect()

    await loop.sleep(240)

    return is_correct


# def _split_share_into_pages(
#     share_words: Sequence[str],
# ) -> tuple[NumberedWords, list[NumberedWords], NumberedWords]:
#     share = list(enumerate(share_words))  # we need to keep track of the word indices
#     first = share[:2]  # two words on the first page
#     length = len(share_words)
#     if length in (12, 20, 24):
#         middle = share[2:-2]
#         last = share[-2:]  # two words on the last page
#     elif length in (18, 33):
#         middle = share[2:]
#         last = []  # no words at the last page, because it does not add up
#     else:
#         # Invalid number of shares. SLIP-39 allows 20 or 33 words, BIP-39 12 or 24
#         raise RuntimeError

#     chunks = utils.chunks(middle, 4)  # 4 words on the middle pages
#     return first, list(chunks), last
