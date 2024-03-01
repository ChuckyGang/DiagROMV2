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
	uint8_t	SerBuf[256];			// Serialbuffer
	uint8_t	b2dString[12];		// Stringbuffer for bindec
	uint8_t	b2dTemp[8];			// Tempbuffer for bindec
	uint8_t	binhexoutput[10];		// Buffer for binhex
	uint8_t	bindecoutput[14];		// Output of old bin->dec routine still used
	uint8_t	NoChar;			// if 0 print a char. if not.  just do not print
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
