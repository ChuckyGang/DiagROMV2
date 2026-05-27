#!/usr/bin/env python3
"""
mkrom_demon.py — Pad/finalise the DeMoN cartridge ROM image to 256 KB.

This is the DeMoN equivalent of tools/fixbin.py + tools/mkrom.py +
tools/checksum.c rolled into one.  Key differences from the Kickstart
flow:

  - Target size is 256 KB (262144 bytes), not 512 KB
  - NO Kickstart checksum is computed (cartridge isn't a Kickstart;
    the 68000 doesn't verify a checksum here, and Diagrom doesn't
    self-verify when entered via NMI vector)
  - NO autovec footer at end-of-ROM (the DeMoN entry is via NMI vector
    at offset $7C, not via the 68000 reset vector + autovec mechanism)
  - The NMI vector longword at offset $7C is validated for sanity
    (must point inside ROM range $A80000-$ABFFFF)

Usage:
    python3 tools/mkrom_demon.py build/diagrom_demon_raw.bin diagrom_demon.bin

Output is exactly 256 KB, padded with 0xFF (flash default erase state).
"""
import sys
import struct

ROM_SIZE   = 256 * 1024
ROM_BASE   = 0x00A80000
ROM_END    = ROM_BASE + ROM_SIZE
NMI_VEC_OFS = 0x7C
PAD_BYTE   = 0xFF


def fail(msg):
    print(f"mkrom_demon: ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.bin> <output.bin>")
        sys.exit(1)

    src_path, dst_path = sys.argv[1], sys.argv[2]

    with open(src_path, 'rb') as f:
        data = f.read()

    if len(data) > ROM_SIZE:
        fail(f"input too large: {len(data)} bytes, max {ROM_SIZE} (256 KB)")

    if len(data) < NMI_VEC_OFS + 4:
        fail(f"input too small: {len(data)} bytes, must be at least {NMI_VEC_OFS + 4}")

    # Validate NMI vector at offset $7C
    nmi_target = struct.unpack(">I", data[NMI_VEC_OFS:NMI_VEC_OFS + 4])[0]
    if not (ROM_BASE <= nmi_target < ROM_END):
        fail(
            f"NMI vector @ ${NMI_VEC_OFS:02X} points to ${nmi_target:08X}, "
            f"which is outside ROM range ${ROM_BASE:08X}-${ROM_END:08X}"
        )

    # Find "End of Code..." marker (placed by checksums.s)
    # and fill subsequent area with addressdata: each longword = its absolute
    # ROM address. This matches what tools/checksum.c does for vanilla Diagrom,
    # and enables the "Check ROM Addressdata" test to pass.
    end_marker = b"End of Code..."
    eoc_pos = data.find(end_marker)
    out = bytearray(ROM_SIZE)
    out[:len(data)] = data
    # Pad remaining with PAD_BYTE first
    for i in range(len(data), ROM_SIZE):
        out[i] = PAD_BYTE

    if eoc_pos < 0:
        print(f"mkrom_demon: WARNING: 'End of Code...' marker not found, no addressdata padding")
    else:
        # Address area: from end_of_code (+ marker length + null + align)
        # up to ROM_SIZE - 16 (last 16 bytes are autovec, leave alone)
        addr_start = eoc_pos + len(end_marker) + 1   # +1 for null terminator
        addr_start = (addr_start + 3) & ~3           # align to longword
        addr_end = ROM_SIZE - 16                     # leave autovec
        print(f"mkrom_demon: filling addressdata ${addr_start:05x}-${addr_end:05x}")
        for offset in range(addr_start, addr_end, 4):
            abs_addr = ROM_BASE + offset
            out[offset+0] = (abs_addr >> 24) & 0xff
            out[offset+1] = (abs_addr >> 16) & 0xff
            out[offset+2] = (abs_addr >>  8) & 0xff
            out[offset+3] = (abs_addr >>  0) & 0xff

    # Autovec at end of ROM (last 16 bytes): 0x0018,0x0019,...,0x001F
    autovec = bytes([
        0x00,0x18, 0x00,0x19, 0x00,0x1a, 0x00,0x1b,
        0x00,0x1c, 0x00,0x1d, 0x00,0x1e, 0x00,0x1f,
    ])
    out[ROM_SIZE-16:ROM_SIZE] = autovec

    out = bytes(out)

    with open(dst_path, 'wb') as f:
        f.write(out)

    print(f"mkrom_demon: {len(data)} bytes code, padded to {ROM_SIZE} bytes")
    print(f"             NMI vector @ ${NMI_VEC_OFS:02X} -> ${nmi_target:08X}  OK")
    print(f"             Wrote {dst_path}")

    # Also produce the two flash chip images (even/odd byte split) for
    # programming via DeMoN's two flash chips.  Same logic as the
    # DeMoN2 firmwareSplitter.py, but kept self-contained here.
    even = bytearray(ROM_SIZE // 2)   # high byte / D8..D15  (UDS)
    odd  = bytearray(ROM_SIZE // 2)   # low  byte / D0..D7   (LDS)
    for i in range(0, ROM_SIZE, 2):
        even[i // 2] = out[i]         # MSB of each 16-bit word
        odd[i  // 2] = out[i + 1]     # LSB of each 16-bit word

    base = dst_path.rsplit('.', 1)[0]
    with open(base + '_hi.bin', 'wb') as f:
        f.write(bytes(even))
    with open(base + '_lo.bin', 'wb') as f:
        f.write(bytes(odd))
    print(f"             Wrote {base}_hi.bin (U7, high byte)")
    print(f"             Wrote {base}_lo.bin (U9, low byte)")


if __name__ == "__main__":
    main()
