
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/DBuffer.h"
#include "../common/bufio.h"
#include "../common/stat.h"

#include "sectStringArray.h"


// decode the raw section data into the given tringList object
bool SIFStringArraySect::Decode(const uint8_t *data, int datalen, std::vector<std::string> *out)
{
const uint8_t *data_end = data + (datalen - 1);
int i, nstrings;

	nstrings = read_U16(&data, data_end);
	for(i=0;i<nstrings;i++)
	{
		if (data > data_end)
		{
			staterr("SIFStringArraySect::Decode: section corrupt: overran end of data");
			return 1;
		}
		
		std::string string;
		ReadPascalString(&data, data_end, &string);
		
		out->push_back(std::string(string.c_str()));
	}
	
	return 0;
}



uint8_t *SIFStringArraySect::Encode(std::vector<std::string> *strings, int *datalen_out)
{
DBuffer buf;

	if (strings->size() > 65535)
	{
		staterr("SIFStringArraySect::Encode: too many strings in list");
		return NULL;
	}
	
	buf.Append16(strings->size());
	for(int i=0;;i++)
	{
		const char *str = strings->at(i).c_str();
		if (!str) break;
		
		WritePascalString(str, &buf);
	}
	
	if (datalen_out) *datalen_out = buf.Length();
	return buf.TakeData();
}

/*
void c------------------------------() {}
*/

void SIFStringArraySect::ReadPascalString(const uint8_t **data, const uint8_t *data_end, std::string *out)
{
	int len = read_U8(data, data_end);
	if (len == 255) len = read_U16(data, data_end);
	
	out->clear();
	for(int i=0;i<len;i++)
	{
		*out+=read_U8(data, data_end);
	}
}

void SIFStringArraySect::WritePascalString(const char *str, DBuffer *out)
{
	int len = strlen(str);
	if (len > 65535) len = 65535;
	
	if (len < 255)
	{
		out->Append8(len);
	}
	else
	{
		out->Append8(0xff);
		out->Append16(len);
	}
	
	for(int j=0;j<len;j++)
		out->Append8(str[j]);
}




