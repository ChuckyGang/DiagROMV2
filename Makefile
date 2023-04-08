.PHONY: builddate.i
AS := vasmm68k_mot 
ASOPTS := -m68851 -m68882 -m68020up -no-opt -Fhunk
LN := vlink 
OBJS := earlystart.o romend.o

diagrom.rom: builddate.i $(OBJS)
	$(LN) -t -x -Bstatic -Cvbcc -s -b rawbin2 $(OBJS) -Rshort -o $@
%.o: %.s 
	$(AS) $(ASOPTS) $< -o $@
builddate.i: 
	date +"%Y-%m-%d" > builddate.i
clean:
	rm -f diagrom.rom *.lst a.out *~ \#* *.o split