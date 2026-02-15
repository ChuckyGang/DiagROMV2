.PHONY: clean

OUTDIR=build

DATEOPS= /t 
CP := copy
MD = md $(subst /,\,$(1)) > nul
ifneq ($(OS),Windows_NT)
DATEOPS= +"%Y-%m-%d"
CP := cp
MD = mkdir -p $(1) > /dev/null
endif
NDK_INC := ./ndk/Include_H 
AS := /opt/amiga/bin/vasmm68k_mot
ASOPTS := -DDEBUG=0 -quiet -m68851 -m68882 -m68020up  -no-opt -Fhunk -I. -I$(OUTDIR)/srcs 
CC := /opt/amiga/bin/m68k-amigaos-gcc 
#CFLAGS := -DDEBUG=2 -mcpu=68000 -O2 -g -mregparm=4 -ffixed-a6 -fomit-frame-pointer -I$(NDK_INC) -I. -Isrcs
CFLAGS := -DDEBUG=2 -mcpu=68000 -O0 -g -ffixed-a6 -fomit-frame-pointer -I$(NDK_INC) -I. -Isrcs

$(info NDK is $(NDK_INC))
LN := /opt/amiga/bin/m68k-amigaos-gcc
#LNFLAGS := -t -M
OC := /opt/amiga/bin/m68k-amigaos-objcopy

SRCS =$(wildcard srcs/**/*.c) $(wildcard srcs/**/*.s)
OBJS =$(addprefix $(OUTDIR)/,$(filter %.o,$(SRCS:.c=.o)))
OBJS+=$(addprefix $(OUTDIR)/,$(filter %.o,$(SRCS:.s=.o)))
OBJS+=$(OUTDIR)/data/TopazFont.o

# Create output dirs
DIRS:=$(OUTDIR) $(patsubst %/,%,$(dir $(OBJS)))
$(foreach dir,$(DIRS),$(shell $(call MD,$(dir))))

# always regenerate builddate.i (only picked up if inputs change)
$(shell date $(DATEOPS) > $(OUTDIR)/srcs/builddate.i)

all: diagrom.rom $(OUTDIR)/diagrom.exe

diagrom.rom: $(OUTDIR)/diagrom_nosum.bin $(OUTDIR)/checksum
	$(OUTDIR)/checksum $< $@
	python3 tools/mkrom.py $@

$(OUTDIR)/diagrom_nosum.exe: $(OBJS)
	$(LN) -nostartfiles -nostdlib -Wl,-Map,$@.txt -T srcs/link.txt $(OBJS) -o $@ $(LNFLAGS)

$(OUTDIR)/%.bin: $(OUTDIR)/%.exe
	$(OC) -O binary $< $@

$(OUTDIR)/diagrom.exe: $(OBJS)
	$(LN) -nostartfiles -nostdlib -Wl,-Map,$@.txt -T srcs/link_exe.txt $(OBJS) -o $@ $(LNFLAGS)

$(OUTDIR)/%.o: %.s
	$(AS) $(ASOPTS) $< -o $@

$(OUTDIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OUTDIR)/data/TopazFont.o: data/TopazFont.bin
	@mkdir -p $(dir $@)
	$(OC) -I binary -O amiga -B m68k $< $@
	$(OC) --redefine-sym _binary_data_TopazFont_bin_start=_RomFont $@

$(OUTDIR)/%.i: %.h
	python3 tools/h2i.py $< -o $@ -- -I libc

$(OUTDIR)/checksum: tools/checksum.c
	gcc $< -o $@


clean:
	rm -fr diagrom.rom $(OUTDIR) *.lst a.out *~ \#* split

# all objects depend on this Makefile
$(OBJS): Makefile

# explicit dependencies for asm sources
ASM_SRCS = $(wildcard srcs/**/*.s)
ASM_OBJS = $(addprefix $(OUTDIR)/,$(filter %.o,$(SRCS:.s=.o)))
$(ASM_OBJS): $(OUTDIR)/srcs/globalvars.i

# quick test run
run_a500: diagrom.rom
	# Use 'socat pty,raw,echo=0,link=/tmp/virtual-serial-port -,raw,echo=0,crlf' to read serial
	fs-uae --kickstart_file=diagrom.rom --console_debugger=1 --serial_port=/tmp/virtual-serial-port  --window_width=1280 --window_height=1024 --scale_x=2.0 --scale_y=2.

# quick test run
run: diagrom.rom
	# Use 'socat pty,raw,echo=0,link=/tmp/virtual-serial-port -,raw,echo=0,crlf' to read serial
	fs-uae --amiga-model=A1200 --cpu=68020 --chip_memory=2048 --fast_memory=8192 --kickstart_file=diagrom.rom --floppy_drive_0=data/Install31.adf --serial_port=/tmp/virtual-serial-port --window_width=1280 --window_height=1024 --scale_x=2.0 --scale_y=2.0
	
