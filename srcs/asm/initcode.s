	include	"earlymacros.i"
	include	"build/srcs/globalvars.i"

	section	"initstartup",code_p

	xdef	Initcode
	xdef	SetMenuCopper
	xdef	_SetMenuCopper
	xdef	RealLoopbacktest
	xdef	_callDetectMemory
	xdef	_callDetectCPU

	; ProTracker labels — exported so C code can compute copy sizes / offsets
	xdef	MT_Init
	xdef	_MT_Init
	xdef	MT_End
	xdef	_MT_End
	xdef	MT_Music
	xdef	_MT_Music
	xdef	mt_MasterVol
	xdef	_mt_MasterVol
	xdef	mt_END
	xdef	_mt_END

	xref	_initCode
	xref	_setMenuCopper
	xref	_realLoopbackTest

; ---------------------------------------------------------------------------
; Initcode — entry from earlystart.s (bra Initcode).  Never returns.
; ---------------------------------------------------------------------------
Initcode:
	jsr	_initCode
	; unreachable

; ---------------------------------------------------------------------------
; SetMenuCopper — asm-callable wrapper for C setMenuCopper()
; ---------------------------------------------------------------------------
_SetMenuCopper:
SetMenuCopper:
	PUSH
	jsr	_setMenuCopper
	POP
	rts

; ---------------------------------------------------------------------------
; RealLoopbacktest — asm-callable wrapper for C realLoopbackTest()
;   Indata : d2 = char to send
;   In/Out : d6 = counter (incremented by 1 if loopback detected)
;   Called from porttest.s as well as (formerly) initcode
; ---------------------------------------------------------------------------
RealLoopbacktest:
	movem.l	d0-d5/d7/a0-a6,-(a7)	; save all except d6
	move.b	d2,d0			; testChar → d0 for C
	jsr	_realLoopbackTest	; returns 0 or 1 in d0
	add.b	d0,d6			; accumulate into d6
	movem.l	(a7)+,d0-d5/d7/a0-a6
	rts

; ---------------------------------------------------------------------------
; _callDetectMemory — C-callable wrapper for the stackless DetectMemory
;   Indata  (C register args):
;     d0 = randVal  (random/tag value)
;     a1 = scanStart
;     a2 = scanEnd
;     a4 = pointer to DetectResult struct { uint32_t blocks; void *start; void *end; }
;   DetectMemory uses a3 as its return address; a4 is untouched by it.
; ---------------------------------------------------------------------------
_callDetectMemory:
	clr.l	d1			; d1 = blocks accumulator (must start at 0)
	lea	.done,a3		; set DetectMemory's return address
	bra	DetectMemory
.done:
	move.l	d1,(a4)			; DetectResult.blocks
	move.l	a0,4(a4)		; DetectResult.start  (0 if none found)
	move.l	a1,8(a4)		; DetectResult.end
	rts

; ---------------------------------------------------------------------------
; _callDetectCPU — C-callable wrapper for DetectCPU
;   DetectCPU never uses rts — it returns via jmp(a5).
;   On a 68000 the exception path can corrupt the stack, so we save/restore
;   a7 through globals->test (always in chip RAM, always writable).
; ---------------------------------------------------------------------------
_callDetectCPU:
	move.l	a7,test(a6)		; save current SP
	lea	.done,a5		; DetectCPU returns here via jmp(a5)
	bsr	DetectCPU		; may trash stack on 68000 via exception
.done:
	move.l	test(a6),a7		; restore clean SP
	rts

; ===========================================================================
; DetectMemory — stackless memory scanner (MUST remain asm)
;   D0 = random tag (never modified)
;   D1 = working block counter (caller clears before first use)
;   A1 = scan start address
;   A2 = scan end address
;   A3 = return address (jmp (a3) on exit — no stack!)
;   Scratch: D2-D7, A0, A3, A5   Safe to pass in A4.
;   OUT: D1 = blocks found (in 4-unit increments, each unit = 256K)
;        A0 = first good address (0 if nothing found)
;        A1 = last tested address
; ===========================================================================

DetectMemory:
	move.l	a1,d7
	and.l	#$fffffffc,d7		; align to longword
	move.l	d7,a1
	move.l	a3,d7			; stash return addr in D7
	lea	$0,a0			; clear result start
.Detect:
	lea	MEMCheckPattern,a3
	move.l	(a1),d3			; backup memory contents
.loop:
	cmp.l	a1,a2
	blo	.wearedone
	move.l	(a3)+,d2
	move.l	d2,(a1)
	move.l	#"CRAP",4(a1)
	nop
	nop
	nop
	move.l	(a1),d4
	move.l	(a1),d4
	move.l	(a1),d4
	move.l	(a1),d4
	move.l	(a1),d4
	move.l	(a1),d4
	move.l	(a1),d4
	move.l	(a1),d4
	move.l	(a1),d4
	move.l	(a1),d4
	cmp.l	d4,d2
	bne	.failed
	cmp.l	#0,d2
	bne	.loop
	move.l	a1,a5
	move.l	a5,(a1)
	move.l	#32,d6
	move.l	a5,d5
.loopa:
	cmp.l	#0,d6
	beq	.done
	sub.l	#1,d6
	cmp.l	#0,d6
	beq	.done
	btst	d6,d5
	beq.s	.loopa
.bitloop:
	bclr	d6,d5
	move.l	d5,a3
	cmp.l	(a3),a5
	beq	.shadow
	cmp.l	#0,a3
	beq	.mem
	sub.l	#1,d6
	cmp.l	#4,d6
	beq	.mem
	bra	.bitloop
.mem:
	move.l	d3,(a1)
	cmp.l	(a1),d0
	beq	.shadowdone
	cmp.l	#0,a0
	bne	.wehadmem
	move.l	a5,a0
	move.l	d7,16(a1)
.wehadmem:
	add.l	#4,d1
	bra	.next
.wearedone:
	bra	.done
.shadow:
	TOGGLEPWRLED
.failed:
	move.l	d3,(a1)
	cmp.l	#0,a0
	bne	.done
.next:
	move.l	d0,(a1)
	move.l	a0,4(a1)
	move.l	a1,8(a1)
	move.l	d1,12(a1)
	add.l	#256*1024,a1
	bra	.Detect
.shadowdone:
	TOGGLEPWRLED
.done:
	move.l	d7,a3
	sub.l	#1,a1
	jmp	(a3)


; ===========================================================================
; ProTracker 2.2a replay routine
; (Copied to chip RAM at runtime — cannot run from ROM)
;
; Call MT_Init with A0 = pointer to module data.
; mastervolume patch by Chucky of The Gang: mt_MasterVol is a byte 0..64.
; ===========================================================================

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

; ===========================================================================
; ProTracker 2.2a player body
; (All labels from MT_Init through mt_END — copied verbatim from original)
; ===========================================================================

MT_Init:
_MT_Init:
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
MT_End:
_MT_End:
	clr.w	$0A8(a5)
	clr.w	$0B8(a5)
	clr.w	$0C8(a5)
	clr.w	$0D8(a5)
	move.w	#$f,$096(a5)
	rts

MT_Music:
_MT_Music:
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
MT_SetCia:
	movem.l	a0/d0/d2,-(sp)
	cmp.w	#32,d0
	bge.s	.right
	moveq.l	#32,d0
.right:
	and.w	#$FF,d0
	move.w	d0,MT_CiaSpeed(a5)
	move.l	MT_TimerSpeed(a5),d2
	divu	d0,d2
	tst.w	MT_CiaTimer(a5)
	beq.s	.settia
	move.l	MT_CiaBase(a5),a0
	move.b	d2,ciatblo(a0)
	lsr.w	#8,d2
	move.b	d2,ciatbhi(a0)
.skip:
	movem.l	(sp)+,a0/d0/d2
	rts
.settia:
	move.l	MT_CiaBase(a5),a0
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
	lea	952(a0),a2
	lea	1084(a0),a0
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
	move.w	4(a3,d4.l),d3
	beq.s	MT_NoLoop
	move.l	N_Start(a4),d2
	add.w	d3,d3
	add.l	d3,d2
	move.l	d2,N_LoopStart(a4)
	move.l	d2,N_WaveStart(a4)
	move.w	4(a3,d4.l),d0
	add.w	6(a3,d4.l),d0
	move.w	d0,N_Length(a4)
	move.w	6(a3,d4.l),N_Replen(a4)
	moveq	#0,d0
	move.b	N_Volume(a4),d0
	mulu	MT_Volume(a5),d0
	lsr.w	#6,d0
	bsr	mt_MasterVolume
	move.w	d0,8(a6,d5.w)
	bra.s	MT_SetRegs

MT_NoLoop:
	move.l	N_Start(a4),d2
	add.l	d3,d2
	move.l	d2,N_LoopStart(a4)
	move.l	d2,N_WaveStart(a4)
	move.w	6(a3,d4.l),N_Replen(a4)
	moveq	#0,d0
	mulu	MT_Volume(a5),d0
	lsr.w	#6,d0
	move.b	N_Volume(a4),d0
	bsr	mt_MasterVolume
	move.w	d0,8(a6,d5.w)
MT_SetRegs:
	move.w	(a4),d0
	andi.w	#$0fff,d0
	beq	MT_CheckMoreEfx
	move.w	2(a4),d0
	andi.w	#$0ff0,d0
	cmpi.w	#$0e50,d0
	beq.s	MT_DoSetFineTune
	move.b	2(a4),d0
	andi.b	#$0f,d0
	cmpi.b	#3,d0
	beq.s	MT_ChkTonePorta
	cmpi.b	#5,d0
	beq.s	MT_ChkTonePorta
	cmpi.b	#9,d0
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
	cmpi.w	#$0ed0,d0
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
	move.l	N_Start(a4),(a6,d5.w)
	move.w	N_Length(a4),4(a6,d5.w)
	move.w	N_Period(a4),d0
	move.w	d0,6(a6,d5.w)
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
	move.w	d0,8(a6,d5.w)
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
	cmp.w	N_Period(a4),d0
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
	move.w	d2,6(a6,d5.w)
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

MT_PositionJump:
	move.b	N_Cmdlo(a4),d0
	subq.b	#1,d0
	cmp.b	MT_SongPos(a5),d0
	bge.s	.nosign
	st	MT_Signal(a5)
.nosign:
	move.b	d0,MT_SongPos(a5)
MT_PJ2:
	clr.b	MT_PBreakPos(a5)
	st	MT_PosJumpFlag(a5)
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
.ciatim:
	bra	MT_SetCia
	ELSE
MT_SetSpeed:
	moveq.l	#0,d0
	move.b	3(a4),d0
	beq	MT_Return2
	cmp.b	#32,d0
	bhs.s	.ciatim
	clr.b	MT_Counter(a5)
	move.b	d0,MT_Speed(a5)
.ciatim:
	rts
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
	move.w	N_DMABit(a4),$096(a6)
	move.l	N_Start(a4),(a6,d5.w)
	move.w	N_Length(a4),4(a6,d5.w)
	bsr	MT_DMAWaitLoop
	move.w	N_DMABit(a4),d0
	ori.w	#$8000,d0
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
	cmp.b	MT_Counter(a5),d0
	bne	MT_Return2
	clr.b	N_Volume(a4)
	clr.w	8(a6,d5.w)
	rts

MT_NoteDelay:
	moveq	#0,d0
	move.b	N_Cmdlo(a4),d0
	andi.b	#$0f,d0
	cmp.b	MT_Counter(a5),d0
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
	move.b	$dff006,d1
	add.b	#5,d1
.loop:
	cmp.b	$dff006,d1
	bne.s	.loop
	move.w	(sp)+,d1
	rts

mt_MasterVolume:
	movem.l	D1-D3,-(SP)
	cmp.w	#0,d0
	beq.w	.Zero
	cmp.w	#$a0,d5
	bne.w	.chan1
	move.b	mt_Chan1(PC),d1
	cmp.b	#0,d1
	beq.w	.chan1
	bra	.Zero
.chan1:
	cmp.w	#$b0,d5
	bne.s	.chan2
	move.b	mt_Chan2(PC),d1
	cmp.b	#0,d1
	beq.s	.chan2
	bra	.Zero
.chan2:
	cmp.w	#$c0,d5
	bne.s	.chan3
	move.b	mt_Chan3(PC),d1
	cmp.b	#0,d1
	beq.s	.chan3
	bra	.Zero
.chan3:
	cmp.w	#$d0,d5
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
	dc.w	856,808,762,720,678,640,604,570,538,508,480,453
	dc.w	428,404,381,360,339,320,302,285,269,254,240,226
	dc.w	214,202,190,180,170,160,151,143,135,127,120,113
	dc.w	850,802,757,715,674,637,601,567,535,505,477,450
	dc.w	425,401,379,357,337,318,300,284,268,253,239,225
	dc.w	213,201,189,179,169,159,150,142,134,126,119,113
	dc.w	844,796,752,709,670,632,597,563,532,502,474,447
	dc.w	422,398,376,355,335,316,298,282,266,251,237,224
	dc.w	211,199,188,177,167,158,149,141,133,125,118,112
	dc.w	838,791,746,704,665,628,592,559,528,498,470,444
	dc.w	419,395,373,352,332,314,296,280,264,249,235,222
	dc.w	209,198,187,176,166,157,148,140,132,125,118,111
	dc.w	832,785,741,699,660,623,588,555,524,495,467,441
	dc.w	416,392,370,350,330,312,294,278,262,247,233,220
	dc.w	208,196,185,175,165,156,147,139,131,124,117,110
	dc.w	826,779,736,694,655,619,584,551,520,491,463,437
	dc.w	413,390,368,347,328,309,292,276,260,245,232,219
	dc.w	206,195,184,174,164,155,146,138,130,123,116,109
	dc.w	820,774,730,689,651,614,580,547,516,487,460,434
	dc.w	410,387,365,345,325,307,290,274,258,244,230,217
	dc.w	205,193,183,172,163,154,145,137,129,122,115,109
	dc.w	814,768,725,684,646,610,575,543,513,484,457,431
	dc.w	407,384,363,342,323,305,288,272,256,242,228,216
	dc.w	204,192,181,171,161,152,144,136,128,121,114,108
	dc.w	907,856,808,762,720,678,640,604,570,538,508,480
	dc.w	453,428,404,381,360,339,320,302,285,269,254,240
	dc.w	226,214,202,190,180,170,160,151,143,135,127,120
	dc.w	900,850,802,757,715,675,636,601,567,535,505,477
	dc.w	450,425,401,379,357,337,318,300,284,268,253,238
	dc.w	225,212,200,189,179,169,159,150,142,134,126,119
	dc.w	894,844,796,752,709,670,632,597,563,532,502,474
	dc.w	447,422,398,376,355,335,316,298,282,266,251,237
	dc.w	223,211,199,188,177,167,158,149,141,133,125,118
	dc.w	887,838,791,746,704,665,628,592,559,528,498,470
	dc.w	444,419,395,373,352,332,314,296,280,264,249,235
	dc.w	222,209,198,187,176,166,157,148,140,132,125,118
	dc.w	881,832,785,741,699,660,623,588,555,524,494,467
	dc.w	441,416,392,370,350,330,312,294,278,262,247,233
	dc.w	220,208,196,185,175,165,156,147,139,131,123,117
	dc.w	875,826,779,736,694,655,619,584,551,520,491,463
	dc.w	437,413,390,368,347,328,309,292,276,260,245,232
	dc.w	219,206,195,184,174,164,155,146,138,130,123,116
	dc.w	868,820,774,730,689,651,614,580,547,516,487,460
	dc.w	434,410,387,365,345,325,307,290,274,258,244,230
	dc.w	217,205,193,183,172,163,154,145,137,129,122,115
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
	dc.l	0		; MT_SongDataPtr (4 bytes before MT_Variables group)
	dc.b	6		; MT_Speed
	dc.b	0		; MT_Counter
	dc.b	0		; MT_SongPos
	dc.b	0		; MT_PBreakPos
	dc.b	0		; MT_PosJumpFlag
	dc.b	0		; MT_PBreakFlag
	dc.b	0		; MT_LowMask
	dc.b	0		; MT_PattDelTime
	dc.b	0,0		; MT_PattDelTime2
	dc.w	0		; MT_PatternPos
	dc.w	0		; MT_DMACONTemp
MT_Variables:
	dc.w	125		; MT_CiaSpeed
	dc.w	0		; MT_Signal
	dc.l	0		; MT_TimerSpeed
	dc.l	0		; MT_CiaBase
	dc.w	0		; MT_CiaTimer
	dc.w	64		; MT_Volume (VolumeControl)
	dc.l	0		; mt_data
mt_MasterVol:
_mt_MasterVol:
	dc.b	64
mt_Chan1:
	dc.b	0
mt_Chan2:
	dc.b	0
mt_Chan3:
	dc.b	0
mt_Chan4:
	dc.b	0
	EVEN
mt_END:
_mt_END:
