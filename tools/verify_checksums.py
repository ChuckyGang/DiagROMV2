#!/usr/bin/env python3
"""Verify that diagrom.rom checksums are self-consistent.

Applies the same algorithm as romChecksum() in genericc.c.
If this script passes but the Amiga shows red blocks, the bug
is in the 68k-compiled romChecksum() C code, not in checksum.c.
"""
import sys
import struct

ROM_SIZE = 524288  # 512KB
ROM_BASE = 0xF80000


def rl(data, offset):
    return struct.unpack_from('>I', data, offset)[0]


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <diagrom.rom>")
        sys.exit(1)

    data = open(sys.argv[1], 'rb').read()
    if len(data) != ROM_SIZE:
        print(f"ERROR: ROM size is {len(data)}, expected {ROM_SIZE}")
        sys.exit(1)

    # Find the checksums area (same logic as checksum.c)
    check_str = b'Checksums:'
    i = data.find(check_str)
    if i < 0:
        print("ERROR: 'Checksums:' marker not found")
        sys.exit(1)
    cs_offset = (i + len(check_str) + 3) & ~3
    cs_end = cs_offset + 4 * 8

    print(f"'Checksums:' at file offset 0x{i:x} (ROM addr 0x{ROM_BASE+i:08x})")
    print(f"Checksum array:  file 0x{cs_offset:x}  ROM 0x{ROM_BASE+cs_offset:08x}")
    print(f"Checksum end:    file 0x{cs_end:x}  ROM 0x{ROM_BASE+cs_end:08x}")

    stored = [rl(data, cs_offset + k * 4) for k in range(8)]
    print(f"Stored checksums: {[f'0x{v:08x}' for v in stored]}")
    print()

    all_ok = True
    for block in range(8):
        computed = 0
        for j in range(0, 0x10000, 4):
            off = block * 0x10000 + j
            if cs_offset <= off < cs_end:
                continue  # exclude checksum values (same as checksum.c)
            computed = (computed + rl(data, off)) & 0xFFFFFFFF

        ok = (computed == stored[block])
        if not ok:
            all_ok = False
        status = "OK" if ok else "FAIL"
        print(f"  Block {block} (ROM 0x{ROM_BASE + block*0x10000:08x}): "
              f"computed=0x{computed:08x}  stored=0x{stored[block]:08x}  {status}")

    print()
    if all_ok:
        print("All checksums verified OK")
        return 0
    else:
        print("ERROR: One or more checksums FAILED")
        return 1


sys.exit(main())
