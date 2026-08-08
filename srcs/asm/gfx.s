       section "gfx",code_p
       xdef   testpicsize
       xdef   TestPic
       testpicsize: equ EndTestPic-TestPic

TestPic:
	ifnd	TARGET_DEMON
	incbin	"data/TestPIC.raw"
	else
; DeMoN 256KB cartridge: the 50KB test picture is excluded. testpicsize
; becomes 0 and menus.c swaps the two picture tests for an explaining stub
; (demonNoTestPic), so nothing ever copies from the empty label.
	endc
EndTestPic: