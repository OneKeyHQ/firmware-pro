CAHIN_ID = '{{"chain_id":{chain_id}}}'
OKX_DATA = '{{"okx":{data}}}'


def gen_extra_data(coin_type):
    chain_id = coin_type
    if coin_type == 60:
        chain_id = 1

    chain = CAHIN_ID.format(chain_id=chain_id)
    okx = OKX_DATA.format(data=chain)
    return okx


def get_coin_name(coin_type):
    if coin_type == 0:
        return "BTC"
    if coin_type == 60:
        return "ETH"
    return None
