       include "earlymacros.i"
       include "build/srcs/globalvars.i"
       section "othertest",code_p
       xref   OtherTest
       xref   Setup
       xref   About
       xref   RTCTest
       xref   AutoConfigDetail
       xref   ShowMemAddress
       xref   TF1260
       xref   SystemInfoTest
       xref   Space3

OtherTest:
       bra    MainMenu

RTCTest:
       bra    MainMenu
AutoConfigDetail:
       bra    MainMenu
ShowMemAddress:
       bra    MainMenu
TF1260:
       bra    MainMenu

Setup:
       bra    MainMenu

About:
       bra    MainMenu

SystemInfoTest:
       bsr	InitScreen
       lea	SystemInfoTxt,a0
       move.w	#2,d1
       jsr	Print
       lea	SystemInfoHWTxt,a0
       move.w	#2,d1
       jsr	Print

       bsr	GetHWReg
       bsr	PrintHWReg

       lea	NewLineTxt,a0
       jsr	Print


       lea	WorkTxt,a0
       move.l	#6,d1
       jsr	Print

       move.l	BaseStart(a6),d0			; Get startaddress of chipmem
       bsr	binhex
       move.l	#2,d1
       jsr	Print

       lea	MinusTxt,a0
       jsr	Print


       move.l	BaseEnd(a6),d0			; Get startaddress of chipmem
       bsr	binhex
       move.l	#2,d1
       jsr	Print


       lea	WorkSizeTxt,a0
       move.l	#6,d1
       jsr	Print


       move.l	BaseEnd(a6),d0
       sub.l	BaseStart(a6),d0
       divu	#1024,d0
       swap	d0
       clr.w	d0
       swap	d0

       jsr	bindec
       move.l	#2,d1
       jsr	Print


       lea	KB,a0
       move.l	#2,d1
       jsr	Print

       lea	RomSizeTxt,a0
       move.l	#6,d1
       jsr	Print


       move.l	EndRom,d0
       sub.l	#rom_base,d0
       divu	#1024,d0
       swap	d0
       clr.w	d0
       swap	d0

       jsr	bindec
       move.l	#2,d1
       jsr	Print

       lea	KB,a0
       jsr	Print

       lea	WorkOrderTxt,a0
       move.l	#6,d1
       jsr	Print

       cmp.b	#0,WorkOrder(a6)
       beq	.normalorder


       lea	StartTxt2,a0
       jsr	Print
       bra	.orderdone
       
.normalorder:
       lea	EndTxt2,a0
       jsr	Print

.orderdone:

       lea	ChipTxt,a0
       move.l	#6,d1
       jsr	Print

       move.l	ChipStart(a6),d0
       bsr	binhex
       move.l	#2,d1
       jsr	Print
       lea	MinusTxt,a0
       jsr	Print
       move.l	ChipEnd(a6),d0
       bsr	binhex
       jsr	Print


       lea	FastTxt,a0
       move.l	#6,d1
       jsr	Print


       move.l	FastStart(a6),d0
       bsr	binhex
       move.l	#2,d1
       jsr	Print
       lea	MinusTxt,a0
       jsr	Print
       move.l	FastEnd(a6),d0
       bsr	binhex
       jsr	Print


       jsr	RomChecksum

       lea	.CpuDone,a5
       bsr	DetectCPU
.CpuDone:
       bsr	PrintCPU

       clr.l	d0
       move.b	CPUGen(a6),d0
       cmp.b	#5,d0
       bne	.no060

       lea	FlagTxt,a0
       bsr	Print
       lea	PCRFlagsTxt,a0
       move.l	#2,d1
       jsr	Print
       move.l	PCRReg(a6),d0
       bsr	binstring
       move.l	#3,d1
       jsr	Print



;	move.l	#2,d1
;	lea	EXPERIMENTAL,a0
;	bsr	Print
;	move.l	#DetMMU,$80
;	trap	#0
;	move.l	d6,d0
;	move.l	#3,d1
;	bsr	bindec
;	bsr	Print

.no060


       move.l	#BusError,$8		; This time to a routine that can present more data.
       move.l	#UnimplInst,$2c
       move.l	#Trap,$80

       lea	NewLineTxt,a0
       jsr	Print
       lea	DebugROM,a0
       move.l	#3,d1
       jsr	Print

       cmp.l	#"DIAG",$0
       bne	.no1114at0
       lea	YES,a0
       move.l	#1,d1
       jsr	Print
       bra	.yes1114at0

.no1114at0:
       lea	NO,a0
       move.l	#2,d1
       jsr	Print
.yes1114at0

       lea	NewLineTxt,a0
       jsr	Print
       lea	DebugROM2,a0
       move.l	#3,d1
       jsr	Print

       cmp.l	#"DIAG",$f80000
       bne	.no1114atf8
       lea	YES,a0
       move.l	#2,d1
       jsr	Print
       bra	.yes1114atf8

.no1114atf8:
       lea	NO,a0
       move.l	#1,d1
       jsr	Print
.yes1114atf8:
       lea	DebugROM3,a0
       move.l	#3,d1
       jsr	Print

       cmp.w	#$1111,$f00000
       bne	.no1111atf0
       lea	YES,a0
       move.l	#2,d1
       jsr	Print
       bra	.donerom

.no1111atf0:
       lea	NO,a0
       move.l	#2,d1
       jsr	Print
.donerom:	

       lea	StuckButtons,a0
       move.l	#3,d1
       jsr	Print

       clr.l	d7				; Clear d7.  set it to 1 if a button was stuck.

       cmp.b	#0,STUCKP1LMB(a6)
       beq	.nop1lmb
       lea	InitP1LMBtxt,a0
       move.l	#1,d7
       move.l	#1,d1
       jsr	Print
.nop1lmb:
       cmp.b	#0,STUCKP2LMB(a6)
       beq	.nop2lmb
       lea	InitP2LMBtxt,a0
       move.l	#1,d1
       move.l	#1,d7
       jsr	Print
.nop2lmb:
       cmp.b	#0,STUCKP1RMB(a6)
       beq	.nop1rmb
       lea	InitP1RMBtxt,a0
       move.l	#1,d1
       move.l	#1,d7
       jsr	Print
.nop1rmb:
       cmp.b	#0,STUCKP2RMB(a6)
       beq	.nop2rmb
       lea	InitP2RMBtxt,a0
       move.l	#1,d1
       move.l	#1,d7
       jsr	Print
.nop2rmb:
       cmp.b	#0,DISPAULA(a6)
       beq	.nobadpaula
       lea	BadPaulaTXT,a0
       move.l	#1,d1
       move.l	#1,d7
       jsr	Print
.nobadpaula:
       cmp.b	#0,OVLErr(a6)
       beq	.noovlerr
       lea	OvlErrTxt,a0
       move.l	#1,d1
       move.l	#1,d7
       jsr	Print
.noovlerr:

       cmp.l	#0,d7
       bne	.stuck
       lea	NONE,a0
       move.l	#2,d1
       jsr	Print
.stuck:
       lea	NewLineTxt,a0
       jsr	Print

       bsr	WaitButton
       bra	MainMenu

PrintHWReg:
       lea	BLTDDATTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	BLTDDAT(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	DMACONRTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	DMACONR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	VPOSRTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	VPOSR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	NewLineTxt,a0
       jsr	Print
       lea	VHPOSRTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	VHPOSR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	DSKDATRTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	DSKDATR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	JOY0DATTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	JOY0DAT(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	NewLineTxt,a0
       jsr	Print
       lea	POT0DATTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	POT0DAT(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	POT1DATTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	POT1DAT(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	POTINPTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	POTINP(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	NewLineTxt,a0
       jsr	Print
       lea	SERDATRTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	SERDATR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	DSKBYTRTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	DSKBYTR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	INTENARTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	INTENAR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	NewLineTxt,a0
       jsr	Print
       lea	INTREQRTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	INTREQR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	DENISEIDTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	DENISEID(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	HHPOSRTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	HHPOSR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	NewLineTxt,a0
       jsr	Print

       rts


SystemInfoTxt:
       dc.b	2,"Information of this machine:",$a,$a,0
SystemInfoHWTxt:
       dc.b	2,"Dump of all readable Custom Chipset HW Registers:",$a,0
WorkTxt:
       dc.b	"Workmem: ",0
WorkSizeTxt:
       dc.b	" Size: ",0
RomSizeTxt:
       dc.b	"   ROM size: ",0
WorkOrderTxt:
       dc.b	"  Order: ",0
StartTxt2:
       dc.b	"Start",0
EndTxt2:
       dc.b	"End",0
ChipTxt:
       dc.b	$a,"Chipmem workarea: ",0
FastTxt:
       dc.b	" Fastmem workarea: ",0
FlagTxt:
       dc.b	$a,"                   -----CPUID-----| CPURev|E*****DE",0
StuckButtons:
       dc.b	$a,$d,"Stuck buttons & keys etc at boot: ",0
InitP1LMBtxt:
       dc.b	"P1LMB ",0
InitP2LMBtxt:
       dc.b	"P2LMB ",0
InitP1RMBtxt:
       dc.b	"P1RMB ",0
InitP2RMBtxt:
       dc.b	"P2RMB ",0
InitP1MMBtxt:
       dc.b	"P1RMB ",0
InitP2MMBtxt:
       dc.b	"P2RMB ",0
BadPaulaTXT:
       dc.b	"BADPAULA",0
OvlErrTxt:
       dc.b	"OVLERROR",0
NONE:
       dc.b	"NONE",0
Space3:
       dc.b	"   ",0
BLTDDATTxt:
       dc.b	"BLTDDAT ($dff000): ",0
DMACONRTxt:
	dc.b	"DMACONR  ($dff002): ",0
VPOSRTxt:
	dc.b	"VPOSR   ($dff004): ",0
VHPOSRTxt:
	dc.b	"VHPOSR  ($dff006): ",0
DSKDATRTxt:
	dc.b	"DSKDATR  ($dff008): ",0
JOY0DATTxt:
	dc.b	"JOY0DAT ($dff00a): ",0
JOY1DATTxt:
	dc.b	"JOY1DAT ($dff00c): ",0
CLXDATTxt:
	dc.b	"CLXDAT   ($dff00e): ",0
ADKCONRTxt:
	dc.b	"ADKCONR ($dff010): ",0
POT0DATTxt:
	dc.b	"POT0DAT ($dff012): ",0
POT1DATTxt:
	dc.b	"POT1DAT  ($dff014): ",0
POTINPTxt:
	dc.b	"POTINP  ($dff016): ",0
SERDATRTxt:
	dc.b	"SERDATR ($dff018): ",0
DSKBYTRTxt:
	dc.b	"DSKBYTR  ($dff01a): ",0
INTENARTxt:
	dc.b	"INTENAR ($dff01c): ",0
INTREQRTxt:
	dc.b	"INTREQR ($dff01e): ",0
DENISEIDTxt:
	dc.b	"DENISEID ($dff07c): ",0
HHPOSRTxt:
	dc.b	"HHPOSR  ($dff1dc): ",0