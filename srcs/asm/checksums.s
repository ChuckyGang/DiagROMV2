	section ".checksums"
	XDEF 	_checksums
	XDEF	_endofcode
	XDEF	_endchecksums
	xref	EndRom:
	dc.b	"Checksums:"
	CNOP	0,4			; Start at even LONGWORD
_checksums:		; Numbers here fits my Kickstart 3.1 rom.
	dc.l	$88ff8999,$27445220,$bf491a45,$f83fe9c5,$1e97ca1e,$7f55b19b,$924d1d33,$67f7f730
_endchecksums:
	dc.b	"This is the brutal end of this ROM, everything after this are just pure noise.    End of Code...",0
	CNOP	0,4			; Start at even LONGWORD
_endofcode:
EndRom: