	section ".autovec"
	XDEF 	_autovec
	XDEF 	_end

_autovec:
	dc.l	$00180019,$001a001b,$001c001d,$001e001f	; 68000-68010 autovec generation
_end:
