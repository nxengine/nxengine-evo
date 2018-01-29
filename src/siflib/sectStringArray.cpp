#include <cstdio>
#include <cstdlib>
#include <cstring>

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

