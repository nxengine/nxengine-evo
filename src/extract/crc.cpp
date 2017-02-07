
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "crc.h"

static uint32_t CRC_Table[256];
static const uint32_t poly = 0x04c11db7;

uint32_t reflect(uint32_t value, int size)
{
uint32_t newbits = 0;
int i;

	for(i=1;i<=size;i++)
	{
		if (value & 1)
			newbits |= (1 << (size - i));
		
		value >>= 1;
	}

	return newbits;
}

void crc_init(void)
{
int i, j;
	
	for(i=0;i<256;i++)
	{
		CRC_Table[i] = reflect(i, 8) << 24;
		
		for(j=0;j<8;j++)
			CRC_Table[i] = (CRC_Table[i] << 1) ^ ((CRC_Table[i] & (1 << 31)) ? poly : 0);
		
		CRC_Table[i] = reflect(CRC_Table[i], 32);
	}
}

uint32_t crc_calc(uint8_t *buf, uint32_t size)
{
uint32_t crc = 0xFFFFFFFF;
	
	while(size)
	{
		crc = (crc >> 8) ^ CRC_Table[(crc & 0xFF) ^ *buf++];
		size--;
	}
	
	return (crc ^ 0xFFFFFFFF);
}

