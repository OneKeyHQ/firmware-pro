def ensure_fido_seed(func):
    def wrapper(*args, **kwargs):
        from trezor.crypto import se_thd89
        from utime import sleep_ms

        while True:
            try:
                ret = se_thd89.fido_seed()
                if ret:
                    break
                else:
                    sleep_ms(100)
                    continue
            except Exception:
                raise Exception("Failed to generate seed.")
        return func(*args, **kwargs)

    return wrapper
