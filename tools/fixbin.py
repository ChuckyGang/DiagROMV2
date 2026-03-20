#!/usr/bin/env python3
"""Normalize ROM binary to exactly ROM_SIZE bytes with autovecs at offset ROM_SIZE-16.

Autovecs are always placed last in the .text section, so they are reliably the
last 16 bytes of the binary regardless of total binary size.
"""
import sys

AUTOVEC = bytes([0x00,0x18,0x00,0x19,0x00,0x1a,0x00,0x1b,0x00,0x1c,0x00,0x1d,0x00,0x1e,0x00,0x1f])
ROM_SIZE = 524288  # 512KB

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <binary>")
        sys.exit(1)

    fname = sys.argv[1]
    data = open(fname, 'rb').read()

    if len(data) == ROM_SIZE:
        print(f"fixbin: {ROM_SIZE} bytes, no fixup needed")
        return

    # Autovecs are placed last in the output section.
    # The linker may add up to 3 trailing alignment bytes after them, so search
    # for the pattern within the last 20 bytes of the binary.
    pos = data.rfind(AUTOVEC, len(data) - 20)
    if pos < 0:
        print(f"fixbin ERROR: autovec pattern not found in last 20 bytes")
        print(f"  last 20 bytes: {data[-20:].hex()}")
        sys.exit(1)

    code = data[:pos]
    if len(code) > ROM_SIZE - 16:
        print(f"fixbin ERROR: code too large: {len(code)} bytes, max {ROM_SIZE - 16}")
        sys.exit(1)

    out = bytearray(ROM_SIZE)
    out[:len(code)] = code
    out[ROM_SIZE - 16:] = AUTOVEC

    open(fname, 'wb').write(bytes(out))
    print(f"fixbin: {len(data)} -> {ROM_SIZE} bytes, autovecs at 0x{ROM_SIZE - 16:x}")

main()
