       section "gfx",code_p
       xdef   testpicsize
       xdef   TestPic
       testpicsize: equ EndTestPic-TestPic

TestPic:
	incbin	"data/TestPIC.raw"
EndTestPic: