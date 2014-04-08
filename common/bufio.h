#ifndef _BUFIO_H
#define _BUFIO_H
#include <stdint.h>
#include "stat.h"

static uint8_t read_U8(const uint8_t **data, const uint8_t *data_end)
{
	if (*data > data_end)
	{
		staterr("read_U8: read past end of buffer: *data > data_end");
		return 0xfe;
	}
	
	return *(*data)++;
}

static uint16_t read_U16(const uint8_t **data, const uint8_t *data_end)
{
	const uint8_t *ptr = *data;
	
	if ((ptr + 1) > data_end)
	{
		staterr("read_U16: read past end of buffer: *data + 1 > data_end");
		return 0xfefe;
	}
	
	*data = (ptr + 2);
	// we should not just cast to a uint16_t, as some processors
	// e.g. ARM would have alignment issues then, plus endian issues on others.
	return (ptr[1] << 8) | ptr[0];
}

#endif
