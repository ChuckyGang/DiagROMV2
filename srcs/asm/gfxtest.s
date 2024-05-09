       include "earlymacros.i"
       include "build/srcs/globalvars.i"
       section "gfxtest",code_p
       xref   GFXtestMenu
       xref   GFXTestScreen
       xref   GFXtest320x200
       xref   GFXTestScroll
       xref   GFXTestRaster
       xref   GFXTestRGB
 
LOWRESSize:	equ	40*256
HIRESSize:	equ	80*512

GFXtestMenu:
	bsr	InitScreen
	move.w	#5,MenuNumber(a6)
	move.b	#1,PrintMenuFlag(a6)
	bra	MainLoop      

GFXTestScreen:
	jsr	ClearScreen
	move.l	#testpicsize,d0
	move.l	d0,d2
	bsr	GetChip
	cmp.l	#0,d0
	beq	.exit
	cmp.l	#1,d0
	beq	.exit
	move.l	#LOWRESSize,d1
	move.l	ECSCopper(a6),a0			; Location of copperlist in memory
	lea	ECSTestColor,a1
	bsr	FixECSCopper
	move.l	d0,a0					; Copy the address of start of screen to a0
	lea	TestPic,a1				; Set a1 to where testscreen is in ROM
.loop:
	move.b	(a1)+,(a0)+				; Copy testimage to Chipmem
	dbf	d2,.loop
.exit:
	bsr	WaitButton
	jsr	SetMenuCopper
	bra	GFXtestMenu

GFXtest320x200:						; Actually this will be repogrammed totally!
	move.w	#$83f0,$dff096				; Turn on all DMA required
	jsr	ClearScreen
	move.l	#HIRESSize*5,d0
	bsr	GetChip
	cmp.l	#0,d0
	beq	.exit
	cmp.l	#1,d0
	beq	.exit
	move.l	#HIRESSize,d1
	move.l	ECSCopper(a6),a0			; Location of copperlist in memory
	lea	ECSColor32,a1
	bsr	FixECSCopper
	clr.l	d0
	clr.l	d1
	move.l	#640,d2
	move.l	#512,d3
	move.l	#6,d4
	bsr	DrawLine
	move.l	#640,d0
	clr.l	d1
	clr.l	d2
	move.l	#512,d3
	move.l	#6,d4
	bsr	DrawLine
	move.l	#640,d7
	move.l	#1,d2
	clr.l	d0
.loop6:
	move.l	#236,d1
	move.l	#40,d6
.loop5:
	bsr	PlotPixel
	add.l	#1,d1
	dbf	d6,.loop5
	move.l	d7,d2
	asr	#4,d2
	add.l	#1,d0
	dbf	d7,.loop6
	clr.l	d0
	move.l	#511,d1
.loop:
	move.l	#1,d2
	bsr	PlotPixel
	dbf	d1,.loop
	move.l	#640,d0
.loop2:
	move.l	#511,d1
	move.l	#1,d2
	bsr	PlotPixel
	dbf	d0,.loop2
	move.l	#639,d0
	move.l	#511,d1
.loop3:
	move.l	#1,d2
	bsr	PlotPixel
	dbf	d1,.loop3
	move.l	#639,d0
	clr.l	d1
.loop4:
	move.l	#1,d2
	bsr	PlotPixel
	dbf	d0,.loop4
	jsr	WaitButton
	move.w	#$3ff,$dff096				; Turn off all DMA
.exit:
	jsr	SetMenuCopper
	bsr	GFXtestMenu


GFXTestScroll:
	move.l	#LOWRESSize+1024,d1	
;	move.l	#LOWRESSize,d1
	move.l	ECSCopper2(a6),a0			; Location of copperlist in memory
	lea	ECSTestColor,a1
	bsr	FixECSCopper2
	move.l	d0,a0					; Copy the address of start of screen to a0
	move.l	d0,d7					; Make a backup of address
	lea	TestPic,a1				; Set a1 to where testscreen is in ROM
	move.w	#1279,d4
.loop2:
	move.w	#39,d3
.loop:
	move.b	(a1)+,(a0)+				; Copy testimage to Chipmem
	dbf	d3,.loop
	add.l	#4,a0
	dbf	d4,.loop2
	add.l	#62*44,d7
.scrollloop:
	cmp.b	#$bf,$dff006
	bne	.scrollloop
	bsr	.blit
	bsr	GetInput
	cmp.b	#1,BUTTON(a6)
	bne	.scrollloop
.exit:
	jsr	SetMenuCopper
	bra	GFXtestMenu
.blit:
	PUSH
	move.w	#4,d6
.blitloop:
	move.w	#113,d4
	move.l	d7,a0
	lea	JunkBuffer(a6),a1
.copyloop:
	move.b	(a0),d5
	and.b	#%10000000,d5
	lsr.l	#7,d5
	move.b	d5,(a1)+
	add.l	#44,a0
	dbf	d4,.copyloop
	clr.w	$dff042
	move.w	#$ffff,$dff044
	move.w	#$ffff,$dff046
	move.w	#$8040,$dff096
	move.w	#$f9f0,$dff040
	move.l	d7,d0
	sub.l	#2,d0
	move.l	d0,d1
	add.l	#2,d1
	move.l	d1,$dff050
	move.l	d0,$dff054
	move.w	#0,$dff066
	move.w	#0,$dff064	
	move.w	#114*64+22,$dff058
	move.b	d5,40(a0)
	VBLT
	move.w	#113,d4
	move.l	d7,a0
	lea	JunkBuffer(a6),a1
.copyloop2:
	move.b	(a1)+,d5
	and.b	#254,39(a0)
	or.b	d5,39(a0)
	add.l	#44,a0
	dbf	d4,.copyloop2
	add.l	#LOWRESSize+1024,d7
	dbf	d6,.blitloop
	POP
	rts



GFXTestRaster:
	jsr	ClearScreen
	move.l	#3,d1
	lea	GFXtestRasterTxt,a0
	jsr	Print
	lea	GFXtestRasterTxt2,a0
	jsr	Print
.loopa:
	bsr	GetInput
	move.l	#70,d0
	move.l	#160,d1
.loop:
	cmp.b	$dff006,d0
	bne	.loop
	add.b	#1,d0			; Wait for next rasterline
	move.b	d0,$dff181
	dbf	d1,.loop
	move.w	#0,$dff180
	cmp.b	#1,BUTTON(a6)
	bne	.loopa
	bra	GFXtestMenu




GFXTestRGB:
	jsr	ClearScreen
	move.l	#257*40,d0					; Amount of memory needed for one bitplan
	add.l	#GFXColTestCopperSize,d0	; Add for size of copperlist
	bsr	GetChip						; Get chipmem needed.
	cmp.l	#2,d0						; Check if 2 or lower
	ble	GFXtestMenu					; if so just exit (I know. bad move not to tell user)
	move.l	d0,a2						; Put start of memory in A2
	move.l	#$ffffffff,10(a2)
	move.l	#$ffffffff,22(a2)
	add.l	#40,a2
	move.l	#$ffffffff,16(a2)
	move.l	#$ffffffff,22(a2)
	move.l	d0,d5						; Make a backup of this address
	add.l	#80,d0						; Add 80 to d0, so we put copperlist after the "bitplane"
	move.l	d0,a2
	move.l	d0,d6
	lea	GFXColTestCopperStart,a1
	add.l	#GFXColTestCopperSize,d7	; Add for size of copperlist
.loop:
	move.b	(a1)+,(a2)+
	dbf	d7,.loop					; Copy in copperlist to start of memory
	move.l	d0,a0
	add.l	#GFXColTestCopperWaitPos,a0	; Fix a0 to where the wait block in copper list starts
	clr.l	d2						; Clear the testcolor
	move.l	a0,a1
	sub.l	#4*4-2,a1					; a1 will now contain address of bitplanepointers
	move.l	d5,d4
	swap	d4
	move.w	d4,(a1)
	add.l	#4,a1
	move.w	d5,(a1)
	add.l	#4,a1
	add.l	#40,d5						; Add for next bitplane
	move.l	d5,d4
	swap	d4
	move.w	d4,(a1)
	add.l	#4,a1
	move.w	d5,(a1)
	add.l	#4,a1
	move.b	#$18,d1						; What ROW to start colors at
	move.l	#15,d7						; Number of colors
.createloop:
	move.l	#$0001ff00,(a0)					; Write the wait command to copperlist
	move.b	d1,(a0)						; Replace first byte with the real row
	add.l	#4,a0
	move.w	#$0180,(a0)+
	move.w	#$fff,(a0)+
	move.w	#$0182,(a0)+
	move.w	d2,d3
	asl.w	#8,d3						; Make color red
	move.w	d3,(a0)+
	move.w	#$0184,(a0)+
	move.w	d2,d3
	asl.w	#4,d3						; Make color green
	move.w	d3,(a0)+
	move.w	#$0186,(a0)+
	move.w	d2,(a0)+					; Write color as blue
	add.b	#1,d1
	move.l	#$0001ff00,(a0)					; Write the wait command to copperlist
	move.b	d1,(a0)						; Replace first byte with the real row
	add.l	#4,a0
	move.w	#$0180,(a0)+
	move.w	#$0,(a0)+
	add.w	#1,d2
	add.b	#$e,d1
	dbf	d7,.createloop
	move.l	#$0001ff00,(a0)					; Write the wait command to copperlist
	move.b	d1,(a0)						; Replace first byte with the real row
	add.l	#4,a0
	move.w	#$0180,(a0)+
	move.w	#$fff,(a0)+
	add.b	#1,d1
	move.l	#$0001ff00,(a0)					; Write the wait command to copperlist
	move.b	d1,(a0)						; Replace first byte with the real row
	add.l	#4,a0
	move.w	#$0180,(a0)+
	move.w	#$0,(a0)+
	add.b	#$e,d1
	move.l	#$0001ff00,(a0)					; Write the wait command to copperlist
	move.b	d1,(a0)						; Replace first byte with the real row
	add.l	#4,a0
	move.w	#$0180,(a0)+
	move.w	#$fff,(a0)+
	add.b	#1,d1
	move.l	#$0001ff00,(a0)					; Write the wait command to copperlist
	move.b	d1,(a0)						; Replace first byte with the real row
	add.l	#4,a0
	move.w	#$0180,(a0)+
	move.w	#$0,(a0)+
;	move.l	#$0001ff00,(a0)					; Write the wait command to copperlist
;	move.b	d1,(a0)						; Replace first byte with the real row
	lea	InitCOP1LCH,a0
	jsr	SendSerial
	move.l	d0,$dff080			;Load new copperlist
	lea	InitDONEtxt,a0
	jsr	SendSerial
	lea	InitCOPJMP1,a0
	jsr	SendSerial
	move.w	$dff088,d0
	lea	InitDONEtxt,a0
	jsr	SendSerial
	lea	InitDMACON,a0
	jsr	SendSerial
	move.w	#$8380,$dff096
	lea	InitDONEtxt,a0
	jsr	SendSerial
	lea	InitBEAMCON0,a0
	jsr	SendSerial
	move.w	#32,$dff1dc
	lea	InitDONEtxt,a0
	jsr	SendSerial
	lea	GFXtestNoSerial,a0
	jsr	SendSerial
.loopa:
	bsr	GetInput
	cmp.b	#1,BUTTON(a6)
	bne	.loopa
	jsr	SetMenuCopper
	bra	GFXtestMenu





							; INDATA:
							;	a0 = ECSCopperlist
							;	a1 = List of colors to be set
							;	d0 = Startaddress of space
							;	d1 = Size of bytes of one screen
FixECSCopper:
	PUSH
	add.l	#96,a0					; Add so we get to the spot where palette starts.
	move.l	#31,d7
	move.w	#$180,d6				; Start with $180
.loop:
	move.w	d6,(a0)+
	move.w	(a1)+,(a0)+
	add.w	#2,d6
	dbf	d7,.loop				; Loop around and do all colors
	move.l	d0,d6
	lea	GfxTestBpl(a6),a2
	move.l	#4,d7
.loop2:
	move.l	d6,(a2)+
	move.w	d6,6(a0)
	swap	d6
	move.w	d6,2(a0)
	swap	d6
	add.l	#8,a0
	add.l	d1,d6
	dbf	d7,.loop2				; Set all bitplanepointers
.Slut:
	lea	InitCOP1LCH,a0
	jsr	SendSerial
	move.l	ECSCopper(a6),d0
	move.l	d0,$dff080			;Load new copperlist
	lea	InitDONEtxt,a0
	jsr	SendSerial
	lea	InitCOPJMP1,a0
	jsr	SendSerial
	move.w	$dff088,d0
	lea	InitDONEtxt,a0
	jsr	SendSerial
	lea	InitDMACON,a0
	jsr	SendSerial
	move.w	#$8380,$dff096
	lea	InitDONEtxt,a0
	jsr	SendSerial
	lea	InitBEAMCON0,a0
	jsr	SendSerial
	move.w	#32,$dff1dc			;Hmmm
	lea	InitDONEtxt,a0
	jsr	SendSerial
	lea	GFXtestNoSerial,a0
	jsr	SendSerial
;.exit:
	POP
	rts

FixECSCopper2:
	PUSH
	add.l	#96,a0					; Add so we get to the spot where palette starts.
	move.l	#31,d7
	move.w	#$180,d6				; Start with $180
.loop:
	move.w	d6,(a0)+
	move.w	(a1)+,(a0)+
	add.w	#2,d6
	dbf	d7,.loop				; Loop around and do all colors
	move.l	d0,d6
	lea.l	GfxTestBpl(a6),a2
	move.l	#4,d7
.loop2:
	move.l	d6,(a2)+
	move.w	d6,6(a0)
	swap	d6
	move.w	d6,2(a0)
	swap	d6
	add.l	#8,a0
	add.l	d1,d6
	dbf	d7,.loop2				; Set all bitplanepointers
.Slut:
	lea	InitCOP1LCH,a0
	jsr	SendSerial
	move.l	ECSCopper2(a6),d0
	move.l	d0,$dff080			;Load new copperlist
	lea	InitDONEtxt,a0
	jsr	SendSerial
	lea	InitCOPJMP1,a0
	jsr	SendSerial
	move.w	$dff088,d0
	lea	InitDONEtxt,a0
	jsr	SendSerial
	lea	InitDMACON,a0
	jsr	SendSerial
	move.w	#$8380,$dff096
	lea	InitDONEtxt,a0
	jsr	SendSerial
	lea	InitBEAMCON0,a0
	jsr	SendSerial
	move.w	#32,$dff1dc			;Hmmm
	lea	InitDONEtxt,a0
	jsr	SendSerial
	lea	GFXtestNoSerial,a0
	jsr	SendSerial
;.exit:
	POP
	rts

DrawLine:							
	PUSH
	asr	#1,d0
	asr	#1,d1
	asr	#1,d2
	asr	#1,d3
	lea	GfxTestBpl(a6),a5		; Load pointerlist for bitplanes
	move.l	#4,d7				; number of bitplanes to handle - 1
	clr.l	d6				; Clear d6, d6 is bit to test for palette
	move.l	#40,a1
.loop:
	move.l	(a5)+,a0				; A0 now contains address of bitplane
	btst	d6,d4				; Check if pixel is to be set of cleared
	bne.s	.set				; it is to be set
	bra.s	.clear				; if not, clear it
.set:
	move.l	#$ffffffff,a2
	bsr	.DrawLine
	bra	.done
.clear:
	move.l	#$0,a2
	bsr	.DrawLine
.done:
	add.l	#1,d6
	dbf	d7,.loop
	POP
	rts

.DrawLine:
	; d0 = x1
	; d1 = y1
	; d2 = x2
	; d3 = y2
	; a0 = Bitplanr
	; a1 = bitplanewidth in bytes
	; a2 = word written directly to mast register
	PUSH
;	asr.l	#1,d0
;	asr.l	#1,d1
;	asr.l	#1,d2
;	asr.l	#1,d3
	clr.l	d5	
	cmp.w	#320,d0
	ble	.nohighx1
	rts
.nohighx1:	
	cmp.w	#256,d1
	ble	.nohighy1
	rts
.nohighy1:	
	cmp.w	#640,d2
	ble	.nohighx2
	rts
.nohighx2:	
	cmp.w	#256,d3
	ble	.nohighy2
	rts
.nohighy2:	
	clr.l	d4
	add.w	d4,d2
	add.w	d4,d3
	move.l	a1,d4			; Width in work register
	mulu	d1,d4			;Y1 * byte per line
	moveq	#-$10,d5		; No leading characters $f0
	and.w	d0,d5			; Bottom four bits masked from x1
	lsr.w	#3,d5			; Reminder divided by 8
	add.w	d5,d4			; Y1 * bytes per line + x1/8
	add.l	a0,d4			; Plus startong adress of the bitplanes
	clr.l	d5
	sub.w	d1,d3			; Y2-Y1 DeltaY from D3
	roxl.b	#1,d5			; Shift leading char from DeltaY in D5
	tst.w	d3			; Restore N-Flag
	bge.s	.y2gy1			; When DeltaY positive, goto g2gy1
	neg.w	d3			; DeltaY invert (if not positive)
.y2gy1:
	sub.w	d0,d2			; X2-X1 DeltaX to D2
	roxl.b	#1,d5			; move leading char in DeltaX to d5
	tst.w	d2			; Restore N-Flag
	bge.s	.x2gx1			; When Delta X positive
	neg.w	d2			; DeltaX invert
.x2gx1:
	move.w	d3,d1			; DeltaY to d1
	sub.w	d2,d1			; DeltaY-DeltaX
	bge.s	.dygdx			; When DeltaY > DeltaX
	exg	d2,d3			; Smaller delta goto d2
.dygdx:	
	roxl.b	#1,d5			; D5 contains result of 3 comparisons
	lea	Octant_Table,a5
	move.b	(a5,d5),d5		; Get matching octants
	add.w	d2,d2			; Smaller Delta * 2
	VBLT
	move.w	d2,$dff062		;2*Smaller delta tp BLTBMOD
	sub.w	d3,d2			; 2*smaller delta - larger delta
	ble.s	.signn1			;When 2*small delta > largedelta to signn1
	or.b	#$40,d5			;Sign flag set
.signn1:
	move.w	d2,$dff052		; 2*smal delta - large delta in BLTAPTL
	sub.w	d3,d2			; 2*smaller delta -2*larger delta
	move.w	d2,$dff064		; tp BLTAMOD
	move.w	#$8000,$dff074		; BLTADAT
	move.w	a2,$dff072		; mask from a2 in BLTBDAT
	move.w	#$ffff,$dff044		; BLTAFWM
	and.w	#$000f,d0		; Bottom 4 bits from X1
	ror.w	#4,d0			; to START0-3
	or.w	#$0bca,d0		; USEx and LFx set
	move.w	d0,$dff040		; BLTCON0
	move.w	d5,$dff042		; Octant ib blitter BLTCON1
	move.l	d4,$dff048		; Start adress of line  BLTCPTH
	move.l	d4,$dff054		; BLTDPTH
	move.w	a1,$dff060		; Width of bitplanes i both BLTCMOD
	move.w	a1,$dff066		; and BLTDMOD registers
	lsl.l	#6,d3			; Length * 64
	addq.w	#2,d3			; Plus wodth=2
	move.w	d3,$dff058		; set size and start blit
	POP
	rts

PlotPixel:						; Plots a pixel
							; INDATA:
							;	D0 = XPos
							;	D1 = YPos
							;	D2 = Color
	PUSH
	asr.l	#1,d0
	asr.l	#1,d1
	move.l	d0,d4				; Make a copy of the X Cordinate
	asr	#3,d0				; Divide te XCordinate with 8 to get what byte to do stuff on
	move.l	d0,d3				; Make a copy of this byte
	asl	#3,d3				; multiply it with 8
	sub.l	d3,d4				; Diff it, so we know what BIT to set
	move.l	#7,d5				; But as it is "reversed" put 7 into d5 and
	sub.l	d4,d5				; Subtract but to do stuff in, d5 now contains bit
	mulu	#40,d1				; Multiply with 40 to get Y position
	add.l	d1,d0				; Add d1 to d0. so d0 now contains how much to add for the pixel
	lea	GfxTestBpl(a6),a2		; Load pointerlist for bitplanes
	move.l	#4,d7				; number of bitplanes to handle - 1
	clr.l	d6				; Clear d6, d6 is bit to test for palette
.loop:
	move.l	(a2)+,a0				; A0 now contains address of bitplane
	btst	d6,d2				; Check if pixel is to be set of cleared
	bne.s	.set				; it is to be set
	bra.s	.clear				; if not, clear it
.set:
	bset	d5,(a0,d0)
	bra	.done
.clear:
	bclr	d5,(a0,d0)
.done:
	add.l	#1,d6
	dbf	d7,.loop
	POP
	rts