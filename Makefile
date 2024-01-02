.PHONY: clean

OUTDIR=build

DATEOPS= /t 
CP := copy
ifneq ($(OS),Windows_NT)
DATEOPS= +"%Y-%m-%d"
CP := cp
endif

AS := vasmm68k_mot 
ASOPTS := -quiet -m68851 -m68882 -m68020up -no-opt -Fhunk -I. -I$(OUTDIR)/srcs
CC := vc
CFLAGS := +aos68k -cpu=68000 -c99 -sc -sd -O2 -size -I$(NDK_INC) -I. -Isrcs
LN := vlink 

SRCS =$(wildcard srcs/**/*.c) $(wildcard srcs/**/*.s)
OBJS =$(addprefix $(OUTDIR)/,$(filter %.o,$(SRCS:.c=.o)))
OBJS+=$(addprefix $(OUTDIR)/,$(filter %.o,$(SRCS:.s=.o)))

# Create output dirs
$(shell mkdir -p $(OUTDIR) $(dir $(OBJS)) > /dev/null)

# always regenerate builddate.i (only picked up if inputs change)
$(shell date $(DATEOPS) > $(OUTDIR)/srcs/builddate.i)

diagrom.rom: $(OUTDIR)/diagrom_nosum.bin $(OUTDIR)/checksum
	$(OUTDIR)//checksum $< $@

$(OUTDIR)/diagrom_nosum.bin: $(OBJS)
	$(LN) -t -x -Bstatic -Cvbcc -s -b rawbin1 -T srcs/link.txt $(OBJS) -M -o $@

$(OUTDIR)/%.o: %.s
	$(AS) $(ASOPTS) $< -o $@

$(OUTDIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OUTDIR)/%.i: %.h
	python3 tools/h2i.py $< -- -I libc > $@

$(OUTDIR)/checksum: tools/checksum.c
	gcc $< -o $@

clean:
	rm -fr diagrom.rom $(OUTDIR) *.lst a.out *~ \#* split

# all objects depend on this Makefile
$(OBJS): Makefile

# explicit dependencies
$(OUTDIR)/srcs/asm/earlystart.o: $(OUTDIR)/srcs/globalvars.i

# quick test run
run_test: diagrom.rom
	# Use 'socat pty,raw,echo=0,link=/tmp/virtual-serial-port -,raw,echo=0,crlf' to read serial
	fs-uae --kickstart_file=diagrom.rom --console_debugger=1 --serial_port=/tmp/virtual-serial-port
