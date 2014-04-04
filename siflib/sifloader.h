
#ifndef _SIFLOADER_H
#define _SIFLOADER_H

#include <vector>
#include <stdint.h>

#define SIF_MAX_GROUPS			255		// limitation of SECTION_GROUPS format

/*
	the .sif file format is designed to hold a number of different sprite-related data
	and to be extensible while still being able to read older versions of the files.
	
	the .sif file is essentially a container format, with individual subclasses to decode
	each specific section type.
	
	sections can then contain additional subsections.
	
	the main section, SIF_SECTION_SPRITES, is an array of sprites. For each sprite entry,
	data typecodes denote a variable in the SIFSprite structure and it's value. Any values
	not mentioned are set to default values when loaded. Values set to defaults are not saved.

*/

enum
{
	SIF_SECTION_SESSION,				// holds SIFEdit session info such as last sprite edited
	SIF_SECTION_SHEETS,					// filenames of spritesheets used by sprites
	SIF_SECTION_SPRITES,				// main sprite array
	SIF_SECTION_NAMES,					// names of sprites in array, minus the SPR_... prefix
	SIF_SECTION_GROUPS,					// names of SIFEdit directories for grouping sprites
	SIF_SECTION_COMMENTS,				// SIFEdit comments about sprites
	SIF_SECTION_PATHS,					// things like base directory of sheets, for SIFEdit
	SIF_SECTION_DIRNAMES,				// names/order of the SIFDir directions
	
	SIF_SECTION_COUNT
};


// section entries in the header index table
struct SIFIndexEntry
{
	uint8_t type;			// section typecode (SIF_SECTION_...)
	uint32_t foffset;		// offset within file
	uint32_t length;		// length of section data
	uint8_t *data;			// the actual data, if it has already been loaded, else NULL
};


class SIFLoader
{
public:
	SIFLoader();
	~SIFLoader();
	
	// open a file handle to the given .sif and load the header and
	// section index into memory.
	bool LoadHeader(const char *filename);
	
	// return a pointer to the section data of type 'type',
	// or NULL if the file doesn't have a section of that type.
	uint8_t *FindSection(int type, int *length_out);

//	---------------------------------------
//	SIF-saving functions. You shouldn't use the same object for saving
//	as currently has a file loaded, just construct a new one.
//  But it is safe to save over the same file you are referencing data
//  from using the "Load" object; because the changes are finalized to disk
//  until you call EndSave().

	// allocate memory etc to save a new file from provided data.
	bool BeginSave();
	
	// add a section to the SIF of the given type, containing the given data.
	bool AddSection(int type, uint8_t *data, int datalen);
	
	// finalize the changes and write them to disk in the given file.
	bool EndSave(const char *filename);
	
//	---------------------------------------

	// free any temporary memory and close the file handle.
	void CloseFile();
	
	
private:
	void ClearIndex();
	
	std::vector<SIFIndexEntry*> fIndex;			// index table from header (list of SIFIndexEntry)
	FILE *fFP;				// open file handle
	
	uint32_t fTotalDataAdded;	// for saving
};


#endif

