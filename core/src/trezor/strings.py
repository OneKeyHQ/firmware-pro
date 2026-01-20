import utime
from micropython import const

_SECONDS_1970_TO_2000 = const(946684800)


def format_amount(amount: int, decimals: int) -> str:
    if amount < 0:
        amount = -amount
        sign = "-"
    else:
        sign = ""
    d = 10**decimals
    integer = amount // d
    decimal = amount % d

    # TODO: bug in mpz: https://github.com/micropython/micropython/issues/8984
    grouped_integer = f"{integer:,}".lstrip(",")

    s = f"{sign}{grouped_integer}.{decimal:0{decimals}}".rstrip("0").rstrip(".")
    return s


def strip_amount(amount_str: str) -> tuple[str, bool]:
    amount_value, *suffix = amount_str.split(" ")
    try:
        amount_value.index(".")
    except ValueError:
        return amount_str, False
    else:
        amount_value_i, amount_value_d = amount_value.split(".")
        amount_value_d_short = amount_value_d[:6]
        suffix = " ".join(suffix)
        return (
            f"{amount_value_i}.{amount_value_d_short}".rstrip("0").rstrip(".")
            + f" {suffix}",
            len(amount_value_d) > 6,
        )


def format_ordinal(number: int) -> str:
    return str(number) + {1: "st", 2: "nd", 3: "rd"}.get(
        4 if 10 <= number % 100 < 20 else number % 10, "th"
    )


def format_plural(string: str, count: int, plural: str) -> str:
    """
    Adds plural form to a string based on `count`.
    !! Does not work with irregular words !!

    Example:
    >>> format_plural("We need {count} more {plural}", 3, "share")
    'We need 3 more shares'
    >>> format_plural("We need {count} more {plural}", 1, "share")
    'We need 1 more share'
    >>> format_plural("{count} {plural}", 4, "candy")
    '4 candies'
    """
    if not all(s in string for s in ("{count}", "{plural}")):
        # string needs to have {count} and {plural} inside
        raise ValueError

    if count == 0 or count > 1:
        # candy -> candies, but key -> keys
        if plural[-1] == "y" and plural[-2] not in "aeiouy":
            plural = plural[:-1] + "ies"
        elif plural[-1] in "hsxz":
            plural = plural + "es"
        else:
            plural = plural + "s"

    return string.format(count=count, plural=plural)


def format_duration_ms(milliseconds: int, maximum_ms: int | None) -> str:
    """
    Returns human-friendly representation of a duration. Truncates all decimals.
    """
    from trezor.lvglui.i18n import gettext as _, keys as i18n_keys

    if maximum_ms and milliseconds == maximum_ms:
        return _(i18n_keys.OPTION__NEVER)
    if milliseconds < 60000:
        seconds = milliseconds // 1000
        formated = _(i18n_keys.OPTION__STR_SECONDS).format(seconds)
    elif milliseconds < 3600000:
        minutes = milliseconds // 60000
        formated = _(
            i18n_keys.OPTION__STR_MINUTE
            if minutes == 1
            else i18n_keys.OPTION__STR_MINUTES
        ).format(minutes)
    else:
        hours = milliseconds // 3600000
        formated = _(
            i18n_keys.OPTION__STR_HOUR if hours == 1 else i18n_keys.OPTION__STR_HOURS
        ).format(hours)
    return formated


def format_timestamp(timestamp: int) -> str:
    """
    Returns human-friendly representation of a unix timestamp (in seconds format).
    Minutes and seconds are always displayed as 2 digits.
    Example:
    >>> format_timestamp_to_human(0)
    '1970-01-01 00:00:00'
    >>> format_timestamp_to_human(1616051824)
    '2021-03-18 07:17:04'
    """
    # By doing the conversion to 2000-based epoch in Python, we take advantage of the
    # bignum implementation, and get another 30 years out of the 32-bit mp_int_t
    # that is used internally.
    d = utime.gmtime2000(timestamp - _SECONDS_1970_TO_2000)
    return f"{d[0]}-{d[1]:02d}-{d[2]:02d} {d[3]:02d}:{d[4]:02d}:{d[5]:02d}"


def format_customer_data(data: bytes | None) -> str:
    """
    Returns human-friendly representation of a customer data.
    """
    if data is None or len(data) == 0:
        return ""
    try:
        formatted = data.decode()
        if all((c in (0x20, 0x0A, 0x0D)) for c in formatted[:10]):
            raise UnicodeError  # whitespace only
        elif any((ord(c) < 0x20 or ord(c) == 0x7F) for c in formatted[:10]):
            raise UnicodeError  # contains non-printable characters
    except UnicodeError:  # mp has no UnicodeDecodeError
        from binascii import hexlify

        formatted = f"0x{hexlify(data).decode()}"
    return formatted
