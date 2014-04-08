
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "DBuffer.h"


DBuffer::DBuffer()
{
	fData = &fBuiltInData[0];
	fAllocSize = DBUFFER_BUILTIN_SIZE;
	fAllocdExternal = false;
	fLength = 0;
}

DBuffer::~DBuffer()
{
	if (fAllocdExternal)
		free(fData);
}

/*
void c------------------------------() {}
*/

// append data to the end of the buffer
void DBuffer::AppendData(const uint8_t *data, int length)
{
	if (length <= 0) return;
	EnsureAlloc(fLength + length);
	
	memcpy(&fData[fLength], data, length);
	fLength += length;
}

void DBuffer::AppendBool(bool value)
{
uint8_t ch = (uint8_t)value;
	AppendData((uint8_t *)&ch, 1);
}

void DBuffer::Append16(uint16_t value)
{
	AppendData((uint8_t *)&value, 2);
}

void DBuffer::Append32(uint32_t value)
{
	AppendData((uint8_t *)&value, 4);
}

void DBuffer::Append24(uint32_t value)
{
	Append16(value);
	Append8(value >> 16);
}


/*
void c------------------------------() {}
*/

// return the data contained in the buffer
uint8_t *DBuffer::Data()
{
	return (uint8_t *)fData;
}

// return the data contained in the buffer, and "steal" the pointer from the DBuffer
// so that the caller obtains ownership of it and the DBuffer contents are lost.
// the contents of the DBuffer are undefined after this function returns.
// It is intended for quickly returning C-style pointers to data from functions which use
// DBuffer internally to build the data.
uint8_t *DBuffer::TakeData()
{
	if (!fAllocdExternal)
	{	// we can't give them ownership of the data, because it's still small enough
		// that it's located within our own object. So give them a copy instead.
		uint8_t *copy = (uint8_t *)malloc(fLength);
		memcpy(copy, fData, fLength);
		return copy;
	}
	
	uint8_t *data = fData;			// save our pointer
	fData = NULL;					// now forget it, so it's not freed in the destructor
	fAllocdExternal = false;		// revert to internal data buffer
	
	return data;
}

// return the length of the buffer. note that this will include
// any null-terminators.
int DBuffer::Length()
{
	return fLength;
}

