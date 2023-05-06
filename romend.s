	section ".autovec"
	XREF 	_romend
	XREF 	_end
Checksums:		; Numbers here fits my Kickstart 3.1 rom.
	dc.l	$88ff8999,$27445220,$bf491a45,$f83fe9c5,$1e97ca1e,$7f55b19b,$924d1d33,$67f7f730
EndChecksums:

	_romend:
	dc.l	$00180019,$001a001b,$001c001d,$001e001f	; or IRQ will TOTALLY screw up on machines with 68000-68010
_end:
	