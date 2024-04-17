       include "earlymacros.i"
       include "build/srcs/globalvars.i"

       section "initstartup",code_p
       XREF   Initcode
       xref   playsize
	xref	NewLineTxt
	xref	EnglishKey
	xref	EnglishKeyShifted
	xref	KB
	xref	MinusTxt

Initcode:                                                      ; OK we have RAM. we can actually work some with real coding no weird JMP to registers etc.
                                                               ; So lets start to actually handle the stuff we stored during bootup and do a proper init.
                                                               ; First lets fins out if we had a stuck mousebutton. so we can ignore them in the future.
	move.l startupflags(a6),d0					; lets do a check what mode to start with by taking status of mousebuttons at p
	
       move.l	d0,d1
       move.l	d0,d2
       and.l	#%00111111000000000000000000000000,d1		; mask out bits from poweron
       and.l	#%00000000000011111100000000000000,d2		; mask out bits from after meminit
       asr.l	#8,d1
       asr.l	#8,d1
       asr.l	#8,d1
       asr.l	#8,d2
       asr.l	#6,d2							; rotate bits to "start" of register
       eor.l	d1,d2							; d2 now will contain what bits was NOT stuck
       move.l	d1,d3
       eor.l	d2,d3							; d3 contains stuck bits.  so 0=no stuck

 	btst	#5,d3
	beq	.NOP1LMB
	move.b	#1,STUCKP1LMB(a6)
.NOP1LMB:
	btst	#4,d3
	beq	.NOP2LMB
	move.b	#1,STUCKP2LMB(a6)
.NOP2LMB:
	btst	#3,d3
	beq	.NOP1RMB
	move.b	#1,STUCKP1RMB(a6)
.NOP1RMB:
	btst	#2,d3
	beq	.NOP2RMB
	move.b	#1,STUCKP2RMB(a6)
.NOP2RMB:
       btst	#1,d3
	beq	.NOP1MMB
	move.b	#1,STUCKP1MMB(a6)
.NOP1MMB:
	btst	#0,d3
	beq	.NOP2MMB
	move.b	#1,STUCKP2MMB(a6)
.NOP2MMB:
                                                               ; OK lets handle other flags!
       btst   #31,d0
       beq    .noserialtimeout
       move.b #1,NoSerial(a6)                                  ; We had timeouts on serial, so lets disable serialport
	PAUSE
.noserialtimeout:
       btst   #30,d0
       beq    .noromadrerr
       move.b #1,RomAdrErr(a6)                                 ; We had errors at ROM Address scan.
.noromadrerr:
       btst   #23,d0
       beq    .nochipbiterr
       move.b #1,ChipBitErr(a6)                                ; Mark we had biterrors in chipmem at boot
.nochipbiterr:
       btst   #22,d0
       beq    .nochipadrerr
       move.b #1,ChipAdrErr(a6)                                ; Mark we had addresserrors in chipmem at boot
.nochipadrerr:
       btst   #21,d0
       beq    .notenoughchip
       move.b #1,NotEnoughChip(a6)                             ; Mark we had not enough chipmem at boot
.notenoughchip:
       btst   #20,d0
       beq    .scannedfastmem
       move.b #1,ScanFastMem(a6)                               ; Mark we did scan for fastmem at boot
.scannedfastmem:
       btst   #13,d0
       beq    .nofastboot
       move.b #1,FastFound(a6)                                 ; Mark we did find fastmem at boot
.nofastboot:
       btst   #12,d0
       beq    .nonodraw
       move.b #1,NoDraw(a6)                                    ; Set "NoDraw" mode, nothing drawn on screen
.nonodraw:
       btst   #11,d0
       beq    .nostuck
       move.b #1,StuckMouse(a6)                                ; Ser we had stuck mousebuttons
.nostuck:
       btst   #10,d0
       beq    .nomem400
       move.b #1,MemAt400                                      ; Set we had memory at $400
.nomem400:
       btst   #9,d0
       beq    .noovlerr
       move.b #1,OVLErr(a6)                                    ; Set we had OVL Errors
.noovlerr:
       btst   #8,d0
       beq    .noreverse
       move.b #1,WorkOrder(a6)
.noreverse:
       move.l ChipStart(a6),d0
       cmp.l  #$400,d0                                         ; if we have 400 as startaddress we have skipped one K. so lets "fix" that
       bne    .not400
       clr.l  d0
.not400:
       move.l ChipEnd(a6),d1
       add.l  #1,d1
       sub.l  d0,d1
       move.l d1,TotalChip(a6)
       cmp.b  #1,NotEnoughChip(a6)                             ; if we did not have enough chipmem skip this part
       beq    .nochip
       cmp.b  #1,WorkOrder(a6)                                 ; Check if we had a reversed workorder      
       beq    .reversed
       move.l ChipStart(a6),d0
       move.l d0,GetChipAddr(a6)
       move.l ChipEnd(a6),d0
       sub.l  #RAMUsage+4,d0
       and.l  #$fffffffe,d0
       move.l ChipStart(a6),d1
       sub.l  d1,d0
	sub.l	#RAMUsage,d0
       move.l d0,ChipUnreserved(a6) 
       bra    .nochip                                          ; ok wrong name but same address anyway.
.reversed:
       move.l ChipStart(a6),d0
       add.l  #RAMUsage+4,d0
       and.l  #$fffffffe,d0                                    ; Make sure it is even
       move.l d0,GetChipAddr(a6)                               ; Store this address as start of "available" memblock
       move.l ChipEnd(a6),d1
       sub.l  d0,d1
	sub.l	#RAMUsage,d1
       move.l d1,ChipUnreserved(a6)                            ; Weird label I know. but "available chipmem"
.nochip:
	PAUSE
	jsr    GetHWReg

       move.l a6,BaseStart(a6)
       move.l a6,d0
       add.l  #RAMUsage,d0
       move.l d0,BaseEnd(a6)

       KPRINTC       LoopSerTest
       bsr    ClearSerial
       bsr    ClearSerial
	clr.l	d6			; Clear d6 as it a counter for how many similiar chars we got back as echo
	move.b	#"<",d2			; Char to test
	bsr	RealLoopbacktest

	move.b	#">",d2			; Char to test
	bsr	RealLoopbacktest


	cmp.b	#0,d6			; Check if we had any return, if so we have a loopbackadapter installed.
	beq	.noloopback

	move.w	#5,SerialSpeed(a6)	; Set serialspeed to 5 ,(same as 0 but mark loopbackadapter)
	move.b	#1,LoopB(a6)

       KPRINTC       DDETECTED
       bra    .loopbackdone

.noloopback:
       KPRINTC       NoLoopback

.loopbackdone:
 	cmp.b	#1,NoSerial(a6)	; Check if noserial is set
	beq	.noser
	cmp.b	#1,LoopB(a6)		; Check if loopbackadapter was attacjhed
	beq	.noserloop		; in that case, no serial output
	move.w	#4,SerialSpeed(a6)	; Set speed 4 (115200)
	bsr	Init_Serial
	bra	.ser

.noserloop:
	move.w	#5,SerialSpeed(a6)	; Set serialspeed to 5 ,(same as 0 but mark loopbackadapter)
	move.b	#1,NoSerial(a6)
	bra	.ser
.noser:
	move.w	#0,SerialSpeed(a6)	; Set Serialspeed to 0
.ser:

	move.w	#$aaa,$dff180
       lea	DetectRasterTxt,a0
	bsr	SendSerial
       move.b	$dff006,d0		; Load value of raster
	jsr	WaitShort
	jsr	WaitShort
	jsr	WaitShort
	move.b	$dff006,d1		; Load value of raster again
	cmp.b	d0,d1
	beq	.noraster		; if raster was the same, We assume we have no working raster
	move.b	#1,RASTER(a6)		; it was different so we assume we have working raster.
	lea	DETECTED,a0
	bsr	SendSerial
	beq	.rastercheckdone

.noraster:
	move.b	#0,RASTER(a6)
	lea	SFAILED,a0
	bsr	SendSerial
.rastercheckdone:

       lea	NewLineTxt,a0
	bsr	SendSerial

	move.l	#EnglishKey,keymap(a6)	; Set english keymap as default
	move.w	#$999,$dff180

	lea	DetChipTxt,a0
	bsr	SendSerial

	move.l	TotalChip(a6),d0
	asr.l	#8,d0
	asr.l	#2,d0
	bsr	bindec
	bsr	SendSerial

	lea	KB,a0
	bsr	SendSerial		; Put out detected chipmem on serialport..
	lea	NewLineTxt,a0
	bsr	SendSerial

       lea	DetMBFastTxt,a0
	bsr	SendSerial
	
	nop
	move.l	BootMBFastmem(a6),d0
	bsr	bindec
	bsr	SendSerial
	bsr	SendSerial
	lea	KB,a0
	bsr	SendSerial
	lea	NewLineTxt,a0
	bsr	SendSerial

       move.w	#$888,$dff180

       lea	ChipSetupInit,a0
	bsr	SendSerial

       ;move.l	a6,a5
       move.l       BPL(a6),a5    ; Load A5 with location of bitplanedata
       move.l       BPLSIZE(a6),d0
	move.l	#"BPL1",(a5)+		; Put the BPL1 string, just to be able to identify BPL1
	move.l	a5,Bpl1Ptr(a6)	; and store the pointer to bitplan
       add.l  d0,a5                ; Add to next biplane
	move.l	#"BPL2",(a5)+		; Put the BPL2 string, just to be able to identify BPL2
	move.l	a5,Bpl2Ptr(a6)	; and store the pointer to bitplane
       add.l  d0,a5                ; Add to next biplane
       move.l	#"BPL3",(a5)+		; Put the BPL3 string, just to be able to identify BPL3
	move.l	a5,Bpl3Ptr(a6)	; and store the pointer to bitplane
       add.l  d0,a5                ; Add to next biplane
	move.l	#"END!",(a5)+        ; Mark as end of block
       clr.l  (a5)                 ; Make sure next longword is clear, as this is the dummysprite
       move.l a5,DummySprite(a6)   ; Store the pointer to the dummysprite
       add.l  4,a5
       bsr    CopyToChip

	move.w	#$777,$dff180

	lea	Bpl1attxt,a0
	bsr	SendSerial
	move.l	Bpl1Ptr(a6),d0
	bsr	binhex
	bsr	SendSerial
	lea	NewLineTxt,a0
	bsr	SendSerial

	lea	Bpl2attxt,a0
	bsr	SendSerial
	move.l	Bpl2Ptr(a6),d0
	bsr	binhex
	bsr	SendSerial
	lea	NewLineTxt,a0
	bsr	SendSerial
	lea	Bpl3attxt,a0
	bsr	SendSerial
	move.l	Bpl3Ptr(a6),d0
	bsr	binhex
	bsr	SendSerial
	lea	NewLineTxt,a0
	bsr	SendSerial
	clr.l	BplNull(a6)
       bsr    InitStuff

	lea	ChipSetupDone,a0
	bsr	SendSerial


       cmp.b	#0,NoDraw(a6)
	bne	.NoChip
	move.w	#$555,$dff180

	bsr	SetMenuCopper

	lea	InitPOTGO,a0
	bsr	SendSerial
	move.w	#$ff00,$dff034
	lea	InitDONEtxt,a0
	bsr	SendSerial

 	move.w	#$333,$dff180
	bsr	ClearScreenNoSerial
	lea	InitTxt,a0
	move.l	#7,d1
	bsr	Print

	bra	.Chip

.NoChip:
	lea	NoDrawTxt,a0
	bsr	SendSerial
.Chip:
	bsr	RomChecksum

	move.l	#.cpureturn,a5
	bsr	DetectCPU
.cpureturn:
	bsr	PrintCPU

	move.l	FastBlocksAtBoot(a6),d1
	lea	FastDetectTxt,a0
	move.l	#3,d1
	bsr	Print
	move.b	$dff006,d0			; load d0 with a "random" number for memdetect to work
	bsr	DetFastMem
	
	lea	WorkAreasTxt,a0
	move.l	#7,d1
	bsr	Print
	move.l	ChipStart(a6),d0
	bsr	binhex
	bsr	Print
	lea	MinusTxt,a0
	bsr	Print
	move.l	ChipEnd(a6),d0
	bsr	binhex
	bsr	Print
	lea	WorkAreasTxt2,a0
	bsr	Print
	move.l	FastStart(a6),d0
	bsr	binhex
	bsr	Print
	lea	MinusTxt,a0
	bsr	Print
	move.l	FastEnd(a6),d0
	bsr	binhex
	bsr	Print
	move.l	FastMem(a6),d1
	asl.l	#6,d1
	move.l	d1,TotalFast(a6)

	lea	IfSoldTxt,a0
	move.l	#5,d1
	bsr	Print
	lea	InitSerial2,a0
	move.l	#7,d1
	bsr	Print
	cmp.b	#1,NoDraw(a6)			; Check if we are in NoDraw Mode. if so. do not disable serialport
	beq	.nodraw
	cmp.b	#1,NoSerial(a6)
	beq	.serialon			; IF Noserial was set, skip this part
	cmp.b	#1,LoopB(a6)			; Same if loopbackadapter was attached
	beq	.serialon
	jsr	ClearBuffer
	move.l	#1200,d7			; read data for a while.. Giving user a possability of try to press a key on serialport
	clr.l	d6
.waitloop:
	move.b	d7,$dff181			; Just flash some colors. so local user can see that something is happening
	bsr	GetInput
	cmp.b	#1,RMB(a6)			; RMB pressed? then turn serial on
	beq	.serialon
	btst	#2,d0
	bne	.serialon			; if any key was pressed turn serial on
	btst	#3,d0
	bne	.serialon
	add.l	#1,d6
	cmp	#16,d6
	bne	.nodot
	lea	DotTxt,a0
	move.l	#7,d1
	bsr	Print
	clr.l	d6
.nodot:
	dbf	d7,.waitloop
	lea	EndSerial,a0			; Send text about "no key pressed"
	move.l	#7,d1
	bsr	SendSerial
	move.w	#0,SerialSpeed(a6)
	bra	.serialon
.nodraw:
	TOGGLEPWRLED
	move.w	#2,SerialSpeed(a6)
	lea	NoDrawTxt,a0
	bsr	SendSerial
.serialon:
	jsr	ClearBuffer

	clr.l	d7
	bsr	DefaultVars

	move.l	#Menus,Menu(a6)
	;move.w	#44,OldMenuNumber(a6)	; Write bogus number
	bra	MainMenu			; Print the mainmenu

;------------------------------------------------------------------------------------------------------------------------------------------------------------------------

ClearScreenNoSerial:				; Clear screen but does not dump to serialport.
	cmp.b	#0,NoDraw(a6)
	bne	.no
	move.l	Bpl1Ptr(a6),a0		; load A0 with address of BPL1
	move.l	Bpl2Ptr(a6),a1		; load A1 with address of BPL2
	move.l	Bpl3Ptr(a6),a2		; load A2 with address of BPL3
	move.l	BPLSIZE(a6),d0
	asr.l	#2,d0
.loop:
	clr.l	(a0)+
	clr.l	(a1)+
	clr.l	(a2)+
	dbf	d0,.loop
.no:
	clr.l	d0
	clr.l	d1
	bsr	SetPosNoSerial
	rts

SetPosNoSerial:					; Set cursor at wanted position on screen but not on serialport
						; Indata:
						; d0 = xpos
						; d1 = ypos

	move.b	d0,Xpos(a6)
	move.b	d1,Ypos(a6)
	rts


SetMenuCopper:
	lea	InitCOP1LCH,a0
	bsr	SendSerial
	move.l	MenuCopper(a6),d0
	move.l	d0,$dff080			;Load new copperlist
	lea	InitDONEtxt,a0
	bsr	SendSerial

	lea	InitCOPJMP1,a0
	bsr	SendSerial
	move.w	$dff088,d0
	lea	InitDONEtxt,a0
	bsr	SendSerial

	lea	InitDMACON,a0
	bsr	SendSerial
	move.w	#$8380,$dff096
	lea	InitDONEtxt,a0
	bsr	SendSerial

	lea	InitBEAMCON0,a0
	bsr	SendSerial
	move.w	#32,$dff1dc			;Hmmm
	lea	InitDONEtxt,a0
	bsr	SendSerial
	rts

FixBitplane:
; Set bitplanes in copperlist
;
; Indata
;
;	A0 = bitplanespointers in copper
;	A1 = List to bitplane pointers, 0 = End of list
;

       move.l	(a1)+,d0
       cmp.l	#0,d0
       beq.s	.End

       move.w	d0,6(a0)
       swap	d0
       move.w	d0,2(a0)
       add.l	#8,a0
       bra.s	FixBitplane
.End:
       rts
       

InitStuff:
       lea	ChipSetup7,a0
	bsr	SendSerial
       move.l a6,a1
       add.l  #Bpl1Ptr,a1
	move.l	MenuCopper(a6),a0
	add.l	#MenuBplPnt-RomMenuCopper,a0
       move.l a0,a4
	bsr	FixBitplane
 	bset	#5,SCRNMODE(a6)		; Set bit in SCRNMODE, to tell that we are in PAL mode
	rts

CopyToChip:					; Copy data that needs to be in Chipmem from ROM for menusystem etc.
       move.l ChipmemBlock(a6),a5
       lea	ChipSetup1,a0
       bsr	SendSerial
       lea	ChipSetup2,a0
	bsr	SendSerial

       move.l a5,d0
       add.l  #MenuCopperList,d0
       move.l d0,MenuCopper(a6)           ; Set pointer to MenuCopperlist
	move.l	d0,a1
       lea	RomMenuCopper,a0
	move.l	#EndRomMenuCopper-RomMenuCopper,d0
	bsr	CopyMem				; Copy the MenuCopper to chipmem
	move.l	MenuCopper(a6),d0
	bsr	binhex
	bsr	SendSerial
	lea	NewLineTxt,a0
	bsr	SendSerial

       lea	ChipSetup3,a0
	bsr	SendSerial
       move.l a5,d0
	add.l	#ECSCopperList,d0
       move.l d0,ECSCopper(a6)
       move.l d0,a1
       lea	RomEcsCopper,a0
       move.l	#EndRomEcsCopper-RomEcsCopper,d0
	bsr	CopyMem
	move.l	ECSCopper(a6),d0
	bsr	binhex
	bsr	SendSerial
	lea	NewLineTxt,a0
	bsr	SendSerial
       lea	ChipSetup4,a0
	bsr	SendSerial
       lea	RomEcsCopper2,a0
	move.l	a5,d0
	add.l	#ECSCopper2List,d0
       move.l d0,ECSCopper2(a6)
       move.l d0,a1
	move.l	#EndRomEcsCopper2-RomEcsCopper2,d0
	bsr	CopyMem
	move.l	ECSCopper2(a6),d0
	bsr	binhex
	bsr	SendSerial
	lea	NewLineTxt,a0
	bsr	SendSerial
	lea	ChipSetup5,a0
	bsr	SendSerial
	move.l	MenuCopper(a6),a0
               move.w	#7,d0
              move.l DummySprite(a6),d1
       .ClearS:
              swap	d1
              move.w	d1,2(a0)
              swap	d1
              move.w	d1,6(a0)
              add.l	#8,a0
              dbf	d0,.ClearS			; A empty dummysprite is now defined
       lea	ChipSetup6,a0
	bsr	SendSerial

       lea	ROMAudioWaves,a0
	move.l ChipmemBlock(a6),d0			; Get Chipmem start
       add.l  #AudioWaveData,d0			; Add to where audio wave data is
       move.l d0,AudioWaves(a6)
       move.l d0,a1
       move.l a1,a4
	move.l	#Wavesize,d0
	bsr	CopyMem				; Copy the font to chipmem
	move.l	AudioWaves(a6),d0
	bsr	binhex
	bsr	SendSerial
	lea	NewLineTxt,a0
	bsr	SendSerial
	lea	ChipSetup8,a0
	bsr	SendSerial
       lea    MT_Init,a0
       move.l a5,d0
       add.l  #ptplayroutine,d0
       move.l d0,ptplay(a6)
       move.l d0,a1
       move.l #mt_END-MT_Init,d0
       bsr    CopyMem
	move.l	ptplay(a6),d0
	bsr	binhex
	bsr	SendSerial
	lea	NewLineTxt,a0
	bsr	SendSerial

       move.l ptplay(a6),a4
       move.l d0,AudioModInit(a6)                ; Store pointer to Init routine
       move.l d0,d2                              ; Backup pointer
       add.l  #mt_END-MT_Init,d0
       move.l d0,AudioModEnd(a6)                 ; Store pointer to end of musicroutine
       move.l d2,d0                              ; Restore the pointer
       add.l  #MT_Music-MT_Init,d0               ; Add where MT_Music is
       move.l d0,AudioModMusic(a6)               ; Store the pointer
       move.l d2,d0
       add.l  #mt_MasterVol-MT_Init,d0
       move.l d0,AudioModMVol(a6)                ; Store pointer to Mastervolume
       RTS

ClearSerial:					; Just read serialport, to empty it
	move.l	#1,d6				; load d6 with 1, so we run this, twice to be sure serialbuffer is cleared
.loop:
	move.w	#$4000,$dff09a
	move.w	#INITBAUD,$dff032			; Set the speed of the serialport (9600BPS)
	move.b	#$4f,$bfd000			; Set DTR high
	move.w	#$0801,$dff09a
	move.w	#$0801,$dff09c

	move.l	#10000,d7
.timeoutloop2
	move.b	$bfe001,d0			;nonsenseread
	cmp.l	#0,d7
	beq	.exitloop
	sub.l	#1,d7
	move.w	$dff018,d0
	btst	#14,d0				; Buffer full, we have a new char
	beq	.timeoutloop2
.exitloop:
	dbf	d6,.loop
	rts

       

RealLoopbacktest:				; Test if we have a loopbackadapter connected.
						; Simply by outputing the char in D2 and check if the same char comes back.
						; if so, 1 is added to D6
	move.w	#$4000,$dff09a
	move.w	#INITBAUD,$dff032			; Set the speed of the serialport (9600BPS)
	move.b	#$4f,$bfd000			; Set DTR high
	move.w	#$0801,$dff09a
	move.w	#$0801,$dff09c

	move.l	#10000,d7			; Load d7 with a timeoutvariable
.timeoutloop:	
	move.b	$bfe001,d1			; just read crapdata, we do not care but reading from CIA is slow... for timeout stuff only
	sub.l	#1,d7				; count down timeout value
	cmp.l	#0,d7				; if 0, timeout.
	beq	.endloop
	move.w	$dff018,d0
	btst	#13,d0				; Check TBE bit
	beq.s	.timeoutloop			; Loop until all is ok to send or until timeout hits
.endloop:

	move.w	#$0100,d1
	move.b	d2,d1
	move.w	d1,$dff030			; send it to serial
	move.w	#$0001,$dff09c			; turn off the TBE bit


	move.l	#10000,d7
.timeoutloop2
	move.b	$bfe001,d0			;nonsenseread
	cmp.l	#0,d7
	beq	.exitloop
	sub.l	#1,d7
	move.w	$dff018,d0
	btst	#14,d0				; Buffer full, we have a new char
	beq	.timeoutloop2
	bra	.check
.exitloop:
	clr.l	d0
.check:
	cmp.b	d0,d2				; Check if in and out was the same char

	bne.w	.exittest				; no so lets exit
	add.b	#1,d6				; Yes, add 1 to d6
.exittest:
	rts

DetFastMem:
	clr.l	FastMem(a6)		; Clear amount of fastmem
	lea	A24BitTxt,a0
	move.l	#7,d1
	bsr	Print
	clr.l	d1
	lea	$0,a0
	lea	$0,a0
	lea	$0,a3
					; as d0 is used as a "random" number in memcheck.  but d0 is also detected chipmem.
					; lets eor this to make it more... "random"
					; this detection is quite.. "poor" as it will stop when finding one block of ram. so fragmented memory only first block
					; will be found
	clr.l	d2			; We set d2 to 0.  if it is anything else than 0 after 24bit tests, we have32bit cpu
	cmp.l	#" PPC",$f00090		; Check if the string "PPC" is located in rom at this address. if so we have a BPPC
					; that will disable the 68k cpu onboard if memory  below $40000000 is tested.
	beq	.bppc
	move.l	#"NONE",$700
	move.l	#"24BT",$40000700	; Write "24BT" to highmem
	cmp.l	#"24BT",$700		; IF memory is readable at $700 instead. we are using a cpu with 24 bit adress. no memory to detect in next routines
	beq	.nop5
	move.l	#1,d2			; blizzards etc will set this to 1..  apollo etc will not
.nop5
	move.l	#"NONE",$700
	move.l	#"24BT",$2000700	; Write "24BT" to highmem
	cmp.l	#"24BT",$700		; IF memory is readable at $700 instead. we are using a cpu with 24 bit adress. no memory to detect in next routines
	beq	.no24
	move.l	#1,d2			; other cards will trigger here
.no24
	move.l	#"NONE",$700
	move.l	#"24BT",$4000700	; Write "24BT" to highmem
	cmp.l	#"24BT",$700		; IF memory is readable at $700 instead. we are using a cpu with 24 bit adress. no memory to detect in next routines
	beq	.no24a
	move.l	#1,d2			; blizzards etc will set this to 1..  apollo etc will not
.no24a:
	cmp.l	#0,d2			; if d2 is 0, we have 24 bit addressing
	beq	.no32bit
	PUSH
	lea	NO,a0
	move.l	#2,d1
	bsr	Print
	lea	NewLineTxt,a0
	bsr	Print
	POP
	PUSH
	lea	A3k4kMemTxt,a0
	move.l	#7,d1
	bsr	Print
	POP
	eor.l	#$01110000,d0
	lea	$1000000,a1		; Detect motherboardmem on A3000/4000
	lea	$7ffffff,a2
	lea	.a3k4kdone,a3
	bra	DetectMemory
.a3k4kdone:				; Again, the wonders without stack.  pasta-code.. :)
	move.l	a0,d5			; Backup startaddress of memory found
	move.l	a1,d4			; Backup endaddress of memory found
	move.l	d1,d3			; Backup data of addresses found to registers not used
	add.l	d1,FastMem(a6)
	cmp.l	#0,a0			; was a0 0?  if so. no memory was found
	beq	.det16
	move.l	a0,FastStart(a6)	; Store startaddress of fastmem
	move.l	a1,FastEnd(a6)	; Store endadress of fastmem
	bsr	.PrintDetected
.det16:
	move.l	d3,d1
	PUSH
	lea	CpuMemTxt,a0
	move.l	#7,d1
	bsr	Print
	POP
	eor.l	#$01110000,d0
	lea	$8000000,a1		; Detect cpuboard on A3000/4000
	lea	$10000000,a2
	lea	.a3k4kcpudone,a3
	bra	DetectMemory
.a3k4kcpudone:
	move.l	a0,d5			; Backup startaddress of memory found
	move.l	a1,d4			; Backup endaddress of memory found
	move.l	d1,d3			; Backup data of addresses found to registers not used
	cmp.l	#0,a0			; was a0 0?  if so. no memory was found
	beq	.det26
	add.l	d1,FastMem(a6)
	move.l	a0,FastStart(a6)	; Store startaddress of fastmem
	move.l	a1,FastEnd(a6)	; Store endadress of fastmem
					; As this memory is usually faster than "onboard" memory, replace start and end addresses of fastem.
	bsr	.PrintDetected
.det26:
	PUSH
	cmp.l	#0,FastStart(a6)	; Check if stored start of fastmem is 0. if not we skip this part as most likly we are not a A1200
	bne	.skipA1200cpu
	lea	A1200CpuMemTxt,a0
	move.l	#7,d1
	bsr	Print
	POP
	eor.l	#$01010000,d0
	bra	.nobppc
.bppc:
	PUSH
	lea	BPPCtxt,a0
	move.l	#6,d1
	bsr	Print
	POP
.nobppc:
	lea	$40000000,a1
	lea	$ee000000,a2
	lea	.det1200cpu,a3
	eor.l	#$11010000,d0
	bra	DetectMemory
.det1200cpu:
	move.l	a0,d5			; Backup startaddress of memory found
	move.l	a1,d4			; Backup endaddress of memory found
	move.l	d1,d3			; Backup data of addresses found to registers not used
	cmp.l	#0,a0			; was a0 0?  if so. no memory was found
	beq	.det36
	move.l	a0,FastStart(a6)	; Store startaddress of fastmem
	move.l	a1,FastEnd(a6)	; Store endadress of fastmem
					; As this memory is usually faster than "onboard" memory, replace start and end addresses of fastem.
	add.l	d1,FastMem(a6)
	bsr	.PrintDetected
.det36:
	bra	.yes32bit
.no32bit:
	PUSH
	lea	YES,a0
	move.l	#2,d1
	bsr	Print
	lea	NewLineTxt,a0
	bsr	Print
	POP
.yes32bit:
	PUSH
.skipA1200cpu:
	lea	a24BitAreaTxt,a0
	move.l	#7,d1
	bsr	Print
	POP
	lea	$200000,a1		; Detect memory on 24 bit range
	lea	$9fffff,a2
	lea	.24bitdone,a3
	eor.l	#$10010000,d0
	bra	DetectMemory
.24bitdone:
	move.l	a0,d5			; Backup startaddress of memory found
	move.l	a1,d4			; Backup endaddress of memory found
	move.l	d1,d3			; Backup data of addresses found to registers not used
	cmp.l	#0,a0			; was a0 0?  if so. no memory was found
	beq	.det46
	cmp.l	#0,FastStart(a6)	; Check if stored start of fastmem is 0.. if so start this as start
	bne	.NoFastStored
	move.l	a0,FastStart(a6)	; Store startaddress of fastmem
	move.l	a1,FastEnd(a6)	; Store endadress of fastmem
					; As this memory is usually faster than "onboard" memory, replace start and end addresses of fastem.
.NoFastStored:
	add.l	d1,FastMem(a6)
	bsr	.PrintDetected
.det46:
	PUSH
	lea	FakeFastTxt,a0
	move.l	#7,d1
	bsr	Print
	POP
	eor.l	#$10010000,d0
	lea	$c00000,a1		; Detect memory on 24 bit range
	lea	$c80000,a2
	eor.l	#$10110000,d0
	lea	.fakefastdone,a3
	bra	DetectMemory
.fakefastdone:
	move.l	a0,d5			; Backup startaddress of memory found
	move.l	a1,d4			; Backup endaddress of memory found
	move.l	d1,d3			; Backup data of addresses found to registers not used
	cmp.l	#0,a0			; was a0 0?  if so. no memory was found
	beq	.det56
	cmp.l	#0,FastStart(a6)	; Check if stored start of fastmem is 0.. if so start this as start
	bne	.NoFastStored2
	move.l	a0,FastStart(a6)	; Store startaddress of fastmem
	move.l	a1,FastEnd(a6)	; Store endadress of fastmem
					; As this memory is usually faster than "onboard" memory, replace start and end addresses of fastem.
.NoFastStored2:
	add.l	d1,FastMem(a6)
	bsr	.PrintDetected
.det56:
	move.l	FastMem(a6),d0
	rts
.PrintDetected:
	PUSH
	lea	FastFoundtxt,a0
	move.l	#6,d1
	bsr	Print
	POP
	PUSH	
	move.l	d5,d0
	bsr	binhex
	add.l	#1,a0
	move.l	#6,d1
	bsr	Print
	POP
	PUSH
	lea	MinusDTxt,a0
	move.l	#6,d1
	bsr	Print
	move.l	d4,d0
	bsr	binhex
	add.l	#1,a0
	move.l	#6,d1
	bsr	Print
	lea	NewLineTxt,a0
	bsr	Print
	POP
	rts

DetectMemory:
	; D1 Total block of known working ram in 16K blocks (clear before first use)
	; A0 first usable addr
	; a1 First addr to scan
	; a2 Addr to end
	; a3 Addr to jump after done (as this does not use any stack
	; only OK registers to use as write: (d1), d2,d3,d4,d5,d6,d7, a0,a1,a2,a5
	; D0 is a special "in" never to be modified but taken as a "random" generator for shadowcontrol
	; OUT:	d1 = blocks of found mem
	;	a0 = first usable address
	;	a1 = last usable address
	move.l	a1,d7
	and.l	#$fffffffc,d7		; just strip so we always work in longword area (just to be sure)
	move.l	d7,a1
	move.l	a3,d7			; Store jumpaddress in D7
	lea	$0,a0			; clear a0
.Detect:
	lea	MEMCheckPattern,a3
	move.l	(a1),d3			; Take a backup of content in memory to D3
.loop:
	cmp.l	a1,a2			; check if we tested all memory
	blo	.wearedone		; we have, we are done!
	move.l	(a3)+,d2		; Store value to test for in D2	
	move.l	d2,(a1)			; Store testvalue to a1
	move.l	#"CRAP",4(a1)		; Just to put crap at databus. so if a stuck buffer reads what is last written will get crap
	nop
	nop
	nop
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
						; Reading several times.  as sometimes reading once will give the correct answer on bad areas.
	cmp.l	d4,d2				; Compare values
	bne	.failed			; ok failed, no working ram here.
	cmp.l	#0,d2				; was value 0? ok end of list
	bne	.loop				; if not, lets do this test again
						; we had 0, we have working RAM
	move.l	a1,a5			; OK lets see if this is actual CORRECT ram and now just a shadow.
	move.l	a5,(a1)			; So we store the address we found in that location.
	move.l	#32,d6			; ok we do test 31 bits
	move.l	a5,d5
.loopa:
	cmp.l	#0,d6
	beq	.done			; we went all to bit 0.. we are done I guess
	sub.l	#1,d6
	cmp.l	#0,d6
	beq	.done			; we went all to bit 0.. we are done I guess	---------
	btst	d6,d5			; scan until it isnt a 0
	beq.s	.loopa
.bitloop:
	bclr	d6,d5			; ok. we are at that address, lets clear first bit of that address
	move.l	d5,a3
	cmp.l	(a3),a5			; ok check if that address contains the address we detected, if so. we have a "shadow"
	beq	.shadow
	cmp.l	#0,a3			; it was 0, so we "assume" we got memory
	beq	.mem
		; ok we didnt have a shadow here
		; a5 will contain address if there was detected ram
	sub.l	#1,d6
	cmp.l	#4,d6
	beq	.mem			; ok we was at 4 bits away..  we can be PRETTY sure we do not have a shadow here.  we found mem
	bra	.bitloop
.mem:
	move.l	d3,(a1)			; restore backup of data
	cmp.l	(a1),d0			; check if value at a1 is the same as d0. this means we have a shadow on top and we have already tested
	beq	.shadowdone			; this memory.  basically: we are done
	cmp.l	#0,a0			; check if a0 was 0, if so, this is the first working address
	bne	.wehadmem
	move.l	a5,a0			; so a5 contained the address we found, copy it to a0
	move.l	d7,16(a1)		; ok store d7 into what a1 points to.. to say that this is a block of mem)
.wehadmem:
	add.l	#4,d1			; OK we found mem, lets add 4 do d1(as old routine was 64K blocks  now 256.  being lazy)
	bra	.next
.wearedone:
	bra	.done
.shadow:
	TOGGLEPWRLED			; Flash with powerled doing this.. 
.failed:
	move.l	d3,(a1)			; restore backup of data
	cmp.l	#0,a0			; ok was a0 0? if so, we havent found memory that works yet, lets loop until all area is tested
	bne	.done
.next:
	move.l	d0,(a1)			; put a note at the first found address. to mark this as already tagged
	move.l	a0,4(a1)		; put a note of first block found
	move.l	a1,8(a1)		; where this block was
	move.l	d1,12(a1)		; total amount of 64k blocks found
					; Strangly enough. this seems to also write onscreen at diagrom?
	add.l	#256*1024,a1		; Add 256k for next block to test
	bra	.Detect
.shadowdone:
	TOGGLEPWRLED			; Flash with powerled doing this.. 
.done:
	move.l	d7,a3			; Restore jumpaddress
	sub.l	#1,a1
	jmp	(a3)


MEMCheckPattern:
	dc.l	$ffffffff,$aaaaaaaa,$55555555,$f0f0f0f0,$0f0f0f0f,$0f0ff0f0,0,0

LoopSerTest:
       dc.b	$a,$d,"Testing if serial loopbackadapter is installed: ",0
DDETECTED:
       dc.b	" DETECTED",$a,$d,0
NoLoopback:
       dc.b	" NOT DETECTED",$a,$d,0
DetectRasterTxt:
       dc.b	"Detecting if we have a working raster: ",0
DETECTED:
       dc.b	27,"[32mDETECTED",27,"[0m",0
SFAILED:
       dc.b	27,"[31mFAILED",27,"[0m",0
NewLineTxt:
       dc.b	$a,$d,0
DetChipTxt:
       dc.b	"Detected Chipmem: ",0
KB:
       dc.b	"kB",0
DetMBFastTxt:
       dc.b	"Detected Motherboard Fastmem (not reliable result): ",0
ChipSetupInit:
       dc.b	" - Doing Initstuff",$a,$d,0

ChipSetup1:
       dc.b	" - Setting up Chipmemdata",$a,$d,0
ChipSetup2:
       dc.b	"   - Copy Menu Copperlist from ROM to memory at: ",0
ChipSetup3:
       dc.b	"   - Copy ECS TestCopperlist from ROM to memory at: ",0
ChipSetup4:
       dc.b	"   - Copy ECS testCopperlist2 from ROM to memory at: ",0
ChipSetup5:
       dc.b	"   - Fixing Bitplane Pointers etc in Menu Copperlist",$a,$d,0
ChipSetup6:
       dc.b	"   - Copy Audio Data from ROM to memory at: ",0
ChipSetup8:
	dc.b	"   - Copy Protracker replayroutine from ROM to memory at: ",0
ChipSetup7:
       dc.b	"   - Do final Bitplanedata in Menu Copperlist",$a,$d,0
ChipSetupDone:
       dc.b	" - Initstuff done!",$a,$d,$a,$d,0
InitCOP1LCH:
		dc.b	"    Set Start of copper (COP1LCH $dff080): ",0
InitCOPJMP1:
	dc.b	"    Starting Copper (COPJMP1 $dff088): ",0
InitDMACON:
	dc.b	"    Set all DMA enablebits (DMACON $dff096) to Enabled: ",0
InitBEAMCON0:
	dc.b	"    Set Beam Conter control register to 32 (PAL) (BEAMCON0 $dff1dc): ",0
InitPOTGO:
	dc.b	"    Set POTGO to all OUTPUT ($FF00) (POTGO $dff034): ",0
InitDONEtxt:
	dc.b	"Done",$a,$d,0
Bpl1attxt:
	dc.b	"   - Bitplane 1 at: $",0
Bpl2attxt:
	dc.b	"   - Bitplane 2 at: $",0
Bpl3attxt:
	dc.b	"   - Bitplane 3 at: $",0
WorkAreasTxt:
	dc.b	$a,"Extra workareas Chipmem: ",0
WorkAreasTxt2:
	dc.b	"  Fastmem: ",0	
MinusTxt:
	dc.b	" - ",0	
InitTxt:
	dc.b	"Amiga DiagROM "
	VERSION
	dc.b	" - By John (Chucky/The Gang) Hertell - "
	incbin	"builddate.i"
	dc.b	$a,$d,$a,$d,0

	EVEN

NoDrawTxt:
       dc.b	"We are in a nonchip/nodraw mode. Serialoutput is all we got.",$a,$d
	dc.b	"colourflash on screen is actually chars that should be printed on screen.",$a,$d
	dc.b	"Just to tell user something happens",$a,$d,$a,$d,0

FastDetectTxt:
	dc.b	$a,$a,"Checking for fastmem",$a
	dc.b	"Pressing left mousebutton will cancel detection (if hanged)",$a,$a,0
A24BitTxt:
	dc.b	"Checking if a 24 Bit address cpu is used: ",0
A3k4kMemTxt:
	dc.b	" - Checking for A3000/A4000 Motherboardmemory",$a,0
CpuMemTxt:
	dc.b	" - Checking for CPU-Board Memory (most A3k/A4k)",$a,0
A1200CpuMemTxt:
	dc.b	" - Checking for CPU-Board Memory (most A1200)",$a,"    (WILL crash with A3640/A3660 and Maprom on)",$a,0
a24BitAreaTxt:
	dc.b	" - Checking for Memory in 24 Bit area (NON AUTOCONFIG)",$a,0
FakeFastTxt:
	dc.b	" - Checking for Memory in Ranger or Fakefast area",$a,0
BPPCtxt:
	dc.b	"   - BPPC Found, detecting in a smaller memoryarea",$a,0
FastFoundtxt:
	dc.b	"  - Fastmem found between: $",0
MinusDTxt:
	dc.b	" - $",0
IfSoldTxt:
	;12345678901234567890123456789012345678901234567890123456789012345678901234567890
	dc.b	$a,$a,"IF This ROM is sold, if above 10eur+hardware cost 25% MUST be donated to",$a
	dc.b	"an LEGITIMATE charity of some kind, like curing cancer for example... ",$a
	dc.b	"If you paid more than 10Eur + Hardware + Shipping, please ask what charity you",$a
	dc.b	"have supported!!!      This software is fully open source and free to use.",$a
	dc.b	"Go to www.diagrom.com or http://github.com/ChuckyGang/DiagROM2 for information",$a,$a,0

InitSerial2:
	dc.b	$a,$d,"Please read the readme.txt file in the download archive for instructions"
	dc.b	$a,$d,"DiagROM is mainly for people with technical knowledge of the Amiga"
	dc.b	$a,$d,"and might not be fully 'stright forward' for all - Delivered AS IS"
	dc.b	$a,$d,$a,$d,"To use serial communication please hold down ANY key now",$a,$d
	dc.b	"OR click the RIGHT mousebutton.",$a,$d,0
EndSerial:
	dc.b	27,"[0m",$a,$d,"No key pressed, disabling any serialcommunications.",$a,$d,0
DotTxt:
	dc.b	".",0
	
RomMenuCopper:
       MenuSprite:
              dc.l	$01200000,$01220000,$01240000,$01260000,$01280000,$012a0000,$012c0000,$012e0000,$01300000,$01320000,$01340000,$01360000,$01380000,$013a0000,$013c0000,$013e0000
       
              dc.l	$0100b200,$0092003c,$009400d4,$008e2c81,$00902cc1,$01020000,$01080000,$010a0000
              dc.l	$01800000,$01820f00,$018400f0,$01860ff0,$0188000f,$018a0f0f,$018c00ff,$018e0fff,$01900ff0
       
       MenuBplPnt:
              dc.l	$00e00000,$00e20000,$00e40000,$00e60000,$00e80000,$00ea0000
              dc.l	$fffffffe	;End of copperlist
EndRomMenuCopper:

RomEcsCopper:
       dc.l	$01200000,$01220000,$01240000,$01260000,$01280000,$012a0000,$012c0000,$012e0000,$01300000,$01320000,$01340000,$01360000,$0138000,$013a0000,$013c0000,$013e0000
       dc.l	$01005200,$00920038,$009400d0,$008e2c81,$00902cc1,$01020000,$01080000,$010a0000

       blk.l	32,0
;MenuBplPnt2:
       dc.l	$00e00000,$00e20000,$00e40000,$00e60000,$00e80000,$00ea0000,$00ec0000,$00ee0000,$00f00000,$00f20000

       dc.l	$fffffffe	;End of copperlist
EndRomEcsCopper:

RomEcsCopper2:
       dc.l	$01200000,$01220000,$01240000,$01260000,$01280000,$012a0000,$012c0000,$012e0000,$01300000,$01320000,$01340000,$01360000,$0138000,$013a0000,$013c0000,$013e0000
       dc.l	$01005200,$00920038,$009400d0,$008e2c81,$00902cc1,$01020000,$01080004,$010a0004

       blk.l	32,0
;MenuBplPnt2:
       dc.l	$00e00000,$00e20000,$00e40000,$00e60000,$00e80000,$00ea0000,$00ec0000,$00ee0000,$00f00000,$00f20000

       dc.l	$fffffffe	;End of copperlist
EndRomEcsCopper2:



       ;      This is the protrackerreplayer, to be copied to chipmem due to the fact it would require too mucg
	; 	of reprogramming to make it work in ROM. <-  YUPP!  being a lazy fuck



* ProTracker2.2a replay routine by Crayon/Noxious. Improved and modified
* by Teeme of Fist! Unlimited in 1992. Share and enjoy! :)
* Rewritten for Devpac (slightly..) by CJ. Devpac does not like bsr.L
* cmpi is compare immediate, it requires immediate data! And some
* labels had upper/lower case wrong...
*
* Now improved to make it work better if CIA timed - thanks Marco!

* Call MT_Init with A0 pointing to your module data...
* mastervolumepatch by Chucky of The Gang
* mt_mastervol is a byte  0 to 64 containing the mastervolume


N_Note = 0  ; W
N_Cmd = 2  ; W
N_Cmdlo = 3  ; B
N_Start = 4  ; L
N_Length = 8  ; W
N_LoopStart = 10 ; L
N_Replen = 14 ; W
N_Period = 16 ; W
N_FineTune = 18 ; B
N_Volume = 19 ; B
N_DMABit = 20 ; W
N_TonePortDirec = 22 ; B
N_TonePortSpeed = 23 ; B
N_WantedPeriod = 24 ; W
N_VibratoCmd = 26 ; B
N_VibratoPos = 27 ; B
N_TremoloCmd = 28 ; B
N_TremoloPos = 29 ; B
N_WaveControl = 30 ; B
N_GlissFunk = 31 ; B
N_SampleOffset = 32 ; B
N_PattPos = 33 ; B
N_LoopCount = 34 ; B
N_FunkOffset = 35 ; B
N_WaveStart = 36 ; L
N_RealLength = 40 ; W
MT_SizeOf = 42*4+22
MT_SongDataPtr = -18
MT_Speed = -14
MT_Counter = -13
MT_SongPos = -12
MT_PBreakPos = -11
MT_PosJumpFlag = -10
MT_PBreakFlag = -9
MT_LowMask = -8
MT_PattDelTime = -7
MT_PattDelTime2 = -6
MT_PatternPos = -4
MT_DMACONTemp = -2
MT_CiaSpeed = 0
MT_Signal = 2
MT_TimerSpeed = 4
MT_CiaBase = 8
MT_CiaTimer = 12
MT_Volume = 14

MT_Init:
	move.l	a5,-(sp)
	lea	MT_Variables(pc),a5
	move.l	a0,MT_SongDataPtr(a5)
	lea	952(a0),a1
	moveq	#127,D0
	moveq	#0,D1
MTLoop:
	move.l	d1,d2
	subq.w	#1,d0
MTLoop2:
	move.b	(a1)+,d1
	cmp.b	d2,d1
	bgt.s	MTLoop
	dbf	d0,MTLoop2
	addq.b	#1,d2
			
	move.l	a5,a1
	suba.w	#142,a1
	asl.l	#8,d2
	asl.l	#2,d2
	addi.l	#1084,d2
	add.l	a0,d2
	move.l	d2,a2
	moveq	#30,d0
MTLoop3:
;	clr.l	(a2)
	move.l	a2,(a1)+
	moveq	#0,d1
	move.w	42(a0),d1
	add.l	d1,d1
	add.l	d1,a2
	adda.w	#30,a0
	dbf	d0,MTLoop3

	ori.b	#2,$bfe001
	move.b	#6,MT_Speed(a5)
	clr.b	MT_Counter(a5)
	clr.b	MT_SongPos(a5)
	clr.w	MT_PatternPos(a5)
	move.l	(sp)+,a5
MT_End:	clr.w	$0A8(a5)
	clr.w	$0B8(a5)
	clr.w	$0C8(a5)
	clr.w	$0D8(a5)
	move.w	#$f,$096(a5)
	rts

MT_Music:
	movem.l	d0-d4/a0-a6,-(a7)
	move.l	a5,a6
	lea	MT_Variables(pc),a5
	addq.b	#1,MT_Counter(a5)
	move.b	MT_Counter(a5),d0
	cmp.b	MT_Speed(a5),d0
	blo.s	MT_NoNewNote
	clr.b	MT_Counter(a5)
	tst.b	MT_PattDelTime2(a5)
	beq	MT_GetNewNote
	bsr.s	MT_NoNewAllChannels
	bra	MT_Dskip

	IFD	ST_CiaOn
MT_SetCia
	movem.l	a0/d0/d2,-(sp)
	cmp.w	#32,d0
	bge.s	.right
	moveq.l	#32,d0
.right	and.w	#$FF,d0
	move.w	d0,MT_CiaSpeed(a5)
	move.l	MT_TimerSpeed(a5),d2
	divu	d0,d2
	tst.w	MT_CiaTimer(a5)
	beq.s	.settia
	move.l	MT_CiaBase(a5),a0
	move.b	d2,ciatblo(a0)
	lsr.w	#8,d2
	move.b	d2,ciatbhi(a0)
.skip	movem.l	(sp)+,a0/d0/d2
	rts
.settia	move.l	MT_CiaBase(a5),a0
	move.b	d2,ciatalo(a0)
	lsr.w	#8,d2
	move.b	d2,ciatahi(a0)
	movem.l	(sp)+,a0/d0/d2
	rts
	ENDC

MT_NoNewNote:
	bsr.s	MT_NoNewAllChannels
	bra	MT_NoNewPosYet
MT_NoNewAllChannels:
	move.w	#$a0,d5
	move.l	a5,a4
	suba.w	#318,a4
	bsr	MT_CheckEfx
	move.w	#$b0,d5
	adda.w	#44,a4
	bsr	MT_CheckEfx
	move.w	#$c0,d5
	adda.w	#44,a4
	bsr	MT_CheckEfx
	move.w	#$d0,d5
	adda.w	#44,a4
	bra	MT_CheckEfx
MT_GetNewNote:
	move.l	MT_SongDataPtr(a5),a0
	lea	12(a0),a3
	lea	952(a0),a2	;pattpo
	lea	1084(a0),a0	;patterndata
	moveq	#0,d0
	moveq	#0,d1
	move.b	MT_SongPos(a5),d0
	move.b	(a2,d0.w),d1
	asl.l	#8,d1
	asl.l	#2,d1
	add.w	MT_PatternPos(a5),d1
	clr.w	MT_DMACONTemp(a5)

	move.w	#$a0,d5
	move.l	a5,a4
	suba.w	#318,a4
	bsr.s	MT_PlayVoice
	move.w	#$b0,d5
	adda.w	#44,a4
	bsr.s	MT_PlayVoice
	move.w	#$c0,d5
	adda.w	#44,a4
	bsr.s	MT_PlayVoice
	move.w	#$d0,d5
	adda.w	#44,a4
	bsr.s	MT_PlayVoice
	bra	MT_SetDMA

MT_PlayVoice:
	tst.l	(a4)
	bne.s	MT_PlvSkip
	bsr	MT_PerNop
MT_PlvSkip:
	move.l	(a0,d1.l),(a4)
	addq.l	#4,d1
	moveq	#0,d2
	move.b	N_Cmd(a4),d2
	andi.b	#$f0,d2
	lsr.b	#4,d2
	move.b	(a4),d0
	andi.b	#$f0,d0
	or.b	d0,d2
	beq	MT_SetRegs
	moveq	#0,d3
	move.l	a5,a1
	suba.w	#142,a1
	move	d2,d4
	subq.l	#1,d2
	asl.l	#2,d2
	mulu	#30,d4
	move.l	(a1,d2.l),N_Start(a4)
	move.w	(a3,d4.l),N_Length(a4)
	move.w	(a3,d4.l),N_RealLength(a4)
	move.b	2(a3,d4.l),N_FineTune(a4)
	move.b	3(a3,d4.l),N_Volume(a4)
	move.w	4(a3,d4.l),d3 ; Get repeat
	beq.s	MT_NoLoop
	move.l	N_Start(a4),d2 ; Get start
	add.w	d3,d3
	add.l	d3,d2		; Add repeat
	move.l	d2,N_LoopStart(a4)
	move.l	d2,N_WaveStart(a4)
	move.w	4(a3,d4.l),d0	; Get repeat
	add.w	6(a3,d4.l),d0	; Add replen
	move.w	d0,N_Length(a4)
	move.w	6(a3,d4.l),N_Replen(a4)	; Save replen
	moveq	#0,d0
	move.b	N_Volume(a4),d0
	mulu	MT_Volume(a5),d0
	lsr.w	#6,d0
	bsr	mt_MasterVolume
	move.w	d0,8(a6,d5.w)	; Set volume
	bra.s	MT_SetRegs

MT_NoLoop:
	move.l	N_Start(a4),d2
	add.l	d3,d2
	move.l	d2,N_LoopStart(a4)
	move.l	d2,N_WaveStart(a4)
	move.w	6(a3,d4.l),N_Replen(a4)	; Save replen
	moveq	#0,d0
	mulu	MT_Volume(a5),d0
	lsr.w	#6,d0
	move.b	N_Volume(a4),d0
	bsr	mt_MasterVolume
	move.w	d0,8(a6,d5.w)	; Set volume
MT_SetRegs:
	move.w	(a4),d0
	andi.w	#$0fff,d0
	beq	MT_CheckMoreEfx	; If no note
	move.w	2(a4),d0
	andi.w	#$0ff0,d0
	cmpi.w	#$0e50,d0
	beq.s	MT_DoSetFineTune
	move.b	2(a4),d0
	andi.b	#$0f,d0
	cmpi.b	#3,d0	; TonePortamento
	beq.s	MT_ChkTonePorta
	cmpi.b	#5,d0
	beq.s	MT_ChkTonePorta
	cmpi.b	#9,d0	; Sample Offset
	bne.s	MT_SetPeriod
	bsr	MT_CheckMoreEfx
	bra.s	MT_SetPeriod

MT_DoSetFineTune:
	bsr	MT_SetFineTune
	bra.s	MT_SetPeriod

MT_ChkTonePorta:
	bsr	MT_SetTonePorta
	bra	MT_CheckMoreEfx

MT_SetPeriod:
	movem.l	d0-d1/a0-a1,-(a7)
	move.w	(a4),d1
	andi.w	#$0fff,d1
	lea	MT_PeriodTable(pc),a1
	moveq	#0,d0
	moveq	#36,d7
MT_FtuLoop:
	cmp.w	(a1,d0.w),d1
	bhs.s	MT_FtuFound
	addq.l	#2,d0
	dbf	d7,MT_FtuLoop
MT_FtuFound:
	moveq	#0,d1
	move.b	N_FineTune(a4),d1
	mulu	#72,d1
	add.l	d1,a1
	move.w	(a1,d0.w),N_Period(a4)
	movem.l	(a7)+,d0-d1/a0-a1

	move.w	2(a4),d0
	andi.w	#$0ff0,d0
	cmpi.w	#$0ed0,d0 ; Notedelay
	beq	MT_CheckMoreEfx

	move.w	N_DMABit(a4),$096(a6)
	btst	#2,N_WaveControl(a4)
	bne.s	MT_Vibnoc
	clr.b	N_VibratoPos(a4)
MT_Vibnoc:
	btst	#6,N_WaveControl(a4)
	bne.s	MT_Trenoc
	clr.b	N_TremoloPos(a4)
MT_Trenoc:
	move.l	N_Start(a4),(a6,d5.w)	; Set start
	move.w	N_Length(a4),4(a6,d5.w)	; Set length
	move.w	N_Period(a4),d0
	move.w	d0,6(a6,d5.w)		; Set period
	move.w	N_DMABit(a4),d0
	or.w	d0,MT_DMACONTemp(a5)
	bra	MT_CheckMoreEfx
 
MT_SetDMA:
	bsr	MT_DMAWaitLoop
	move.w	MT_DMACONTemp(a5),d0
	ori.w	#$8000,d0
	move.w	d0,$096(a6)
	bsr	MT_DMAWaitLoop
	move.l	a5,a4
	suba.w	#186,a4
	move.l	N_LoopStart(a4),$d0(a6)
	move.w	N_Replen(a4),$d4(a6)
	suba.w	#44,a4
	move.l	N_LoopStart(a4),$c0(a6)
	move.w	N_Replen(a4),$c4(a6)
	suba.w	#44,a4
	move.l	N_LoopStart(a4),$b0(a6)
	move.w	N_Replen(a4),$b4(a6)
	suba.w	#44,a4
	move.l	N_LoopStart(a4),$a0(a6)
	move.w	N_Replen(a4),$a4(a6)

MT_Dskip:
	addi.w	#16,MT_PatternPos(a5)
	move.b	MT_PattDelTime(a5),d0
	beq.s	MT_Dskc
	move.b	d0,MT_PattDelTime2(a5)
	clr.b	MT_PattDelTime(a5)
MT_Dskc:
	tst.b	MT_PattDelTime2(a5)
	beq.s	MT_Dska
	subq.b	#1,MT_PattDelTime2(a5)
	beq.s	MT_Dska
	sub.w	#16,MT_PatternPos(a5)
MT_Dska:
	tst.b	MT_PBreakFlag(a5)
	beq.s	MT_Nnpysk
	clr.b	MT_PBreakFlag(a5)
	moveq	#0,d0
	move.b	MT_PBreakPos(a5),d0
	clr.b	MT_PBreakPos(a5)
	lsl.w	#4,d0
	move.w	d0,MT_PatternPos(a5)
MT_Nnpysk:
	cmpi.w	#1024,MT_PatternPos(a5)
	blo.s	MT_NoNewPosYet
MT_NextPosition:	
	moveq	#0,d0
	move.b	MT_PBreakPos(a5),d0
	lsl.w	#4,d0
	move.w	d0,MT_PatternPos(a5)
	clr.b	MT_PBreakPos(a5)
	clr.b	MT_PosJumpFlag(a5)
	addq.b	#1,MT_SongPos(a5)
	andi.b	#$7F,MT_SongPos(a5)
	move.b	MT_SongPos(a5),d1
	move.l	MT_SongDataPtr(a5),a0
	cmp.b	950(a0),d1
	blo.s	MT_NoNewPosYet
	clr.b	MT_SongPos(a5)
	st	MT_Signal(a5)
MT_NoNewPosYet:	
	tst.b	MT_PosJumpFlag(a5)
	bne.s	MT_NextPosition
	movem.l	(a7)+,d0-d4/a0-a6
	rts

MT_CheckEfx:
	bsr	MT_UpdateFunk
	move.w	N_Cmd(a4),d0
	andi.w	#$0fff,d0
	beq.s	MT_PerNop
	move.b	N_Cmd(a4),d0
	andi.b	#$0f,d0
	beq.s	MT_Arpeggio
	cmpi.b	#1,d0
	beq	MT_PortaUp
	cmpi.b	#2,d0
	beq	MT_PortaDown
	cmpi.b	#3,d0
	beq	MT_TonePortamento
	cmpi.b	#4,d0
	beq	MT_Vibrato
	cmpi.b	#5,d0
	beq	MT_TonePlusVolSlide
	cmpi.b	#6,d0
	beq	MT_VibratoPlusVolSlide
	cmpi.b	#$E,d0
	beq	MT_E_Commands
SetBack:
	move.w	N_Period(a4),6(a6,d5.w)
	cmpi.b	#7,d0
	beq	MT_Tremolo
	cmpi.b	#$a,d0
	beq	MT_VolumeSlide
MT_Return2:
	rts

MT_PerNop:
	move.b	N_Volume(a4),d0
	mulu	MT_Volume(a5),d0
	lsr.w	#6,d0
	bsr	mt_MasterVolume
	move.w	d0,8(a6,d5.w)	; Set volume
	move.w	N_Period(a4),6(a6,d5.w)
	rts

MT_Arpeggio:
	moveq	#0,d0
	move.b	MT_Counter(a5),d0
	divs	#3,d0
	swap	d0
	tst.w	D0
	beq.s	MT_Arpeggio2
	cmpi.w	#2,d0
	beq.s	MT_Arpeggio1
	moveq	#0,d0
	move.b	N_Cmdlo(a4),d0
	lsr.b	#4,d0
	bra.s	MT_Arpeggio3

MT_Arpeggio1:
	moveq	#0,d0
	move.b	N_Cmdlo(a4),d0
	andi.b	#15,d0
	bra.s	MT_Arpeggio3

MT_Arpeggio2:
	move.w	N_Period(a4),d2
	bra.s	MT_Arpeggio4

MT_Arpeggio3:
	add.w	d0,d0
	moveq	#0,d1
	move.b	N_FineTune(a4),d1
	mulu	#72,d1
	lea	MT_PeriodTable(pc),a0
	add.w	d1,a0
	moveq	#0,d1
	move.w	N_Period(a4),d1
	moveq	#36,d7
MT_ArpLoop:
	move.w	(a0,d0.w),d2
	cmp.w	(a0),d1
	bhs.s	MT_Arpeggio4
	addq.w	#2,a0
	dbf	d7,MT_ArpLoop
	rts

MT_Arpeggio4:
	move.w	d2,6(a6,d5.w)
	rts

MT_FinePortaUp:
	tst.b	MT_Counter(a5)
	bne.w	MT_Return2
	move.b	#$0f,MT_LowMask(a5)
MT_PortaUp:
	moveq	#0,d0
	move.b	N_Cmdlo(a4),d0
	and.b	MT_LowMask(a5),d0
	st	MT_LowMask(a5)
	sub.w	d0,N_Period(a4)
	move.w	N_Period(a4),d0
	andi.w	#$0fff,d0
	cmpi.w	#113,d0
	bpl.s	MT_PortaUskip
	andi.w	#$f000,N_Period(a4)
	ori.w	#113,N_Period(a4)
MT_PortaUskip:
	move.w	N_Period(a4),d0
	andi.w	#$0fff,d0
	move.w	d0,6(a6,d5.w)
	rts
 
MT_FinePortaDown:
	tst.b	MT_Counter(a5)
	bne	MT_Return2
	move.b	#$0f,MT_LowMask(a5)
MT_PortaDown:
	clr.w	d0
	move.b	N_Cmdlo(a4),d0
	and.b	MT_LowMask(a5),d0
	st	MT_LowMask(a5)
	add.w	d0,N_Period(a4)
	move.w	N_Period(a4),d0
	andi.w	#$0fff,d0
	cmpi.w	#856,d0
	bmi.s	MT_PortaDskip
	andi.w	#$f000,N_Period(a4)
	ori.w	#856,N_Period(a4)
MT_PortaDskip:
	move.w	N_Period(a4),d0
	andi.w	#$0fff,d0
	move.w	d0,6(a6,d5.w)
	rts

MT_SetTonePorta:
	move.l	a0,-(a7)
	move.w	(a4),d2
	andi.w	#$0fff,d2
	moveq	#0,d0
	move.b	N_FineTune(a4),d0
	mulu	#74,d0
	lea	MT_PeriodTable(pc),a0
	add.w	d0,a0
	moveq	#0,d0
MT_StpLoop:
	cmp.w	(a0,d0.w),d2
	bhs.s	MT_StpFound
	addq.w	#2,d0
	cmpi.w	#74,d0
	blo.s	MT_StpLoop
	moveq	#70,d0
MT_StpFound:
	move.b	N_FineTune(a4),d2
	andi.b	#8,d2
	beq.s	MT_StpGoss
	tst.w	d0
	beq.s	MT_StpGoss
	subq.w	#2,d0
MT_StpGoss:
	move.w	(a0,d0.w),d2
	move.l	(a7)+,a0
	move.w	d2,N_WantedPeriod(a4)
	move.w	N_Period(a4),d0
	clr.b	N_TonePortDirec(a4)
	cmp.w	d0,d2
	beq.s	MT_ClearTonePorta
	bge	MT_Return2
	move.b	#1,N_TonePortDirec(a4)
	rts

MT_ClearTonePorta:
	clr.w	N_WantedPeriod(a4)
	rts

MT_TonePortamento:
	move.b	N_Cmdlo(a4),d0
	beq.s	MT_TonePortNoChange
	move.b	d0,N_TonePortSpeed(a4)
	clr.b	N_Cmdlo(a4)
MT_TonePortNoChange:
	tst.w	N_WantedPeriod(a4)
	beq	MT_Return2
	moveq	#0,d0
	move.b	N_TonePortSpeed(a4),d0
	tst.b	N_TonePortDirec(a4)
	bne.s	MT_TonePortaUp
MT_TonePortaDown:
	add.w	d0,N_Period(a4)
	move.w	N_WantedPeriod(a4),d0
	cmp.w	N_Period(a4),d0
	bgt.s	MT_TonePortaSetPer
	move.w	N_WantedPeriod(a4),N_Period(a4)
	clr.w	N_WantedPeriod(a4)
	bra.s	MT_TonePortaSetPer

MT_TonePortaUp:
	sub.w	d0,N_Period(a4)
	move.w	N_WantedPeriod(a4),d0
	cmp.w	N_Period(a4),d0     	; was cmpi!!!!
	blt.s	MT_TonePortaSetPer
	move.w	N_WantedPeriod(a4),N_Period(a4)
	clr.w	N_WantedPeriod(a4)

MT_TonePortaSetPer:
	move.w	N_Period(a4),d2
	move.b	N_GlissFunk(a4),d0
	andi.b	#$0f,d0
	beq.s	MT_GlissSkip
	moveq	#0,d0
	move.b	N_FineTune(a4),d0
	mulu	#72,d0
	lea	MT_PeriodTable(pc),a0
	add.w	d0,a0
	moveq	#0,d0
MT_GlissLoop:
	cmp.w	(a0,d0.w),d2
	bhs.s	MT_GlissFound
	addq.w	#2,d0
	cmpi.w	#72,d0
	blo.s	MT_GlissLoop
	moveq	#70,d0
MT_GlissFound:
	move.w	(a0,d0.w),d2
MT_GlissSkip:
	move.w	d2,6(a6,d5.w) ; Set period
	rts

MT_Vibrato:
	move.b	N_Cmdlo(a4),d0
	beq.s	MT_Vibrato2
	move.b	N_VibratoCmd(a4),d2
	andi.b	#$0f,d0
	beq.s	MT_VibSkip
	andi.b	#$f0,d2
	or.b	d0,d2
MT_VibSkip:
	move.b	N_Cmdlo(a4),d0
	andi.b	#$f0,d0
	beq.s	MT_VibSkip2
	andi.b	#$0f,d2
	or.b	d0,d2
MT_VibSkip2:
	move.b	d2,N_VibratoCmd(a4)
MT_Vibrato2:
	move.b	N_VibratoPos(a4),d0
	lea	MT_VibratoTable(pc),a0
	lsr.w	#2,d0
	andi.w	#$001f,d0
	moveq	#0,d2
	move.b	N_WaveControl(a4),d2
	andi.b	#$03,d2
	beq.s	MT_Vib_Sine
	lsl.b	#3,d0
	cmpi.b	#1,d2
	beq.s	MT_Vib_RampDown
	st	d2
	bra.s	MT_Vib_Set
MT_Vib_RampDown:
	tst.b	N_VibratoPos(a4)
	bpl.s	MT_Vib_RampDown2
	st	d2
	sub.b	d0,d2
	bra.s	MT_Vib_Set
MT_Vib_RampDown2:
	move.b	d0,d2
	bra.s	MT_Vib_Set
MT_Vib_Sine:
	move.b	(a0,d0.w),d2
MT_Vib_Set:
	move.b	N_VibratoCmd(a4),d0
	andi.w	#15,d0
	mulu	d0,d2
	lsr.w	#7,d2
	move.w	N_Period(a4),d0
	tst.b	N_VibratoPos(a4)
	bmi.s	MT_VibratoNeg
	add.w	d2,d0
	bra.s	MT_Vibrato3
MT_VibratoNeg:
	sub.w	d2,d0
MT_Vibrato3:
	move.w	d0,6(a6,d5.w)
	move.b	N_VibratoCmd(a4),d0
	lsr.w	#2,d0
	andi.w	#$3C,d0
	add.b	d0,N_VibratoPos(a4)
	rts

MT_TonePlusVolSlide:
	bsr	MT_TonePortNoChange
	bra	MT_VolumeSlide

MT_VibratoPlusVolSlide:
	bsr.s	MT_Vibrato2
	bra	MT_VolumeSlide

MT_Tremolo:
	move.b	N_Cmdlo(a4),d0
	beq.s	MT_Tremolo2
	move.b	N_TremoloCmd(a4),d2
	andi.b	#$0f,d0
	beq.s	MT_TreSkip
	andi.b	#$f0,d2
	or.b	d0,d2
MT_TreSkip:
	move.b	N_Cmdlo(a4),d0
	and.b	#$f0,d0
	beq.s	MT_TreSkip2
	andi.b	#$0f,d2
	or.b	d0,d2
MT_TreSkip2:
	move.b	d2,N_TremoloCmd(a4)
MT_Tremolo2:
	move.b	N_TremoloPos(a4),d0
	lea	MT_VibratoTable(pc),a0
	lsr.w	#2,d0
	andi.w	#$1f,d0
	moveq	#0,d2
	move.b	N_WaveControl(a4),d2
	lsr.b	#4,d2
	andi.b	#3,d2
	beq.s	MT_Tre_Sine
	lsl.b	#3,d0
	cmpi.b	#1,d2
	beq.s	MT_Tre_RampDown
	st	d2
	bra.s	MT_Tre_Set
MT_Tre_RampDown:
	tst.b	N_VibratoPos(a4)
	bpl.s	MT_Tre_RampDown2
	st	d2
	sub.b	d0,d2
	bra.s	MT_Tre_Set
MT_Tre_RampDown2:
	move.b	d0,d2
	bra.s	MT_Tre_Set
MT_Tre_Sine:
	move.b	(a0,d0.w),d2
MT_Tre_Set:
	move.b	N_TremoloCmd(a4),d0
	andi.w	#15,d0
	mulu	d0,d2
	lsr.w	#6,d2
	moveq	#0,d0
	move.b	N_Volume(a4),d0
	tst.b	N_TremoloPos(a4)
	bmi.s	MT_TremoloNeg
	add.w	d2,d0
	bra.s	MT_Tremolo3
MT_TremoloNeg:
	sub.w	d2,d0
MT_Tremolo3:
	bpl.s	MT_TremoloSkip
	clr.w	d0
MT_TremoloSkip:
	cmpi.w	#$40,d0
	bls.s	MT_TremoloOk
	move.w	#$40,d0
MT_TremoloOk:
	mulu	MT_Volume(a5),d0
	lsr.w	#6,d0
	bsr	mt_MasterVolume
	move.w	d0,8(a6,d5.w)
	move.b	N_TremoloCmd(a4),d0
	lsr.w	#2,d0
	andi.w	#$3c,d0
	add.b	d0,N_TremoloPos(a4)
	rts

MT_SampleOffset:
	moveq	#0,d0
	move.b	N_Cmdlo(a4),d0
	beq.s	MT_SoNoNew
	move.b	d0,N_SampleOffset(a4)
MT_SoNoNew:
	move.b	N_SampleOffset(a4),d0
	lsl.w	#7,d0
	cmp.w	N_Length(a4),d0
	bge.s	MT_SofSkip
	sub.w	d0,N_Length(a4)
	add.w	d0,d0
	add.l	d0,N_Start(a4)
	rts
MT_SofSkip:
	move.w	#1,N_Length(a4)
	rts

MT_VolumeSlide:
	moveq	#0,d0
	move.b	N_Cmdlo(a4),d0
	lsr.b	#4,d0
	tst.b	d0
	beq.s	MT_VolSlideDown
MT_VolSlideUp:
	add.b	d0,N_Volume(a4)
	cmpi.b	#$40,N_Volume(a4)
	bmi.s	MT_VsuSkip
	move.b	#$40,N_Volume(a4)
MT_VsuSkip:
	move.b	N_Volume(a4),d0
	mulu	MT_Volume(a5),d0
	lsr.w	#6,d0
	bsr	mt_MasterVolume
	move.w	d0,8(a6,d5.w)
	rts

MT_VolSlideDown:
	moveq	#0,d0
	move.b	N_Cmdlo(a4),d0
	andi.b	#$0f,d0
MT_VolSlideDown2:
	sub.b	d0,N_Volume(a4)
	bpl.s	MT_VsdSkip
	clr.b	N_Volume(a4)
MT_VsdSkip:
	move.b	N_Volume(a4),d0
	mulu	MT_Volume(a5),d0
	lsr.w	#6,d0
	bsr	mt_MasterVolume
	move.w	d0,8(a6,d5.w)
	rts

MT_PositionJump
	move.b	N_Cmdlo(a4),d0
	subq.b	#1,d0
	cmp.b	MT_SongPos(a5),d0
	bge.s	.nosign
	st	MT_Signal(a5)
.nosign	move.b	d0,MT_SongPos(a5)
MT_PJ2	clr.b	MT_PBreakPos(a5)
	st 	MT_PosJumpFlag(a5)
	rts

MT_VolumeChange:
	moveq	#0,d0
	move.b	N_Cmdlo(a4),d0
	cmpi.b	#$40,d0
	bls.s	MT_VolumeOk
	moveq	#$40,d0
MT_VolumeOk:
	move.b	d0,N_Volume(a4)
	mulu	MT_Volume(a5),d0
	lsr.w	#6,d0
	bsr	mt_MasterVolume
	move.w	d0,8(a6,d5.w)
	rts

MT_PatternBreak:
	moveq	#0,d0
	move.b	N_Cmdlo(a4),d0
	move.l	d0,d2
	lsr.b	#4,d0
	mulu	#10,d0
	andi.b	#$0f,d2
	add.b	d2,d0
	cmpi.b	#63,d0
	bhi.s	MT_PJ2
	move.b	d0,MT_PBreakPos(a5)
	st	MT_PosJumpFlag(a5)
	rts

	IFD	ST_CiaOn
MT_SetSpeed:
	moveq.l	#0,d0
	move.b	3(a4),d0
	beq	MT_Return2
	cmp.b	#32,d0
	bhs.s	.ciatim
	clr.b	MT_Counter(a5)
	move.b	d0,MT_Speed(a5)
	rts
.ciatim	bra	MT_SetCia
	ELSE
MT_SetSpeed:
	moveq.l	#0,d0
	move.b	3(a4),d0
	beq	MT_Return2
	cmp.b	#32,d0
	bhs.s	.ciatim
	clr.b	MT_Counter(a5)
	move.b	d0,MT_Speed(a5)
.ciatim	rts
	ENDC

MT_CheckMoreEfx:
	bsr	MT_UpdateFunk
	move.b	2(a4),d0
	andi.b	#$0f,d0
	cmpi.b	#$9,d0
	beq	MT_SampleOffset
	cmpi.b	#$b,d0
	beq	MT_PositionJump
	cmpi.b	#$d,d0
	beq.s	MT_PatternBreak
	cmpi.b	#$e,d0
	beq.s	MT_E_Commands
	cmpi.b	#$f,d0
	beq.s	MT_SetSpeed
	cmpi.b	#$c,d0
	beq	MT_VolumeChange
	bra	MT_PerNop

MT_E_Commands:
	move.b	N_Cmdlo(a4),d0
	andi.b	#$f0,d0
	lsr.b	#4,d0
	beq.s	MT_FilterOnOff
	cmpi.b	#1,d0
	beq	MT_FinePortaUp
	cmpi.b	#2,d0
	beq	MT_FinePortaDown
	cmpi.b	#3,d0
	beq.s	MT_SetGlissControl
	cmpi.b	#4,d0
	beq	MT_SetVibratoControl
	cmpi.b	#5,d0
	beq	MT_SetFineTune
	cmpi.b	#6,d0
	beq	MT_JumpLoop
	cmpi.b	#7,d0
	beq	MT_SetTremoloControl
	cmpi.b	#9,d0
	beq	MT_RetrigNote
	cmpi.b	#$a,d0
	beq	MT_VolumeFineUp
	cmpi.b	#$b,d0
	beq	MT_VolumeFineDown
	cmpi.b	#$c,d0
	beq	MT_NoteCut
	cmpi.b	#$d,d0
	beq	MT_NoteDelay
	cmpi.b	#$e,d0
	beq	MT_PatternDelay
	cmpi.b	#$f,d0
	beq	MT_FunkIt
	rts

MT_FilterOnOff:
	move.b	N_Cmdlo(a4),d0
	andi.b	#1,d0
	add.b	d0,d0
	andi.b	#$fd,$bfe001
	or.b	d0,$bfe001
	rts

MT_SetGlissControl:
	move.b	N_Cmdlo(a4),d0
	andi.b	#$0f,d0
	andi.b	#$f0,N_GlissFunk(a4)
	or.b	d0,N_GlissFunk(a4)
	rts

MT_SetVibratoControl:
	move.b	N_Cmdlo(a4),d0
	andi.b	#$0f,d0
	andi.b	#$f0,N_WaveControl(a4)
	or.b	d0,N_WaveControl(a4)
	rts

MT_SetFineTune:
	move.b	N_Cmdlo(a4),d0
	andi.b	#$0f,d0
	move.b	d0,N_FineTune(a4)
	rts

MT_JumpLoop:
	tst.b	MT_Counter(a5)
	bne	MT_Return2
	move.b	N_Cmdlo(a4),d0
	andi.b	#$0f,d0
	beq.s	MT_SetLoop
	tst.b	N_LoopCount(a4)
	beq.s	MT_JumpCnt
	subq.b	#1,N_LoopCount(a4)
	beq	MT_Return2
MT_JmpLoop:
	move.b	N_PattPos(a4),MT_PBreakPos(a5)
	st	MT_PBreakFlag(a5)
	rts

MT_JumpCnt:
	move.b	d0,N_LoopCount(a4)
	bra.s	MT_JmpLoop

MT_SetLoop:
	move.w	MT_PatternPos(a5),d0
	lsr.w	#4,d0
	move.b	d0,N_PattPos(a4)
	rts

MT_SetTremoloControl:
	move.b	N_Cmdlo(a4),d0
*	andi.b	#$0f,d0
	lsl.b	#4,d0
	andi.b	#$0f,N_WaveControl(a4)
	or.b	d0,N_WaveControl(a4)
	rts

MT_RetrigNote:
	move.l	d1,-(a7)
	moveq	#0,d0
	move.b	N_Cmdlo(a4),d0
	andi.b	#$0f,d0
	beq.s	MT_RtnEnd
	moveq	#0,d1
	move.b	MT_Counter(a5),d1
	bne.s	MT_RtnSkp
	move.w	(a4),d1
	andi.w	#$0fff,d1
	bne.s	MT_RtnEnd
	moveq	#0,d1
	move.b	MT_Counter(a5),d1
MT_RtnSkp:
	divu	d0,d1
	swap	d1
	tst.w	d1
	bne.s	MT_RtnEnd
MT_DoRetrig:
	move.w	N_DMABit(a4),$096(a6)	; Channel DMA off
	move.l	N_Start(a4),(a6,d5.w)	; Set sampledata pointer
	move.w	N_Length(a4),4(a6,d5.w)	; Set length
	bsr	MT_DMAWaitLoop
	move.w	N_DMABit(a4),d0
	ori.w	#$8000,d0
*	bset	#15,d0
	move.w	d0,$096(a6)
	bsr	MT_DMAWaitLoop
	move.l	N_LoopStart(a4),(a6,d5.w)
	move.l	N_Replen(a4),4(a6,d5.w)
MT_RtnEnd:
	move.l	(a7)+,d1
	rts

MT_VolumeFineUp:
	tst.b	MT_Counter(a5)
	bne	MT_Return2
	moveq	#0,d0
	move.b	N_Cmdlo(a4),d0
	andi.b	#$0f,d0
	bra	MT_VolSlideUp

MT_VolumeFineDown:
	tst.b	MT_Counter(a5)
	bne	MT_Return2
	moveq	#0,d0
	move.b	N_Cmdlo(a4),d0
	andi.b	#$0f,d0
	bra	MT_VolSlideDown2

MT_NoteCut:
	moveq	#0,d0
	move.b	N_Cmdlo(a4),d0
	andi.b	#$0f,d0
	cmp.b	MT_Counter(a5),d0   ; was cmpi!!!
	bne	MT_Return2
	clr.b	N_Volume(a4)
	clr.w	8(a6,d5.w)
	rts

MT_NoteDelay:
	moveq	#0,d0
	move.b	N_Cmdlo(a4),d0
	andi.b	#$0f,d0
	cmp.b	MT_Counter(a5),d0   ; was cmpi!!!
	bne	MT_Return2
	move.w	(a4),d0
	beq	MT_Return2
	move.l	d1,-(a7)
	bra	MT_DoRetrig

MT_PatternDelay:
	tst.b	MT_Counter(a5)
	bne	MT_Return2
	moveq	#0,d0
	move.b	N_Cmdlo(a4),d0
	andi.b	#$0f,d0
	tst.b	MT_PattDelTime2(a5)
	bne	MT_Return2
	addq.b	#1,d0
	move.b	d0,MT_PattDelTime(a5)
	rts

MT_FunkIt:
	tst.b	MT_Counter(a5)
	bne	MT_Return2
	move.b	N_Cmdlo(a4),d0
*	andi.b	#$0f,d0
	lsl.b	#4,d0
	andi.b	#$0f,N_GlissFunk(a4)
	or.b	d0,N_GlissFunk(a4)
	tst.b	d0
	beq	MT_Return2
MT_UpdateFunk:
	movem.l	a0/d1,-(a7)
	moveq	#0,d0
	move.b	N_GlissFunk(a4),d0
	lsr.b	#4,d0
	beq.s	MT_FunkEnd
	lea	MT_FunkTable(pc),a0
	move.b	(a0,d0.w),d0
	add.b	d0,N_FunkOffset(a4)
	btst	#7,N_FunkOffset(a4)
	beq.s	MT_FunkEnd
	clr.b	N_FunkOffset(a4)

	move.l	N_LoopStart(a4),d0
	moveq	#0,d1
	move.w	N_Replen(a4),d1
	add.l	d1,d0
	add.l	d1,d0
	move.l	N_WaveStart(a4),a0
	addq.w	#1,a0
	cmp.l	d0,a0
	blo.s	MT_FunkOk
	move.l	N_LoopStart(a4),a0
MT_FunkOk:
	move.l	a0,N_WaveStart(a4)
	moveq	#-1,d0
	sub.b	(a0),d0
	move.b	d0,(a0)
MT_FunkEnd:
	movem.l	(a7)+,a0/d1
	rts

MT_DMAWaitLoop:
	move.w	d1,-(sp)
;	moveq	#5,d0		; wait 5+1 lines
;.loop	move.b	6(a6),d1		; read current raster position
;.wait	cmp.b	6(a6),d1
;	beq.s	.wait		; wait until it changes
;	dbf	d0,.loop		; do it again


	move.b	$dff006,d1
	add.b	#5,d1
.loop:	cmp.b	$dff006,d1
	bne.s	.loop

	move.w	(sp)+,d1
	rts


mt_MasterVolume:	; Patch by Chucky of The Gang
;	rts
			; Mastervolumesupport
			; IN:  D0 = Wanted volume
			; OUT: D0 = Real Volume after fade with mt_MasterVol
	movem.l D1-D3,-(SP)
	cmp.w	#0,d0
	beq.w	.Zero
			; First check if one chnnel is to be muted
	cmp.w	#$a0,d5	; chan1?
	bne.w	.chan1
	move.b	mt_Chan1(PC),d1
	cmp.b	#0,d1
	beq.w	.chan1
	bra	.Zero

.chan1:
	cmp.w	#$b0,d5	; chan2?
	bne.s	.chan2
	move.b	mt_Chan2(PC),d1
	cmp.b	#0,d1
	beq.s	.chan2
	bra	.Zero

.chan2:
	cmp.w	#$c0,d5	; chan3?
	bne.s	.chan3
	move.b	mt_Chan3(PC),d1
	cmp.b	#0,d1
	beq.s	.chan3
	bra	.Zero

.chan3:
	cmp.w	#$d0,d5	; chan4?
	bne.s	.chan4
	move.b	mt_Chan4(PC),d1
	cmp.b	#0,d1
	beq.s	.chan4
	bra	.Zero

.chan4:
	clr.l	d1
	move.b	mt_MasterVol(PC),d1
	clr.l	d2
	clr.l	d3
	move.w	d0,d2
	move.w	#64,d3
	cmp.w	#0,d2
	beq.s	.Zero
	divu	d2,d3
	cmp.w	#0,d3
	beq.s	.Zero
	divu	d3,d1	
	cmp.w	d1,d0
	blt	.stor
	bra.s	.exit
.Zero:
	clr.l	d1
.exit:
	move.l	d1,d0
	movem.l	(SP)+,D1-D3
	rts

.stor:
	move.w	d0,d1
	bra.s	.exit

MT_FunkTable:
	dc.b	0,5,6,7,8,10,11,13,16,19,22,26,32,43,64,128

MT_VibratoTable:
	dc.b	0,24,49,74,97,120,141,161
	dc.b	180,197,212,224,235,244,250,253
	dc.b	255,253,250,244,235,224,212,197
	dc.b	180,161,141,120,97,74,49,24

MT_PeriodTable:
; Tuning 0, Normal
	dc.w	856,808,762,720,678,640,604,570,538,508,480,453
	dc.w	428,404,381,360,339,320,302,285,269,254,240,226
	dc.w	214,202,190,180,170,160,151,143,135,127,120,113
; Tuning 1
	dc.w	850,802,757,715,674,637,601,567,535,505,477,450
	dc.w	425,401,379,357,337,318,300,284,268,253,239,225
	dc.w	213,201,189,179,169,159,150,142,134,126,119,113
; Tuning 2
	dc.w	844,796,752,709,670,632,597,563,532,502,474,447
	dc.w	422,398,376,355,335,316,298,282,266,251,237,224
	dc.w	211,199,188,177,167,158,149,141,133,125,118,112
; Tuning 3
	dc.w	838,791,746,704,665,628,592,559,528,498,470,444
	dc.w	419,395,373,352,332,314,296,280,264,249,235,222
	dc.w	209,198,187,176,166,157,148,140,132,125,118,111
; Tuning 4
	dc.w	832,785,741,699,660,623,588,555,524,495,467,441
	dc.w	416,392,370,350,330,312,294,278,262,247,233,220
	dc.w	208,196,185,175,165,156,147,139,131,124,117,110
; Tuning 5
	dc.w	826,779,736,694,655,619,584,551,520,491,463,437
	dc.w	413,390,368,347,328,309,292,276,260,245,232,219
	dc.w	206,195,184,174,164,155,146,138,130,123,116,109
; Tuning 6
	dc.w	820,774,730,689,651,614,580,547,516,487,460,434
	dc.w	410,387,365,345,325,307,290,274,258,244,230,217
	dc.w	205,193,183,172,163,154,145,137,129,122,115,109
; Tuning 7
	dc.w	814,768,725,684,646,610,575,543,513,484,457,431
	dc.w	407,384,363,342,323,305,288,272,256,242,228,216
	dc.w	204,192,181,171,161,152,144,136,128,121,114,108
; Tuning -8
	dc.w	907,856,808,762,720,678,640,604,570,538,508,480
	dc.w	453,428,404,381,360,339,320,302,285,269,254,240
	dc.w	226,214,202,190,180,170,160,151,143,135,127,120
; Tuning -7
	dc.w	900,850,802,757,715,675,636,601,567,535,505,477
	dc.w	450,425,401,379,357,337,318,300,284,268,253,238
	dc.w	225,212,200,189,179,169,159,150,142,134,126,119
; Tuning -6
	dc.w	894,844,796,752,709,670,632,597,563,532,502,474
	dc.w	447,422,398,376,355,335,316,298,282,266,251,237
	dc.w	223,211,199,188,177,167,158,149,141,133,125,118
; Tuning -5
	dc.w	887,838,791,746,704,665,628,592,559,528,498,470
	dc.w	444,419,395,373,352,332,314,296,280,264,249,235
	dc.w	222,209,198,187,176,166,157,148,140,132,125,118
; Tuning -4
	dc.w	881,832,785,741,699,660,623,588,555,524,494,467
	dc.w	441,416,392,370,350,330,312,294,278,262,247,233
	dc.w	220,208,196,185,175,165,156,147,139,131,123,117
; Tuning -3
	dc.w	875,826,779,736,694,655,619,584,551,520,491,463
	dc.w	437,413,390,368,347,328,309,292,276,260,245,232
	dc.w	219,206,195,184,174,164,155,146,138,130,123,116
; Tuning -2
	dc.w	868,820,774,730,689,651,614,580,547,516,487,460
	dc.w	434,410,387,365,345,325,307,290,274,258,244,230
	dc.w	217,205,193,183,172,163,154,145,137,129,122,115
; Tuning -1
	dc.w	862,814,768,725,684,646,610,575,543,513,484,457
	dc.w	431,407,384,363,342,323,305,288,272,256,242,228
	dc.w	216,203,192,181,171,161,152,144,136,128,121,114

MT_Chan1Temp:
	dc.l	0,0,0,0,0,$00010000,0,0,0,0,0
MT_Chan2Temp:
	dc.l	0,0,0,0,0,$00020000,0,0,0,0,0
MT_Chan3Temp:
	dc.l	0,0,0,0,0,$00040000,0,0,0,0,0
MT_Chan4Temp:
	dc.l	0,0,0,0,0,$00080000,0,0,0,0,0
MT_SampleStarts:
	dc.l	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dc.l	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
*MT_SongDataPtr:
	dc.l	0
*MT_Speed:
	dc.b	6
*MT_Counter:
	dc.b	0
*MT_SongPos:
	dc.b	0
*MT_PBreakPos:
	dc.b	0
*MT_PosJumpFlag:
	dc.b	0
*MT_PBreakFlag:
	dc.b	0
*MT_LowMask:
	dc.b	0
*MT_PattDelTime:
	dc.b	0
*MT_PattDelTime2:
	dc.b	0,0
*MT_PatternPos:
	dc.w	0
*MT_DMACONtemp:
	dc.w	0
MT_Variables:
*MT_CiaSpeed
	dc.w	125
*MT_Signal
	dc.w	0
*MT_TimerSpeed
	dc.l	0
*MT_CiaBase
	dc.l	0
*MT_CiaTimer
	dc.w	0
*MT_VolumeControl
	dc.w	64
*mt_data:
	dc.l	0
mt_MasterVol:
	dc.b	64
mt_Chan1:			; If not 0 channel is to be muted
	dc.b	0		; NO Data between Mastervol and this or you will have BUGS
mt_Chan2:
	dc.b	0
mt_Chan3:
	dc.b	0
mt_Chan4:
	dc.b	0
	EVEN
mt_END: