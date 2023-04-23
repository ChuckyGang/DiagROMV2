		include "earlymacros.i"

		section "startup",code_p

		XDEF	AnsiNull

_start:
		dc.w $1114		; this just have to be here for ROM. code starts at $2

		jmp	_begin
	
		dc.l	POSTBusError				; Hardcoded pointers
		dc.l	POSTAddressError			; if something is wrong rom starts at $0
		dc.l	POSTIllegalError			; so this will actually be pointers to
		dc.l	POSTDivByZero				; traps.
		dc.l	POSTChkInst
		dc.l	POSTTrapV
		dc.l	POSTPrivViol
		dc.l	POSTTrace
		dc.l	POSTUnimplInst

_strstart:
		dc.b	"IHOL : :6U6U,A,B1U1U5767U,U,8181 1 0    "	; This string will make a readable text on each 32 bit
		dc.b	"HILO: : U6U6A,B,U1U17576,U,U18181 0     "	; rom what socket to use. (SOME programmingsoftware does byteshift so both orders)
	
		dc.b	"$VER: DiagROM Amiga Diagnostic by John Hertell. "
;		dc.b	"www.diagrom.com "
		incbin	"builddate.i"

		dc.b	"- "
			
		VERSION
_strstop:
		
		blk.b	166-(_strstop-_strstart),0		; Crapdata that needs to be here
	
		EVEN

		cnop 0,16

	
_begin:	

		clr.l	d0
		clr.l	d1
		clr.l	d2
		clr.l	d3
		clr.l	d4
		clr.l	d5
		clr.l	d6
		clr.l	d7
		lea	$0,a0
		lea	$0,a1
		lea	$0,a2
		lea	$0,a3
		lea	$0,a4
		lea	$0,a5
		lea	$0,a6

		lea	$0,SP			; Set the stack. BUT!!! do not use it yet. we need to check chipmem first! so meanwhile we use it as a dirty register

		move.b	#$ff,$bfe200
		move.b	#$ff,$bfe300

		move.b	#0,$bfe001			; Clear register.
		move.b	#$ff,$bfe301
		move.b	#$0,$bfe101
		move.b	#3,$bfe201	
		move.b	#0,$bfe001		; Powerled will go ON! so user can see that CPU works
		move.b	#$40,$bfed01
		move.w	#$f0f,$dff180
		move.w	#$ff00,$dff034
		move.w	#$0ff,$dff180
		move.b	#$ff,$bfd300
		or.b	#$f8,$bfd100
		nop
		and.b	#$87,$bfd100
		nop
		or.b	#$78,$bfd100

		; Lets check status of mousebuttons at start.  AAAND we have ONE register not used in
		move.l	#POSTBusError,$8
		move.l	#POSTAddressError,$c
		move.l	#POSTIllegalError,$10
		move.l	#POSTDivByZero,$14
		move.l	#POSTChkInst,$18
		move.l	#POSTTrapV,$1c
		move.l	#POSTPrivViol,$20
		move.l	#POSTTrace,$24
		move.l	#POSTUnimplInst,$28
		move.l	#POSTUnimplInst,$2c

		; all code.  A4.. so lets store the result therea		
		move.b	#$88,$bfed01
		or.b	#$40,$bfee01		; For keyboard
						; We will print the result on the serialport later.
						
		move.l	#0,d0			; Make sure D0 is cleared.


	;jsr	_test_function(pc)

	move.l	#255,d0
	DBINDEC
	KPRINT
	move.l	#255,d0
	DBINHEX
	KPRINT
	KPRINTC String
	KPRINTC	String2

		loop:
			move.b	$dff006,$dff181
			bra	loop
	

		KPRINTC	AnsiNull



POSTBusError:				; Hardcoded pointers
POSTAddressError:			; if something is wrong rom starts at $0
POSTIllegalError:			; so this will actually be pointers to
POSTDivByZero:				; traps.
POSTChkInst:
POSTTrapV:
POSTPrivViol:
POSTTrace:
POSTUnimplInst:
		rts


		DumpSerial:
			move.w	#$4000,$dff09a
			move.w	#32,$dff032			; Set the speed of the serialport (115200BPS)
			move.b	#$4f,$bfd000		; Set DTR high
			move.w	#$0801,$dff09a
			move.w	#$0801,$dff09c
		
			clr.l	d7					; Clear d7
		.loop:
			move.b	(a0)+,d7
			cmp.b	#0,d7				; end of string?
			beq	.nomore					; yes
		
			move.l	#40000,d6			; Load d6 with a timeoutvariable. only test this number of times.
										; if paula cannot tell if serial is output we will not end up in a wait-forever-loop.
										; and as we cannot use timers. we have to do this dirty style of coding...
		.timeoutloop:	
			move.b	$bfe001,d5			; just read crapdata, we do not care but reading from CIA is slow... for timeout stuff only
			sub.l	#1,d6				; count down timeout value
			cmp.l	#0,d6				; if 0, timeout.
			beq	.endloop
			move.w	$dff018,d5
			btst	#13,d5				; Check TBE bit
			beq.s	.timeoutloop
			bra	.notimeout
		.endloop:
		.notimeout:
			move.w	#$0100,d5
			move.b	d7,d5
			move.w	d5,$dff030			; send it to serial
			move.w	#$0001,$dff09c		; turn off the TBE bit
			bra.s	.loop
		.nomore:
			jmp	(a5)
		
		DumpBinHex:
			lea	Hexnumbers,a0
			asl.l	#2,d0
			add.l	d0,a0
			jmp	(a5)
			
		DumpBinDec:
				lea	Decnumbers,a0
				asl.l	#2,d0
				add.l	d0,a0
				jmp	(a5)
				
		
String:
			dc.b	"String to put out on serialport",$a,$d,0
String2:
		dc.b	"another string!!",$a,$d,0

Decnumbers:
	dc.b "0",0,0,0
	dc.b "1",0,0,0
	dc.b "2",0,0,0
	dc.b "3",0,0,0
	dc.b "4",0,0,0
	dc.b "5",0,0,0
	dc.b "6",0,0,0
	dc.b "7",0,0,0
	dc.b "8",0,0,0
	dc.b "9",0,0,0
	dc.b "10",0,0
	dc.b "11",0,0
	dc.b "12",0,0
	dc.b "13",0,0
	dc.b "14",0,0
	dc.b "15",0,0
	dc.b "16",0,0
	dc.b "17",0,0
	dc.b "18",0,0
	dc.b "19",0,0
	dc.b "20",0,0
	dc.b "21",0,0
	dc.b "22",0,0
	dc.b "23",0,0
	dc.b "24",0,0
	dc.b "25",0,0
	dc.b "26",0,0
	dc.b "27",0,0
	dc.b "28",0,0
	dc.b "29",0,0
	dc.b "30",0,0
	dc.b "31",0,0
	dc.b "32",0,0
	dc.b "33",0,0
	dc.b "34",0,0
	dc.b "35",0,0
	dc.b "36",0,0
	dc.b "37",0,0
	dc.b "38",0,0
	dc.b "39",0,0
	dc.b "40",0,0
	dc.b "41",0,0
	dc.b "42",0,0
	dc.b "43",0,0
	dc.b "44",0,0
	dc.b "45",0,0
	dc.b "46",0,0
	dc.b "47",0,0
	dc.b "48",0,0
	dc.b "49",0,0
	dc.b "50",0,0
	dc.b "51",0,0
	dc.b "52",0,0
	dc.b "53",0,0
	dc.b "54",0,0
	dc.b "55",0,0
	dc.b "56",0,0
	dc.b "57",0,0
	dc.b "58",0,0
	dc.b "59",0,0
	dc.b "60",0,0
	dc.b "61",0,0
	dc.b "62",0,0
	dc.b "63",0,0
	dc.b "64",0,0
	dc.b "65",0,0
	dc.b "66",0,0
	dc.b "67",0,0
	dc.b "68",0,0
	dc.b "69",0,0
	dc.b "70",0,0
	dc.b "71",0,0
	dc.b "72",0,0
	dc.b "73",0,0
	dc.b "74",0,0
	dc.b "75",0,0
	dc.b "76",0,0
	dc.b "77",0,0
	dc.b "78",0,0
	dc.b "79",0,0
	dc.b "80",0,0
	dc.b "81",0,0
	dc.b "82",0,0
	dc.b "83",0,0
	dc.b "84",0,0
	dc.b "85",0,0
	dc.b "86",0,0
	dc.b "87",0,0
	dc.b "88",0,0
	dc.b "89",0,0
	dc.b "90",0,0
	dc.b "91",0,0
	dc.b "92",0,0
	dc.b "93",0,0
	dc.b "94",0,0
	dc.b "95",0,0
	dc.b "96",0,0
	dc.b "97",0,0
	dc.b "98",0,0
	dc.b "99",0,0
	dc.b "100",0
	dc.b "101",0
	dc.b "102",0
	dc.b "103",0
	dc.b "104",0
	dc.b "105",0
	dc.b "106",0
	dc.b "107",0
	dc.b "108",0
	dc.b "109",0
	dc.b "110",0
	dc.b "111",0
	dc.b "112",0
	dc.b "113",0
	dc.b "114",0
	dc.b "115",0
	dc.b "116",0
	dc.b "117",0
	dc.b "118",0
	dc.b "119",0
	dc.b "120",0
	dc.b "121",0
	dc.b "122",0
	dc.b "123",0
	dc.b "124",0
	dc.b "125",0
	dc.b "126",0
	dc.b "127",0
	dc.b "128",0
	dc.b "129",0
	dc.b "130",0
	dc.b "131",0
	dc.b "132",0
	dc.b "133",0
	dc.b "134",0
	dc.b "135",0
	dc.b "136",0
	dc.b "137",0
	dc.b "138",0
	dc.b "139",0
	dc.b "140",0
	dc.b "141",0
	dc.b "142",0
	dc.b "143",0
	dc.b "144",0
	dc.b "145",0
	dc.b "146",0
	dc.b "147",0
	dc.b "148",0
	dc.b "149",0
	dc.b "150",0
	dc.b "151",0
	dc.b "152",0
	dc.b "153",0
	dc.b "154",0
	dc.b "155",0
	dc.b "156",0
	dc.b "157",0
	dc.b "158",0
	dc.b "159",0
	dc.b "160",0
	dc.b "161",0
	dc.b "162",0
	dc.b "163",0
	dc.b "164",0
	dc.b "165",0
	dc.b "166",0
	dc.b "167",0
	dc.b "168",0
	dc.b "169",0
	dc.b "170",0
	dc.b "171",0
	dc.b "172",0
	dc.b "173",0
	dc.b "174",0
	dc.b "175",0
	dc.b "176",0
	dc.b "177",0
	dc.b "178",0
	dc.b "179",0
	dc.b "180",0
	dc.b "181",0
	dc.b "182",0
	dc.b "183",0
	dc.b "184",0
	dc.b "185",0
	dc.b "186",0
	dc.b "187",0
	dc.b "188",0
	dc.b "189",0
	dc.b "190",0
	dc.b "191",0
	dc.b "192",0
	dc.b "193",0
	dc.b "194",0
	dc.b "195",0
	dc.b "196",0
	dc.b "197",0
	dc.b "198",0
	dc.b "199",0
	dc.b "200",0
	dc.b "201",0
	dc.b "202",0
	dc.b "203",0
	dc.b "204",0
	dc.b "205",0
	dc.b "206",0
	dc.b "207",0
	dc.b "208",0
	dc.b "209",0
	dc.b "210",0
	dc.b "211",0
	dc.b "212",0
	dc.b "213",0
	dc.b "214",0
	dc.b "215",0
	dc.b "216",0
	dc.b "217",0
	dc.b "218",0
	dc.b "219",0
	dc.b "220",0
	dc.b "221",0
	dc.b "222",0
	dc.b "223",0
	dc.b "224",0
	dc.b "225",0
	dc.b "226",0
	dc.b "227",0
	dc.b "228",0
	dc.b "229",0
	dc.b "230",0
	dc.b "231",0
	dc.b "232",0
	dc.b "233",0
	dc.b "234",0
	dc.b "235",0
	dc.b "236",0
	dc.b "237",0
	dc.b "238",0
	dc.b "239",0
	dc.b "240",0
	dc.b "241",0
	dc.b "242",0
	dc.b "243",0
	dc.b "244",0
	dc.b "245",0
	dc.b "246",0
	dc.b "247",0
	dc.b "248",0
	dc.b "249",0
	dc.b "250",0
	dc.b "251",0
	dc.b "252",0
	dc.b "253",0
	dc.b "254",0
	dc.b "255",0

Hexnumbers:
			dc.b "00",0,0
			dc.b "01",0,0
			dc.b "02",0,0
			dc.b "03",0,0
			dc.b "04",0,0
			dc.b "05",0,0
			dc.b "06",0,0
			dc.b "07",0,0
			dc.b "08",0,0
			dc.b "09",0,0
			dc.b "0A",0,0
			dc.b "0B",0,0
			dc.b "0C",0,0
			dc.b "0D",0,0
			dc.b "0E",0,0
			dc.b "0F",0,0
			dc.b "10",0,0
			dc.b "11",0,0
			dc.b "12",0,0
			dc.b "13",0,0
			dc.b "14",0,0
			dc.b "15",0,0
			dc.b "16",0,0
			dc.b "17",0,0
			dc.b "18",0,0
			dc.b "19",0,0
			dc.b "1A",0,0
			dc.b "1B",0,0
			dc.b "1C",0,0
			dc.b "1D",0,0
			dc.b "1E",0,0
			dc.b "1F",0,0
			dc.b "20",0,0
			dc.b "21",0,0
			dc.b "22",0,0
			dc.b "23",0,0
			dc.b "24",0,0
			dc.b "25",0,0
			dc.b "26",0,0
			dc.b "27",0,0
			dc.b "28",0,0
			dc.b "29",0,0
			dc.b "2A",0,0
			dc.b "2B",0,0
			dc.b "2C",0,0
			dc.b "2D",0,0
			dc.b "2E",0,0
			dc.b "2F",0,0
			dc.b "30",0,0
			dc.b "31",0,0
			dc.b "32",0,0
			dc.b "33",0,0
			dc.b "34",0,0
			dc.b "35",0,0
			dc.b "36",0,0
			dc.b "37",0,0
			dc.b "38",0,0
			dc.b "39",0,0
			dc.b "3A",0,0
			dc.b "3B",0,0
			dc.b "3C",0,0
			dc.b "3D",0,0
			dc.b "3E",0,0
			dc.b "3F",0,0
			dc.b "40",0,0
			dc.b "41",0,0
			dc.b "42",0,0
			dc.b "43",0,0
			dc.b "44",0,0
			dc.b "45",0,0
			dc.b "46",0,0
			dc.b "47",0,0
			dc.b "48",0,0
			dc.b "49",0,0
			dc.b "4A",0,0
			dc.b "4B",0,0
			dc.b "4C",0,0
			dc.b "4D",0,0
			dc.b "4E",0,0
			dc.b "4F",0,0
			dc.b "50",0,0
			dc.b "51",0,0
			dc.b "52",0,0
			dc.b "53",0,0
			dc.b "54",0,0
			dc.b "55",0,0
			dc.b "56",0,0
			dc.b "57",0,0
			dc.b "58",0,0
			dc.b "59",0,0
			dc.b "5A",0,0
			dc.b "5B",0,0
			dc.b "5C",0,0
			dc.b "5D",0,0
			dc.b "5E",0,0
			dc.b "5F",0,0
			dc.b "60",0,0
			dc.b "61",0,0
			dc.b "62",0,0
			dc.b "63",0,0
			dc.b "64",0,0
			dc.b "65",0,0
			dc.b "66",0,0
			dc.b "67",0,0
			dc.b "68",0,0
			dc.b "69",0,0
			dc.b "6A",0,0
			dc.b "6B",0,0
			dc.b "6C",0,0
			dc.b "6D",0,0
			dc.b "6E",0,0
			dc.b "6F",0,0
			dc.b "70",0,0
			dc.b "71",0,0
			dc.b "72",0,0
			dc.b "73",0,0
			dc.b "74",0,0
			dc.b "75",0,0
			dc.b "76",0,0
			dc.b "77",0,0
			dc.b "78",0,0
			dc.b "79",0,0
			dc.b "7A",0,0
			dc.b "7B",0,0
			dc.b "7C",0,0
			dc.b "7D",0,0
			dc.b "7E",0,0
			dc.b "7F",0,0
			dc.b "80",0,0
			dc.b "81",0,0
			dc.b "82",0,0
			dc.b "83",0,0
			dc.b "84",0,0
			dc.b "85",0,0
			dc.b "86",0,0
			dc.b "87",0,0
			dc.b "88",0,0
			dc.b "89",0,0
			dc.b "8A",0,0
			dc.b "8B",0,0
			dc.b "8C",0,0
			dc.b "8D",0,0
			dc.b "8E",0,0
			dc.b "8F",0,0
			dc.b "90",0,0
			dc.b "91",0,0
			dc.b "92",0,0
			dc.b "93",0,0
			dc.b "94",0,0
			dc.b "95",0,0
			dc.b "96",0,0
			dc.b "97",0,0
			dc.b "98",0,0
			dc.b "99",0,0
			dc.b "9A",0,0
			dc.b "9B",0,0
			dc.b "9C",0,0
			dc.b "9D",0,0
			dc.b "9E",0,0
			dc.b "9F",0,0
			dc.b "A0",0,0
			dc.b "A1",0,0
			dc.b "A2",0,0
			dc.b "A3",0,0
			dc.b "A4",0,0
			dc.b "A5",0,0
			dc.b "A6",0,0
			dc.b "A7",0,0
			dc.b "A8",0,0
			dc.b "A9",0,0
			dc.b "AA",0,0
			dc.b "AB",0,0
			dc.b "AC",0,0
			dc.b "AD",0,0
			dc.b "AE",0,0
			dc.b "AF",0,0
			dc.b "B0",0,0
			dc.b "B1",0,0
			dc.b "B2",0,0
			dc.b "B3",0,0
			dc.b "B4",0,0
			dc.b "B5",0,0
			dc.b "B6",0,0
			dc.b "B7",0,0
			dc.b "B8",0,0
			dc.b "B9",0,0
			dc.b "BA",0,0
			dc.b "BB",0,0
			dc.b "BC",0,0
			dc.b "BD",0,0
			dc.b "BE",0,0
			dc.b "BF",0,0
			dc.b "C0",0,0
			dc.b "C1",0,0
			dc.b "C2",0,0
			dc.b "C3",0,0
			dc.b "C4",0,0
			dc.b "C5",0,0
			dc.b "C6",0,0
			dc.b "C7",0,0
			dc.b "C8",0,0
			dc.b "C9",0,0
			dc.b "CA",0,0
			dc.b "CB",0,0
			dc.b "CC",0,0
			dc.b "CD",0,0
			dc.b "CE",0,0
			dc.b "CF",0,0
			dc.b "D0",0,0
			dc.b "D1",0,0
			dc.b "D2",0,0
			dc.b "D3",0,0
			dc.b "D4",0,0
			dc.b "D5",0,0
			dc.b "D6",0,0
			dc.b "D7",0,0
			dc.b "D8",0,0
			dc.b "D9",0,0
			dc.b "DA",0,0
			dc.b "DB",0,0
			dc.b "DC",0,0
			dc.b "DD",0,0
			dc.b "DE",0,0
			dc.b "DF",0,0
			dc.b "E0",0,0
			dc.b "E1",0,0
			dc.b "E2",0,0
			dc.b "E3",0,0
			dc.b "E4",0,0
			dc.b "E5",0,0
			dc.b "E6",0,0
			dc.b "E7",0,0
			dc.b "E8",0,0
			dc.b "E9",0,0
			dc.b "EA",0,0
			dc.b "EB",0,0
			dc.b "EC",0,0
			dc.b "ED",0,0
			dc.b "EE",0,0
			dc.b "EF",0,0
			dc.b "F0",0,0
			dc.b "F1",0,0
			dc.b "F2",0,0
			dc.b "F3",0,0
			dc.b "F4",0,0
			dc.b "F5",0,0
			dc.b "F6",0,0
			dc.b "F7",0,0
			dc.b "F8",0,0
			dc.b "F9",0,0
			dc.b "FA",0,0
			dc.b "FB",0,0
			dc.b "FC",0,0
			dc.b "FD",0,0
			dc.b "FE",0,0
			dc.b "FF",0,0
