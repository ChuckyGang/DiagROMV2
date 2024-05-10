       include "earlymacros.i"
       include "build/srcs/globalvars.i"
       section "memtest",code_p
       xdef   MemtestMenu
       xdef   CheckDetectedChip
       xdef   CheckExtendedChip
       xdef   CheckDetectedMBMem
       xdef   CheckExtended16MBMem
       xdef   ForceExtended16MBMem
       xdef   Detectallmemory
       xdef   CheckMemManual
       xdef   CheckMemEdit
    	
MemtestMenu:
	bsr	ClearBuffer
	bsr	InitScreen
	move.w	#3,MenuNumber(a6)
	move.b	#1,PrintMenuFlag(a6)
	bra	MainLoop

CheckDetectedChip:
	bsr	ClearScreen
	lea	MemtestDetChipTxt,a0
	move.l	#2,d1
	bsr	Print
	move.b	#0,CheckMemNoShadow(a6)
	move.l	ChipStart(a6),d0
       move.l ChipEnd(a6),d1
       move.l	d0,CheckMemFrom(a6)
	move.l	d1,CheckMemTo(a6)
	move.b	#14,LogYpos(a6)		; Store first row of log
	move.l	#0,CheckMemStepSize(a6)	; Set how many bytes to step between every memorytest
	bsr	MemTesterInit
.loop:
	bsr	MemTesterNewBlock
	bsr	MemoryTester
	cmp.w	#1,CheckMemCancel(a6)
	beq	.cancel
	bsr	MemTesterNewPass
	bra	.loop
.cancel:
;		move.l	#0,d2
;		clr.b	CheckMemRow(a6)
;		bsr	CheckMemory
	bsr	WaitButton
	bra	MemtestMenu


CheckDetectedMBMem:
	jsr	ClearScreen
	clr.w	MemDetected(a6)
	move.w	#"DE",DetectMemRnd(a6)	; "put in RN" at detectMemRnd to have some data
	add.w	#1,DetectMemRnd+2(a6)		; Increase by 1 to have a number that changes every call
	clr.l	FastmemBlock(a6)
	lea	$200000,a1
	lea	$d00000,a4	; endaddress of this pass
	bsr	.memloop	
	cmp.b	#1,ADR24BIT(a6)	; Check if we had 24 bit cpu...
	bne	.no24bit
	bra	.24bit
.no24bit:
	cmp.l	#" PPC",$f00090	; Check if the string "PPC" is located in rom at this address. if so we have a BPPC
				; that will disable the 68k cpu onboard if memory  below $40000000 is tested.
	bne	.nobppc
	lea	$40000000,a1	; Strangly enough.  bppc detected memory will be totally just plain WRONG! I guess it does stuff in rom
	bra	.bppc		; that makes a more decent memorymap. Now it just finds lots of smaller shadows..
.nobppc:
	lea	$1000000,a1
.bppc:
	lea	$f0000000,a4	; endaddress of this pass
	bsr	.memloop	
.24bit:
	jsr	LogLine
	lea	AnyKeyMouseTxt,a0
	move.l	#5,d1
	jsr	Print
	bsr	WaitPressed
	bsr	WaitReleased
       bra	MemtestMenu
.memloop:
	clr.l	d1
	move.l	a4,a2		; Set a2 to endaddress of scan
	lea	.leadone,a3
	move.l	DetectMemRnd(a6),d0	; store a "random" data in d0 for shadowcontrol
	bra	DetectMemory
.leadone:
	add.l	d1,FastmemBlock(a6)
	cmp.l	#0,a0
	bne	.mem
	bra	.end
.mem:
	cmp.l	#0,d1		; check if size was 0, that means this memory is "illegal" and should be skipped
	beq	.blockdone
	move.l	a0,a2		; Store address of first mem found into a2
	move.l	d1,d2		; copy size to d2
	cmp.w	#0,MemDetected(a6)	; did we have any detected ram yet?
	bne	.yesdetected		; if it wasn't null we had mem
	move.w	#1,MemDetected(a6)
	jsr	.initmemtest
.yesdetected
	PUSH
	clr.l	d0
	clr.l	d1
	bsr	SetPos
	lea	EmptyRowTxt,a0
	jsr	Print
	clr.l	d0
	clr.l	d1
	bsr	SetPos
	POP
	lea	DetMem,a0
	move.l	#2,d1
	jsr	Print
	move.l	d2,d0		; copy size to d0
	bsr	.PrintSize
	lea	DetOfmem,a0
	jsr	Print
	move.l	a2,d0		; Print first memaddress
	move.l	a2,CheckMemFrom(a6)
	bsr	binhex
	jsr	Print
	lea	MinusTxt,a0
	jsr	Print
	move.l	a1,d0		; Print end memaddress
	bsr	binhex
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	PUSH
	bsr	LogLine
	lea	DetMem,a0
	move.l	#2,d1
	jsr	Print
	move.l	d2,d0		; copy size to d0
	bsr	.PrintSize
	lea	DetOfmem,a0
	jsr	Print
	move.l	a2,d0		; Print first memaddress
	bsr	binhex
	jsr	Print
	lea	MinusTxt,a0
	jsr	Print
	move.l	a1,d0		; Print end memaddress
	bsr	binhex
	move.l	a1,CheckMemTo(a6)
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	bsr	MemTesterNewBlock
	bsr	MemoryTester
	POP
				; ok we now had the endaddress at the same register Detectmemory uses as START. so lets check if we are at end of
.blockdone
				; memarea and if not, just loop until we are done.
	add.l	#64*1024,a1	; Add 64k for next block to test, just in case
	cmp.l	a4,a1
	blo	.memloop
.end:
	rts
.PrintSize:
	cmp.l	#32,d0		; Check if we had more than 4 blocks (2048k)  if so.. lets show in MB instead.
	bge	.showMB
	asl.l	#6,d0		; convert number of 16k blocks to real value of kb
	bsr	bindec
	move.l	#2,d1
	jsr	Print		; print it
	lea	KB,a0
	jsr	Print
	bra	.donesize
.showMB:			; convert number of 16k blocks to real value of mb
	asr.l	#4,d0
	bsr	bindec
	move.l	#2,d1
	jsr	Print		; print it
	lea	MB,a0
	jsr	Print
.donesize:
	rts
.initmemtest:
	PUSH
	move.l	#$7000000,CheckMemFrom(a6)
	move.l	#$7ffffff,CheckMemTo(a6)
	move.b	#14,LogYpos(a6)		; Store first row of log
	move.l	#0,CheckMemStepSize(a6)	; Set how many bytes to step between every memorytest
	bsr	MemTesterInit
	POP
	rts


Detectallmemory:
	jsr	ClearScreen
	move.w	#"RN",DetectMemRnd(a6)	; "put in RN" at detectMemRnd to have some data
	add.w	#1,DetectMemRnd+2(a6)		; Increase by 1 to have a number that changes every call
	lea	Det24bittxt,a0
	move.l	#5,d1
	jsr	Print
	clr.l	FastmemBlock(a6)
	lea	$200000,a1
	lea	$d00000,a4	; endaddress of this pass
	bsr	.memloop	
	lea	Det32bittxt,a0
	move.l	#5,d1
	jsr	Print
	cmp.b	#1,ADR24BIT(a6)	; Check if we had 24 bit cpu...
	bne	.no24bit
	lea	No32bittxt,a0
	move.l	#1,d1
	jsr	Print
	bra	.24bit
.no24bit:
	cmp.l	#" PPC",$f00090	; Check if the string "PPC" is located in rom at this address. if so we have a BPPC
				; that will disable the 68k cpu onboard if memory  below $40000000 is tested.
	bne	.nobppc
	lea	$40000000,a1	; Strangly enough.  bppc detected memory will be totally just plain WRONG! I guess it does stuff in rom
	bra	.bppc		; that makes a more decent memorymap. Now it just finds lots of smaller shadows..
.nobppc:
	lea	$1000000,a1
.bppc:
	lea	$f0000000,a4	; endaddress of this pass
	bsr	.memloop	
.24bit:
	lea	Totmemtxt,a0
	move.l	#6,d1
	jsr	Print
	move.l	FastmemBlock(a6),d0
	move.l	d0,d1
	asl.l	#6,d1
	move.l	d1,TotalFast(a6)
	bsr	.PrintSize
	lea	NewLineTxt,a0
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	lea	AnyKeyMouseTxt,a0
	move.l	#5,d1
	jsr	Print
	bsr	WaitPressed
	bsr	WaitReleased
	bra	MemtestMenu
.memloop:
	clr.l	d1
	move.l	a4,a2		; Set a2 to endaddress of scan
	lea	.leadone,a3
	move.l	DetectMemRnd(a6),d0	; store a "random" data in d0 for shadowcontrol
       bra	DetectMemory
.leadone:
	add.l	d1,FastmemBlock(a6)
	cmp.l	#0,a0
	bne	.mem
	lea	EndMemTxt,a0
	move.l	#3,d1
	jsr	Print
	bra	.end
.mem:
	cmp.l	#0,d1		; check if size was 0, that means this memory is "illegal" and should be skipped
	beq	.blockdone
	move.l	a0,a2		; Store address of first mem found into a2
	move.l	d1,d2		; copy size to d2
	lea	DetMem,a0
	move.l	#2,d1
	jsr	Print
	move.l	d2,d0		; copy size to d0
	bsr	.PrintSize
	lea	DetOfmem,a0
	jsr	Print
	move.l	a2,d0		; Print first memaddress
	bsr	binhex
	jsr	Print
	lea	MinusTxt,a0
	jsr	Print
	move.l	a1,d0		; Print end memaddress
	bsr	binhex
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
				; ok we now had the endaddress at the same register Detectmemory uses as START. so lets check if we are at end of
.blockdone
				; memarea and if not, just loop until we are done.
	add.l	#64*1024,a1	; Add 64k for next block to test, just in case
	cmp.l	a4,a1
	blo	.memloop
.end:
	rts
.PrintSize:
	cmp.l	#32,d0		; Check if we had more than 4 blocks (2048k)  if so.. lets show in MB instead.
	bge	.showMB
	asl.l	#6,d0		; convert number of 16k blocks to real value of kb
	bsr	bindec
	move.l	#2,d1
	jsr	Print		; print it
	lea	KB,a0
	jsr	Print
	bra	.donesize
.showMB:			; convert number of 16k blocks to real value of mb
	asr.l	#4,d0
	bsr	bindec
	move.l	#2,d1
	jsr	Print		; print it
	lea	MB,a0
	jsr	Print
.donesize:
	rts

CheckMemManual:
	bsr	ClearScreen
	lea	MemtestManualTxt,a0
	move.l	#6,d1
	bsr	Print
	move.b	#41,d0
	move.b	#13,d1
	lea	$0,a0
	bsr	InputHexNum
	cmp.l	#-1,d0
	beq	.exit
	move.l	d0,d6
	lea	MemtestManualEndTxt,a0
	move.l	#6,d1
	bsr	Print
	lea	$0,a0
	bsr	InputHexNum
	cmp.l	#-1,d0
	beq	.exit
	move.l	d0,d7
	lea	MemtestManualBlockTxt,a0
	move.l	#6,d1
	bsr	Print
	lea	1,a0
	bsr	InputDecNum
	cmp.l	#-1,d0
	beq	.exit
	cmp.l	#0,d0
	bne	.nonull
	move.l	#1,d0			; ok someone was funny and entered 0, change to 1
.nonull:
	cmp.l	#512,d0
	blt	.nomax
	move.l	#512,d0			; ok someone entered more than 512. lets put that to a limit..
.nomax:
	cmp.l	d6,d7	
	beq	.exit			; end and start was the same, skip all
	cmp.l	d6,d7			; check if start is higher than end
	bgt	.nothigher
					; OK that was it. lets swap result
	move.l	d6,d5
	move.l	d7,d6
	move.l	d5,d7
.nothigher:
	move.l	d6,CheckMemFrom(a6)
	move.l	d7,CheckMemTo(a6)
	mulu	#4,d0
	move.l	d0,CheckMemStepSize(a6)	; Set how many bytes to step between every memorytest
	move.b	#14,LogYpos(a6)		; Store first row of log
;	lea	MemoryTestManual,a0
;	move.l	#MemTestEndcode-MemoryTestManual,d0
;	jsr	RunCode
	jsr	MemoryTestManual
	bsr	ClearBuffer
	bsr	WaitPressed
	bsr	WaitReleased
.exit:
	bra	MemtestMenu
CheckMemEdit:
	bsr	ClearScreen
	lea	CheckMemEditTxt,a0
	move.l	#2,d1
	bsr	Print
	move.l	#34,d0
	move.l	#1,d1
	bsr	SetPos
	lea	OFF,a0
	move.l	#3,d1
	bsr	Print
;	clr.b	CpuCache(a6)			; Set status to off
	move.b	#0,CheckMemEditXpos(a6)
	move.b	#0,CheckMemEditYpos(a6)	; Clear X and Y positions
	move.b	#0,CheckMemEditOldXpos(a6)
	move.b	#0,CheckMemEditOldYpos(a6)	; Clear X and Y positions
	clr.l	d0
	move.l	#3,d1
	bsr	SetPos
	move.l	CheckMemEditScreenAdr(a6),d0
	move.l	d0,a0
	bsr	CheckMemEditUpdateScreen
.loop:
	bsr	.putcursor
	bsr	GetMouse
	cmp.b	#1,RMB(a6)
	beq	.exit
.ansimode:
	bsr	GetChar
	cmp.b	#$1b,d0
	beq	.exit
	cmp.b	#1,d0
	beq	.pgup
	cmp.b	#2,d0
	beq	.pgdown
	cmp.b	#30,d0
	beq	.up	
	cmp.b	#31,d0
	beq	.down
	cmp.b	#28,d0
	beq	.right
	cmp.b	#29,d0
	beq	.left
	move.b	d0,d1				; Copy char to d1, so we do not trash for hexnumbers
	bclr	#5,d1				; make it uppercase
	cmp.b	#"q",d0
	beq.w	.pgup
	cmp.b	#"z",d0
	beq.w	.pgdown
	cmp.b	#"G",d1
	beq	.GotoMem			; G was pressed, let user enter address to dump
	cmp.b	#"R",d1
	beq	.Refresh
	cmp.b	#"H",d1
	beq	.Cache
	cmp.b	#"X",d1
	beq	.Execute
	bsr	GetHex				; OK, convert it to hex. if anything is left now, we have a hexdigit that
	cmp.b	#"0",d0
	blt	.nohex
.tobin:
	cmp.b	#"A",d0				; Check if it is "A"
	blt	.nochar				; Lower then A, this is not a char
	sub.l	#7,d0				; ok we have a char, subtract 7
.nochar:
	sub.l	#$30,d0				; Subtract $30, converting it to binary.
	move.l	d0,d2				; Store d0 into d2 temporary
	move.b	CheckMemEditXpos(a6),d0
	move.b	CheckMemEditYpos(a6),d1
	bsr	.getcursoradr			; a0 will now contain the address of memoryadress where cursor is
	clr.l	d7
	move.b	(a0),d7				; and D7 will contain what that address contains
	move.l	d2,d0				; Restore d2
	cmp.b	#0,CheckMemEditCharPos(a6)
	bne	.nocurleft
	and.b	#$f,d7				; Strip out high nibble from d7
	asl	#4,d0				; rotate input data to high nibble
	add.b	d0,d7				; add them together
	move.b	d7,(a0)				; store in memory
	add.b	#1,CheckMemEditCharPos(a6)	; add 1 to pos, for next nibble
	bra	.editdone
.nocurleft:
	and.b	#$f0,d7				; Strip out low nibble
	add.b	d0,d7				; add indata with the rest of d7
	move.b	d7,(a0)				; store in memory
	clr.b	CheckMemEditCharPos(a6)	; Clear charpos
	cmp.b	#15,CheckMemEditXpos(a6)
	beq	.noright
	add.b	#1,CheckMemEditXpos(a6)	; move one step to the right
.editdone:
						; Should go into memory. (Whaa.  BANGING on da shit here)
.nohex:
.keydone:
	bra	.loop
.getcursoradr:					; Get memoryaddress of X, Y pos
						; INDATA:
						;	d0 = xpos
						;	d1 = ypos
						;
						; OUTDATA:
						;	a0 = memoryaddress
	and.l	#$ff,d0
	and.l	#$ff,d1
	move.l	CheckMemEditScreenAdr(a6),a0
	add.l	d0,a0
	asl.l	#4,d1
	add.l	d1,a0
	rts
.putcursor:
	clr.l	d2
	clr.l	d3
	clr.l	d4
	clr.l	d5
	move.b	CheckMemEditXpos(a6),d2
	move.b	CheckMemEditYpos(a6),d3
	move.b	CheckMemEditOldXpos(a6),d4
	move.b	CheckMemEditOldYpos(a6),d5
	cmp.b	d2,d4
	bne	.notequal
	cmp.b	d3,d5
	bne	.notequal			; ok cursorpos have changed.. lets put a nonrevesed char in spot
.equal:
	move.l	#10,d0
	move.l	#4,d1
	mulu	#3,d2
	add.l	d2,d0
	add.l	d3,d1
	bsr	SetPos				; First byte
	move.b	CheckMemEditXpos(a6),d0
	move.b	CheckMemEditYpos(a6),d1
	bsr	.getcursoradr
	clr.l	d0
	move.b	(a0),d0
	move.l	d0,d7				; Store d0 to d7 temporary
	bsr	binhexbyte
	move.l	#11,d1
	bsr	Print				; Print what is in current memorypos as a HEX digit and yellow.
	move.l	#60,d0
	move.l	#4,d1
	add.b	CheckMemEditXpos(a6),d0
	add.b	CheckMemEditYpos(a6),d1
	bsr	SetPos
	move.b	CheckMemEditXpos(a6),d0
	move.b	CheckMemEditYpos(a6),d1
	bsr	.getcursoradr
	clr.l	d0
	move.b	(a0),d0
	bsr	MakePrintable
	move.l	#11,d1
	bsr	PrintChar
	move.l	#17,d0
	move.l	#25,d1
	bsr	SetPos
	move.l	a0,d0
	bsr	binhex
	move.l	#3,d1
	bsr	Print	
	move.l	#52,d0
	move.l	#25,d1
	bsr	SetPos
	move.l	d7,d0				; restore d0 with value from current pos
	bsr	binstringbyte
	move.l	#3,d1
	bsr	Print
	rts
.notequal:					; We had movement.  lets put stuff to "normal" case
	clr.b	CheckMemEditCharPos(a6)	; Clear charpos
	move.b	d2,CheckMemEditOldXpos(a6)
	move.b	d3,CheckMemEditOldYpos(a6)	; Set current pos to "old" pos
	move.l	d4,d7				; Copy d4 to d7 so we do not screw up data for later
	move.l	#10,d0
	move.l	#4,d1
	mulu	#3,d7				; Multiply X pos with 3 so we have space for 2 hexchars and a space
	add.l	d7,d0
	add.l	d5,d1
	PUSH					; Store this in stack, we will need it later
	bsr	SetPos				; Put cursor on screen
	move.l	d4,d0
	move.l	d5,d1
	bsr	.getcursoradr			; Get what memoryaddress we are pointing on
	clr.l	d0
	move.b	(a0),d0
	bsr	binhexbyte
	move.l	#7,d1
	bsr	Print				; Print that byte.
	POP					; ok roll back stack, we will need this data again
	move.l	d4,d0
	move.l	d5,d1
	add.l	#60,d0
	add.l	#4,d1
	bsr	SetPos
	move.l	d4,d0
	move.l	d5,d1
	bsr	.getcursoradr
	clr.l	d0
	move.b	(a0),d0
	bsr	MakePrintable
	move.l	#7,d1
	bsr	PrintChar
	bra	.equal
	PUSH					; Store in stack
	bsr	SetPos
	POP					; ok. d4 and d5 still contains x and y
	move.l	d4,d0
	move.l	d5,d1
	bsr	.getcursoradr
	add.l	#10,d0
	add.l	#4,d1
	bsr	SetPos
	move.b	(a0),d0
	bsr	binhexbyte
	move.l	#7,d1
	bsr	Print
	bra	.equal
.GotoMem:
	clr.b	CheckMemEditCharPos(a6)	; Clear charpos
	move.l	#0,d0
	move.l	#3,d1
	bsr	SetPos
	lea	CheckMemEditGotoTxt,a0
	move.l	#2,d1
	bsr	Print
	move.l	CheckMemEditScreenAdr(a6),d0	; Read the screenaddress currently showed
	add.l	#$150,d0			; Add $150 to that, (next screen)
	move.l	d0,a0
	bsr	InputHexNum
	cmp.l	#-1,d0
	beq	.exit
	move.l	d0,CheckMemEditScreenAdr(a6)	; Store address in memory
	move.l	d0,a0
	bsr	CheckMemEditUpdateScreen	; Update the screen
	bsr	.ClearCommandRow		; Clear the "goto" row.
	bra	.loop
.Cache:
	cmp.b	#1,CPUGen(a6)			; Check if CPUGen is 010 or less
	ble	.Refresh
	bchg	#1,CPUCache(a6)		; Change status of Cacheflag
	clr.l	d0
	move.l	#34,d0
	move.l	#1,d1
	bsr	SetPos
	move.b	CPUCache(a6),d0
	cmp.b	#0,d0				; is it off?
	beq	.CacheOff
						; no, it is on
	lea	ON,a0
	move.l	#2,d1
	bsr	Print
	bsr	EnableCache
	bra	.Refresh
.CacheOff:
	lea	OFF,a0
	move.l	#3,d1
	bsr	Print
	bsr	DisableCache
	bra	.Refresh
.Execute:
	clr.b	CheckMemEditCharPos(a6)	; Clear charpos
	move.l	#0,d0
	move.l	#3,d1
	bsr	SetPos
	lea	CheckMemExecuteTxt,a0
	move.l	#2,d1
	bsr	Print
	move.l	CheckMemEditScreenAdr(a6),d0
	bsr	binhex
	move.l	#3,d1
	bsr	Print
	lea	CheckMemExecuteTxt2,a0
	move.l	#2,d1
	bsr	Print
.Execloop:
	bsr	GetChar
	bclr	#5,d0				; Make it uppercase
	cmp.b	#"Y",d0
	beq	.Executeit
	cmp.b	#"N",d0
	bne.s	.Execloop
	bra	.ExecuteExit
.Executeit:
	move.l	CheckMemEditScreenAdr(a6),a0
	PUSH
	jsr	(a0)				; Doing HARDCORE crash?
	POP
.ExecuteExit:
.Refresh:
	clr.b	CheckMemEditCharPos(a6)	; Clear charpos
	move.l	CheckMemEditScreenAdr(a6),a0	; Get address
	bsr	CheckMemEditUpdateScreen	; Update the screen
	bsr	.ClearCommandRow		; Clear the "goto" row.
	bra	.loop
.exit:
	bra	MemtestMenu
.pgup:
	move.l	CheckMemEditScreenAdr(a6),d0	; Read the screenaddress currently showed
	sub.l	#$150,d0
	move.l	d0,CheckMemEditScreenAdr(a6)	; Store address in memory
	move.l	d0,a0
	bsr	CheckMemEditUpdateScreen
	bra	.keydone
.pgdown:
	move.l	CheckMemEditScreenAdr(a6),d0	; Read the screenaddress currently showed
	add.l	#$150,d0
	move.l	d0,CheckMemEditScreenAdr(a6)	; Store address in memory
	move.l	d0,a0
	bsr	CheckMemEditUpdateScreen
	bra	.keydone
.up:
	cmp.b	#0,CheckMemEditYpos(a6)
	beq	.noup
	sub.b	#1,CheckMemEditYpos(a6)
	bra	.keydone
.noup:
	move.l	CheckMemEditScreenAdr(a6),d0	; Read the screenaddress currently showed
	sub.l	#$10,d0
	move.l	d0,CheckMemEditScreenAdr(a6)	; Store address in memory
	move.l	d0,a0
	bsr	CheckMemEditUpdateScreen
	bra	.keydone
.down:
	cmp.b	#20,CheckMemEditYpos(a6)
	beq	.nodown
	add.b	#1,CheckMemEditYpos(a6)
	bra	.keydone
.nodown:
	move.l	CheckMemEditScreenAdr(a6),d0	; Read the screenaddress currently showed
	add.l	#$10,d0
	move.l	d0,CheckMemEditScreenAdr(a6)	; Store address in memory
	move.l	d0,a0
	bsr	CheckMemEditUpdateScreen
	bra	.keydone
.right:
	cmp.b	#15,CheckMemEditXpos(a6)
	beq	.noright
	add.b	#1,CheckMemEditXpos(a6)
.noright:
	bra	.keydone
.left:
	cmp.b	#0,CheckMemEditXpos(a6)
	beq	.noleft
	sub.b	#1,CheckMemEditXpos(a6)
.noleft:
       bra	.keydone
.ClearCommandRow:
	move.l	#0,d0
	move.l	#3,d1
	bsr	SetPos
	lea	EmptyRowTxt,a0
	bsr	Print
	rts
CheckMemEditUpdateScreen:			; Updates the whole screen with memorydump
						; INDATA:
						;	A0 = Startaddress
	move.l	#1,d0
	move.l	#20,d7
.loop:
	bsr	CheckMemEditUpdateRow
	add.l	#16,a0
	add.l	#1,d0
	dbf	d7,.loop			; Print 21 rows of memorydump on screen
	move.l	#0,d0
	move.l	#25,d1
	bsr	SetPos
	lea	CheckMemAdrTxt,a0
	move.l	#2,d1
	bsr	Print
	move.l	#28,d0
	move.l	#25,d1
	bsr	SetPos
	lea	CheckMemBinaryTxt,a0
	move.l	#2,d1
	bsr	Print
	rts

CheckMemEditUpdateRow:
	;	Show memoryadress on screen
	;	INDATA:
	;		a0 = memory address
	;		d0 = row to update
	PUSH
	move.l	a0,a1				; store a0 in a1 for usage here.. as a0 is used
	add.l	#3,d0				; Add 3 to line to work on.
	move.l	d0,d1				; copy d0 to d1 to use it as Y adress
	clr.l	d0				; clear X pos
	bsr	SetPos				; Set position
	move.l	a0,d0
	bsr	binhex
	move.l	#6,d1
	bsr	Print				; Print address
	clr.l	d2				; Column to print
	move.l	#15,d7
.loop:
	lea	SpaceTxt,a0
	bsr	Print
	clr.l	d0				; Clear d0 just to be sure
	move.b	(a1,d2),d0
	bsr	binhexbyte			; Convert that byte to hex
	move.l	#7,d1
	bsr	Print				; Print it
	add.l	#1,d2
	dbf	d7,.loop
	lea	ColonTxt,a0			; Print a Colon
	move.l	#3,d1
	bsr	Print
	move.l	#15,d7				; Now print the same bytes.  as chars instead	
	clr.l	d2
.loop2:
	clr.l	d0
	move.b	(a1,d2),d0
	bsr	MakePrintable			; make the char printable.  strip controlstuff..
	add.l	#1,d2
	move.l	#7,d1
	bsr	PrintChar
	dbf	d7,.loop2
	POP
	rts


ForceExtended16MBMem:
	bsr	ClearScreen
	lea	MemtestExtMBMemTxt,a0
	move.l	#2,d1
	bsr	Print
	move.b	#14,LogYpos(a6)		; Store first row of log
	lea	MemoryTest16MB,a0
		move.l	#MemTestEndcode-MemoryTest16MB,d0
	jsr	RunCode
	bsr	ClearBuffer
	bsr	WaitPressed
	bsr	WaitReleased
	bra	MemtestMenu
CheckExtended16MBMem:
	bsr	ClearScreen
	lea	MemtestExtMBMemTxt,a0
	move.l	#2,d1
	bsr	Print
	move.b	#14,LogYpos(a6)		; Store first row of log
	lea	MemoryTest16MBQuick,a0
	move.l	#MemTestEndcode-MemoryTest16MBQuick,d0
	jsr	RunCode
	bsr	ClearBuffer
	bsr	WaitPressed
	bsr	WaitReleased
	bra	MemtestMenu
CheckExtendedChip:
	bsr	ClearScreen
	lea	MemtestExtChipTxt,a0
	move.l	#2,d1
	bsr	Print
	move.b	#14,LogYpos(a6)		; Store first row of log
	lea	MemoryTestChipExt,a0
	move.l	#MemTestEndcode-MemoryTestChipExt,d0
	jsr	RunCode
	bsr	ClearBuffer
	bsr	WaitPressed
	bsr	WaitReleased
	bra	MemtestMenu
MemoryTest16MB:
	move.l	#$7000000,CheckMemFrom(a6)
	move.l	#$7ffffff,CheckMemTo(a6)
	move.l	#0,CheckMemStepSize(a6)	; Set how many bytes to step between every memorytest
	bsr	MemTesterInit
	bsr	MemTesterNewBlock
	bsr	MemoryTester
	cmp.w	#1,CheckMemCancel(a6)
	beq	.cancel
	jsr	LogLine
	jsr	LogLine
	lea	AnyKeyMouseTxt,a0
	move.l	#2,d1
	jsr	Print	
.cancel:
	rts	
MemoryTest16MBQuick:
	move.l	#$7000000,CheckMemFrom(a6)
	move.l	#$7ffffff,CheckMemTo(a6)
	move.l	#4096,CheckMemStepSize(a6)	; Set how many bytes to step between every memorytest
	bsr	MemTesterInit
	bsr	MemTesterNewBlock
	bsr	MemoryTester
	cmp.w	#1,CheckMemCancel(a6)
	beq	.cancel
	jsr	LogLine
	jsr	LogLine
	lea	AnyKeyMouseTxt,a0
	move.l	#2,d1
	jsr	Print	
.cancel:
	rts	
MemoryTestChipExt:
	move.l	#$400,d0
	move.l	#$200000,d1
	move.l	d0,CheckMemFrom(a6)
	move.l	d1,CheckMemTo(a6)
	move.l	#0,CheckMemStepSize(a6)	; Set how many bytes to step between every memorytest
	bsr	MemTesterInit
	move.l	#$400,CheckMemFrom(a6)
	move.l	#$1fffff,CheckMemTo(a6)
	bsr	MemTesterNewBlock
	bsr	MemoryTester
	cmp.w	#1,CheckMemCancel(a6)
	beq	.cancel
	jsr	LogLine
	jsr	LogLine
	lea	AnyKeyMouseTxt,a0
	move.l	#2,d1
	jsr	Print	
.cancel:
	rts	
MemoryTestManual:
.passloop:
	bsr	MemTesterInit
.loop:
	bsr	MemTesterNewBlock
	bsr	MemoryTester
	cmp.w	#1,CheckMemCancel(a6)
	beq	.cancel
	bsr	MemTesterNewPass
	bra	.loop
.cancel:
	rts	
MemoryTest:
;	move.w	#0,CheckMemRandom(a6)
;	move.w	#0,CheckMemQuick(a6)
	move.l	#0,CheckMemStepSize(a6)	; Set how many bytes to step between every memorytest
	bsr	MemTesterInit
.loop:
	move.l	#$4000,CheckMemFrom(a6)
	move.l	#$7fff,CheckMemTo(a6)
	bsr	MemTesterNewBlock
	bsr	MemoryTester
	cmp.w	#1,CheckMemCancel(a6)
	beq	.cancel
	move.l	#$7000000,CheckMemFrom(a6)
	move.l	#$7ffffff,CheckMemTo(a6)
	bsr	MemTesterNewBlock
	bsr	MemoryTester
	cmp.w	#1,CheckMemCancel(a6)
	beq	.cancel
	bsr	MemTesterNewPass
	bra	.loop
.cancel:
	rts	
MemoryTester:						; Does the actual real test
	cmp.l	#0,CheckMemPreFail(a6)		; Check if prefail is 0, if not cancel this block
	bne	.passquit
	cmp.w	#1,CheckMemCancel(a6)
	beq	.passquit
	bsr	MemTesterUpdate
.passloop:
	bsr	MemTesterTest
	bsr	MemTesterHandle				; How to handle the result
	move.l	CheckMemStepSize(a6),d6					; Set how much to step to next testlocation
	bsr	MemTesterStep
	cmp.w	#1,CheckMemPassQuit(a6)
	beq	.passquit
	cmp.w	#1,CheckMemCancel(a6)
	bne	.passloop
.passquit:
	clr.w	CheckMemPassQuit(a6)
	rts
MemTesterSkipTest:				; IN:	a2 = current address
						; out: D0 = 0 outside workarea    all other: inside SKIP THIS
	move.l	RunCodeStart(a6),d0
	cmp.l	d0,a2				; This routine check if we are testing workareas. if so tell testroutine to skip this.
	ble	.no				; we assume it as working.
	cmp.l	RunCodeEnd(a6),a2
	bge	.no
	move.l	#-1,d0
	rts
.no:
	move.l	BaseStart(a6),d0
	cmp.l	d0,a2
	ble	.no2
	cmp.l	BaseEnd(a6),a2
	bge	.no2
	move.l	#-1,d0
	rts
.no2:
	clr.l	d0
	rts
MemTesterHandle:					; Lets evaulate the result of the test.	
	cmp.l	#0,d7					; If d7 was 0, we had no errors
	beq	.wasok
							; we had an error..  lets check what type of error. we see this on CheckMemBitError
	cmp.l	#$ffffffff,CheckMemBitError(a6)	; if it was all 1. this is a dead area!
	beq	.wasdead
	move.b	#2,CheckMemType(a6)			; it was not all bits.  we are in a bad area
	bra	.typedone
.wasdead:
	move.b	#3,CheckMemType(a6)
	bra	.typedone
.wasok:
	move.b	#1,CheckMemType(a6)
.typedone:
.runagain:
	clr.l	d2
	move.b	CheckMemType(a6),d2
	move.b	CheckMemOldType(a6),d3
	cmp.b	d3,d2					; Check if we had a change of type
	beq	.notypechange
	move.b	d2,CheckMemOldType(a6)		; Store the new type as the old. we have a copy in d3 for future use
							; Memype is 1=good, 2=bad 3=dead  -1=Scan just started
							; We had a change of type here.. lets handle it
	clr.l	d6					; if null we wasn't at end of block
	cmp.b	#-1,d3					; if we had a -1. no block is ended. just a new is started.
	beq	.juststarted
	cmp.b	#-2,d3
	bne	.notend					; check if it was end of block.. if not go to notend
	cmp.b	#1,d2					; Check if we was in a good block.
	bne	.notend
	move.b	d2,d6					; set d6 to non-zero to tell we was at end of bock
.notend:
	cmp.b	#0,CheckMemTypeEnd(a6)
	bne	.end
	move.b	#1,CheckMemTypeEnd(a6)
	clr.l	d0
	clr.l	d1
	move.b	savexpos(a6),d0
	move.b	saveypos(a6),d1
	jsr	SetPos					; Set cursorpos to the stored position
	clr.l	d1
	move.b	savecol(a6),d1
	move.b	#-1,CheckMemOldType(a6)		; we was at end of a block, so mark this as "just started" and force a restart oftypetest
							; to handle next block
	lea	CheckMemEndAtTxt,a0
	jsr	Print
	move.l	CheckMemCurrent(a6),d0
	sub.l	#1,d0
	jsr	binhex
	jsr	Print
	lea	CheckMemSizeOfTxt,a0
	jsr	Print
	move.l	CheckMemTypeStart(a6),d1
	move.l	CheckMemCurrent(a6),d0
	sub.l	d1,d0
	asr.l	#8,d0
	asr.l	#2,d0				; Divide d0 with 1024 so we know how much memory in kb we got
	jsr	bindec
	clr.l	d1
	move.b	savecol(a6),d1
	jsr	Print				; Print out number of KB
	lea	KB,a0
	jsr	Print				; we have now a Block done...
	clr.l	d0
	cmp.b	#0,d6				;if d6 is 0 we wasn't at end of blockl
	beq	.adrcheck		 	;if not. say last 
	move.b	d6,d3
.adrcheck:
	cmp.b	#1,d3				; Check if it was a good block
	bne	.notgoodblock
	bsr	.checkgoodblock
	bra	.runagain
.notgoodblock:
	bra	.notypechange
.checkgoodblock:
	jsr	LogLine
	lea	CheckMemGoodBlockTxt,a0
	jsr	Print
	bsr	.addresscheck
.end:
	rts
.juststarted:
	move.l	CheckMemCurrent(a6),d0
	cmp.l	CheckMemTo(a6),d0
	bge	.notypechange			; if we was outside the testarea just exit
	cmp.b	#1,d2				; Check if type was Good
	bne	.notgood
	clr.b	CheckMemTypeEnd(a6)
	jsr	LogLine				; Start a new logline
	lea	CheckMemGoodTxt,a0
	move.l	#2,d1
	jsr	Print
	move.l	CheckMemCurrent(a6),d0
	move.l	d0,CheckMemTypeStart(a6)
	jsr	binhex
	jsr	Print
	bra	.startdone
.notgood:
	cmp.b	#2,d2				; Check if type was Good
	bne	.notbad
	jsr	LogLine				; Start a new logline
	lea	CheckMemBadTxt,a0
	move.l	#5,d1
	jsr	Print
	move.l	CheckMemCurrent(a6),d0
	jsr	binhex
	jsr	Print
	bra	.startdone
.notbad:
	cmp.b	#3,d2				; Check if type was Good
	bne	.notdead
	jsr	LogLine				; Start a new logline
	lea	CheckMemDeadTxt,a0
	move.l	#1,d1
	jsr	Print
	move.l	CheckMemCurrent(a6),d0
	jsr	binhex
	jsr	Print
	bra	.startdone
.notdead:
.startdone:
	move.b	d1,savecol(a6)
	jsr	GetPos
	move.b	d0,savexpos(a6)
	move.b	d1,saveypos(a6)
.notypechange:
	rts
.addresscheck:					; Check for addresserrors in block.
	jsr	LogLine
	lea	CheckMemAdrFillTxt,a0
	move.l	#3,d1
	jsr	Print
	move.l	CheckMemTypeStart(a6),a1
	move.l	CheckMemCurrent(a6),a2
	move.l	a1,d7
	move.l	a2,d6
	sub.l	d7,d6				;d6 now contains how many bytes to handle
	asr.l	#2,d6
	asr.l	#5,d6
	clr.l	d5				; Clear d5 as we will use it as a counter
	sub.l	#4,a2				; Subtract one longword at end. as we will write at the LAST longword
	move.l	CheckMemAdrRnd(a6),d2
							; Memory is now filled with addressdata
.filldata:					; Fill area with its memaddress.  do it backwards as that usually screws up when addressbits is bad
	add.l	#1,d5
	cmp.l	d5,d6				; if d5 is equal to d6, print a dot
	bne	.nodot
	clr.l	d5
	move.l	#".",d0
	move.l	#3,d1
	jsr	PrintChar
.nodot:
	jsr	MemTesterSkipTest
	beq	.doit
	sub.l	#4,a2				; skip this
	bra	.done
	move.l	a2,d7
.doit:
	sub.l	#4,a2				; subtract memadress to write to
	move.l	a2,d3				
	eor.l	d2,d3				; Eor with D2 that contains the random number. by doing this. old data will be "invalid"
	move.l	d3,(a2)				; Write address to ram
.done:
	cmp.l	a1,a2
	bge	.filldata
	jsr	LogLine
	lea	CheckMemAdrCheckTxt,a0
	move.l	#3,d1
	jsr	Print
						; Lets check if it is the same, if there is an addresserror it will not be.
	move.l	CheckMemTypeStart(a6),a2
	move.l	CheckMemCurrent(a6),a1
	sub.l	#4,a1
	lea	0,a4				; clear a4, is is used as a flag. if anything else than 0. we had an error
	clr.l	d5
	clr.l	d3				;d3 will contain a mask of all tested data
	clr.l	d2
.checkdata:
	add.l	#1,d5
	cmp.l	d5,d6				; if d5 is equal to d6, print a dot
	bne	.nodot2
	clr.l	d5
	cmp.l	#0,a4				; Check if there was an error in last block
	beq	.noerr
	move.l	#"E",d0
					;KUK
	move.l	#1,d1				; if so. print dot in red
	bra	.print
.noerr:
	move.l	#2,d1
	move.l	#".",d0
.print:	jsr	PrintChar
	clr.l	d7				; Clear d7
.nodot2:
	add.l	#4,a2
	jsr	MemTesterSkipTest
	cmp.w	#0,d0
	beq	.doit2
	move.l	a2,d4
	bra	.done2
.doit2:
	sub.l	#4,a2				; ok we cheated some.  fooled the checkroutine that we was 4 bytes longer than expected. lets fix later
	move.l	a2,d4
	move.l	(a2)+,d0
	move.l	CheckMemAdrRnd(a6),d1
	eor.l	d1,d0
	cmp.l	d0,d4
	beq	.done2
	add.l	#1,a4				; add 1 for each error

	or.l	d4,d2
	bra	.done3
.done2:
	or.l	d4,d3
.done3:
	move.l	a1,a3
	cmp.l	a2,a3
	bgt	.checkdata
	move.l	CheckMemTypeStart(a6),d6
	move.l	CheckMemCurrent(a6),d7
	sub.l	d6,d7				; D7 will contain size of block
	cmp.l	#0,d2
	beq	.noerror
	eor.l	d2,d3
	bra	.error
.noerror:
	add.l	d7,CheckMemUsable(a6)		; Add block as usable ram
	PUSH
	clr.l	CheckMemBitError(a6)
	bsr	MemTesterUpdate
	POP
	clr.l	d3
	bra	.runagain
.error:						; Test is done
	jsr	LogLine
	lea	RamAdrErrTxt,a0
	move.l	#5,d1
	jsr	Print
	jsr	LogLine
	move.l	d3,d0
	or.l	d3,CheckMemAdrError(a6)
	move.l	#1,d1
	jsr	binstring
	jsr	Print
	lea	RamAdrErrSkipTxt,a0
	jsr	Print
	move.l	d7,d0
	add.l	d7,CheckMemNonUsable(a6)	; Mark block as nonusable
	PUSH
	bsr	MemTesterUpdate
	POP
	rts
MemTesterTest:					; Does the actual memorytesting of this address
	clr.l	d7
	movem.l a0-a6/d0-d6,-(a7)		;Store all registers in the stack	except d7 thats why we do not use PUSH
	move.l	CheckMemCurrent(a6),a0	; Load a0 with current address
	move.l	a0,a2				; as the skiptest routine requires address in a2....
	jsr	MemTesterSkipTest
	beq	.doit
	clr.l	d7				; We are in a workarea, skip this assumeall is ok!
	bra	.skiptest
.doit:
	move.l	(a0),d0				; make a backup of memorycontent
	lea	MEMCheckPattern,a1
.testloop:
	move.l	(a1)+,d2				; Load d2 with value to test
	move.l	d2,(a0)				; write it to RAM.
	move.l	#"CRAP",4(a0)			; Write "CRAP" to next longword. just to put crap in databus so stuck buffer will not give fale posetive
	nop
	nop					; Just 2 nops here.  040 etc might want this.
	move.l	(a0),d3				; load from ram to d3.  BUT do it several times, just to be sure we read correct value.
						; broken chips can report diferent values everytime, but first often the "wanted" one.
	move.l	(a0),d3
	move.l	(a0),d3				; ok this shold be enough.. lets trust d3 now contain what it thinks is in memory
	cmp.l	d3,d2				; Compare if they are equal.
	bne	.error
.back:
	cmp.l	#0,d2				; Check if we was at end of testlist
	bne	.testloop			; if not.  test next value
	move.l	d0,(a0)				; Restore memory
.skiptest:
	movem.l (a7)+,a0-a6/d0-d6		;Restore the registers from the stack
	rts
.error:
       move.l	d3,d4
	eor.l	d2,d4				; D4 bits that differs
	or.l	d4,CheckMemBitError(a6)	; or it into register to get a complete list of errors
	move.l	d3,d5
	and.l	d2,d5
	eor.l	d3,d5				; D5 all wrong HIGH bits
	or.l	d5,CheckMemHighError(a6)
	move.l	d5,d6
	eor.l	d4,d6				; D6 all wrong LOW bits
	or.l	d6,CheckMemLowError(a6)
	move.l	#1,d7				; Set d7 to 1 to mark we had an error
	bra	.back
MemTesterStep:
	move.l	CheckMemCurrent(a6),d0
	move.l	CheckMemTo(a6),d2
	cmp.l	d0,d2				; Check if we are done with the block
	blt	.passdone
	move.l	CheckMemBlockDone(a6),d1
	cmp.l	#$2000,d1			; Check if it is time to update
	blt	.noupdate
	clr.l	CheckMemBlockDone(a6)
	jsr	GetInput
	cmp.b	#1,BUTTON(a6)
	beq	.passquit
	PUSH
	TOGGLEPWRLED
	bsr	MemTesterUpdate
	POP
.noupdate:
	add.l	d6,CheckMemBlockDone(a6)	; Add to how much of block is done
	add.l	d6,CheckMemCurrent(a6)	; Add to next adress to test
	add.l	d6,CheckMemChecked(a6)
	cmp.l	#1,d7				; Check if d7 was 1 then we had a error on memtest, assume this whole block is bad
	bne	.noerr
	add.l	#1,CheckMemErrors(a6)
	add.l	d6,CheckMemNonUsable(a6)
.noerr:
	rts
.passdone:
	move.b	#-2,CheckMemOldType(a6)
	bsr	MemTesterHandle
	move.w	#1,CheckMemPassQuit(a6)
	rts
.passquit:
	move.l	d0,CheckMemCancelReason(a6)
	move.w	#1,CheckMemCancel(a6)
	clr.l	CheckMemStepSize(a6)		; clear the stepsize..
	bsr	LogLine
	lea	CheckMemCancelled,a0
	move.l	#6,d1
	jsr	Print
	move.l	CheckMemCancelReason(a6),d0
	btst.l	#3,d0
	bne	.serial
	btst	#2,d0
	bne	.key
	btst	#1,d0
	bne	.mouse
	lea	OtherPressTxt,a0
	bra	.reasondone
.serial:
	lea	SerialPressTxt,a0
	bra	.reasondone
.key:
	lea	KeyPressTxt,a0
	bra	.reasondone
.mouse:
	lea	MousePressTxt,a0
	bra	.reasondone
.reasondone:	
	move.l	#2,d1
	jsr	Print
	bsr	LogLine
	bsr	LogLine
	lea	AnyKeyMouseTxt,a0
	move.l	#2,d1
	jsr	Print
	clr.l	RunCodeStart(a6)		; make sure start of runcode is cleared for future use
	rts
MemTesterNewBlock:	
	clr.l	CheckMemPreFail(a6)		; Clear the prefail flag
	move.l	CheckMemFrom(a6),d0
	move.l	d0,d1
	and.l	#$ffffff,d1
	cmp.l	d0,d1				; are those the same.  then do not test if we have some 24bit adr. issue
	beq	.no24bit
	move.l	d1,a1
	move.l	d0,a0
	move.l	#"TEST",(a0)
	cmp.l	#"TEST",(a1)			; check if we get the same data at a1.  this means we are reading same data within 24bit adr.
	bne	.no24bit
	move.l	#-1,CheckMemPreFail(a6)	; Set the prefailflag to -1 telling we had an error
	jsr	LogLine
	lea	CheckMem24bitTxt,a0
	move.l	#5,d1
	jsr	Print
	move.l	CheckMemFrom(a6),d0
	jsr	binhex
	jsr	Print
	jsr	LogLine
	lea	CheckMem24bitTxt2,a0
	move.l	#1,d1
	jsr	Print
	bra	.nope
.no24bit:
	move.b	#-1,CheckMemOldType(a6)
	clr.l	CheckMemBitError(a6)
	clr.l	CheckMemHighError(a6)
	clr.l	CheckMemLowError(a6)
	move.l	#21,d0
	move.l	#3,d1
	jsr	SetPos
	move.l	CheckMemFrom(a6),d0
	jsr	binhex
	move.l	#2,d1
	jsr	Print
	move.l	#34,d0
	move.l	#3,d1
	jsr	SetPos
	move.l	CheckMemTo(a6),d0
	jsr	binhex
	move.l	#2,d1
	jsr	Print
	move.l	CheckMemFrom(a6),d0
	move.l	d0,CheckMemCurrent(a6)
.nope:
	rts
						; Update passes
MemTesterUpdate:				; Update from and to
	move.l	CheckMemPass(a6),d0
	cmp.l	CheckMemPassOLD(a6),d0
	beq	.passchange
	move.l	d0,CheckMemPassOLD(a6)
	jsr	bindec
	move.l	#12,d0
	move.l	#4,d1
	jsr	SetPos
	move.l	#2,d1
	jsr	Print
.passchange:
	move.l	CheckMemPassOK(a6),d0
	cmp.l	CheckMemPassOKOLD(a6),d0
	beq	.passchangeok
	move.l	d0,CheckMemPassOKOLD(a6)
	jsr	bindec
	move.l	#34,d0
	move.l	#4,d1
	jsr	SetPos
	move.l	#2,d1
	jsr	Print
.passchangeok:
	move.l	CheckMemPassFail(a6),d0
	move.l	d0,d7
	cmp.l	CheckMemPassFailOLD(a6),d0
	beq	.passchangeerror
	move.l	d0,CheckMemPassFailOLD(a6)
	jsr	bindec
	move.l	#57,d0
	move.l	#4,d1
	jsr	SetPos
	move.l	#2,d1
	cmp.l	#0,d7
	beq.s	.wasok
	move.l	#1,d1
.wasok:	
	jsr	Print
.passchangeerror:
	move.l	CheckMemCurrent(a6),d0
	cmp.l	CheckMemCurrentOLD(a6),d0
	beq	.current
	move.l	d0,CheckMemCurrentOLD(a6)
	jsr	binhex
	move.l	#18,d0
	move.l	#5,d1
	jsr	SetPos
	move.l	#3,d1
	jsr	Print
.current:
	move.l	CheckMemErrors(a6),d0
	move.l	CheckMemErrorsOLD(a6),d1
	cmp.l	d1,d0
	beq	.noerrors
	move.l	d0,d7
	move.l	d0,CheckMemErrorsOLD(a6)
	jsr	bindec
	move.l	#56,d0
	move.l	#7,d1
	jsr	SetPos
	move.l	#2,d1
	cmp.l	#0,d7
	beq	.noerr
	move.l	#1,d1
.noerr:
	jsr	Print
	move.l	#13,d0
	move.l	#8,d1
	jsr	SetPos
	lea	OK,a0
	move.l	#31,d6
	clr.l	d7
	move.l	CheckMemBitError(a6),d2	; Load what bits HAD errors
	move.l	CheckMemHighError(a6),d3	; Load what bits had stuck 1
	move.l	CheckMemLowError(a6),d4	; Load what bits had stuck 0
	move.l	d4,d5
	and.l	d3,d5				; D5 will now contain what bits had BOTH stuck 0 and 1 (varying bit)
.bitloop:
	cmp.w	#8,d7				; If this is the 8th char, do 2 spaces
	bne	.nospace
	lea	SpacesTxt,a0
	jsr	Print
	clr.l	d7				; Clear charcounter
.nospace:
	addq	#1,d7				; Add one to charcounter
	btst	d6,d2				; Check bit d6 of register to see error
	bne	.yeserr
	move.l	#"-",d0				; It was no error, so we print a green X
	move.l	#2,d1
	jsr	PrintChar
	bra	.errdone
.yeserr:					; OK we had an error
	btst	d6,d5				; Check if bit had both 0 or 1.
	bne.s	.yesboth
	btst	d6,d3				; Check for stuck 1
	bne.s	.yesone
						; as it wasn't 1.  and wasn't both. lets print as stuck 0
	move.l	#"0",d0
	move.l	#1,d1
	jsr	PrintChar
	bra	.errdone
.yesone:
	move.l	#"1",d0
	move.l	#1,d1
	jsr	PrintChar
	bra	.errdone
.yesboth:
	move.l	#"X",d0
	move.l	#1,d1
	jsr	PrintChar
.errdone:
	dbf	d6,.bitloop
.noerrors:
	move.l	CheckMemChecked(a6),d0
	cmp.l	CheckMemCheckedOLD(a6),d0
	beq	.nochecked
	move.l	d0,CheckMemCheckedOLD(a6)
	move.l	#16,d0
	move.l	#13,d1
	jsr	SetPos
	move.l	#2,d1
	move.l	CheckMemChecked(a6),d0
	jsr	ToKB
	jsr	bindec
	jsr	Print
.nochecked:
	move.l	CheckMemUsable(a6),d0
	cmp.l	CheckMemUsableOLD(a6),d0
	beq	.nousable
	move.l	d0,CheckMemUsableOLD(a6)

	move.l	#41,d0
	move.l	#13,d1
	jsr	SetPos
	move.l	#2,d1
	move.l	CheckMemUsable(a6),d0
	jsr	ToKB
	jsr	bindec
	jsr	Print
.nousable:
	move.l	CheckMemNonUsable(a6),d0
	cmp.l	CheckMemNonUsableOLD(a6),d0
	beq	.nonusable
	move.l	d0,CheckMemNonUsableOLD(a6)
	move.l	#70,d0
	move.l	#13,d1
	jsr	SetPos
	move.l	#2,d1
	move.l	CheckMemNonUsable(a6),d0
	move.l	d0,d7
	jsr	ToKB
	jsr	bindec
	move.l	#2,d1
	cmp.l	#0,d7
	beq	.noerr2
	move.l	#1,d1
.noerr2:
	jsr	Print
.nonusable:
	move.l	CheckMemAdrError(a6),d0
	cmp.l	CheckMemAdrErrorOLD(a6),d0
	beq	.noadr
	TOGGLEPWRLED
	move.l	d0,d7
	move.l	d0,CheckMemAdrErrorOLD(a6)
	move.l	#13,d0
	move.l	#11,d1
	jsr	SetPos
	move.l	d7,d3
	clr.l	d7
	move.l	#31,d6
.adrloop:
	cmp.b	#8,d7
	bne	.noadrspace
	lea	SpacesTxt,a0
	jsr	Print
	clr.l	d7				; Clear charcounter
.noadrspace:
	add.b	#1,d7
	btst	d6,d3
	bne	.adrerr
	move.l	#"-",d0
	move.l	#2,d1
	bra	.adrnoerr
.adrerr:
	move.l	#"E",d0
	move.l	#1,d1
.adrnoerr:
	jsr	PrintChar
	dbf	d6,.adrloop
.noadr:
	rts
MemTesterClear:
	clr.l	CheckMemBlockDone(a6)
	clr.w	CheckMemCancel(a6)
	clr.l	CheckMemNoErrors(a6)		; Clear number of errors
	clr.l	CheckMemAdrError2(a6)
	clr.l	CheckMemNonUsable(a6)
	clr.l	CheckMemErrors(a6)
	clr.l	CheckMemChecked(a6)
	clr.l	CheckMemUsable(a6)
	clr.l	CheckMemCancelReason(a6)
	rts
MemTesterInit:
	bsr	MemTesterClear
	jsr	Random				; Create a random number
	move.l	d0,CheckMemAdrRnd(a6)		; Store it as a token for addresserror test
	clr.l	CheckMemPassFail(a6)
	clr.l	CheckMemPassOK(a6)
	clr.l	CheckMemPass(a6)
	move.l	#31,d7
.clearloop:
	clr.b	(a0)+
	dbf	d7,.clearloop
	move.l	#-1,CheckMemPassOKOLD(a6)
	move.l	#-1,CheckMemPassOLD(a6)
	move.l	#-1,CheckMemPassFailOLD(a6)
	move.l	#-1,CheckMemCurrentOLD(a6)
	move.l	#-1,CheckMemCheckedOLD(a6)
	move.l	#-1,CheckMemErrorsOLD(a6)
	move.l	#-1,CheckMemUsableOLD(a6)
	move.l	#-1,CheckMemNonUsableOLD(a6)
	move.l	#-1,CheckMemAdrErrorOLD(a6)
	move.b	#-1,CheckMemOldType(a6)
       cmp.l	#4,CheckMemStepSize(a6)
	bge	.sizeok
	move.l	#4,CheckMemStepSize(a6)	; we had a too low size. so change it to 4
.sizeok:
	jsr	ClearScreen
	clr.l	MemTestPass(a6)
	lea	NewLineTxt,a0
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	lea	CheckMemRangeTxt,a0
	move.l	#7,d1
	jsr	Print				; Print checking memory from...
	lea	NewLineTxt,a0
	jsr	Print
	lea	CheckMemNo,a0
	move.l	#7,d1
	jsr	Print				; Print checking memory from...
	lea	NewLineTxt,a0
	jsr	Print
	lea	CheckMemCheckAdrTxt,a0
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	lea	CheckMemBitErrsTxt,a0
	move.l	#7,d1
	jsr	Print				; Print Bit error shows max....
	lea	CheckMemDBitErrorsTxt,a0
	move.l	#3,d1				; Print Biterros and byte errors
	jsr	Print
	move.l	#45,d0
	move.l	#5,d1
	jsr	SetPos
	lea	CheckMemStepSizeTxt,a0
	move.l	#2,d1
	jsr	Print
	move.l	CheckMemStepSize(a6),d0
	jsr	bindec
	move.l	#2,d1
	jsr	Print
	move.l	#0,d0
	move.l	#9,d1
	jsr	SetPos
	lea	CheckMem16bitTxt,a0
	move.l	#5,d1
	jsr	Print
	lea	CheckMemABitErrorsTxt,a0
	move.l	#3,d1				; Print Biterros and byte errors
	jsr	Print
	move.l	#0,d0
	move.l	#12,d1
	jsr	SetPos
	lea	CheckMemAdrErrTxt,a0
	move.l	#5,d1
	jsr	Print
	move.l	#56,d0
	move.l	#6,d1
	jsr	SetPos
	lea	CheckMemNumErrTxt,a0
	move.l	#3,d1
	jsr	Print				; Print "Number of errors"
	bra	.nofast
.fastmode:
	lea	CheckMemFastModeTxt,a0
	move.l	#2,d1
	jsr	Print
.nofast:
	move.l	#0,d0
	move.l	#13,d1
	jsr	SetPos
	lea	CheckMemCheckedTxt,a0
	move.l	#6,d1
	jsr	Print
	move.l	#26,d0
	move.l	#13,d1
	jsr	SetPos
	lea	CheckMemUsableTxt,a0
	move.l	#6,d1
	jsr	Print
	move.l	#52,d0
	move.l	#13,d1
	jsr	SetPos
	lea	CheckMemNonUsableTxt,a0
	move.l	#6,d1
	jsr	Print
	clr.l	d0
	move.l	#14,d1
	jsr	SetPos
	lea	DividerTxt,a0
	move.l	#4,d1
	jsr	Print
	clr.l	CheckMemPassOK(a6)
	clr.l	CheckMemPassFail(a6)
	move.l	RunCodeStart(a6),d0
	cmp.l	#0,d0
	beq	.skipcode				; if it was 0.  we skipped even to try to run in ram
	jsr	LogLine
	lea	CheckMemCodeAreaTxt,a0
	move.l	#7,d1
	jsr	Print
	move.l	RunCodeStart(a6),d0
	jsr	binhex
	jsr	Print
	lea	MinusTxt,a0
	jsr	Print
	move.l	RunCodeEnd(a6),d0
	jsr	binhex
	jsr	Print
.skipcode:
	jsr	LogLine
	lea	CheckMemWorkAreaTxt,a0
	move.l	#7,d1
	jsr	Print
	move.l	BaseStart(a6),d0			; Get startaddress of chipmem
	jsr	binhex
	jsr	Print
	lea	MinusTxt,a0
	jsr	Print
	move.l	BaseEnd(a6),d0			; Get startaddress of chipmem
	jsr	binhex
	jsr	Print
							; Directly after the init.  we do a "new pass"
MemTesterNewPass:
	add.l	#1,CheckMemPass(a6)
	cmp.l	#1,CheckMemPass(a6)			; Check if we are in the first pass, then do not bother checking for result
	beq	.wehaderr				; by jumping to "wehaderr"  not correct label but correct location
	bsr	LogLine
	move.l	#"-",d0
	move.l	#6,d1
	jsr	PrintChar
	lea	CheckMemCheckedTxt,a0
	jsr	Print
	move.l	CheckMemChecked(a6),d0
	jsr	ToKB
	jsr	bindec
	jsr	Print
	lea	KB,a0
	jsr	Print
	lea	SpaceTxt,a0
	jsr	Print
	lea	CheckMemUsableTxt,a0
	move.l	#6,d1
	jsr	Print
	move.l	CheckMemUsable(a6),d0
	jsr	ToKB
	jsr	bindec
	jsr	Print
	lea	KB,a0
	jsr	Print
	lea	SpaceTxt,a0
	jsr	Print
	lea	CheckMemNonUsableTxt,a0
	move.l	#6,d1
	jsr	Print
	move.l	CheckMemNonUsable(a6),d0
	jsr	ToKB
	jsr	bindec
	jsr	Print
	lea	KB,a0
	jsr	Print
       move.l	#16,d0
	move.l	#13,d1
	jsr	SetPos
	lea	TenSpacesTxt,a0
	move.l	#1,d1
	jsr	Print
	move.l	#41,d0
	move.l	#13,d1
	jsr	SetPos
	lea	TenSpacesTxt,a0
	move.l	#1,d1
	jsr	Print
	move.l	#70,d0
	move.l	#13,d1
	jsr	SetPos
	lea	TenSpacesTxt,a0
	move.l	#1,d1
	jsr	Print					; Now we have cleaed the texts. so we can begin from scratch
	move.l	#56,d0
	move.l	#7,d1
	jsr	SetPos
	lea	TenSpacesTxt,a0
	move.l	#1,d1
	jsr	Print
	bsr	LogLine
	cmp.l	#0,CheckMemErrors(a6)
	beq	.noerr
	add.l	#1,CheckMemPassFail(a6)
	clr.l	CheckMemErrors(a6)
	bra	.wehaderr
.noerr:
	add.l	#1,CheckMemPassOK(a6)
.wehaderr:
	bsr	MemTesterClear
	rts
LogLine:					; Sets new line of log to print at
	PUSH
	clr.l	d1
	move.b	LogYpos(a6),d1
	add.b	#1,d1
	cmp.b	#31,d1
	beq	.endline
.setline:
	clr.l	d0
	jsr	SetPos
	move.b	d1,LogYpos(a6)
	POP
	rts
.endline:
	move.l	#15,d0
	jsr	DeleteLine
	sub.b	#1,d1
	bra	.setline
MemTestEndcode:
