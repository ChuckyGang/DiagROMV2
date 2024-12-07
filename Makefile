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
NDK_INC := .\NDK3.2R4\Include_H\ 
AS := vasmm68k_mot 
ASOPTS := -quiet -m68851 -m68882 -m68020up -no-opt -Fhunk -I. -I$(OUTDIR)/srcs
CC := vc
CFLAGS := +aos68k -cpu=68000 -c99 -O2 -size -I$(NDK_INC) -I. -Isrcs
$(info NDK is $(NDK_INC))
LN := vlink 
#LNFLAGS := -t -M

SRCS =$(wildcard srcs/**/*.c) $(wildcard srcs/**/*.s)
OBJS =$(addprefix $(OUTDIR)/,$(filter %.o,$(SRCS:.c=.o)))
OBJS+=$(addprefix $(OUTDIR)/,$(filter %.o,$(SRCS:.s=.o)))

# Create output dirs
DIRS:=$(OUTDIR) $(patsubst %/,%,$(dir $(OBJS)))
$(foreach dir,$(DIRS),$(shell $(call MD,$(dir))))

# always regenerate builddate.i (only picked up if inputs change)
$(shell date $(DATEOPS) > $(OUTDIR)/srcs/builddate.i)

all: diagrom.rom $(OUTDIR)/diagrom.exe

diagrom.rom: $(OUTDIR)/diagrom_nosum.bin $(OUTDIR)/checksum
	$(OUTDIR)//checksum $< $@

$(OUTDIR)/diagrom_nosum.bin: $(OBJS)
	$(LN) -x -Bstatic -Cvbcc -s -b rawbin1 -T srcs/link.txt $(OBJS) -o $@ $(LNFLAGS)

$(OUTDIR)/diagrom.exe: $(OBJS)
	$(LN) -x -Bstatic -Cvbcc -s -b amigahunk -T srcs/link.txt $(OBJS) -o $@ $(LNFLAGS)

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

# explicit dependencies for asm sources
ASM_SRCS = $(wildcard srcs/**/*.s)
ASM_OBJS = $(addprefix $(OUTDIR)/,$(filter %.o,$(SRCS:.s=.o)))
$(ASM_OBJS): $(OUTDIR)/srcs/globalvars.i

# quick test run
run_test: diagrom.rom
	# Use 'socat pty,raw,echo=0,link=/tmp/virtual-serial-port -,raw,echo=0,crlf' to read serial
	fs-uae --kickstart_file=diagrom.rom --console_debugger=1 --serial_port=/tmp/virtual-serial-port

diagrom.adf: $(OUTDIR)\diagrom.exe
	$(CP) $< diagrom.exe
	echo diagrom.exe > build/startup-sequence
	xdftool -f $@ create + format diagrom + boot install + write diagrom.exe + makedir s + write build/startup-sequence s

run_test_disk: diagrom.adf
	fs-uae --console_debugger=1 --serial_port=/tmp/virtual-serial-port --floppy_drive_0=$<
