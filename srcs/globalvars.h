//typedef unsigned int uint32_t;
//typedef unsigned char uint8_t;
#include <stdint.h>
typedef struct GlobalVars
{
	void*		stack_mem;
	void*		startblock;
	void*		endblock;
	void*		StartChip;
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
	uint32_t	null;				// Nullpointer
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

	uint32_t	startupflags;
	uint32_t 	stack_size;
	uint32_t	current_vhpos;
	uint32_t	BPLSIZE;			// Size of one bitplane
	uint32_t	TotalChip;			// Total amount of Chipmem Found
	uint32_t	ChipUnreserved;		// Total amount of not used chipmem
	uint32_t	BootMBFastmem;		// Fastmem found during boot
	uint32_t	BLTDDAT;
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
	uint16_t	SerialSpeed;			// What serialspeed is used (in list, not real baudrate)
	uint16_t	CurX;
	uint16_t	CurY;
	uint16_t	CurAddX;
	uint16_t	CurSubX;
	uint16_t	CurAddY;
	uint16_t	CurSubY;
	uint16_t	SerAnsiChecks;		// Number of checks with a result of 0 in Ansimode

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
	uint8_t	keypressed[2];
	uint8_t	keypressedshifted[2];
	uint8_t	SerBuf[256];			// Serialbuffer
	uint8_t	b2dString[12];		// Stringbuffer for bindec
	uint8_t	b2dTemp[8];			// Tempbuffer for bindec
	uint8_t	binhexoutput[10];		// Buffer for binhex
	uint8_t	bindecoutput[14];		// Output of old bin->dec routine still used
	uint8_t	binstringoutput[33];
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
	uint32_t	MenuCopperList[40];		// Menucopperlist
	uint32_t	ECSCopperList[67];		// Copperlist for ECS Test
	uint32_t	ECSCopper2List[67];		// Copperlist for ECS2 Test
	uint8_t	ptplayroutine[4538];		// Space for Protracker replayroutine
	uint8_t	AudioWaveData[247];		// Audiodata

} Bitplanes;
