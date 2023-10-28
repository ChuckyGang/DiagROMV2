.PHONY:

DATEOPS= /t 
CP := copy
ifneq ($(OS),Windows_NT)
DATEOPS= +"%Y-%m-%d"
CP := cp
endif

# always regenerate builddate.i (only picked up if inputs change)
$(shell date $(DATEOPS) > builddate.i)

AS := vasmm68k_mot 
ASOPTS := -quiet -m68851 -m68882 -m68020up -no-opt -Fhunk
CC := vc
CFLAGS := +aos68k -cpu=68000 -c99 -sc -sd -O2 -size -I$(NDK_INC) -I.
LN := vlink 
OBJS := earlystart.o constants.o test_c.o checksums.o autovec.o

diagrom.rom: diagrom_nosum.bin checksum
	./checksum $< $@
diagrom_nosum.bin: $(OBJS)
	$(LN) -t -x -Bstatic -Cvbcc -s -b rawbin1 -T link.txt $(OBJS) -M -o $@
%.o: %.s
	$(AS) $(ASOPTS) $< -o $@
%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
checksum: checksum.c
	gcc checksum.c -o checksum
clean:
	rm -f diagrom.rom *.lst a.out *~ \#* *.o split checksum builddate.i
