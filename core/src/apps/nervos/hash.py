from trezor.crypto import  hashlib




def bytes_to_hex_str(bytes_obj):
    return ''.join('{:02x}'.format(byte) for byte in bytes_obj)


def ckb_hasher():
     return hashlib.blake2b(outlen=32, personal=b"ckb-default-hash")



def ckb_hash(message: bytes) -> str:
    hasher = ckb_hasher()
    print("hash:"+str(hasher))
    hasher.update(message)
    hash_bytes =  hasher.digest()
    hex_str = bytes_to_hex_str(hash_bytes)
    hasher2 = '0x' + hex_str
    print("hasher2:"+str(hasher2))
    return hasher2


def ckb_blake160(message: bytes) -> str:
    return ckb_hash(message)[0:42]
