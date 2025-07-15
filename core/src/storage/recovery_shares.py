from trezor.crypto import slip39

# Mnemonics stored during SLIP-39 recovery process.
# Each mnemonic is stored under key = index.

mnemonics = {}


def set(index: int, group_index: int, mnemonic: str) -> None:
    # common.set(
    #     common.APP_RECOVERY_SHARES,
    #     index + group_index * slip39.MAX_SHARE_COUNT,
    #     mnemonic.encode(),
    # )
    mnemonics[index + group_index * slip39.MAX_SHARE_COUNT] = mnemonic


def get(index: int, group_index: int) -> str | None:
    # if utils.USE_THD89:
    #     return None
    # m = common.get(
    #     common.APP_RECOVERY_SHARES, index + group_index * slip39.MAX_SHARE_COUNT
    # )
    # if m:
    #     return m.decode()
    # return None
    return mnemonics.get(index + group_index * slip39.MAX_SHARE_COUNT, None)


def fetch_group(group_index: int) -> list[str]:
    # if utils.USE_THD89:
    #     return []
    mnemonics = []
    for index in range(slip39.MAX_SHARE_COUNT):
        m = get(index, group_index)
        if m:
            mnemonics.append(m)

    return mnemonics


def delete() -> None:
    # if utils.USE_THD89:
    #     return None
    # for index in range(slip39.MAX_SHARE_COUNT * slip39.MAX_GROUP_COUNT):
    #     common.delete(common.APP_RECOVERY_SHARES, index)
    mnemonics.clear()
    return None
