#!/bin/bash

date +"%Y-%m-%d" > BootDate.txt
vasmm68k_mot  -quiet -m68851 -m68882 -m68020up -no-opt -Fbin DiagROM.s -o DiagROM.bin -L DiagROM.txt
dd if=diagrom.rom of=DiagROM bs=1024 skip=15872
gcc checksum.c -o checksum && ./checksum DiagROM
rm diagrom.rom

# test:
# $ fs-uae --kickstart_file=DiagROM_F8.rom --console_debugger=1 --serial_port=/tmp/virtual-serial-port
# serial
# $ socat pty,raw,echo=0,link=/tmp/virtual-serial-port -,raw,echo=0,crlf
