from typing import *


# extmod/modtrezorcrypto/modtrezorcrypto-slip39.h
def complete_word(prefix: str) -> str | None:
    """
    Return the first word from the wordlist starting with prefix.
    """


# extmod/modtrezorcrypto/modtrezorcrypto-slip39.h
def word_completion_mask(prefix: str) -> int:
    """
    Return possible 1-letter suffixes for given word prefix.
    Result is a bitmask, with 'a' on the lowest bit, 'b' on the second
    lowest, etc.
    """


# extmod/modtrezorcrypto/modtrezorcrypto-slip39.h
def button_sequence_to_word(prefix: int) -> str:
    """
    Finds the first word that fits the given button prefix.
    """


# extmod/modtrezorcrypto/modtrezorcrypto-slip39.h
def word_index(word: str) -> int:
    """
    Finds index of given word.
    Raises ValueError if not found.
    """


# extmod/modtrezorcrypto/modtrezorcrypto-slip39.h
def get_word(index: int) -> str:
    """
    Returns word on position 'index'.
    """
