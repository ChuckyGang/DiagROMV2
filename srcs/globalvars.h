typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef struct GlobalVars
{
	void*		stack_mem;
	void*		startblock;
	void*		endblock;
	void*		startchip;
	void*		startofchip;
	void*		endofchip;
	uint32_t	startupflags;
	uint32_t 	stack_size;
	uint32_t	current_vhpos;
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
	uint8_t	OVLErr;
} GlobalVars;

typedef struct Bitplanes
{
	uint32_t	Bpl1str;
	uint8_t	Bpl1[80*256];
	uint32_t	Bpl2str;
	uint8_t	Bpl2[80*256];
	uint32_t	Bpl3str;
	uint8_t	test;
	uint8_t	Bpl3[80*256];
} Bitplanes;
