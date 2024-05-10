       section "gfx",code_p
       xref   testpicsize
       xref   TestPic
       testpicsize: equ EndTestPic-TestPic

TestPic:
	incbin	"data/TestPIC.raw"
EndTestPic: