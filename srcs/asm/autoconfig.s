	include "earlymacros.i"
	include "build/srcs/globalvars.i"
	section "autoconfig",code_p
	xdef	AutoConfig
	xdef	AutoConfigDetail
	xdef	DoAutoconfig

AutoConfig:	
				; Do Autoconfigmagic
	jsr	ClearScreen
	move.b	#0,AutoConfMode(a6)		; Set that we do not want a more detailed autoconfig mode
	jsr	DoAutoconfig
	jsr	PrintBoards
	lea	NewLineTxt,a0
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	jsr	_ClearBuffer
	lea	AnyKeyMouseTxt,a0
	move.l	#3,d1
	jsr	Print
	jsr	_WaitButton
	jmp	MemtestMenu


AutoConfigDetail:				; Do Autoconfigmagic
	jsr	ClearScreen
	move.b	#1,AutoConfMode(a6)		; Set that we want a more detailed autoconfig mode
	jsr	DoAutoconfig

	jsr	PrintBoards
	lea	NewLineTxt,a0
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	jsr	_ClearBuffer


	lea	AnyKeyMouseTxt,a0
	move.l	#3,d1
	jsr	Print
	jsr	_WaitButton
	jmp	MainMenu


; Autoconfigcode.  based much Terriblefires code, Added support for several cards
; and more information.

E_EXPANSIONBASE		EQU	$e80000
EZ3_EXPANSIONBASE	EQU	$ff000000

ERT_TYPEMASK		EQU	$c0	;Bits 7-6
ERT_TYPEBIT		EQU	6
ERT_TYPESIZE		EQU	2
ERT_NEWBOARD		EQU	$c0
ERT_ZORROII		EQU	ERT_NEWBOARD
ERT_ZORROIII		EQU	$80
; ** other bits defined in er_Type **
; ** er_Type field memory size bits ** 
ERT_MEMMASK		EQU	$07	;Bits 2-0
ERT_MEMBIT		EQU	0
ERT_MEMSIZE		EQU	3
	
			rsreset
er_Type 		rs.b	1	;Board type, size and flags
er_Product		rs.b	1	;Product number, assigned by manufacturer
er_Flags		rs.b	1	;Flags
er_Reserved03		rs.b	1	;Must be zero ($ff inverted)
er_Manufacturer 	rs.w	1	;Unique ID,ASSIGNED BY COMMODORE-AMIGA!
er_SerialNumber 	rs.l	1	;Available for use by manufacturer
er_InitDiagVec	rs.w	1	;Offset to optional "DiagArea" structure
er_Reserved0c		rs.b	1
er_Reserved0d		rs.b	1
er_Reserved0e		rs.b	1
er_Reserved0f		rs.b	1
ExpansionRom_SIZEOF	rs.b	0

			rsreset
ec_Interrupt		rs.b	1	;Optional interrupt control register
ec_Z3_HighBase	rs.b	1	;Zorro III   : Bits 24-31 of config address
ec_BaseAddress	rs.b	1	;Zorro II/III: Bits 16-23 of config address
ec_Shutup		rs.b	1	;The system writes here to shut up a board
ec_Reserved14		rs.b	1
ec_Reserved15		rs.b	1
ec_Reserved16		rs.b	1
ec_Reserved17		rs.b	1
ec_Reserved18		rs.b	1
ec_Reserved19		rs.b	1
ec_Reserved1a		rs.b	1
ec_Reserved1b		rs.b	1
ec_Reserved1c		rs.b	1
ec_Reserved1d		rs.b	1
ec_Reserved1e		rs.b	1
ec_Reserved1f		rs.b	1
ExpansionControl_SIZEOF rs.b	0

DoAutoconfig:
	lea	AutoConfBuffer(a6),a2
	move.b	#$20,AutoConfZ2Ram(a6)
	move.w	#$4000,AutoConfZ3(a6)	; Set defaultvalues for different cardtypes
	move.b	#$20,AutoConfZ2Ram(a6)
	move.b	#$e9,AutoConfZ2IO(a6)
	lea	AutoConfZ2Txt,a0
	move.l	#6,d1
	jsr	Print


	move.l	#1,d6			; Clear boardnumber
.loopz2:
	lea	E_EXPANSIONBASE,a0
	jsr	.ReadRom
	cmp.b	#0,AutoConfType(a6)	; Check type of card, if 0, no card found
	beq	.noz2	
	lea	E_EXPANSIONBASE,a0
	jsr	.WriteByte

	add.l	#1,d6
	cmp.l	#32,d6			; if we hit 32 boards.. something is wrong, exit
	bgt	.toomuch
	cmp.b	#0,AutoConfExit(a6)	; Check the force exitflag
	bne	.noz3
	bra	.loopz2
.noz2:


	lea	AutoConfZ3Txt,a0
	move.l	#6,d1
	jsr	Print

.loopz3:
	lea	EZ3_EXPANSIONBASE,a0
	jsr	.ReadRom
	cmp.b	#0,AutoConfType(a6)	; Check type of card, if 0, no card found
	beq	.noz3	
	jsr	.WriteByte
	add.l	#1,d6
	cmp.l	#32,d6			; if we hit 32 boards.. something is wrong, exit
	bgt	.toomuch
	cmp.b	#0,AutoConfExit(a6)	; Check the force exitflag
	bne	.noz3

	bra	.loopz3
.noz3:

	lea	AutoConfAllTxt,a0
	move.l	#6,d1
	jsr	Print

	rts	
.toomuch:
	lea	AutoConfToomuchTxt,a0
	move.l	#1,d1
	jmp	Print

.ReadRom:
	clr.b	AutoConfType(a6)	; Set type to 0 (no card found)
	clr.b	AutoConfZorro(a6)	; Set zorrotype to 2 (0)
	clr.l	AutoConfSize(a6)	; Clear the size of the board

	clr.l	d0
	move.l	a0,a3			; Backup of card
	move.l	a2,a4			; Backup of zorrobuffer
	jsr	.ReadByte

	move.b	d0,(a2)+
	; All other bytes are inverted
	moveq.l	#1,d2
.ReadRomLoop:
	move.l	d2,d0
	move.l	a3,a0			; Huh
	jsr	.ReadByte
	not.b	d0
	move.b	d0,(a2)+
	addq.w	#1,d2
	cmp.w	#ExpansionRom_SIZEOF,d2	; check if we read enough data
	bls.s	.ReadRomLoop

	move.l	a4,a2			; Restore zorrobuffer

	tst.b	er_Reserved03(a2)	; Check if it is 0, if not, we have no card
	bne	.NoCard

	tst	er_Manufacturer(a2)	; Check if it is 0, if so, we have no card
	beq	.NoCard

	move.b	er_Flags(a2),AutoConfFlag(a6)


	cmp.b	#0,AutoConfMode(a6)
	beq	.nodetail


	PUSH
	lea	AutoConfBoardTxt,a0
	move.l	#5,d1
	jsr	Print
	move.l	d6,d0			; Take boardnumber to d0
	jsr	bindec
	move.l	#2,d1
	jsr	Print

	lea	AutoConfManuTxt,a0
	move.l	#3,d1
	jsr	Print

	move.w	er_Manufacturer(a2),d0
	jsr	bindec
	move.w	#2,d1
	jsr	Print

	lea	AutoConfSerTxt,a0
	move.l	#3,d1
	jsr	Print

	move.w	er_SerialNumber(a2),d0
	jsr	bindec
	move.w	#2,d1
	jsr	Print

	lea	AutoConfZorTypeTxt,a0
	move.l	#3,d1
	jsr	Print	

	clr.l	d0			; Print if it is Zorro II or III
	move.b	er_Type(a2),d0
	and.b	#$c0,d0			; Strip out all except top 2 bits
	cmp.b	#$c0,d0
	beq	.readz2
	lea	III,a0
	move.l	#6,d1
	jsr	Print
	bra	.readz3
.readz2:
	lea	II,a0
	move.l	#6,d1
	jsr	Print

.readz3:

	lea	AutoConfLinkTxt,a0
	move.l	#3,d1
	jsr	Print	

	btst	#5,er_Type(a2)		; Check if it is Linked to system pool (RAM)
	beq	.readnomem
	jsr	PrintYes
	bra	.readmem
.readnomem:
	jsr	PrintNo
.readmem:
	lea	AutoConfAutoBTxt,a0
	move.l	#3,d1
	jsr	Print	

	btst	#4,er_Type(a2)		; Check if there is any Autobootstuff
	bne	.readnoboot
	jsr	PrintNo
	bra	.readboot
.readnoboot:
	jsr	PrintYes
.readboot:


	lea	AutoConfLinked2NextTxt,a0
	move.l	#3,d1
	jsr	Print	

	btst	#4,er_Type(a2)		; Check if linked to next card
	beq	.readnolink
	jsr	PrintYes
	bra	.readlink
.readnolink:
	jsr	PrintNo
.readlink:

	lea	AutoConfExtSizeTxt,a0
	move.l	#3,d1
	jsr	Print

	clr.l	d7			; Clear d7 to have as a variable. if changed we have extended size

	btst	#5,er_Flags(a2)		; Check if Extended sizes will be used
	beq	.readnoextsize
	moveq.l	#1,d7			; Set d7 to 1, we have extended sizes
	jsr	PrintYes
	bra	.readextsize
.readnoextsize:
	jsr	PrintNo
.readextsize:
	lea	AutoConfSizeTxt,a0
	move.l	#3,d1
	jsr	Print	

	clr.l	d0
	move.b	er_Type(a2),d0
	move.b	d0,AutoConfFlag(a6)
	and.b	#7,d0			; D0 now contains sizebits
	asl	#2,d0			; Multiply with 4, to get correct location in pointerlist
	lea	SizeTxtPointer,a0
	cmp.b	#0,d7			; Check if d7 is 0, if so we have not extended size
	beq	.readnoext
	lea	ExtSizeTxtPointer,a0
.readnoext:
	move.l	(a0,d0.l),a0		; A0 now points to the correct textstring
	move.l	#2,d1
	jsr	Print

	lea	AutoConfBufTxt,a0
	move.l	#6,d1
	jsr	Print
	move.l	a2,a1
	move.l	#ExpansionRom_SIZEOF-1,d7
.printloop:
	move.b	(a1)+,d0
	jsr	binhexbyte
	move.l	#2,d1
	jsr	Print
	lea	SpaceTxt,a0
	jsr	Print
	dbf	d7,.printloop

	move.l	a4,a2			; Restore backup of zorrobuffer
	POP
.nodetail:

					; ok detailed VERBOSE output done, lets do it "again" quiet and set variables.



	btst	#5,er_Type(a2)		; Check if it is Linked to system pool (RAM)
	beq	.readsetnomem
	move.b	#2,AutoConfType(a6)	; Set type to 2 = RAM
	bra	.readsetmem
.readsetnomem:
	clr.l	d0
	move.b	er_Type(a2),d0
	and	#7,d0
	
	cmp	#2,d0			; Check if space is more than 128K then allocate it to Z2 area instead. (but not ram)
	bge	.readsetz2space


	move.b	#1,AutoConfType(a6)	; Set type to 1 = ROM
.readsetmem:


	clr.l	d0			; Check if it is Zorro II or III
	move.b	er_Type(a2),d0
	and.b	#$c0,d0			; Strip out all except top 2 bits
	cmp.b	#$c0,d0
	beq	.readsetz2
	move.b	#1,AutoConfZorro(a6)
	bra	.readsetz3
.readsetz2space:			; To be assigned in Z2 space, but not RAM
	move.b	#3,AutoConfType(a6)	; Set type to 3 = Z2Space no ram
	bra	.readsetz3

.readsetz2:
	move.b	#0,AutoConfZorro(a6)
	bra	.noz3force
.readsetz3:

	move.b	#1,AutoConfZorro(a6)
.noz3force:

	clr.l	d7			; Clear d7 to have as a variable. if changed we have extended size
	btst	#5,er_Flags(a2)		; Check if Extended sizes will be used
	beq	.readsetnoextsize
	moveq.l	#1,d7			; Set d7 to 1, we have extended sizes
.readsetnoextsize:

	clr.l	d0
	move.b	er_Type(a2),d0
	and.b	#7,d0			; D0 now contains sizebits
	asl	#2,d0			; Multiply with 4, to get correct location in pointerlist


	lea	SizePointer,a0
	cmp.b	#0,d7			; Check if d7 is 0, if so we have not extended size
	beq	.readsetnoext
	lea	ExtSizePointer,a0
.readsetnoext:
	move.l	(a0,d0.l),d0		; D0 now contains the size of the card.

	move.l	d0,AutoConfSize(a6)	; Write the size to the buffer
	rts
.NoCard:
	move.b	#0,AutoConfType(a6)	; Ser that we have no card
	rts

.ReadByte:			; Reads one byte from Cardexpansion.

	jsr	WaitLong	; Put in some wait here, so slow boards can wake up aswell
	jsr	WaitLong	; Put in some wait here, so slow boards can wake up aswell


				; IN:
				; 	D0 = Location into buffer to read
				;	A0 = Card
				;	A2 = Destionationbuffer
				; OUT:
				;	D0 = Byte read

	lsl.w	#2,d0		;	Multiply with 4
	lea.l	0(a0,d0.w),a0	; a0 now contain pointer to real card.

	move.l	a0,d1
	bmi	.Z3		; Check for Z3
	move.b	$2(a0),d1
	bra	.doRead
.Z3:
	move.b	$100(a0),d1
.doRead:
	lsr.b	#4,d1		; Strip away so we just keep a nibble
	moveq.l	#0,d0
	move.b	(a0),d0
	and.b	#$f0,d0		; Strip away so we just keep a nibble
	or.b	d1,d0		; Put those 2 nibbles together, and we get a byte read.
	rts

.WriteByte:				; Write configbyte to configure card. (ok WORD for Z3!)
	clr.b	AutoConfIllegal(a6)	; Clear the illegalflag
	clr.l	d0
	lea	NewLineTxt,a0
	jsr	Print

	move.b	AutoConfType(a6),d0	; Get what type of card
	cmp.b	#0,d0			; No card found
	beq	.exit
	cmp.b	#1,AutoConfZorro(a6)	; Check if Z3 Card
	beq	.WriteZ3
	cmp.b	#1,d0			; Check if Z2 ROM
	beq	.WriteZ2IO
	cmp.b	#3,d0			; Check if Z3 Area card (NO RAM)
	beq	.WriteZ2noram

	lea	AutoConfRamCardTxt,a0	; We got a Z2 RAM Card
	clr.l	d1
	move.b	AutoConfZ2Ram(a6),d1
	move.w	d1,AutoConfWByte(a6)
	move.l	d1,d0
	swap	d0
	move.l	d0,AutoConfAddr(a6)
	add.l	AutoConfSize(a6),d0
	cmp.l	#$a00002,d0
	blo	.writenoz2illegal
	PUSH
	lea	AutoConfIllegalTxt,a0
	move.l	#1,d1
	jsr	Print
	move.b	#1,AutoConfIllegal(a6)	; Set the illegal flag
	POP
.writenoz2illegal:
	swap	d0
	move.b	d0,AutoConfZ2Ram(a6)
	move.l	a3,a1			; Copy backup of expansionbase to a1.
	bra	.Write

.WriteZ2noram:
	lea	AutoConfRomCardTxt,a0	; We got a Z2 RAM Card
	clr.l	d1
	move.b	AutoConfZ2Ram(a6),d1
	move.w	d1,AutoConfWByte(a6)
	move.l	d1,d0
	swap	d0
	move.l	d0,AutoConfAddr(a6)
	add.l	AutoConfSize(a6),d0
	cmp.l	#$c0000002,d0
	blo	.writenoz3illegal
	PUSH
	lea	AutoConfIllegalTxt,a0
	move.l	#1,d1
	jsr	Print
	move.b	#1,AutoConfIllegal(a6)	; Set the illegal flag
	POP
.writenoz3illegal:


	swap	d0
	move.b	d0,AutoConfZ2Ram(a6)
	move.l	a3,a1
	bra	.Write


.WriteZ3:
	lea	AutoConfZ3CardTxt,a0	; We got a Z3 Card
	clr.l	d1
	move.w	AutoConfZ3(a6),d1	; Get address to assign to
	move.w	d1,AutoConfWByte(a6)	; Write that info to the byte to be written
	move.l	d1,d0
	swap	d0
	move.l	d0,AutoConfAddr(a6)	; Write it to the register to keep info about adr
	add.l	AutoConfSize(a6),d0
	swap	d0
	move.w	d0,AutoConfZ3(a6)	; Set the size?
	move.l	a3,a1
	bra	.Write			; write the data

.WriteZ2IO:
	lea	AutoConfRomCardTxt,a0	; We got a Z2 ROM Card
	clr.l	d1

	move.b	AutoConfZ2IO(a6),d1
	clr.l	d1
	move.b	AutoConfZ2IO(a6),d1
	move.w	d1,AutoConfWByte(a6)
	move.l	d1,d0
	swap	d0
	move.l	d0,AutoConfAddr(a6)
	add.l	AutoConfSize(a6),d0
	swap	d0
	move.b	d0,AutoConfZ2IO(a6)
	move.l	a3,a1
	bra	.Write

.Write:					; OK! we have a card, not Z3 or Z2io. it must be Z2 RAM!

	move.b	#1,AutoConfDone(a6)		; Set that we have done autoconfig
	cmp.b	#0,AutoConfIllegal(a6)	; Check if the illegalfag was set
	bne	.WriteNoAssign			; it was not 0, so it is set, shutdown card

	move.l	AutoConfBoards(a6),d3
	mulu	#14,d3
	lea	AutoConfList(a6),a5
	add.l	d3,a5
	add.l	#1,AutoConfBoards(a6)
	move.w	er_Manufacturer(a2),(a5)+
	move.w	er_SerialNumber(a2),(a5)+
	move.b	er_Type(a2),(a5)+
	move.b	er_Flags(a2),(a5)+


	move.l	d1,d3				; Store the address into d3 as backup
	move.l	#2,d1
	jsr	Print				; print the string stored in a0

					; IN now:
					; A0 = String to output
					; D0 = Startadr of Autoconfig, cleartext
					; D2 = Endadr
					; D3 = Startadr of Autoconfig, short
					; A1 = Expansionbase

	move.l	AutoConfAddr(a6),d0	; Get address to assign board to
	move.l	d0,d2			; Store size in D2
	move.l	d0,(a5)+
	jsr	binhex
	move.l	#6,d1
	jsr	Print			; Prints the address to write
	lea	MinusTxt,a0
	move.l	#3,d1
	jsr	Print			; Prints " - "

	move.l	d2,d0			; move back size to D0
	add.l	AutoConfSize(a6),d0	; Add the size, to get the endaddress
	sub.l	#1,d0
	move.l	d0,(a5)+
	jsr	binhex
	move.l	#6,d1
	jsr	Print			; Prints the and address

	lea	NewLineTxt,a0
	jsr	Print			; Makes a new line

	cmp.b	#0,AutoConfMode(a6)
	beq	.WriteFast		; ok we are in "fast" mode.. so no verbose...
	lea	AutoConfEnableTxt,a0
	move.l	#2,d1
	jsr	Print

	clr.b	AutoConfExit(a6)	; Clear the force exitflag
.WriteLoop:
	jsr	GetInput		; Get inputdata
	cmp.b	#0,BUTTON(a6)
	beq	.WriteLoop
	cmp.b	#1,LMB(a6)
	beq	.WriteFast
	cmp.b	#1,RMB(a6)	
	beq	.WriteNoAssign
	move.b	GetCharData(a6),d7	; Get chardata
	bclr	#5,d7			; Make it uppercase
	cmp.b	#"Y",d7
	beq	.WriteFast
	cmp.b	#"N",d7
	beq	.WriteNoAssign
	cmp.b	#$1b,d7
	beq	.forceexit
	bra	.WriteLoop		; Simply we have now printed all data, and asked user what to do. and loop until answered.

.WriteFast:
	move.l	a1,a0			; Set correct Expansionbase
	move.l	d3,d1
	jsr	.WriteCard
	bra	.EndWrite

.WriteNoAssign:
	move.l	a1,a0			; Set correct Expansionbase
	moveq	#ec_Shutup+ExpansionRom_SIZEOF,d0
	jsr	.WriteCard		; Send shutup
	move.l	#-2,d0

.exit:
	bra	.EndWrite
.forceexit:
	move.b	#1,AutoConfExit(a6)	; Set force exit flag
.EndWrite:
	rts				; Card is written and we are done!


.WriteCard:
	move.l	#ec_BaseAddress+ExpansionRom_SIZEOF,d0

	clr.l	d1
	move.w	AutoConfWByte(a6),d1	; Get data to write

;	cmp.b	#0,AutoConfMode(a6)	; Check if we want a more verbose config-
;	beq	.WriteCardFast		; Why the F..  did I do this?? kinda pointless..  humm
	
.WriteCardFast:
	lsl.l	#2,d0			; Multiply with 4

	move.l	a0,a1
	lea.l	0(a0,d0.w),a0

	cmp.b	#1,AutoConfZorro(a6)	; Check if the board is Z3
	beq	.writez3

	move.l	d1,d0			; take the byte to write
	lsl.b	#4,d0			; Split it up to nibbles
	move.b	d0,$2(a0)
	move.b	d1,(a0)			; Write the byte to the card (as 2 nibbles)
	rts

.writez3:
	move.l	d1,d2
	move.l	d1,d0
	lsl.b	#4,d0
	move.b	d0,$100(a0)
	move.b	d1,(a0)
	move.l	a1,a0
	move.l	#$48,d0
	lea	0(a0,d0.w),a0
	move.b	d2,(a0)


	move.l	a1,a0
	move.l	#$44,d0
	lea	0(a0,d0.w),a0
	move.w	d2,(a0)

.dowrite:
	rts


PrintBoards:
	lea	AutoConfBoardsTxt,a0
	move.l	#6,d1
	jsr	Print

	lea	AutoConfList(a6),a1
	move.l	AutoConfBoards(a6),d0		; Get number of boards

	move.l	d0,d6
	jsr	bindec
	move.l	#2,d1
	jsr	Print

	move.l	d6,d0


	cmp.l	#0,d0
	beq	.printdone

	sub.l	#1,d6
	
.loop:
	lea	AutoConfManuTxt2,a0
	jsr	Print
	clr.l	d0
	move.w	(a1)+,d0			; Get manufacturer
	jsr	bindec
	jsr	Print

	lea	SlashTxt,a0
	jsr	Print

	move.w	(a1)+,d0			; Get Serialno
	jsr	bindec
	jsr	Print

	lea	AutoconfZorType2Txt,a0
	jsr	Print

	clr.l	d0
	move.b	(a1)+,d0			; Get type
	move.b	(a1)+,d5			;get Flag

	move.b	d0,d7			; Store d0 into d7 for future use

	and.b	#$c0,d0			; Strip out all except top 2 bits
	cmp.b	#$c0,d0
	beq	.readz2
	lea	III,a0
	move.l	#6,d1
	bset	#2,d7			; to fool print that we are not io
	jsr	Print
	bra	.readz3
.readz2:
	lea	II,a0
	move.l	#6,d1
	jsr	Print

.readz3:
	lea	SpaceTxt,a0
	jsr	Print

	btst	#2,d7
	beq	.readnomem
	lea	RAMTxt,a0
	jsr	Print
	bra	.readmem
.readnomem:
	lea	IOTxt,a0
	jsr	Print
.readmem:

	lea	SpaceTxt,a0
	jsr	Print


	move.l	d7,d0			; Restore d7 to d0 to handle what size we have
	clr.l	d7			; Clear d7 to have as a variable. if changed we have extended size
	btst	#5,d5			; Check if Extended sizes will be used
	beq	.readnoextsize
	moveq.l	#1,d7			; Set d7 to 1, we have extended sizes
.readnoextsize:


	and.b	#7,d0			; D0 now contains sizebits
	asl	#2,d0			; Multiply with 4, to get correct location in pointerlist


	lea	SizeTxtPointer,a0
	cmp.b	#0,d7			; Check if d7 is 0, if so we have not extended size
	beq	.readnoext
	lea	ExtSizeTxtPointer,a0
.readnoext:
	move.l	(a0,d0.l),a0		; A0 now points to the correct textstring
	move.l	#2,d1
	jsr	Print



	lea	StartTxt,a0
	jsr	Print

	move.l	(a1)+,d0
	jsr	binhex
	jsr	Print
	lea	EndTxt,a0
	jsr	Print
	move.l	(a1)+,d0
	jsr	binhex
	jsr	Print	
	
	dbf	d6,.loop
.printdone:
	rts

