def ensure_fido_seed_ready() -> None:
    from trezor.crypto import se_thd89
    from utime import sleep_ms

    while True:
        try:
            ret = se_thd89.fido_seed()
            if ret:
                return
            sleep_ms(100)
        except Exception:
            raise Exception("Failed to generate seed.")


def ensure_fido_seed(func):
    def wrapper(*args, **kwargs):
        ensure_fido_seed_ready()
        return func(*args, **kwargs)

    return wrapper
