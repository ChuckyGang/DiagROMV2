.PHONY: builddate.i

DATEOPS= /t 
ifneq ($(OS),Windows_NT)
DATEOPS= +"%Y-%m-%d"
endif

AS := vasmm68k_mot 
ASOPTS := -quiet -m68851 -m68882 -m68020up -no-opt -Fhunk
CC := vc
CFLAGS := +aos68k -cpu=68000 -c99 -sc -sd -O2 -size -I$(NDK_INC) -I.
LN := vlink 
OBJS := earlystart.o constants.o test_c.o checksums.o autovec.o

diagrom.rom: diagrom_nosum.bin
	@echo
	gcc checksum.c -o checksum
	cp $< $@
	./checksum $@
diagrom_nosum.bin: $(OBJS)
	$(LN) -t -x -Bstatic -Cvbcc -s -b rawbin1 -T link.txt $(OBJS) -M -o $@
%.o: %.s builddate.i
	$(AS) $(ASOPTS) $< -o $@
%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
builddate.i: 
	date $(DATEOPS) > builddate.i
clean:
	rm -f diagrom.rom *.lst a.out *~ \#* *.o split
