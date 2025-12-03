from typing import *


# extmod/modtrezorcrypto/modtrezorcrypto-random.h
def uniform(n: int) -> int:
    """
    Compute uniform random number from interval 0 ... n - 1.
    """
import builtins


# extmod/modtrezorcrypto/modtrezorcrypto-random.h
def bytes(len: int, source: int = 1) -> builtins.bytes:
    """
    Generate random bytes sequence of length len.
    source: 0 = use random_buffer, 1 = use se_random_encrypted (default)
    """


# extmod/modtrezorcrypto/modtrezorcrypto-random.h
def shuffle(data: list) -> None:
    """
    Shuffles items of given list (in-place).
    """


# extmod/modtrezorcrypto/modtrezorcrypto-random.h
def reseed(value: int) -> None:
    """
    Re-seed the RNG with given value.
    """
