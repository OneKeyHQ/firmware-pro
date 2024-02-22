#!/usr/bin/env python3
import argparse

from hashlib import sha256


FWHEADER_SIZE = 1024
SIGNATURES_START = 6 * 4 + 8 + 512
INDEXES_START = SIGNATURES_START + 4 * 64


def parse_args():
    parser = argparse.ArgumentParser(
        description="Commandline tool for classic1s bin hash."
    )
    parser.add_argument("-t", "--type", dest="bintype", help="Bin type")
    parser.add_argument("-f", "--file", dest="path", help="Bin file")

    return parser.parse_args()


def compute_hashes(data):
    # process chunks
    hash = sha256(data).digest()
    return hash


def compute_boot_hashes(data):
    d = data[1024:]
    bh = sha256(sha256(d).digest()).digest()
    return bh


def compute_bt_hashes(data):
    hash = sha256(data).digest()
    return hash


def main(args):
    if not args.path:
        raise Exception("-f/--file is required")

    if not args.bintype:
        raise Exception("-t/--type is required")


    if args.bintype == "firmware":
        data = open(args.path, "rb").read()
        hash = compute_hashes(data[FWHEADER_SIZE:])
        print("firmware hash: ", hash.hex())
    elif args.bintype == "se":
        data = open(args.path, "rb").read()
        hash = compute_hashes(data[FWHEADER_SIZE:])
        print("se app hash: ", hash.hex())
    elif args.bintype == "bootloader":
        data = open(args.path, "rb").read()
        hash = compute_boot_hashes(data)
        print("bootloader hash: ", hash.hex())
    elif args.bintype == "bluetooth":
        data = open(args.path, "rb").read()
        hash = compute_bt_hashes(data)
        print("bluetooth app hash: ", hash.hex())
    else:
        print("no support")

    data = open(args.path, "rb").read()
    compute_hashes(data[FWHEADER_SIZE:])


if __name__ == "__main__":
    args = parse_args()
    main(args)
