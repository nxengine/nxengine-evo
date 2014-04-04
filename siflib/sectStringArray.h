
#ifndef _SECT_STRINGARRAY_H
#define _SECT_STRINGARRAY_H
#include <string>
#include <vector>
class DBuffer;

// the SectStringArray handler decodes sif sections which consist of an array
// of pascal strings. This includes SIF_SECTION_SHEETS and SIF_SECTION_DIRECTORY.

class SIFStringArraySect
{
public:
	static bool Decode(const uint8_t *data, int datalen, std::vector<std::string> *out);
	static uint8_t *Encode(std::vector<std::string> *strings, int *datalen_out);
	
	static void ReadPascalString(const uint8_t **data, const uint8_t *data_end, std::string *out);
	static void WritePascalString(const char *str, DBuffer *out);
};


#endif
