#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// checksum <diagromfile>

#define RB(ptr,offset) (uint32_t)(((uint8_t*)ptr)[offset])
#define RW(ptr,offset) ((RB(ptr,offset+0) <<  8) | (RB(ptr,offset+1) << 0))
#define RL(ptr,offset) ((RW(ptr,offset+0) << 16) | (RW(ptr,offset+2) << 0))

#define WB(ptr,offset,x) (((uint8_t*)ptr)[offset] = (uint8_t)((x) & 0xff))
#define WW(ptr,offset,x) (WB(ptr,offset+0, x >>  8), WB(ptr,offset+1, x >> 0))
#define WL(ptr,offset,x) (WW(ptr,offset+0, x >> 16), WW(ptr,offset+2, x >> 0))

int main(int argc, const char** argv)
{
	if (argc != 2)
	{
		printf("%s <diagromfile>\n", argv[0]);
		return -1;
	}

	// Read in the raw DiagROM
	FILE* f = fopen(argv[1], "rb");
	if (!f)
	{
		printf("failed to open '%s'\n", argv[1]);
		return -1;
	}

	fseek(f, 0, SEEK_END);
	long int size = ftell(f);
	fseek(f, 0, SEEK_SET);

	uint8_t* mem = malloc(size);
	if (!mem)
	{
		printf("failed to alloc %zd bytes\n", size);
		return -1;
	}
	if (size != fread(mem, sizeof(uint8_t), size, f))
	{
		printf("failed to read file\n");
		return -1;
	}
	fclose(f);

	// Find the end-of-code marker and patch the contents up until the end of the ROM
	uint32_t address_area_start = 0;
	uint32_t address_area_end = 0x00fffff0 - 0x00f80000; // ends at autovec area
	for (int i = 0; i < size; ++i)
	{
		const char end_str[] = "End of Code...";
		if (mem[i] == end_str[0] && !memcmp(mem+i, end_str, sizeof(end_str)-1))
		{
			address_area_start = i;
			address_area_start += sizeof(end_str);
			address_area_start += 0x3;
			address_area_start &= ~0x3;
			break;
		}
	}
	if (address_area_start == 0)
	{
		printf("End-of-code marker string not found!\n");
		return -1;
	}

	printf("End-of-code : %08x\n", address_area_start);

	for (uint32_t offset = address_area_start; offset != address_area_end; offset+=4)
	{
		uint32_t addr = 0xf80000 + offset;
		WL(mem, offset, addr);
	}

	// Find Checksum area, and patch it
	uint32_t checksum_area_start = 0;
	uint32_t checksum_value_offset = 0;
	uint32_t checksum_area_end = 0;

	for (int i = 0; i < size; ++i)
	{
		const char check_str[] = "Checksums:";
		if (mem[i] == check_str[0] && !memcmp(mem+i, check_str, sizeof(check_str)-1))
		{
			checksum_area_start = i;
			checksum_value_offset = (i + (sizeof(check_str)-1) + 3) & ~0x3;
			checksum_area_end = checksum_value_offset + sizeof(uint32_t) * 8;
			break;
		}
	}

	if (checksum_area_start == 0)
	{
		printf("Checksum marker string not found!\n");
		return -1;
	}

	printf("Checksum area start  %08x\n", checksum_area_start);
	printf("Checksum value array %08x\n", checksum_value_offset);
	printf("Checksum area end    %08x\n", checksum_area_end);

	printf("\nChecksums:\n");
	for (int region_nr = 0; region_nr < 8; region_nr++)
	{
		uint32_t checksum = 0;

		for (int i = 0; i < 0x10000; i+=4)
		{
			uint32_t offset = region_nr * 0x10000 + i;
			// NB! the DiagROM checksum test uses incorrect boundary conditions!
			if ((checksum_value_offset+4) <= offset && offset < (checksum_area_end+4))
				continue;
			uint32_t v = RL(mem,offset);
			checksum += v;
		}
		printf("  >>> %d -> %08x\n", region_nr, checksum);
		WL(mem, checksum_value_offset + region_nr * sizeof(uint32_t), checksum);
	}

	// Write out the patched DiagROM
	f = fopen(argv[1], "wb");
	if (!f)
	{
		printf("failed to open '%s'\n", argv[1]);
		return -1;
	}
	if (size != fwrite(mem, sizeof(uint8_t), size, f))
	{
		printf("failed to write file\n");
		return -1;
	}
	fclose(f);

	return 0;
}
