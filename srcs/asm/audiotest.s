       include "earlymacros.i"
       include "build/srcs/globalvars.i"
       section "audiotest",code_p
       xdef AudioMenu
       xdef AudioSimple
       xdef AudioMod

AudioMenu:
       bsr	InitScreen
       move.w	#2,MenuNumber(a6)
       move.b	#1,PrintMenuFlag(a6)
       bra	MainLoop

AudioSimple:
       
       bsr	ClearScreen
       move.w	#0,MenuNumber(a6)
       move.b	#1,PrintMenuFlag(a6)
       move.l	#AudioSimpleMenu,Menu(a6)	; Set different menu
                                          ;	OK we have variables to handle here
       move.l	a6,d0
       lea	AudSimpChan1(a6),a0
       add.l	#AudSimpVar,d0
       move.l	d0,MenuVariable(a6)
                                         ; ok lets populate this with default values.
       move.b	#0,(a0)+
       move.b	#0,(a0)+
       move.b	#0,(a0)+
       move.b	#0,(a0)+
       move.b	#64,(a0)+
       move.b	#12,(a0)+
       move.b	#0,(a0)+
       move.b #0,(a0)+
       bsr	.setvar
.loop:
       bsr	.Playaudio
       bsr	PrintMenu
       bsr	GetInput
       bsr	WaitLong
       cmp.b	#0,d0
       beq	.no
       move.b	keyresult(a6),d1		; Read value from last keyboardread
       cmp.b	#$a,d1				; if it was enter, select this item
       beq	.action
       move.b	Serial(a6),d2			; Read value from last serialread
       cmp.b	#$a,d2
       beq	.action
       cmp.b	#1,LMB(a6)
       beq	.action
       cmp.b	#1,RMB(a6)
       beq	.action
       lea	AudioSimpleWaveKeys,a5		; Load list of keys in menu
       clr.l	d0				; Clear d0, this is selected item in list
.keyloop:
       move.b	(a5)+,d3				; Read item
       cmp.b	#0,d3				; Check if end of list
       beq	.nokey
       cmp.b	d1,d3				; fits with keyboardread?
       beq	.goaction
       cmp.b	d2,d3				; fits with serialread?
       beq	.goaction				; if so..  do it
       add.l	#1,d0				; Add one to d0, selecting next item
       bra	.keyloop
.goaction:
       move.b	d0,MenuPos(a6)
       bra	.action
.nokey:	
       cmp.b	#1,RMB
       beq	Exit
       btst	#1,d0
       beq	.no
.action:
       clr.l	d0
       move.b	MenuPos(a6),d0
       cmp.b	#0,d0
       beq	.chan1
       cmp.b	#1,d0
       beq	.chan2
       cmp.b	#2,d0
       beq	.chan3
       cmp.b	#3,d0
       beq	.chan4
       cmp.b	#4,d0
       beq	.vol
       cmp.b	#5,d0
       beq	.wave
       cmp.b	#6,d0
       beq	.filter
       cmp.b	#7,d0
       beq	.exit
.no:
       move.b	MenuPos(a6),d0
       cmp.b	#4,d0				; Check if we are on the line for volume
       bne.w	.novol
.volume:
       cmp.b	#0,AudioVolSelect(a6)		; Have volume been selected before?
       bne	.yesselected
       move.b	#1,AudioVolSelect(a6)
       move.l	#0,d0
       bsr	SetPos
       lea	AudioSimpleVolTxt,a0
       move.l	#2,d1
       bsr	Print
.yesselected
       cmp.b	#29,GetCharData(a6)
       bne	.noleft
       bsr	.voldown
       move.b	#5,UpdateMenuNumber(a6)
       move.b	#2,PrintMenuFlag(a6)
       bsr	.setvar
       bra	.volset
.noleft:
       cmp.b	#28,GetCharData(a6)
       bne	.volset
       bsr	.volup
       move.b	#5,UpdateMenuNumber(a6)
       move.b	#2,PrintMenuFlag(a6)
       bsr	.setvar
       bra	.volset
.novol:
       cmp.b	#0,AudioVolSelect(a6)		; Have volume been selected before?
       beq	.volset				; if 0, it haven't
       clr.b	AudioVolSelect(a6)
       move.l	#0,d0
       move.l	#22,d1
       bsr	SetPos
       lea	EmptyRowTxt,a0
       move.l	#0,d1
       bsr	Print
.volset:
       bra	.loop
.setvar:
       move.l	a6,a0
       move.l	a6,a1
       add.l	#AudSimpChan1,a0	
       add.l	#AudSimpVar,a1
       bsr	CheckOnOff
       move.w	d0,(a1)+			; Write color
       move.l	d1,(a1)+			; Write Stringpointer
       bsr	CheckOnOff
       move.w	d0,(a1)+			; Write color
       move.l	d1,(a1)+			; Write Stringpointer
       bsr	CheckOnOff
       move.w	d0,(a1)+			; Write color
       move.l	d1,(a1)+			; Write Stringpointer
       bsr	CheckOnOff
       move.w	d0,(a1)+			; Write color
       move.l	d1,(a1)+			; Write Stringpointer
       clr.l	d0
       move.b	(a0)+,d0			; Get Volume
       lea	AudSimpVolStr(a6),a2
       move.w	#3,(a1)+
       move.l	a2,(a1)+	
       PUSH	
       bsr	bindec
       move.l	#7,d0				; Lets copy output to a safe location
       move.l	#"    ",(a2)
.setvarloop:
       move.b	(a0)+,d7
       cmp.b	#0,d7				; end of string?
       beq	.decdone
       move.b	d7,(a2)+
       dbf	d0,.setvarloop
.decdone:
       POP
       add.l	#1,a0	
       move.w	#2,(a1)+			; Write color
       lea	AudSimpWave(a6),a2
       clr.l	d1
       move.b	(a2),d1
       asl.l	#2,d1
       lea.l	AudioName,a2
       move.l	(a2,d1.w),(a1)+			; Write Stringpointer
       bsr	CheckOnOff
       move.w	d0,(a1)+			; Write color
       move.l	d1,(a1)+			; Write Stringpointer
       rts
.Playaudio:
       clr.l	d0
       move.b	AudSimpWave(a6),d0
       move.l	d0,d1
       mulu	#2,d1
       mulu	#4,d0
       lea	AudioPointers,a2
       move.l	AudioWaves(a6),a0
       add.l	(a2,d0.l),a0
       lea	AudSimpChan1(a6),a1
       move.l	a0,$dff0a0			;Wave
       move.l	a0,$dff0b0			;Wave
       move.l	a0,$dff0c0			;Wave
       move.l	a0,$dff0d0			;Wave
       cmp.b	#0,(a1)+
       beq	.noch1
       clr.l	d7
       move.b	AudSimpVol(a6),d7
       move.w	d7,$dff0a8			;volume
       lea	AudioLen,a2
       move.w	(a2,d1),$dff0a4			;number of words
       lea    AudioPer,a2
       move.w	(a2,d1),$dff0a6
       move.w	#$8201,$dff096
       bra	.checkch2
.noch1:
       move.w	#$1,$dff096
.checkch2:
       cmp.b	#0,(a1)+
       beq	.noch2
       clr.l	d7
       move.b	AudSimpVol(a6),d7
       move.w	d7,$dff0b8			;volume
       lea	AudioLen,a2
       move.w	(a2,d1),$dff0b4			;number of words
       lea	AudioPer,a2
       move.w	(a2,d1),$dff0b6			;frequency
       move.w	#$8202,$dff096
       bra	.checkch3
.noch2:
       move.w	#$2,$dff096
.checkch3:
       cmp.b	#0,(a1)+
       beq	.noch3
       clr.l	d7
       move.b	AudSimpVol(a6),d7
       move.w	d7,$dff0c8			;volume
       lea	AudioLen,a2
       move.w	(a2,d1),$dff0c4			;number of words
       lea	AudioPer,a2
       move.w	(a2,d1),$dff0c6			;frequency
       move.w	#$8204,$dff096
       bra	.checkch4
.noch3:
       move.w	#$4,$dff096
.checkch4:
       cmp.b	#0,(a1)+
       beq	.noch4
       clr.l	d7
       move.b	AudSimpVol(a6),d7
       move.w	d7,$dff0d8			;volume
       lea	AudioLen,a2
       move.w	(a2,d1),$dff0d4			;number of words
       lea	AudioPer,a2
       move.w	(a2,d1),$dff0d6			;frequency
       move.w	#$8208,$dff096
       bra	.checkdone
.noch4:
       move.w	#$8,$dff096
.checkdone:
       rts
.chan1:
       bchg	#0,AudSimpChan1(a6)
       move.b	#1,UpdateMenuNumber(a6)
       move.b	#2,PrintMenuFlag(a6)
       bsr	CheckKeyReleased
       bsr	.setvar
       bra	.loop
.chan2:
       bchg	#0,AudSimpChan2(a6)
       move.b	#2,UpdateMenuNumber(a6)
       move.b	#2,PrintMenuFlag(a6)
       bsr	CheckKeyReleased
       bsr	.setvar
       bra	.loop
.chan3:
       bchg	#0,AudSimpChan3(a6)
       move.b	#3,UpdateMenuNumber(a6)
       move.b	#2,PrintMenuFlag(a6)
       bsr	CheckKeyReleased
       bsr	.setvar
       bra	.loop
.chan4:
       bchg	#0,AudSimpChan4(a6)
       move.b	#4,UpdateMenuNumber(a6)
       move.b	#2,PrintMenuFlag(a6)
       bsr	CheckKeyReleased
       bsr	.setvar
       bra	.loop
.vol:
       move.l	a6,a0
       cmp.b	#1,LMB(a6)		;Check if Left mousebutton was pressed
       bne	.nolmb			;if not skip
       bsr	.voldown
.nolmb:
       cmp.b	#1,RMB(a6)		;Check if Right mousebutton was pressed
       bne	.normb
       bsr	.volup
.normb:
       move.w	#4,(a0)+
       move.l	#OFF,(a0)+
       move.b	#5,UpdateMenuNumber(a6)
       move.b	#2,PrintMenuFlag(a6)
       bsr	CheckKeyReleased
       bsr	.setvar
       bra	.volume
.wave:
       TOGGLEPWRLED
       clr.l	d7
       move.b	AudSimpWave(a6),d7
       cmp.b	#17,d7
       beq	.notmax
       add.b	#1,d7
       bra	.wave2
.notmax:
       clr.l	d7
.wave2:
       move.b	d7,AudSimpWave(a6)
       move.b	#6,UpdateMenuNumber(a6)
       move.b	#2,PrintMenuFlag(a6)
       bsr	CheckKeyReleased
       bsr	.setvar
       bra	.loop
.filter:
       bchg	#0,AudSimpFilter(a6)
       btst	#0,AudSimpFilter(a6)
       beq	.off
       bclr	#1,$bfe001
       bra	.on
.off:
       bset	#1,$bfe001
.on:
       move.b	#7,UpdateMenuNumber(a6)
       move.b	#2,PrintMenuFlag(a6)
       bsr	CheckKeyReleased
       bsr	.setvar
       bra	.loop
.volup:
       cmp.b	#64,AudSimpVol(a6)	;	are we ar maxvol?
       beq	.atmax
       add.b	#1,AudSimpVol(a6)
.atmax:
       rts
.voldown:
       cmp.b	#0,AudSimpVol(a6)	;	are we ar lowvol?
       beq	.atmax
       sub.b	#1,AudSimpVol(a6)
       rts
.exit:
       move.w	#15,$dff096
       bsr	WaitReleased
       move.l	#Menus,Menu(a6)		; Set Menus as default menu. if different set another manually
       move.l	#0,MenuVariable(a6)
       bra	AudioMenu

CheckKeyReleased:
       bsr	GetInput
       btst	#1,d0
       bne.s	CheckKeyReleased
       rts

CheckOnOff:					; Checks if a0 is pointing to a variable that is on or off
                                          ; OUTPUT =   D0 = Color
                                          ;	     D1 = Address of String
       cmp.b	#0,(a0)+
       bne	.on
       move.l	#1,d0
       move.l	#OFF,d1
       rts
.on:
       move.l	#2,d0
       move.l	#ON,d1
       rts


AudioMod:
       bsr	FilterOFF
       bsr	ClearScreen
	lea	AudioModStatData(a6),a1	; Get statusvariable
	clr.l	(a1)+
	bset	#1,(a1)+			; Set Default filter off
	move.b	#64,(a1)+			; Set Default MasterVolume
	clr.w	(a1)+
	move.l	#-1,(a1)+
	move.l	#-1,(a1)			; and the "former" values aswell
						; but to something that never can happen
						; forcing an update first run
	lea	AudioModTxt,a0
	move.l	#2,d1
	bsr	Print
	move.l	#MusicSize,d0
	bsr	GetChip				; Get memory for module
	cmp.l	#0,d0				; if it is 0, no chipmem avaible
	bne	.chip
	move.l	#1,d1
	lea	NoChiptxt,a0
	bsr	Print				; We did not have enough chipmem
	bra	.exit
.chip:
	cmp	#1,d0
	bne	.enough
	move.l	#1,d1
	lea	NotEnoughChipTxt,a0
	bsr	Print
	bra	.exit
.enough:
	move.l	d0,AudioModAddr(a6)		; Store address of module
	move.l	d0,AudioModData(a6)
	lea	AudioModCopyTxt,a0
	move.l	#3,d1
	bsr	Print
	move.l	AudioModAddr(a6),a0
	move.l	#MusicSize,d0		; get size of module
	asr.l	#2,d0				; Divide by 4 to get number of longwords
	lea	Music,a1			; Get address where module is in ROM
.loop:
	move.l	(a1)+,(a0)+
	dbf	d0,.loop			; Copy module into chipmem
	move.l	#2,d1
	lea	Donetxt,a0
	bsr	Print
	lea	AudioModInitTxt,a0
	move.l	#3,d1
	bsr	Print
	move.l	AudioModAddr(a6),a0
	move.l	AudioModInit(a6),a1
	lea	$dff000,a5
	jsr	(a1)				; Call MT_Init
	lea	Donetxt,a0
	move.l	#2,d1
	bsr	Print
	move.l	AudioModMVol(a6),a0
	move.b	#4,(a0)				; Set Mastervolume
	lea	AudioModName,a0
	move.l	#3,d1
	bsr	Print
	move.l	AudioModAddr(a6),a0
	move.l	#5,d1
	bsr	Print
	lea	AudioModInst,a0
	move.l	#3,d1
	bsr	Print
	move.l	AudioModAddr(a6),a1
	add.l	#20,a1
	move.l	#1,d7
	move.l	#15,d6
	move.l	#7,d5
.instloop:
	clr.l	d0
	move.l	d5,d1
	bsr	SetPos
	move.l	d7,d0
	bsr	binhexbyte
	move.l	#2,d1
	bsr	Print
	lea	SpaceTxt,a0
	bsr	Print
	move.l	a1,a0
	move.l	#5,d1
	bsr	Print
	add.l	#30,a1
	add.l	#1,d7
.noprint:
	lea	NewLineTxt,a0
	bsr	Print
	add.l	#1,d5
	dbf	d6,.instloop
	move.l	#15,d6
	move.l	#7,d5
.instloop2:
	move.l	#40,d0
	move.l	d5,d1
	bsr	SetPos
	move.l	d7,d0
	cmp.l	#$20,d0
	beq	.noprint2
	bsr	binhexbyte
	move.l	#2,d1
	bsr	Print
	lea	SpaceTxt,a0
	bsr	Print
	move.l	a1,a0
	move.l	#5,d1
	bsr	Print
	add.l	#30,a1
	add.l	#1,d7
.noprint2:
	lea	NewLineTxt,a0
	bsr	Print
	add.l	#1,d5
	dbf	d6,.instloop2
	lea	NewLineTxt,a0
	bsr	Print
	lea	AudioModPlayTxt,a0
	move.l	#3,d1
	bsr	Print
	lea	AudioModOptionTxt,a0
	move.l	#3,d1
	bsr	Print
	lea	AudioModEndTxt,a0
	move.l	#3,d1
	bsr	Print
	bsr	AudioModStatus
.loopa:
	cmp.b	#$e0,$dff006
	bne.s	.loopa
	move.l	AudioModMusic(a6),a1
	lea	$dff000,a5
	jsr	(a1)				; Call MT_Music
	bsr	GetInput
	lea	AudioModStatData(a6),a1	; Get statusvariable
	cmp.b	#"1",GetCharData(a6)
	beq	.chan1
	cmp.b	#"2",GetCharData(a6)
	beq	.chan2
	cmp.b	#"3",GetCharData(a6)
	beq	.chan3
	cmp.b	#"4",GetCharData(a6)
	beq	.chan4
	cmp.b	#"f",GetCharData(a6)
	beq	.filter
	cmp.b	#"+",GetCharData(a6)
	beq	.volup
	cmp.b	#"-",GetCharData(a6)
	beq	.voldown
	cmp.b	#"l",GetCharData(a6)
	beq	.left
	cmp.b	#"r",GetCharData(a6)
	beq	.right
	cmp.b	#$1b,GetCharData(a6)
	beq	.exitit
.keydone:
	bsr	AudioModStatus
	cmp.b	#1,LMB(a6)
	bne.w	.loopa
.exitit:
	move.l	AudioModEnd(a6),a1
	lea	$dff000,a5
	jsr	(a1)				; Call MT_End
.exit:
	bra	AudioMenu
.chan1:
	bchg	#1,(a1)
	clr.b	BUTTON(a6)
	bra	.keydone
.chan2:
	bchg	#1,1(a1)
	clr.b	BUTTON(a6)
	bra	.keydone
.chan3:
	bchg	#1,2(a1)
	clr.b	BUTTON(a6)
	bra	.keydone
.chan4:
	bchg	#1,3(a1)
	clr.b	BUTTON(a6)
	bra	.keydone
.filter:
	bchg	#1,4(a1)
	clr.b	BUTTON(a6)
	bra	.keydone
.volup:
	cmp.b	#64,5(a1)			; Check if volume is already at max
	beq	.volmax
	add.b	#1,5(a1)
.volmax:
	clr.b	BUTTON(a6)
	bra	.keydone
.voldown:
	cmp.b	#0,5(a1)
	beq	.volmin
	sub.b	#1,5(a1)
.volmin:
	clr.b	BUTTON(a6)
	bra	.keydone
.left:
	bchg	#1,6(a1)			; Toggle left. copy it to chan1 & 4
	move.b	6(a1),(a1)
	move.b	6(a1),3(a1)
	clr.b	BUTTON(a6)
	bra	.keydone
.right:
	bchg	#1,7(a1)
	move.b	7(a1),1(a1)
	move.b	7(a1),2(a1)
	clr.b	BUTTON(a6)
	bra	.keydone
	
AudioModStatus:
	PUSH
	lea	AudioModStatData(a6),a1	; Get statusvariable
	lea	AudioModStatFormerData(a6),a2	; Get statusvariable
	move.l	#3,d7
	move.l	#20,d6
.loop:
	move.b	(a1),d2
	cmp.b	(a2),d2			; Compare 2 data, if we had a change
	beq	.done			; if no change do not do anything
	move.b	d2,(a2)			; Store the real value into former data

	move.l	d6,d0
	move.l	#26,d1
	bsr	SetPos

	cmp.b	#0,(a1)			; if it is 0, channel is on
	bne	.off
	lea	ON,a0			; Print ON
	move.l	#2,d1
	bsr	Print
	beq	.done
.off:
	lea	OFF,a0			; Print OFF
	move.l	#1,d1
	bsr	Print
.done:
	add.l	#1,a2			; add 1 to fomerdatapos
	add.l	#1,a1
	add.l	#16,d6			; change variable to put next string 16 chars away
	dbf	d7,.loop	
	move.b	(a1),d2
	cmp.b	(a2),d2			; Compare 2 data, if we had a change
	beq	.donefilter		; if no change do not do anything
	move.b	d2,(a2)			; Store the real value into former data
	move.l	#33,d0
	move.l	#27,d1
	bsr	SetPos
	cmp.b	#0,(a1)			; if it is 0, channel is on
	bne	.filteroff
	bsr	FilterON
	lea	ON,a0			; Print ON
	move.l	#2,d1
	bsr	Print
	beq	.donefilter
.filteroff:
	bsr	FilterOFF
	lea	OFF,a0			; Print OFF
	move.l	#1,d1
	bsr	Print
.donefilter:	
	add.l	#1,a2			; add 1 to fomerdatapos
	add.l	#1,a1
	move.b	(a1),d2
	cmp.b	(a2),d2			; Compare 2 data, if we had a change
	beq	.donevol		; if no change do not do anything
	move.b	d2,(a2)			; Store the real value into former data
	move.l	#59,d0
	move.l	#27,d1
	bsr	SetPos
	clr.l	d0
	move.b	d2,d0
	bsr	bindec
	move.l	#2,d1
	bsr	Print
	lea	Space3,a0
	bsr	Print
.donevol:
	move.l	AudioModMVol(a6),a0
	move.b	(a1),(a0)+				; Set Mastervolume
	lea	AudioModStatData(a6),a1	; Get statusvariable
	move.b	(a1)+,(a0)+	
	move.b	(a1)+,(a0)+	
	move.b	(a1)+,(a0)+	
	move.b	(a1)+,(a0)+			; Copy data to protrackerroutine.
						; move.l WILL crash on non 020+ machines
	POP
	rts