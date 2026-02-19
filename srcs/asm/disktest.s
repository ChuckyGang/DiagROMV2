       include "earlymacros.i"
       include "build/srcs/globalvars.i"
       section "disktest",code_p
       xdef   DiskTest
       xdef   DiskdriveTest
       xdef   GayleTest
       xdef   GayleExp
	xdef	PrintYes
	xdef	PrintNo


DiskTest:
	bsr	InitScreen
	move.w	#8,MenuNumber(a6)
	move.b	#1,PrintMenuFlag(a6)
	jmp	MainLoop



DiskdriveTest:
	move.l	#12980,d0			; Size of a track
	jsr	GetChip			; get chipmemaddress for this block
	cmp.l	#0,d0
	beq	.exit
	move.l	d0,trackbuff(a6)
	bsr	.Initdisk
.DiskdriveTester:
	jsr	ClearScreen
	clr.b	oldbfe001(a6)
	clr.b	oldbfd100(a6)
	move.w	#0,MenuNumber(a6)
	move.b	#1,PrintMenuFlag(a6)
	move.l	#DriveTestMenu,Menu(a6)	; Set different menu
	move.l	a6,d0
	add.l	#DriveTestVar,d0		; Pointer to variables
	move.l	d0,a0
	move.l	d0,MenuVariable(a6)
.loop:
	bsr	.StepToTrack			; Check if drivestepping is needed, if so do it
	move.l	MenuVariable(a6),a0		; Load a0 with pointer to where variable is
	clr.l	d0
	move.w	DriveNo(a6),d0		; Load with drivenumber
	mulu	#5,d0				; Multiply it with 4 to get to correct drivetext
	lea	DF0,a1
	add.l	d0,a1
	move.w	DriveOK(a6),d0
	cmp.b	#0,d0
	bne	.driveok
	move.w	#1,(a0)+
	bra	.okdone
.driveok:
	move.w	#2,(a0)+
.okdone:
	move.l	a1,(a0)+
	move.l	#6,d0				; Show Tracknumber
	move.l	#2,d1
	jsr	SetPos
	lea	Track,a0
	move.l	#3,d1
	jsr	Print
	lea	Space3,a0
	jsr	Print
	move.l	#13,d0
	move.l	#2,d1
	jsr	SetPos
	clr.l	d0
	move.b	TrackNo(a6),d0
	jsr	bindec
	move.l	#5,d1
	jsr	Print
	clr.l	d7				; Clear d7, if still clear after those 2 tests, nothing to update
	move.b	$bfe001,d0
	cmp.b	oldbfe001(a6),d0
	beq	.nobfe001change
	move.b	d0,oldbfe001(a6)
	move.b	#1,d7				; just set d7 to something
.nobfe001change:
	move.b	$bfd100,d0
	cmp.b	oldbfd100(a6),d0
	beq	.nobfd100change
	move.b	d0,oldbfd100(a6)
	move.b	#1,d7
.nobfd100change:
	cmp.b	#0,d7
	beq	.noupdate			; we had no update. skip to print it
	move.l	#18,d0				; Show diskside
	move.l	#2,d1
	jsr	SetPos
	lea	Side,a0
	move.l	#3,d1
	jsr	Print
	cmp.b	#0,SideNo(a6)
	bne	.Lower
	lea	UPPER,a0
	move.l	#5,d1
	jsr	Print
;	bclr.b	#2,$bfd100
	bra	.SideDone
.Lower:
	lea	LOWER,a0
	move.l	#5,d1
	jsr	Print
;	bset.b	#2,$bfd100
.SideDone:
	move.l	#32,d0				; Show motorstatus
	move.l	#2,d1
	jsr	SetPos
	lea	Motor,a0
	move.l	#3,d1
	jsr	Print
	btst	#7,$bfd100
	beq	.MotorIsOn
	lea	OFF,a0
	move.l	#5,d1
	jsr	Print
	bra	.MotorDone
.MotorIsOn:
	lea	ON,a0
	move.l	#5,d1
	jsr	Print
.MotorDone:
	move.l	#45,d0				; Show writeprotectionstatus
	move.l	#2,d1
	jsr	SetPos
	lea	WProtect,a0
	move.l	#3,d1
	jsr	Print
	btst	#3,$bfe001
	beq	.nowrite
	lea	OFF,a0
	move.l	#2,d1
	jsr	Print
	bra	.writedone
.nowrite:
	lea	ON,a0
	move.l	#1,d1
	jsr	Print
.writedone:
	move.l	#64,d0				; Show disk status
	move.l	#2,d1
	jsr	SetPos
	lea	DiskIN,a0
	move.l	#3,d1
	jsr	Print
	btst	#2,$bfe001
	beq	.nochange
	lea	YES,a0
	move.l	#2,d1
	jsr	Print
	bra	.changedone
.nochange:
	lea	NO,a0
	move.l	#1,d1
	jsr	Print
.changedone:
	move.l	#6,d0				; Show Tracknumber
	move.l	#3,d1
	jsr	SetPos
	lea	RDY,a0
	move.l	#3,d1
	jsr	Print
	btst	#5,$bfe001
	bne	.nordy
	lea	YES,a0
	move.l	#2,d1
	jsr	Print
	bra	.rdydone
.nordy:
	lea	NO,a0
	move.l	#1,d1
	jsr	Print
.rdydone:
	move.l	#18,d0				; Show Tracknumber
	move.l	#3,d1
	jsr	SetPos
	lea	TRACK0,a0
	move.l	#3,d1
	jsr	Print
	btst	#4,$bfe001
	bne	.notrk0
	lea	YES,a0
	move.l	#2,d1
	jsr	Print
	bra	.trk0done
.notrk0:
	lea	NO,a0
	move.l	#1,d1
	jsr	Print
.trk0done:
	move.l	#36,d0
	move.l	#3,d1
	jsr	SetPos
	lea	BFE001Txt,a0
	move.l	#3,d1
	jsr	Print
	move.b	$bfe001,d0
	jsr	binstringbyte
	move.l	#6,d1
	jsr	Print
	move.l	#56,d0
	move.l	#3,d1
	jsr	SetPos
	lea	BFD100Txt,a0
	move.l	#3,d1
	jsr	Print
	move.b	$bfd100,d0
	jsr	binstringbyte
	move.l	#6,d1
	jsr	Print
.noupdate:
	jsr	PrintMenu
	jsr	GetInput
	jsr	WaitLong
	cmp.b	#0,d0
	beq	.no
	move.b	keyresult(a6),d1		; Read value from last keyboard read
	cmp.b	#$a,d1				; if it was enter, select this item
	beq	.action
	move.b	Serial(a6),d2			; Read value from last serialread
	cmp.b	#$a,d2
	beq	.action
	cmp.b	#1,LMB(a6)
	beq	.action
	lea	DriveTestMenuKey,a5		; Load list of keys in menu
	clr.l	d0				; Clear d0, this is selected item in list
.keyloop:
	move.b	(a5)+,d3			; Read item
	cmp.b	#0,d3				; Check if end of list
	beq	.nokey
	cmp.b	d1,d3				; fits with keyboardread?
	beq	.goaction
	cmp.b	d2,d3				; fits with serialread?
	beq	.goaction			; if so..  do it
	add.l	#1,d0				; Add one to d0, selecting next item
	bra	.keyloop
.goaction:
	move.b	d0,MenuPos(a6)
	bra	.action
.nokey:	
	cmp.b	#1,RMB
	beq	.Exitjump
	bra	.no
.Exitjump:
	jmp	Exit
.action:
	jsr	WaitReleased
	clr.l	d0
	move.b	MenuPos(a6),d0
	cmp.b	#0,d0				; Check if it is item 0, meaning change drive ID
	beq	.ChangeDrive
	cmp.b	#1,d0
	beq	.Motor
	cmp.b	#2,d0
	beq	.Side
	cmp.b	#3,d0
	beq	.TrackOut
	cmp.b	#4,d0
	beq	.TrackIn
	cmp.b	#5,d0
	beq	.Track10Out
	cmp.b	#6,d0
	beq	.Track10In
	cmp.b	#7,d0
	beq	.ReadTrack
	cmp.b	#8,d0
	beq	.WriteTrack
	cmp.b	#9,d0
	beq	.ShowMem
	cmp.b	#11,d0
	beq	MainMenu
.no:
	bra	.loop
.ChangeDrive:
	move.b	#1,UpdateMenuNumber(a6)
	move.b	#2,PrintMenuFlag(a6)
	add.w	#1,DriveNo(a6)		; Bump up drivenumber with 1
	cmp.w	#4,DriveNo(a6)		; if it is too high
	bne	.nowrap
	clr.w	DriveNo(a6)			; Reset the counter
.nowrap
	bsr	.SelectDrive
	bsr	.GotoZero
	bra	.no
.Motor:
	bchg	#0,DriveMotor(a6)
	cmp.b	#0,DriveMotor(a6)
	bne	.SetMotorOn
	bsr	.MotorOff
	bra	.MotorSetDone
.SetMotorOn:
	lea	ON,a0
	move.l	#5,d1
	jsr	Print
	bsr	.MotorOn
.MotorSetDone:
	bra	.no
.Side:
	bchg	#0,SideNo(a6)
	bra	.no
.TrackOut:
	add.b	#1,WantedTrackNo(a6)
	cmp.b	#84,WantedTrackNo(a6)
	beq	.outover
	bra	.no
.outover:
	move.b	#83,WantedTrackNo(a6)
	bra	.no
.TrackIn:
	sub.b	#1,WantedTrackNo(a6)
	cmp.b	#255,WantedTrackNo(a6)
	beq	.outless
	bra	.no
.outless
	move.b	#0,WantedTrackNo(a6)
	bra	.no
.Initdisk:
	move.w	#$4489,$dff07e			; Disk sync pattern register for disk read
	move.w	#$7f00,$dff09e			; Audio, Disk, UART Control
	move.w	#$9500,$dff09e			; Check later what it does... MEMPREC, FAST, WORDSYNC set
	clr.w	DriveNo(a6)			; Set drivenumber to 0
	bsr	.SelectDrive			; Select the drive
	bsr	.GotoZero			; Step to track 0
	bsr	.UnSelectDrive
	rts
.Track10Out:
	add.b	#10,WantedTrackNo(a6)
	cmp.b	#83,WantedTrackNo(a6)
	bge	.outover
	bra	.no
.Track10In:
	sub.b	#10,WantedTrackNo(a6)
	cmp.b	#255,WantedTrackNo(a6)
	ble	.outless
	bra	.no
.GotoZero:					; Steps back to track 0
	move.w	#1,DriveOK(a6)		; Set Drive as OK
	move.l	#84,d7				; Do this for 85 times (meaning more tracks than on a disk)
.ZeroLoop
	jsr	WaitLong			; Wait for a while
	btst	#4,$bfe001			; Are we at track 0?
	bne	.nozero
	clr.b	TrackNo(a6)			; Clear trackno
	clr.b	WantedTrackNo(a6)		; Also clear the wanted trackno, or it will just step there.
	rts
.nozero:
	move.w	#$ff,$dff180
	bset.b	#1,$bfd100			; CIAB_DSKDIREC
	bclr.b	#0,$bfd100			; Step
	tst	$dff1fe
	bset	#0,$bfd100
	dbf	d7,.ZeroLoop			; do this until all "tracks" are done, if this loop goes to an end. we have for sure no diskdrive
	clr.w	DriveOK(a6)			; Set that drive was NOT ok
	rts
.StepToTrack:					; Check the WantedTrackNo and steps one step to that direction
	clr.l	d0
	move.b	WantedTrackNo(a6),d0		; Load D0 with the wanted track
	cmp.b	TrackNo(a6),d0
	beq	.AlreadyThere			; ok we are already at wanted position. do nothing
	blt	.GoIn				; Lets step in
	bgt	.GoOut
.AlreadyThere:
	rts
.GoOut:
	bsr	.SelectDrive
	jsr	WaitLong
	bclr.b	#1,$bfd100			; CIAB_DSKDIREC
	bclr.b	#0,$bfd100			; Step
	tst	$dff1fe
	bset	#0,$bfd100
	add.b	#1,TrackNo(a6)
	jsr	WaitLong
	bsr	.UnSelectDrive
	rts
.GoIn:
	bsr	.SelectDrive
	jsr	WaitLong
	bset.b	#1,$bfd100			; CIAB_DSKDIREC
	bclr.b	#0,$bfd100			; Step
	tst	$dff1fe
	bset	#0,$bfd100
	sub.b	#1,TrackNo(a6)
	jsr	WaitLong
	bsr	.UnSelectDrive
	rts
.MotorOff:
	bsr	.SelectDrive
	or.b	#$78,$bfd100			; Deselect all drives
	jsr	WaitLong
	clr.l	d0
	move.w	DriveNo(a6),d0		; load A6 with drive to select
	add.w	#3,d0				; Add 3 to it.  now we know what bit to clear to select drive
	bset.b	#7,$bfd100			; CIAB_DSKMOTOR
	jsr	WaitLong
	bclr.b	d0,$bfd100			; CIAB_DSKSEL0	Select that drive
	bsr	.UnSelectDrive
	rts
.MotorOn:
	bsr	.SelectDrive
	jsr	WaitLong
	clr.l	d0
	move.w	DriveNo(a6),d0		; load A6 with drive to select
	add.w	#3,d0				; Add 3 to it.  now we know what bit to clear to select drive
	bclr.b	#7,$bfd100			; CIAB_DSKMOTOR
	jsr	WaitLong
	bclr.b	d0,$bfd100			; CIAB_DSKSEL0	Select that drive
	rts
.SelectDrive:
	bsr	.UnSelectDrive
	clr.l	d0
	move.w	DriveNo(a6),d0		; load D0 with drive to select
	add.w	#3,d0				; Add 3 to it.  now we know what bit to clear to select drive
	bclr.b	d0,$bfd100			; Select that drive
	jsr	WaitLong
	rts
.UnSelectDrive:
	or.b	#$78,$bfd100			; Deselect all drives
	jsr	WaitLong
	rts
.ReadTrack:
	bsr	.SelectDrive
	jsr	WaitLong
	bsr	.MotorOn
	bsr	.WaitReady
	bsr	.SelectDrive			; YEAH!  again
	move.w	#$4000,$dff024
	move.l	trackbuff(a6),$dff020
	move.w	#$4489,$dff07e
	move.w	#$7f00,$dff09e
	move.w	#$9500,$dff09e
	move.w	#2,$dff09c	
	move.w	#$8210,$dff096
	move.w	#$8000+$1900,d1
	move.w	d1,$dff024
	move.w	d1,$dff024
	move.w	#$fff,$dff180
	move.l	#$ffffff,d7
.waittrack:
	move.b	$bfe001,d0			; Nonsenseread, to just make loop slower
	sub.l	#1,d7
	cmp.l	#0,d7
	beq	.timeout
	btst	#1,$dff01f
	beq	.waittrack
.timeout:
	move.w	#$4000,$dff024
	jsr	WaitLong
	bsr	.MotorOff
	jsr	WaitLong
	bsr	.UnSelectDrive
	bra	.no
.WriteTrack:
	bsr	.SelectDrive
	jsr	WaitLong
	bsr	.MotorOn
	bsr	.WaitReady
	bsr	.SelectDrive			; YEAH!  again!
;	move.w	#$4000,$dff024
	move.l	trackbuff(a6),$dff020
;	move.w	#$4489,$dff07e
	move.w	#$7f00,$dff09e
	move.w	#$8100,$dff09e
	move.w	#2,$dff09c	
	move.w	#$8210,$dff096
	move.w	#$d978,$dff024
	move.w	#$d978,$dff024
	move.w	#$f00,$dff180
	move.l	#$ffffff,d7
.waittrack2:
	move.b	$bfe001,d0			; Nonsenseread, to just make loop slower
	sub.l	#1,d7
	cmp.l	#0,d7
	beq	.timeout2
	btst	#1,$dff01f
	beq	.waittrack2
.timeout2:
	move.w	#$4000,$dff024
	bsr	.UnSelectDrive
	bra	.no
.WaitReady:
	move.l	#$ffff,d7
.waitrdy:
	move.b	$bfe001,d0
	sub.l	#1,d7
	cmp.l	#0,d7
	beq	.readytimeout
	btst	#5,$bfe001
	beq	.waitrdy
.readytimeout:
       rts
.ShowMem:
	clr.l	d3				; Sector to find
.Showmems:
	jsr	ClearScreen
	PUSH
	bsr	.FindSector
	cmp.l	#-1,d0				; Was d0 -1, then we had an error
	beq	.sectorerror
	lea	$38(a1),a1
	clr.l	d6				; Clear rowcounter
	clr.l	d5				; clear "address" of how far into buffer we are
.secloop:
	lea	sectorbuff(a6),a2
	bsr	.decodebuffer
	PUSH
	move.l	d6,d0
	lea	sectorbuff(a6),a0
	bsr	.Showdata
	POP
	add.l	#$10,d5
	add.l	#1,d6
	cmp.l	#20,d6
	bne	.secloop	
	POP
	lea	AnyKeyMouseTxt,a0
	move.l	#4,d1
	jsr	Print
.exloop:
	jsr	GetInput
	cmp.b	#0,BUTTON(a6)
	beq	.exloop
	cmp.b	#1,RMB(a6)
; 	bne.w	.Showmems
	bra	.DiskdriveTester
.exit:
	jmp	MainMenu
.decodebuffer:					; Decodes a small part of the MFM buffer to be able to print it.
	move.l	#$55555555,d7
	move.l	#3,d4
.decode:
	move.l	$200(a1),d1
	move.l	(a1)+,d0
	and.l	d7,d0
	asl.l	#1,d0
	and.l	d7,d1
	or.l	d1,d0
	move.l	d0,(a2)+			; Store it in the small buffer
	dbra	d4,.decode
	rts
.sectorerror
	jsr	ClearScreen
	lea	SectorErrorTxt,a0
	move.l	#1,d1
	jsr	Print
	lea	AnyKeyMouseTxt,a0
	move.l	#4,d1
	jsr	Print
	jsr	_WaitButton
	bra	.DiskdriveTester
.FindSector
	move.l	trackbuff(a6),a0		; A0 now contains pointer to where the MFM data is
	move.l	a0,a1
	move.w	#$4489,d5			; Syncword
	move.l	#$55555555,d7
	clr.l	d3
;	move.b	sector(a6),d3			; Load d3 with the wanted sector
	move.l	a0,d6
	add.l	#12980,d6			; D6 now contains the last address of trackbuffer
.getsync:
	cmp.l	a1,d6
	blt	.overflow			; ok we went too far
	cmp.w	(a1)+,d5
	bne.s	.getsync
;	cmp.w	(a1),d5				; Another syncword?
;	beq.s	.getsync
;	add.l	#6,a1
	move.l	(a1),d0
	move.l	4(a1),d1
	and.l	d7,d0
	asl.l	#1,d0
	and.l	d7,d1
	or.l	d1,d0
	ror.l	#8,d0
	PUSH
	jsr	binhex
	move.l	#3,d1
	jsr	Print
	clr.l	d0
	move.b	#" ",d0
	jsr	PrintChar
	move.l	a1,d0
	jsr	binhex
	move.l	#5,d1
	jsr	Print
	clr.l	d0
	move.b	#" ",d0
	jsr	PrintChar
	POP
;	cmp.b	d3,d0				; Are we on correct sector?
;	beq	.sectorOK			; Yes
	move.b	d0,sector(a6)
.sectorOK:	
	clr.l	d0
	rts
.overflow:
	move.l	#-1,d0				; Set d0 to -1 to show that we had an error.
	rts
.Showdata:
	PUSH
	move.l	a0,a1				; store a0 in a1 for usage here.. as a0 is used
	add.l	#3,d0				; Add 3 to line to work on.
	move.l	d0,d1				; copy d0 to d1 to use it as Y adress
	clr.l	d0				; clear X pos
	jsr	SetPos				; Set position
	move.l	d5,d0
	jsr	binhex
	move.l	#6,d1
	jsr	Print				; Print address
	clr.l	d2				; Column to print
	move.l	#15,d7
.showloop:
	lea	SpaceTxt,a0
	jsr	Print
	clr.l	d0				; Clear d0 just to be sure
	move.b	(a1,d2),d0
	jsr	binhexbyte			; Convert that byte to hex
	move.l	#7,d1
	jsr	Print				; Print it
	add.l	#1,d2
	dbf	d7,.showloop
	lea	ColonTxt,a0			; Print a Colon
	move.l	#3,d1
	jsr	Print
	move.l	#15,d7				; Now print the same bytes.  as chars instead	
	clr.l	d2
.showloop2:
	clr.l	d0
	move.b	(a1,d2),d0
	jsr	MakePrintable			; make the char printable.  strip controlstuff..
	add.l	#1,d2
	move.l	#7,d1
	jsr	PrintChar
	dbf	d7,.showloop2
	POP
	rts

GayleTest:
       jsr	ClearScreen
	lea	GayleCheckMirrorTxt,a0
	move.l	#6,d1
	jsr	Print
	lea	GAYLE_ID_ADDR,a1			; Read Gayle address
	move.w	$dff01c,-(sp)			; Store value if intena in stack
	move.w	#$bfff,$9a(a1)			; set all enables
	move.w	#$3fff,d2			; also flag for no mirror
	cmp.w	$dff01c,d2
	bne	.nomirror
	move.w	d2,$9a(a1)			; Clear all enables
	tst	$dff01c
	bne.s	.nomirror
	moveq	#0,d2				; Mirrored
	lea	GayleMirrorTxt,a0
	move.l	#3,d1
	jsr	Print
.nomirror:
	move.w	#$3fff,$dff09c			; Clear bits
	ori.w	#$8000,(sp)			; add setbit
	move.w	(sp)+,$dff09c			; Reset values
	tst.w	d2				; Did we find mirroring
	beq	.no_hw				; yes we did. quit
	lea	GayleNoMirrorTxt,a0
	move.l	#3,d1
	jsr	Print
	lea	GayleVerTxt,a0
	move.l	#7,d1
	jsr	Print
	moveq	#0,d2
	move.b	d2,(a1)				; Value doesnt matter, just a write needed
	bsr	.get_gid_bit			; Get 4 bits
	bsr	.get_gid_bit
	bsr	.get_gid_bit
	bsr	.get_gid_bit
	bsr	.get_gid_bit			; Get 4 bits
	bsr	.get_gid_bit
	bsr	.get_gid_bit
	bsr	.get_gid_bit
	move.w	d2,d0
	jsr	binhexbyte
	move.l	#2,d1
	jsr	Print
	cmp.b	#$d1,d2				; Check for version $d1 (A1200 Gayle)
	beq.w	.A1200
	cmp.b	#$d0,d2				; Check for version $d0 (A600 Gayle)
	beq.s	.A600
	and.b	#$d0,d2				; mask out numbers.. so we can find if there are any other Dx versions..
	cmp.b	#$d0,d2				; Check for version $d0 can now be any Dx version.. so lets call it "unknown" if found
	beq.s	.Other
	lea	NoGayleTxt,a0
	move.l	#2,d1
	jsr	Print
	lea	AnyKeyMouseTxt,a0
	move.l	#4,d1
	jsr	Print
.done:
	jsr	WaitPressed
	jmp	MainMenu
.no_hw:
	lea	GayleNoIDETxt,a0
	move.w	#1,d1
	jsr	Print
	bra	.done
.Other:
	lea	UnknownTxt,a0
	move.l	#3,d1
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	bra	.GayleFound
.A600:
	lea	A600Txt,a0
	move.l	#3,d1
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	bra	.GayleFound
.A1200:
	lea	A1200Txt,a0
	move.l	#3,d1
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	bra	.GayleFound
.GayleFound:
	lea	GayleIDETxt,a0
	move.w	#2,d1
	jsr	Print
	lea	$da001c,a5
	bsr	WaitRDY
	lea	NewLineTxt,a0
	jsr	Print
	cmp.b	#0,d2
	beq	.nointexit			; we had a timeout.  go to nointexit
	move.b	#$ec,d0				; Read the drive ID
	bsr	.IDECommand
.BoardServer:
	PUSH
	lea	IDEInterruptCheck,a0
	move.l	#3,d1
	jsr	Print
	POP
	moveq.l	#0,d0				; Assume it is not out interrupt
	lea	GAYLE_ADDR,a0			; Point to the board
	move.b	$1000(a0),d1			; IntChange check for int!
	bpl	.nointexit			; not ours
	PUSH
	lea	IDEInterruptDetected,a0
	move.l	#3,d1
	jsr	Print
	POP
	bsr	IDECheckStatus
						; Our interrupt, clear it
						; must clear drive first, then gayle
	move.l	$da0000,a2			; IDE_Slow
	move.b	$1c(a2),d1			; AT_Status. clears interrupt (IDE)
	PUSH
	lea	IDEInterruptCleared,a0
	move.l	#3,d1
	jsr	Print
	POP
	bsr	IDECheckStatus
	move.w	SR,d2				; Save current SR
	ori.w	#$700,sr			; Raise int priority to level 7
	move.b	$1000(a0),d1			; Gayle_intchange
	PUSH
	lea	IDEInterruptChangedReading,a0
	move.l	#3,d1
	jsr	Print
	POP
	move.w	d1,d0
	PUSH
	jsr	binhex
	move.l	#3,d1
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	POP
	move.b	d1,$1000(a0)			; Clear latch in gayle
	move.w	d2,sr				; Reenable normal int level
	lea	$da2002,a2			; IDE_Slow
	bsr	IDEReadData
	move.l	DiskBuffer(a6),d0
	move.l	d0,a4				; A4 now contains pointer to diskbuffer
	lea	IDESurfacesTxt,a0
	move.l	#3,d1
	jsr	Print
	clr.l	d0
	move.w	$6a(a4),d0
	jsr	bindec
	move.l	#2,d1
	jsr	Print
	lea	IDESectorsTxt,a0
	move.l	#3,d1
	jsr	Print
	move.w	$6c(a4),d0
	jsr	bindec
	move.l	#2,d1
	jsr	Print
	lea	IDECylindersTxt,a0
	move.l	#3,d1
	jsr	Print
	move.w	$68(a4),d0
	jsr	bindec
	move.l	#2,d1
	jsr	Print
	lea	IDEBlkSize,a0
	move.l	#3,d1
	jsr	Print
	move.w	$6(a4),d0
	jsr	bindec
	move.l	#2,d1
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	lea	IDEUnitTxt,a0
	move.l	#3,d1
	jsr	Print
	move.l	a4,a5
	add.l	#$32,a5				; step to unitname  (2d)
	move.l	#31,d7
.unitloop:
	clr.l	d0
	move.b	(a5)+,d0
	jsr	MakePrintable
	move.l	#2,d1
	jsr	PrintChar
	dbf	d7,.unitloop
	lea	REVTxt,a0
	move.l	#3,d1
	jsr	Print
	move.l	a4,a5
	add.l	#$2d,a5				; step to unitname  (2d)
	move.l	#4,d7
.unitrevloop:
	clr.l	d0
	move.b	(a5)+,d0
	jsr	MakePrintable
	move.l	#2,d1
	jsr	PrintChar
	dbf	d7,.unitrevloop
	lea	NewLineTxt,a0
	jsr	Print
	bsr	IDECheckStatus
.nointexit:
.endIDTests:
	bra	.done
.get_gid_bit:					; Read a gary/Gayle bit
	move.b	(a1),d0
	lsl.b	#1,d0
	roxl.b	#1,d2
	rts
.IDECommand:
	move.b	d0,$da001c
	rts


GAYLE_ADDR: 	equ	$da8000
GAYLE_ID_ADDR:	equ	$de1000


; Gaylecodehelp from Stephen Leary

GayleExp:
	jsr	ClearScreen
	lea	$dd201c,a5
	bsr	WaitRDY
	lea	NewLineTxt,a0
	jsr	Print
	cmp.b	#0,d2
	beq	.exit			; we had a timeout.  go to nointexit
	lea	NewLineTxt,a0
	jsr	Print
	move.b	#$ec,d0				; Read the drive ID
	move.b	d0,$dd201c
	move.l	$dd2020,a2			; IDE_Slow
	move.b	$1c(a2),d1			; AT_Status. clears interrupt (IDE)
	PUSH
	lea	IDEInterruptCleared,a0
	move.l	#3,d1
	jsr	Print
	POP
	bsr	IDECheckStatus
	move.w	SR,d2				; Save current SR
	ori.w	#$700,sr			; Raise int priority to level 7
	move.b	$1000(a0),d1			; Gayle_intchange
	PUSH
	lea	IDEInterruptChangedReading,a0
	move.l	#3,d1
	jsr	Print
	POP
	move.w	d1,d0
	PUSH
	jsr	binhex
	move.l	#3,d1
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	POP
	lea	$dd2020,a2			; IDE_Slow
	bsr	IDEReadData
	move.l	DiskBuffer(a6),d0
	move.l	d0,a4				; A4 now contains pointer to diskbuffer
	lea	IDESurfacesTxt,a0
	move.l	#3,d1
	jsr	Print
	clr.l	d0
	move.w	$6a(a4),d0
	jsr	bindec
	move.l	#2,d1
	jsr	Print
	lea	IDESectorsTxt,a0
	move.l	#3,d1
	jsr	Print
	move.w	$6c(a4),d0
	jsr	bindec
	move.l	#2,d1
	jsr	Print
	lea	IDECylindersTxt,a0
	move.l	#3,d1
	jsr	Print
	move.w	$68(a4),d0
	jsr	bindec
	move.l	#2,d1
	jsr	Print
	lea	IDEBlkSize,a0
	move.l	#3,d1
	jsr	Print
	move.w	$6(a4),d0
	jsr	bindec
	move.l	#2,d1
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	lea	IDEUnitTxt,a0
	move.l	#3,d1
	jsr	Print
	move.l	a4,a5
	add.l	#$32,a5				; step to unitname  (2d)
	move.l	#31,d7
.unitloop:
	clr.l	d0
	move.b	(a5)+,d0
	jsr	MakePrintable
	move.l	#2,d1
	jsr	PrintChar
	dbf	d7,.unitloop
	lea	REVTxt,a0
	move.l	#3,d1
	jsr	Print
	move.l	a4,a5
	add.l	#$2d,a5				; step to unitname  (2d)
	move.l	#4,d7
.unitrevloop:
	clr.l	d0
	move.b	(a5)+,d0
	jsr	MakePrintable
	move.l	#2,d1
	jsr	PrintChar
	dbf	d7,.unitrevloop
	lea	NewLineTxt,a0
	jsr	Print
	bsr	IDECheckStatus
.exit:
	lea	AnyKeyMouseTxt,a0
	move.l	#4,d1
	jsr	Print
	jsr	WaitPressed
.no_hw:
	jmp	MainMenu

WaitRDY:
	move.w	#50,d0
	move.w	d0,GayleData(a6)
	move.l	#1,d7				; retrycounter
.loop:
	move.w	GayleData(a6),d2
	sub.w	#1,d2
	move.w	d2,GayleData(a6)
	PUSH
	lea	GayleRDYTxt,a0
	move.l	#2,d1
	jsr	Print
	POP
	move.b	(a5),d0			; Statuscommand
	and.b	#$c1,d0
       move.b	d0,d4
	PUSH
	jsr	binhexbyte
	move.w	#3,d1
	jsr	Print
	move.l	#32,d0
	jsr	PrintChar
	move.l	#40,d0
	jsr	PrintChar
	POP
	move.w	(a5),d0
	PUSH
	jsr	binhexword
	move.l	#3,d1
	jsr	Print
	move.l	#41,d0
	jsr	PrintChar
	lea	TryTxt,a0
	jsr	Print
	move.l	d7,d0
	jsr	bindec
	jsr	Print
	jsr	SameRow
	POP	
	add.l	#1,d7
	move.w	GayleData(a6),d2
	cmp.w	#0,d2
	beq	.nodisk
	cmp.b	#$40,d4
	bne	.loop
	bra	.exit
.nodisk:
	lea	NoDiskTxt,a0
	move.l	#3,d1
	jsr	Print
.exit:
	rts
PrintYes:
	lea	YES,a0
	move.l	#2,d1
	jsr	Print
	rts
PrintNo:
	lea	NO,a0
	move.l	#1,d1
	jsr	Print
	rts

IDECheckStatus:
	PUSH
	lea	IDEInterruptStatusReading,a0
	move.l	#3,d1
	jsr	Print
	POP
	move.b	$da8000,d0
	PUSH
	jsr	binhex
	move.l	#3,d1
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	POP
	rts

IDEReadData:
	move.l	#4096,d0
	jsr	GetChip				; Get block of 4K
	move.l	d0,DiskBuffer(a6)
	move.l	d0,a4				; move memoryaddress to a4 so we can use it
	; print about ide rea
	PUSH
	lea	GayleIDERead,a0
	move.l	#3,d1
	jsr	Print
	POP
	move.l	#0,d3
.loopide:
	move.w	0(a2),d0			; AT_Data;
	move.l	d0,d1
	asr.l	#8,d1
	asl.l	#8,d0
	add.b	d1,d0				; now word is byteswapped
	move.w	d0,(a4)+
	add.l	#1,d3
	cmp.l	#1024,d3
	ble	.loopide
	lea	Donetxt,a0
	move.l	#3,d1
	jsr	Print
.nomem:
	rts

floppyTestC::
	bsr	_floppyTestC
       bra    MainMenu