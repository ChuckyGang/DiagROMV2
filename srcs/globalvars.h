#pragma once
//typedef unsigned int uint32_t;
//typedef unsigned char uint8_t;
#include <stdint.h>
typedef struct GlobalVars
{
	
	void*		stack_mem;
	void*		startblock;
	void*		endblock;
	void*		ChipStart;
	void*		ChipEnd;
	void*		GetChipAddr;
	void*		BaseStart;
	void*		BaseEnd;
	void*		ChipmemBlock;			// Pointer to the Chipmem bock
	void*		keymap;			// Keymap used
	void*		BPL;				// Pointer to bitplaneblock
	void*		Bpl1Ptr;			// Pointer to Bitplane1 // Those 3 must be stored in a row
	void*		Bpl2Ptr;			// Pointer to Bitplane2
	void*		Bpl3Ptr;			// Pointer to Bitplane3
	void*		BplNull;			// This should follow bitplanes and point to 0
	void*		DummySprite;			// Pointer to the dummysprite
	void*		MenuCopper;			// Pointer to Menu Copperlist
	void*		ECSCopper;			// Pointer to ECS Copperlist
	void*		ECSCopper2;			// Pointer to ECS Copperlist2
	void*		AudioWaves;			// Pointer to Audiowavedata
	void*		AudioModAddr;			// Address of module in modtest
	void*		AudioModInit;			// Address to MT_Init
	void*		AudioModEnd;			// Address to MT_End
	void*		AudioModMusic;		// Address to MT_Music
	void*		AudioModMVol;			// Address to MasterVolume
	void*		AudioModData;			// Pointer to module
	void*		ptplay;			// Pointer to Protracker playroutine
	void*		CPUPointer;			// Pointer to CPU String
	void*		FPUPointer;			// Pointer to FPU String
	void*		CheckMemEditScreenAdr;	// Startaddress of memorydump on screen in edit-mode
	void*		Menu;				// Pointer to the Menu to use
	void*		MenuVariable;			// List of pointers to variables to print after menuitem
	void*		test;
	void*		ChipUnreservedAddr;		// End of reserved space
	void*		CheckMemFrom;			// Startadr of mem to check
	void*		CheckMemTo;			// Endadr of mem to check
	void*		RunCodeStart;			// will contain start of adr to run code in
	void*		RunCodeEnd;			// End...
	void*		CheckMemCurrent;		// Current adr to check
	void*		CheckMemCurrentOLD;
	void*		MemAdr;			// Pointer to memory from getmemory
	void*		GfxTestBpl[8];		// Pointers to bitplanes for gfxtest
	void*		trackbuff;			// Pointer to trackbuffer
	void*		DiskBuffer;			// Pointer to diskbuffer
	void*		ShowMemAdr;			// Address to show at shomemaddr
	void*		AutoConfAddr;			// Address to config board to
	void*		AutoConfFrom;
	void*		AutoConfTo;
	void*		TF1260IOStart;
	void*		TF1260IOEnd;
	void*		TF1260MemStart;
	void*		TF1260MemEnd;

	uint32_t	startupflags;
	uint32_t 	stack_size;
	uint32_t	current_vhpos;
	uint32_t	BPLSIZE;			// Size of one bitplane
	uint32_t	TotalChip;			// Total amount of Chipmem Found
	uint32_t	ChipUnreserved;		// Total amount of not used chipmem
	uint32_t	BootMBFastmem;		// Fastmem found during boot
	uint32_t	Xpos;				// Variable for X position on screen to print on
	uint32_t	Ypos;				// Variable for Y position on screen to print on
	uint32_t	PCRReg;			// Calue of PCRReg IF 060, if not, this is 0
	uint32_t	CPU;				// Type of CPU
	uint32_t	CPUGen;			// Generation of CPU
	uint32_t	FPU;				// Type of FPU
	uint32_t	DebugA0;			// Store variables for debughandling!
	uint32_t	DebugD1;
	uint32_t	DebD0;
	uint32_t	DebD1;
	uint32_t	DebD2;
	uint32_t	DebD3;
	uint32_t	DebD4;
	uint32_t	DebD5;
	uint32_t	DebD6;
	uint32_t	DebD7;
	uint32_t	DebD8;
	uint32_t	DebA0;
	uint32_t	DebA1;
	uint32_t	DebA2;
	uint32_t	DebA3;
	uint32_t	DebA4;
	uint32_t	DebA5;
	uint32_t	DebA6;
	uint32_t	DebA7;
	uint32_t	DebSR;
	uint32_t	DebPC;
	uint32_t	PowerONStatus;
	uint32_t	InputRegister;		// Value of D0 of GetInput is stored here aswell (apparently)
	uint32_t	FastMem;
	uint32_t	TotalFast;
	void*		FastStart;
	void*		FastEnd;
	uint32_t	FastBlocksAtBoot;		// Amount of fastmemblocks found at boot
	uint32_t	LogYpos;
	uint32_t	CheckMemStepSize;
	uint32_t	CheckMemBitError;		// Contains the biterrors
	uint32_t	CheckMemTypeStart;
	uint32_t	CheckMemAdrRnd;		// Store a random number for addresstest
	uint32_t	CheckMemUsable;		// How much usable memory
	uint32_t	CheckMemPassQuit;		// If not 0, we quit this pass
	uint32_t	CheckMemPreFail;		// shold be 0 or something failed preparing the block and do not test this block
	uint32_t	CheckMemAdrError;		// Contain mask of addresserror
	uint32_t	CheckMemAdrErrorOLD;
	uint32_t	CheckMemNonUsable;
	uint32_t	CheckMemNonUsableOLD;
	uint32_t	CheckMemHighError;		// Will contain all bits with stuck 1
	uint32_t	CheckMemLowError;		// Will contain all bits with stuck 0
	uint32_t	CheckMemBlockDone;		// How many blocks is done
	uint32_t	CheckMemChecked;		// How much memory is checked
	uint32_t	CheckMemCheckedOLD;
	uint32_t	CheckMemErrors;		// Number of errors found
	uint32_t	CheckMemErrorsOLD;
	uint32_t	CheckMemCancelReason;
	uint32_t	CheckMemPass;			// Number of passes done
	uint32_t	CheckMemPassOLD;
	uint32_t	CheckMemPassOK;		// Number of OK passes
	uint32_t	CheckMemPassOKOLD;
	uint32_t	CheckMemPassFail;		// Number of failed passes
	uint32_t	CheckMemPassFailOLD;
	uint32_t	CheckMemUsableOLD;
	uint32_t	CheckMemNoErrors;		// Total number of memoryerrors
	uint32_t	CheckMemAdrError2;		// Total number of adresserrors
	uint32_t	MemTestPass;			// Number of passes
	uint32_t	DetectMemRnd;			// Used as a flag to tag for shadowram
	uint32_t	FastmemBlock;			// Number of fastmem block found
	uint32_t	HexBinBin;
	uint32_t	DecBinBin;
	uint32_t	CIACtrl;
	uint32_t	Ticks;				// Number of ticks in CIA test
	uint32_t	Passno;			// Number of passes
	uint32_t	PortJoy0;			// Detected directions of Joystick 0
	uint32_t	PortJoy1;
	uint32_t	PortJoy0OLD;
	uint32_t	PortJoy1OLD;
	uint32_t	GayleData;			// Data from Gayletest
	uint32_t	RTCold;			// How RTC first longword was read
	uint32_t	AutoConfBoards;		// how many boards are autoconfigured
	uint32_t	AutoConfSize;			// Size of current board
	uint32_t	Frames;			// Number of frames shown

	uint16_t	CheckMemCancel;		// if not 0 we had to cancel test
	uint16_t	MemDetected;			// If mem was detected
	uint16_t	IRQLev7;			// If 0 not lev 7
	uint16_t	IRQLevDone;
	uint16_t	TickFrame;			// How many frames reached when CIA test was done

	uint16_t	DMACONR;
	uint16_t	VPOSR;
	uint16_t	VHPOSR;
	uint16_t	DSKDATR;
	uint16_t	JOY0DAT;
	uint16_t	JOY1DAT;
	uint16_t	CLXDAT;
	uint16_t	ADKCONR;
	uint16_t	POT0DAT;
	uint16_t	POT1DAT;
	uint16_t	POTINP;
	uint16_t	SERDATR;
	uint16_t	DSKBYTR;
	uint16_t	INTENAR;
	uint16_t	INTREQR;
	uint16_t	DENISEID;
	uint16_t	HHPOSR;
	uint16_t	BLTDDAT;
	uint16_t	CIAAPRA;
	uint16_t	SerialSpeed;			// What serialspeed is used (in list, not real baudrate)
	uint16_t	CurX;
	uint16_t	CurY;
	uint16_t	CurAddX;
	uint16_t	CurSubX;
	uint16_t	CurAddY;
	uint16_t	CurSubY;
	uint16_t	SerAnsiChecks;		// Number of checks with a result of 0 in Ansimode
	uint16_t	MenuNumber;			// Contains the menunum ber to be printed, from the Menus list
	uint16_t	MenuMouseSub;
	uint16_t	MenuMouseAdd;
	uint16_t	OldMarkItem;			// Contains the item being marked before
	uint16_t	OldMenuNumber;		// Contains old menunumber
	uint16_t	SerTstBps;			// BPS of serialtest
	uint16_t	P0Fire;			// Detected fire on Joystick 0
	uint16_t	P1Fire;
	uint16_t	P0FireOLD;
	uint16_t	P1FireOLD;
	uint16_t	DriveNo;			// Drivenumber to test
	uint16_t	DriveOK;			// Status of drive, 0=not ok, 1=OK
	uint16_t	RTC1secframe;			// Number of frames in 1 sec
	uint16_t	RTC10secframe;		// Number of frames in 10 sec
	uint16_t	RTCsec;			// Number of seconds RTC test have been running
	uint16_t	RTCirq;			// 0 if IRQ is off
	uint16_t	AutoConfZ3;			// Where to config next Z3 card
	uint16_t	BackupAutoConffZ3;
	uint16_t	AutoConfWByte;		// "Byte" to write to autoconfigboards (word for Z3)

	uint32_t	DriveTestVar[4];
	uint32_t	sectorbuff[4];		// Small part of MFM decoded sectordata
	uint32_t	AutoConfList[14*33];		// Store data for 33 boards

	uint16_t	AudSimpVar;			// Those must all be in one block!!
	uint32_t	AudSimpVar1;
	uint16_t	AudSimpVar2;
	uint32_t	AudSimpVar3;
	uint16_t	AudSimpVar4;
	uint32_t	AudSimpVar5;
	uint16_t	AudSimpVar6;
	uint32_t	AudSimpVar7;
	uint16_t	AudSimpVar8;
	uint32_t	AudSimpVar9;
	uint16_t	AudSimpVar10;
	uint32_t	AudSimpVar11;
	uint16_t	AudSimpVar12;
	uint32_t	AudSimpVar13;
	uint16_t	AudSimpVar14;
	uint32_t	AudSimpVar15;
	uint16_t	AudSimpVar16;
	uint32_t	AudSimpVar17;			// Those must all be in one block!
	uint8_t	AudioVolSelect;		// Was Vol Selection in menu selected
	uint8_t	NoSerial;			// No serial output
	uint8_t	STUCKP1LMB;			// If LMB1 was stuck
	uint8_t	STUCKP2LMB;
	uint8_t	STUCKP1RMB;
	uint8_t	STUCKP2RMB;
	uint8_t	STUCKP1MMB;
	uint8_t	STUCKP2MMB;
	uint8_t	RomAdrErr;			// Did we have errors in ROM address-scan
	uint8_t	ChipBitErr;			// Did we have biterrors in chipmem at boot
	uint8_t	ChipAdrErr;			// Did we have addresserrors in chipmem at boot
	uint8_t	NotEnoughChip;		// Did we have out of mem for chipmem at boot
	uint8_t	ScanFastMem;			// Did we scan for fastmem at boot
	uint8_t	FastFound;			// Did we found fastmem at boot
	uint8_t	NoDraw;			// Set we do not draw anything on screen
	uint8_t	StuckMouse;			// Did we have any stuck mouse
	uint8_t	MemAt400;			// Set if we had memory at $400
	uint8_t	OVLErr;			// Set if we had OVL Errors
	uint8_t	WorkOrder;			// Set if we had reversed workorder (start instead of end of RAM)
	uint8_t	LoopB;				// Set if we had a Loopback adapter
	uint8_t	OldSerial;			// Contains the last char that was detected on the serialport
	uint8_t	SerData;			// if 0 we had no serialdata
	uint8_t	BUTTON;			// if 0 we had no button is pressed
	uint8_t	SerBufLen;			// Current length of serialbuffer
	uint8_t	RASTER;			// if set to 1 we had a working Raster
	uint8_t	SCRNMODE;			// If 0 we are in PAL mode, any other is NTSC
	uint8_t	Color;				// Current color
	uint8_t	Inverted;			// if 0, former what was not inverted
	uint8_t	NoChar;			// if 0 print a char. if not.  just do not print
	uint8_t	CPU060Rev;			// Rev of 060 CPU
	uint8_t	MMU;				// If 0, there is no MMU
	uint8_t	ADR24BIT;			// If 0, no 24 bit address CPU
	uint8_t	MOUSE;				// if not 0 mouse is moved
	uint8_t	MBUTTON;			// if not 0 a mousebutton is pressed
	uint8_t	LMB;				// if not 0 LMB is pressed
	uint8_t	RMB;				// if not 0 RMB is pressed
	uint8_t	MMB;
	uint8_t	P1LMB;				// P1 LMB
	uint8_t	P2LMB;				// P2 LMB
	uint8_t	P1RMB;				// P1 RMB
	uint8_t	P2RMB;				// P2 RMB
	uint8_t	P1MMB;				// P1 MMB
	uint8_t	P2MMB;				// P2 MMB
	uint8_t	DISPAULA;			// If not 0, Paula seems to be bad! so  no paulatests shold be done to check keypresses etc.
	uint8_t	Serial;			// Will contain output from serialport
	uint8_t	key;				// Current Keycode
	uint8_t	OldMouse1Y;
	uint8_t	OldMouse2Y;
	uint8_t	OldMouse1X;
	uint8_t	OldMouse2X;
	uint8_t	OldMouseX;
	uint8_t	OldMouseY;
	uint8_t	MouseX;
	uint8_t	MouseY;
	uint8_t	GetCharData;			// Result of GetChar
	uint8_t	SerAnsiFlag;			// Nonzero means we are in buffermode (number is actually number of chars in buffer)
	uint8_t	SerAnsi35Flag;
	uint8_t	SerAnsi36Flag;
	uint8_t	skipnextkey;			// If set to other then 0, next keypress will be ignored
	uint8_t	keyresult;			// Actual result to be printed on screen
	uint8_t	keynew;			// if 1 the keypress is new
	uint8_t	keyup;				// if 1 a key is pressed
	uint8_t	keydown;
	uint8_t	scancode;
	uint8_t	keyalt;
	uint8_t	keyctrl;
	uint8_t	keycaps;
	uint8_t	keyshift;
	uint8_t	keystatus;
	uint8_t	MenuChoose;			// If anything else then 0, user have chosen this item in the menu
	uint8_t	MarkItem;			// Contains the item being marked
	uint8_t	PrintMenuFlag;		// If set to anything else then 0, print the menu
	uint8_t	MenuPos;			// What menu utom to highlight
	uint8_t	MenuEntrys;			// Will contain number of entrys in the menu being displayed
	uint8_t	UpdateMenuFlag;		// If set to anything then 0, update menu
	uint8_t	UpdateMenuNumber;		// What itemnumber to update. 0 = all  (0 is the only that prints label)
	uint8_t	AudSimpChan1;		// This until filter needs to be unchanged or audiotest will break
	uint8_t	AudSimpChan2;
	uint8_t	AudSimpChan3;
	uint8_t	AudSimpChan4;
	uint8_t	AudSimpVol;
	uint8_t	AudSimpWave;
	uint8_t	AudSimpFilter;
	uint8_t	CheckMemNoShadow;		// If anything else then 0 no shadowcheck will be done
	uint8_t	CheckMemType;			// Type of memory detected last time 0=none, 1=error, 2=good
	uint8_t	CheckMemOldType;
	uint8_t	CheckMemTypeEnd;		// If this is  0 tyhen we can have a "end" text
	uint8_t	savexpos;
	uint8_t	saveypos;
	uint8_t	savecol;
	uint8_t	CheckMemManualX;
	uint8_t	CheckMemManualY;
	uint8_t	CheckMemEditXpos;
	uint8_t	CheckMemEditYpos;
	uint8_t	CheckMemEditOldXpos;
	uint8_t	CheckMemEditOldYpos;
	uint8_t	CheckMemEditCharPos;
	uint8_t	CPUCache;
	uint8_t	oldbfe001;			// Contains old value of bfe001
	uint8_t	oldbfd100;
	uint8_t	SideNo;			// Side of disk 0=Upper
	uint8_t	DriveMotor;			// Floppymotor. 0=off
	uint8_t	TrackNo;			// Current tracknumber
	uint8_t	WantedTrackNo;		// Wanted tracknumber
	uint8_t	sector;			// Current sector
	uint8_t	KeyBOld;			// Stores old scancode of keyboard
	uint8_t	AutoConfZ2Ram;		// Where to config ram to next Z2 card
	uint8_t	AutoConfZ2IO;			// Where to config rom to next Z2 card
	uint8_t	AutoConfType;			// If set to 0 no autoconfig was found, 1=ROM, 2=RAM, 3=Z2Space, no RAM
	uint8_t	AutoConfShutD;		// if not 0, we had to shutdown of a card
	uint8_t	AutoConfMode;			// if anything but 0, a detailed (and manual) autoconfig till be done
	uint8_t	AutoConfDone;			// if set to anything but 0, autoconfig has been done
	uint8_t	AutoConfFlag;
	uint8_t	BackupAutoConfZ2Ram;
	uint8_t	BackupAutoConfZ2IO;
	uint8_t	AutoConfExit;			// If anything then 0, force exit of loop
	uint8_t	AutoConfIllegal;		// if not 0, autoconfig was illegal force shutdown of card
	uint8_t	AutoConfZorro;		// 0 = Z2, 1=Z3
	uint8_t	IRQ1OK;			// If set to 1 IRQ is tested ok
	uint8_t	IRQ2OK;
	uint8_t	IRQ3OK;
	uint8_t	IRQ4OK;
	uint8_t	IRQ5OK;
	uint8_t	IRQ6OK;
	uint8_t	IRQ7OK;
	uint8_t	IRQ1;				// If set to 1 IRQ is triggered
	uint8_t	IRQ2;
	uint8_t	IRQ3;
	uint8_t	IRQ4;
	uint8_t	IRQ5;
	uint8_t	IRQ6;
	uint8_t	IRQ7;
	uint8_t	ICR;
	uint8_t	NTSC;				// Set to 1 if a NTSC machine
	uint8_t	AGNUS;				// What Agnus/Alice chip is used
	uint8_t	ODDCIATIMEROK;
	uint8_t	ODDTODOK;
	uint8_t	ODDIRQOK;
	uint8_t	ODDALARMOK;
	uint8_t	EVENCIATIMEROK;
	uint8_t	EVENIRQOK;
	uint8_t	EVENALARMOK;
	uint8_t	EVENTODOK;

	uint8_t	temp[10];
	uint8_t	CheckMemStartAdrTxt[9];
	uint8_t	CheckMemEndAdrTxt[9];
	uint8_t	keypressed[2];
	uint8_t	keypressedshifted[2];
	uint8_t	SerBuf[256];			// Serialbuffer
	uint8_t	bindecoutput[14];		// Output of old bin->dec routine still used
	uint8_t	binstringoutput[33];
	uint16_t	NULL2;				// need to be before ModStatData
	uint8_t	AudSimpVolStr[10];
	uint8_t	AudioModStatData[8];		// Audiomod status.
	uint8_t	AudioModStatFormerData[8];	// Should follow AudioModStatData
	uint8_t	JunkBuffer[256];		// A small crapbuffer
	uint8_t	RTCString[14];		// Block of RTC Data
	uint8_t	AutoConfBuffer[20];		// Autoconfigbuffer
	uint16_t	NULL3;
	uint8_t	b2dString[12];		// Stringbuffer for bindec
	uint8_t	binhexoutput[10];		// Buffer for binhex
	uint8_t	b2dTemp[8];			// Tempbuffer for bindec
	void*		EndVar;			// End of variables
} GlobalVars;

typedef struct Chipmemstuff
{
	uint32_t	Bpl1str;
	uint8_t	Bpl1[80*256];
	uint32_t	Bpl2str;
	uint8_t	Bpl2[80*256];
	uint32_t	Bpl3str;
	uint8_t	Bpl3[80*256];
	uint8_t	NULL;
	uint32_t	End;
	uint32_t	dummysprite;
	uint32_t	MenuCopperList[41];		// Menucopperlist
	uint32_t	ECSCopperList[68];		// Copperlist for ECS Test
	uint32_t	ECSCopper2List[68];		// Copperlist for ECS2 Test
	uint8_t	ptplayroutine[4538];		// Space for Protracker replayroutine
	uint8_t	AudioWaveData[247];		// Audiodata
} Chipmemstuff;
