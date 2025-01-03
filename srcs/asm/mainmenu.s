       include "earlymacros.i"
       include "build/srcs/globalvars.i"
       section "mainmenu",code_p
       
	xdef	_MainMenu
       xdef   MainMenu
	xdef	PrintStatus
	xdef	UpdateStatus
	xdef	_InitScreen
	xdef	InitScreen
	xdef	MainLoop
	xdef	PrintMenu
	xdef	Exit
	xdef	FilterON
	xdef	FilterOFF
	xdef	SwapMode

MainLoop:
       move.l	#0,a0
       bsr	PrintMenu			; Print or update the menu
       bsr	GetInput			; Scan keyboard, mouse, buttons, serialport etc..
       clr.l	d0
      ; move.l	#27,d1
       ;bsr	SetPos
       ;clr.l	d0
.no:
       bsr	HandleMenu			; ok, LMB pressed, do menuhandling
.notpressed:
       clr.l	d0
       move.l	InputRegister(a6),d0        ; sometime. check what the f.  this is :)
       btst	#0,d0
       bra	MainLoop
              
_MainMenu:
MainMenu:
	jsr	FilterON
	bsr	ClearScreen			; Clear the screen
	bsr	PrintStatus			; Print the statusline
	bsr	UpdateStatus			; Update "static" data of statusline
	clr.l	d0
	clr.l	d1
	bsr	SetPos
	move.l	#Menus,Menu(a6)		; Set Menus as default menu. if different set another manually
	move.l	#0,MenuVariable(a6)
	move.w	#0,MenuNumber(a6)
	move.b	#1,PrintMenuFlag(a6)
	bra	MainLoop

Exit:
	move.w	#0,$dff180
	cmp.b	#$f0,$dff006
	bne	Exit
	move.w	#$fff,$dff181
	bra	Exit
       move.b #1,$e90064           ; Disable TF1260 Maprom
       move.l $f8004,a0
       lea    $dff09a,a6
       move.w #$4000,(a6)
       move.w #$2700,sr
       reset
       jmp    (a0)                 ; Reset (going back to kickstart)

PrintStatus:
	move.l	#0,d0
	move.l	#31,d1
	bsr	SetPos
	lea	StatusLine,a0
	move.l	#3,d1
	bsr	Print
	rts


UpdateStatus:
	move.l	#8,d0				; Print Serialspeed
	move.l	#31,d1
	bsr	SetPos
	clr.l	d0
	move.w	SerialSpeed(a6),d0		; Get SerialSpeed value
	mulu	#4,d0				; Multiply with 4
	lea	SerText,a0			; Load table of pointers to different text
	move.l	(a0,d0.l),a0			; load a0 with the value that a0+d0 points to (text of speed)
	move.l	#7,d1
	bsr	Print
	move.l	#25,d0				; Print CPU type
	move.l	#31,d1
	bsr	SetPos
	move.l	CPUPointer(a6),a0
	move.l	#7,d1
	bsr	Print
	move.l	#40,d0				; Print Chipmem
	move.l	#31,d1
	bsr	SetPos
	move.l	TotalChip(a6),d0
	bsr	ToKB
	bsr	bindec
	move.l	#7,d1
	bsr	Print
	lea	KB,a0
	bsr	Print
	move.l	#57,d0
	move.l	#31,d1
	bsr	SetPos
	move.l	#7,d1
	move.l	TotalFast(a6),d0
	bsr	bindec
	bsr	Print
	move.l	#70,d0
	move.l	#31,d1
	bsr	SetPos
	move.l	a6,d0
	bsr	binhex
	move.l	#7,d1
	bsr	Print
	rts

FilterOFF:
	bset	#1,$bfe001
	rts
FilterON:
	bclr	#1,$bfe001
	rts

	
SwapMode:
       move.w	#$fff,$dff180
       bchg	#5,SCRNMODE(a6)
       clr.l	d0
       move.b	SCRNMODE(a6),d0
       move.w	d0,$dff1dc			; Set BEAMCON90
       bra	MainMenu

HandleMenu:					; Routine that handles menus.
       cmp.b	#0,MenuChoose(a6)		; If this item chosen with keyboard etc?
       bne	.released			; if so.  go to "releaaed" (after LMB is released again..)
       cmp.b	#1,MBUTTON(a6)
       bne	.nobutton			; no mousebutton pressed
.CheckButton:
	bsr	GetInput
	bsr	WaitShort
	cmp.b	#0,MBUTTON(a6)
	bne	.CheckButton
.released:
	clr.b	MenuChoose(a6)		; Clear value of choosen item
	clr.l	d0
	move.w	MenuNumber(a6),d0
	lea	MenuCode,a0			; Get list of pointers to list for the menu
	mulu	#4,d0				; Multiply menunumber with 4
	add.l	d0,a0				; read pointer to the correct menu
	move.l	(a0),a0			; a0 now contains address of menu routines
	clr.l	d0
	move.b	MarkItem(a6),d0		; Get the marked item
	mulu	#4,d0
	add.l	d0,a0				; a0 now contains the address of the pointer to the routing
	move.l	(a0),a0			; a0 now contains the address of the routine.
	jmp	(a0)				; go there
.nobutton:
	clr.l	d0
	move.w	MenuNumber(a6),d0
	lea	MenuKeys,a0
	mulu	#4,d0
	add.l	d0,a0
	move.l	(a0),a0			; A0 now contains pointer to where list of interesting keys are.
	clr.l	d0				; Clear d0
.loop:
	cmp.b	#0,(a0)			; does A0 point to 0? in that case, out of list
	beq	.nokey
	move.b	GetCharData(a6),d7		; d7 is now what the last keycode was.
	cmp.b	(a0),d7			; check if value in list is the same as pressed keycode
	beq	.Pressed
.nokeyboard:
	add.l	#1,a0
	add.l	#1,d0
	bra	.loop
.nokey:	
	rts
.Pressed:					; ok we have a match of key or serial.
	move.b	d0,MarkItem(a6)		; store it to marked item
	bra	.released			; so jump to the part of the code that actually executes the routine


PrintMenu:
						; Prints out menu.
						; INDATA = D0 - MenuNumber
	PUSH
	clr.l	d1
	clr.l	d2
	clr.l	d3
	clr.l	d4
	clr.l	d5
	clr.l	d6
	clr.l	d7
	move.w	MenuNumber(a6),d0
	cmp.w	OldMenuNumber(a6),d0		; Check if menu is changed since last call
	beq	.nochange
	clr.b	MarkItem(a6)			; Clear variables for marked item etc
	clr.b	OldMarkItem(a6)	
	move.w	d0,OldMenuNumber(a6)
.nochange:
	cmp.b	#0,PrintMenuFlag(a6)
	beq	.noprint			; if flag is 0, menu is already printed.
	cmp.b	#2,PrintMenuFlag(a6)		; Check if we just want to update
	beq	.noupdatemenu
	move.b	#0,MenuPos(a6)		; Clear menuposition. always start at the top as we didnt want to update
.noupdatemenu:	
	clr.l	d0
	move.w	MenuNumber(a6),d0		; Load what menunumber to print
	move.l	MenuVariable(a6),a2		; A2 now contains pointer to pointerlist of variables. if 0 = no variables ignore
	move.l	Menu(a6),a0			; Load a0 with pointer to list of menus.
.nozero:
	mulu	#4,d0				; multiply d0 with 4 to point on the correct item on list.
	add.l	d0,a0				; A0 now points on the correct item in the menulist
	move.l	(a0),a1				; A1 now contains the menuinfo.
	clr.l	d0
	clr.l	d1
	bsr	SetPos
	move.l	(a1),a0				; Print first line (label) of the menu
	move.l	#7,d1
	cmp.b	#0,UpdateMenuNumber(a6)	; Check if we will only update one line.
	bne	.nolabel			; if so, skip printing label
	bsr	Print				; Print label of the menu
.nolabel:
	add.l	#4,a1				; Skip first row of itemlist as it was the label
	move.l	a1,a0				; Copy a1 to a0
	clr.l	d1				; Clear D1
	move.l	a0,a1
.loop:
	add.l	#1,d1				; Add 1 to D1 for number of entrys in list
	cmp.l	#0,(a1)+			; is A1 pointing to a 0?
	bne.s	.loop				; no, we are not at end of list if items in the menu.
	sub.l	#2,d1				; we ARE at end of itmes, and as we counted the last 0 aswell.. subtract with 2
						; d1 now contrains number of items in this menu.
	move.b	d1,MenuEntrys(a6)
	move.l	d1,d5				; Copy d1 to d5
	move.l	#20,d6				; Set d6 to X pos of text in menu
	move.l	#5,d7				; Set d7 to 5, where to start text on the meny on the Y pos
	move.l	a0,a1				; a1 is now list of items in menu
	clr.l	d4
.loop2:
	add.l	#1,d4
	move.l	(a1)+,a0
	cmp.b	#0,UpdateMenuNumber(a6)
	beq	.prnt
	cmp.b	UpdateMenuNumber(a6),d4	; Check if we just should update one line
	bne	.noprnt	
.prnt:
	move.l	d6,d0
	move.l	d7,d1
	bsr	SetPos				; Set position on screen for next item to be printed.
	move.l	#6,d1
	bsr	Print
.noprnt:
	cmp.l	#0,a2				; Check if A2 is 0.  if so, do not do anything with variables
	beq	.novar
						; OK, we have variables to be printed after the normal menuitem
						; A2 is a pointer to where a list of variables are located.
						; it is actually just a list if pointers to strings to be printed.
						; first word is color to print, next longword is pointer to string to be
						; printed.
	cmp.b	#0,UpdateMenuNumber(a6)
	beq	.prntvar
	cmp.b	UpdateMenuNumber(a6),d4	; Check if we just should update one line
	bne	.noprntvar
.prntvar:
	lea	SPACE,a0
	bsr	Print
	move.w	(a2),d1				; Set color
	move.l	2(a2),a0			; Set string
	cmp.l	#0,a0				; is A0 0? then skip printing
	beq	.novar
	bsr	Print				; Print it
.noprntvar:
	add.l	#6,a2				; add 6 to a2 for next varaibledata to print
.novar:
	add.l	#2,d7				; Add 2 to next row to print.
	dbf	d5,.loop2			; Print all items on the menu
	move.b	#1,UpdateMenuFlag(a6)
	move.b	#0,UpdateMenuNumber(a6)
.noprint:
						; ok we have printed the menu (or skipped it, depending on flag)
						; now lets see if it needs to be updated.
	clr.l	d7
	move.b	MenuEntrys(a6),d7
						; but to be sure, we add 1 to the result.
	clr.l	d0
	move.b	GetCharData(a6),d0
	cmp.b	#30,d0
	bne	.NoUp
	bsr	.Up
	bra	.NoKeyMove
.NoUp:
	cmp.b	#31,d0
	bne	.NoDown
	bsr	.Down
	bra	.NoKeyMove
.NoDown:
	cmp.b	#$a,d0
	bne	.NoEnter
	move.b	#1,MenuChoose(a6)
	bsr	ClearBuffer			; Make sure inputbuffer is cleared
.NoEnter:
.NoKeyMove:
	move.w	CurAddY(a6),d7		; Load d7 with any value of added (lower) mousemovement
	cmp.w	#0,d7
	beq	.noadd				; no movements down...
	clr.w	MenuMouseSub(a6)		; ok we add, then clear any sub variable
	add.w	d7,MenuMouseAdd(a6)		; add it to mouseadd variable
	cmp	#40,MenuMouseAdd(a6)		; Check if we moved enough to bump menu one step
	blt	.noadd	
	clr.w	MenuMouseAdd(a6)
	bsr	.Down
.noadd:
	move.w	CurSubY(a6),d7
	cmp.w	#0,d7
	beq	.nosub
	clr.w	MenuMouseAdd(a6)
	add.w	d7,MenuMouseSub(a6)
	cmp.w	#40,MenuMouseSub(a6)
	blt	.nosub
	clr.w	MenuMouseSub(a6)
	bsr	.Up
.nosub:
	clr.l	d7
	move.b	MenuPos(a6),d7		; Load d7 with menupostin to highlight
	move.b	d7,MarkItem(a6)
	cmp.b	#0,PrintMenuFlag(a6)		; check if menu is printed
	bne	.forceupdate			; if so, also force update of the marked line etc
	cmp.b	OldMarkItem(a6),d7		; Compare the value with the old marked item
	beq	.noupdate			; no changes done, no updates needed.
.forceupdate:
	clr.l	d7
	move.b	OldMarkItem(a6),d7		; d7 now contains the number of the FORMERLY marked item.
	move.b	#6,d6				; Set Color
	bsr	.PrintItem			; print it.
       clr.l	d7
	move.b	MarkItem(a6),d7
	move.b	d7,OldMarkItem(a6)
	move.l	#13,d6
	bsr	.PrintItem
.noupdate:				
	clr.b	PrintMenuFlag(a6)		; ok, clear the print menu flag..  we do not want this to be printed again;
	POP
	rts
.Up:
	cmp.b	#0,MenuPos(a6)		; check if we already are at the top
	beq	.No
	sub.b	#1,MenuPos(a6)		; Move up one step
.No:	rts
.Down:
	clr.l	d7
	move.b	MenuEntrys(a6),d7
	cmp.b	MenuPos(a6),d7
	beq	.No
	add.b	#1,MenuPos(a6)
	rts
.PrintItem:					; Prints item on menu.
						; d7 = item to print
						; d6 = color to use when printing
	PUSH					
	move.w	MenuNumber(a6),d0		; Load what menunumber to print
	move.l	Menu(a6),a0			; Load a0 with pointer to list of menus.
	mulu	#4,d0				; multiply d0 with 4 to point on the correct item on list.
	add.l	d0,a0				; A0 now points on the correct item in the menulist
	move.l	(a0),a1			; A1 now contains the menuinfo.
	add.l	#4,a1				; Skip first item as it is the label anyway.
	move.l	d7,d2				; copy d1 to d2 so d2 also contains the item to highlight.
	mulu	#2,d2				; Multiply d2 with 2 to get the row to print the text on.
	move.l	#20,d0				; Load d0 with X Postition of menu
	move.l	#5,d1				; Load d1 with beginning of Y position
	add.l	d2,d1				; add number of lines for the item to update
	bsr	SetPos				; Set screenposition
	move.l	d7,d2				; copy d7 to d2
	mulu	#4,d2				; Multiply with 4, so we know what item in list to point to
	add.l	d2,a1				; add it to a1, a1 now points to pointer of string to update
	move.l	(a1),a0			; load A0 with actual string
	move.l	d6,d1				; Set color
	bsr	Print				; Print it.
	POP
	rts

_InitScreen:
InitScreen:
	bsr	ClearScreen
	bsr	PrintStatus
	bsr	UpdateStatus
	clr.l	d0
	clr.l	d1
	bsr	SetPos
	rts
