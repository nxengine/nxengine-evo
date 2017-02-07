
#include <stdio.h>
#include <stdint.h>
#include "common/stat.h"
#include "common/misc.h"

/*
	290.rec contains the tick value 4 times, followed by the 4 byte key
	to decrypt each instance, for a total of 20 bytes.
*/

static const char *getfname()
{
	// might do stuff with sprintf here to place it in other subdirectories, etc.
	// is just here for expansion.
	return "290.rec";
}

// load the contents of 290.rec and store in value_out. Returns 0 on success.
// If there is no such file or an error occurs, writes 0 to value_out.
bool niku_load(uint32_t *value_out)
{
FILE *fp;
uint8_t buffer[20];
uint32_t *result = (uint32_t *)buffer;
int i, j;

	const char *fname = getfname();
	fp = fopen(fname, "rb");
	if (!fp)
	{
		stat("niku_load: couldn't open file '%s'", fname);
		if (value_out) *value_out = 0;
		return 1;
	}
	
	fread(buffer, 20, 1, fp);
	fclose(fp);
	
	for(i=0;i<4;i++)
	{
		uint8_t key = buffer[i+16];
		
		j = i * 4;
		buffer[j] -= key;
		buffer[j+1] -= key;
		buffer[j+2] -= key;
		buffer[j+3] -= (key / 2);
	}
	
	if ((result[0] != result[1]) || \
		(result[0] != result[2]) || \
		(result[0] != result[3]))
	{
		stat("niku_load: value mismatch; '%s' corrupt", fname);
		if (value_out) *value_out = 0;
	}
	else
	{
		stat("niku_load: loaded value 0x%x from %s", *result, fname);
		if (value_out) *value_out = *result;
	}
	
	return 0;
}

// save the timestamp in value to 290.rec.
bool niku_save(uint32_t value)
{
uint8_t buf_byte[20];
uint32_t *buf_dword = (uint32_t *)buf_byte;

	// place values
	buf_dword[0] = value;
	buf_dword[1] = value;
	buf_dword[2] = value;
	buf_dword[3] = value;
	
	// generate keys
	buf_byte[16] = random(0, 255);
	buf_byte[17] = random(0, 255);
	buf_byte[18] = random(0, 255);
	buf_byte[19] = random(0, 255);
	
	// encode each copy
	for(int i=0;i<4;i++)
	{
		uint8_t *ptr = (uint8_t *)&buf_dword[i];
		uint8_t key = buf_byte[i+16];
		
		ptr[0] += key;
		ptr[1] += key;
		ptr[2] += key;
		ptr[3] += key / 2;
	}
	
	const char *fname = getfname();
	FILE *fp = fopen(fname, "wb");
	if (!fp)
	{
		staterr("niku_save: failed to open '%s'", fname);
		return 1;
	}
	
	fwrite(buf_byte, 20, 1, fp);
	fclose(fp);
	
	stat("niku_save: wrote value 0x%08x", value);
	return 0;
}

