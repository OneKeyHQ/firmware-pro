QR_ACCOUNT_INDEX = 0


def get_derivation_path() -> list[int]:
    # Stardard path: "m/44'/44'/60'/0/X"
    path = [2147483692, 2147483708, 2147483648, 0]
    path.append(QR_ACCOUNT_INDEX)
    return path


def get_account_index() -> int:
    return QR_ACCOUNT_INDEX


def set_account_index(index: int) -> None:
    global QR_ACCOUNT_INDEX
    QR_ACCOUNT_INDEX = index
