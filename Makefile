.PHONY: builddate.i

ifeq ($(OS),Windows_NT)
	DATEOPS:= /t 
else 
	DATEOPS:= +"%Y-%m-%d"
endif

AS := vasmm68k_mot 
ASOPTS := -m68851 -m68882 -m68020up -no-opt -Fhunk
LN := vlink 
OBJS := earlystart.o constants.o romend.o

diagrom.rom: builddate.i $(OBJS)
	$(LN) -t -x -Bstatic -Cvbcc -s -b amigahunk -mrel $(OBJS) -Rshort -o $@
%.o: %.s 
	$(AS) $(ASOPTS) $< -o $@
builddate.i: 
	date $(DATEOPS) > builddate.i
clean:
	rm -f diagrom.rom *.lst a.out *~ \#* *.o split