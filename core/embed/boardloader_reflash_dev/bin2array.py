#!/usr/bin/env python3
import binascii
import argparse

parser = argparse.ArgumentParser(description='Convert binary file to C-style array initializer.')
parser.add_argument("-i", "--input", help="the file to be converted")
parser.add_argument("-o", "--output", help="write output to a file")
parser.add_argument("-l", "--linebreak", type=int, default=8, help="add linebreak after every N element")
args = parser.parse_args()

if __name__ == "__main__":

    c_code:str = ""

    with open(args.input, 'rb') as f:
        file_content = f.read()
        
    hexstr = binascii.hexlify(file_content).decode("UTF-8")
    hexstr = ["0x" + hexstr[i:i + 2] + ", " for i in range(0, len(hexstr), 2)]

    c_code += '#include <stdint.h>\n'
    c_code += f'const __attribute__((section(".flash"))) uint8_t payload[{len(hexstr)}] = {{\n'

    if args.linebreak > 0:
        for i in range(0, len(hexstr), args.linebreak):
            c_code += '    '
            c_code += ''.join(hexstr[i:i+args.linebreak])
            c_code += '\n'

    c_code += '};\n'

    if args.output:
        with open(args.output, 'w') as f:
            f.write(c_code)
    else:
        print(c_code)