       include "earlymacros.i"
       include "build/srcs/globalvars.i"
       section "mainmenu",code_p

	xdef	_MainMenu
       xdef   MainMenu
	xdef	_InitScreen
	xdef	InitScreen
	xdef	MainLoop
	xdef	PrintMenu
	xdef	Exit
	xdef	FilterON
	xdef	FilterOFF
	xdef	SwapMode

	xref	_mainMenu
	xref	_mainLoop
	xref	_initScreen
	xref	_printMenu
	xref	_filterON
	xref	_filterOFF
	xref	_swapMode
	xref	_exitDiag

; Functions that never return — plain jump, no PUSH/POP/rts
_MainMenu:
MainMenu:
	jmp	_mainMenu

MainLoop:
	jmp	_mainLoop

SwapMode:
	jmp	_swapMode

Exit:
	jmp	_exitDiag

; Functions that return normally — PUSH/jsr _func/POP/rts
_InitScreen:
InitScreen:
	PUSH
	jsr	_initScreen
	POP
	rts

PrintMenu:
	PUSH
	jsr	_printMenu
	POP
	rts

; FilterON/OFF: originals had no PUSH/POP, C versions are trivial one-liners
FilterON:
	jsr	_filterON
	rts

FilterOFF:
	jsr	_filterOFF
	rts
