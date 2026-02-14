#!/usr/bin/env python3
import sys
from pathlib import Path

def byteswap_16(data: bytes) -> bytes:
    """Swap every pair of bytes."""
    result = bytearray(len(data))
    for i in range(0, len(data), 2):
        result[i] = data[i + 1]
        result[i + 1] = data[i]
    return bytes(result)

def split_32bit(data: bytes) -> tuple[bytes, bytes]:
    """Split into HI (first word) and LO (second word) of each longword."""
    hi = bytearray()
    lo = bytearray()
    for i in range(0, len(data), 4):
        hi.extend(data[i:i+2])
        lo.extend(data[i+2:i+4])
    return bytes(hi), bytes(lo)

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <diagrom.rom> [output_dir]")
        sys.exit(1)

    rom_path = Path(sys.argv[1])
    out_dir = Path(sys.argv[2]) if len(sys.argv) > 2 else rom_path.parent

    data = rom_path.read_bytes()
    rom_size = len(data)

    # 16bit.bin - byteswapped
    swapped = byteswap_16(data)
    (out_dir / "16bit.bin").write_bytes(swapped)

    # Split into HI/LO words
    hi, lo = split_32bit(data)

    # Byteswap each half
    hi_swapped = byteswap_16(hi)
    lo_swapped = byteswap_16(lo)

    # Duplicate to fill EPROM (each half is rom_size/2, duplicate to rom_size)
    hi_padded = hi_swapped * 2
    lo_padded = lo_swapped * 2

    (out_dir / "32bitHI.bin").write_bytes(hi_padded)
    (out_dir / "32bitLO.bin").write_bytes(lo_padded)

    # CD32.bin - HI + LO concatenated
    (out_dir / "CD32.bin").write_bytes(hi_padded + lo_padded)

    print(f"Created: 16bit.bin, 32bitHI.bin, 32bitLO.bin, CD32.bin")

if __name__ == "__main__":
    main()
