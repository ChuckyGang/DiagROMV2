; Stuff that gets used a lot 
		XREF Ansi
		XREF AnsiNull
		XREF Black 

Ansi:
	dc.b	27,"[",0
AnsiNull:
	dc.b	27,"[0m",27,"[40m",27,"[37m",0
Black:
	dc.b	27,"[30m",0

		cnop 0,16
