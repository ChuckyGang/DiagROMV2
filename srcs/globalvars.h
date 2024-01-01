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
