
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined (_WIN32)
#include <direct.h>
#include <io.h>
#endif


#include "crc.h"
#include "extractfiles.h"


#define HEADER_LEN		25
#define MAX_FILE_SIZE	32768

// Windows .bmp resources don't include the BMP-file headers
const uint8_t credit_header[] = \
{
	0x42, 0x4D, 0x76, 0x4B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x76, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0xA0, 0x00,
	0x00, 0x00, 0xF0, 0x00, 0x00
};

const uint8_t pixel_header[] = \
{
	0x42, 0x4D, 0x76, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x76, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0xA0, 0x00,
	0x00, 0x00, 0x10, 0x00, 0x00
};

static struct
{
	const char *filename;
	uint32_t offset;
	uint32_t length;
	uint32_t crc;
	const uint8_t *header;
}
files[] =
{
	{"data/endpic/credit01.bmp", 0x117047, 19293, 0xeb87b19b, credit_header},
	{"data/endpic/credit02.bmp", 0x11bbaf, 19293, 0x239c1a37, credit_header},
	{"data/endpic/credit03.bmp", 0x120717, 19293, 0x4398bbda, credit_header},
	{"data/endpic/credit04.bmp", 0x12527f, 19293, 0x44bae3ac, credit_header},
	{"data/endpic/credit05.bmp", 0x129de7, 19293, 0xd1b876ad, credit_header},
	{"data/endpic/credit06.bmp", 0x12e94f, 19293, 0x5a60082e, credit_header},
	{"data/endpic/credit07.bmp", 0x1334b7, 19293, 0xc1e9db91, credit_header},
	{"data/endpic/credit08.bmp", 0x13801f, 19293, 0xcbbcc7fa, credit_header},
	{"data/endpic/credit09.bmp", 0x13cb87, 19293, 0xfa7177b1, credit_header},
	{"data/endpic/credit10.bmp", 0x1416ef, 19293, 0x56390a07, credit_header},
	{"data/endpic/credit11.bmp", 0x146257, 19293, 0xff3d6d83, credit_header},
	{"data/endpic/credit12.bmp", 0x14adbf, 19293, 0x9e948dc2, credit_header},
	{"data/endpic/credit14.bmp", 0x14f927, 19293, 0x32b6ce2d, credit_header},
	{"data/endpic/credit15.bmp", 0x15448f, 19293, 0x88539803, credit_header},
	{"data/endpic/credit16.bmp", 0x158ff7, 19293, 0xc0ef9adf, credit_header},
	{"data/endpic/credit17.bmp", 0x15db5f, 19293, 0x8c5a003d, credit_header},
	{"data/endpic/credit18.bmp", 0x1626c7, 19293, 0x66bcbf22, credit_header},
	{"data/endpic/pixel.bmp",    0x16722f, 1373,  0x6181d0a1, pixel_header},
	{"data/wavetable.dat",       0x110664, 25600, 0xb3a3b7ef, NULL},
	{"data/org/access.org",      0x09b35c, 1138,  0xd965dddb, NULL},
	{"data/org/balcony.org",     0x09dbbc, 3082,  0x892345ca, NULL},
	{"data/org/ginsuke.org",     0x0b45a0, 5970,  0xb02093b8, NULL},
	{"data/org/bdown.org",       0x09f5bc, 2570,  0xf80dd62a, NULL},
	{"data/org/cemetery.org",    0x09ffc8, 4578,  0x2ce377cc, NULL},
	{"data/org/lastbtl.org",     0x0d28d4, 2770,  0x10dec9d5, NULL},
	{"data/org/ending.org",      0x0a7eac, 17898, 0xa9ed4834, NULL},
	{"data/org/wanpaku.org",     0x0feb20, 19626, 0xb651047e, NULL},
	{"data/org/fireeye.org",     0x0aedc0, 21354, 0x6b5ff989, NULL},
	{"data/org/fanfale1.org",    0x0ae25c, 914,   0xaefd547b, NULL},
	{"data/org/fanfale2.org",    0x0ae98c, 1074,  0x3a5170a6, NULL},
	{"data/org/fanfale3.org",    0x0ae5f0, 922,   0x85813929, NULL},
	{"data/org/gameover.org",    0x0b412c, 1138,  0x1f87b446, NULL},
	{"data/org/grand.org",       0x0b5cf4, 13466, 0xdb4795ac, NULL},
	{"data/org/vivi.org",        0x0f83c8, 10458, 0xce2e68c1, NULL},
	{"data/org/gravity.org",     0x0b9190, 20578, 0x64a9318d, NULL},
	{"data/org/weed.org",        0x1037cc, 23706, 0xa27883b6, NULL},
	{"data/org/hell.org",        0x0be1f4, 18386, 0x93bbf277, NULL},
	{"data/org/requiem.org",     0x0f1598, 9722,  0xfc64d0d0, NULL},
	{"data/org/jenka.org",       0x0c5e54, 8306,  0xb42d7eaa, NULL},
	{"data/org/jenka2.org",      0x0c7ec8, 11986, 0xc095cbe1, NULL},
	{"data/org/maze.org",        0x0dbcb8, 14786, 0x0292cf2c, NULL},
	{"data/org/lastbt3.org",     0x0cd650, 21122, 0x8888dac9, NULL},
	{"data/org/lastcave.org",    0x0d33a8, 18122, 0x469b38b9, NULL},
	{"data/org/mdown2.org",      0x0df67c, 21074, 0x83d08aed, NULL},
	{"data/org/ironh.org",       0x0c29c8, 13450, 0x3ce4cdbe, NULL},
	{"data/org/oside.org",       0x0e725c, 25634, 0x1e33b095, NULL},
	{"data/org/plant.org",       0x0ed680, 11378, 0x3911e040, NULL},
	{"data/org/kodou.org",       0x0cad9c, 10418, 0x92ef0330, NULL},
	{"data/org/quiet.org",       0x0f02f4, 4770,  0x0e95a468, NULL},
	{"data/org/escape.org",      0x0ac498, 7618,  0x65a4bb85, NULL},
	{"data/org/anzen.org",       0x09b7d0, 9194,  0x779e83c2, NULL},
	{"data/org/wanpak2.org",     0x0faca4, 15994, 0xd09341e2, NULL},
	{"data/org/ballos.org",      0x09e7c8, 3570,  0x373988ad, NULL},
	{"data/org/curly.org",       0x0a11ac, 25738, 0xf5ace8b0, NULL},
	{"data/org/toroko.org",      0x0f3b94, 18482, 0xc202de07, NULL},
	{"data/org/mura.org",        0x0e48d0, 10634, 0x6a6aa627, NULL},
	{"data/org/dr.org",          0x0a7638, 2162,  0xc64dc450, NULL},
	{"data/org/marine.org",      0x0d7a74, 16962, 0xb533d72a, NULL},
	{"data/org/white.org",       0x109468, 23714, 0xcff0fb34, NULL},
	{"data/org/zonbie.org",      0x10f180, 5346,  0xd217cc29, NULL},
	{NULL}
};

void createdir(const char *fname)
{
	char *dir = strdup(fname);
	char *ptr = strrchr(dir, '/');
	if (ptr)
	{
		*ptr = 0;
		
		#if defined(_WIN32) || defined(_WIN64)
			_mkdir(dir);
		#else
			mkdir(dir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
		#endif
	}
	
	free(dir);
}

bool extract_files(FILE *exefp)
{
uint8_t *buffer;
uint8_t *file;
uint32_t length;
uint32_t crc;
bool check_crc = true;
//bool first_crc_failure = true;

	buffer = (uint8_t *)malloc(MAX_FILE_SIZE);
	crc_init();
	
	for(int i=0;;i++)
	{
		const char *outfilename = files[i].filename;
		if (!outfilename) break;
		
		printf("[ %s ]\n", outfilename);
		
		// initialize header if any
		file = buffer;
		length = files[i].length;
		
		if (files[i].header)
		{
			memcpy(buffer, files[i].header, HEADER_LEN);
			file += HEADER_LEN;
			length += HEADER_LEN;
		}
		
		// read data from exe
		fseek(exefp, files[i].offset, SEEK_SET);
		fread(file, files[i].length, 1, exefp);
		
		if (check_crc)
		{
			crc = crc_calc(file, files[i].length);
			if (crc != files[i].crc)
			{
				printf("File '%s' failed CRC check.\n", outfilename);
				
//				first_crc_failure = false;
			}
		}
		
		// write out the file
		createdir(outfilename);
		
		FILE *fp = fopen(outfilename, "wb");
		if (!fp)
		{
			printf("Failed to open '%s' for writing.\n", outfilename);
			free(buffer);
			return 1;
		}
		
		fwrite(buffer, length, 1, fp);
		fclose(fp);
	}
	
	free(buffer);
	return 0;
}
