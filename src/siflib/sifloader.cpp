
#include "sifloader.h"

#include "../common/misc.h"
#include "../Utils/Logger.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#define SIF_MAGICK (('S' << 24) + ('I' << 16) + ('F' << 8) + '2')

SIFLoader::SIFLoader()
{
  fFP = NULL;
}

SIFLoader::~SIFLoader()
{
  ClearIndex();
  if (fFP)
    fclose(fFP);
}

/*
void c------------------------------() {}
*/

void SIFLoader::ClearIndex()
{
  for (unsigned int i = 0; i < fIndex.size(); i++)
  {
    SIFIndexEntry *entry = (SIFIndexEntry *)fIndex.at(i);

    if (entry->data)
      free(entry->data);
    delete entry;
  }

  fIndex.clear();
}

void SIFLoader::CloseFile()
{
  ClearIndex();

  if (fFP)
  {
    fclose(fFP);
    fFP = NULL;
  }
}

/*
void c------------------------------() {}
*/

bool SIFLoader::LoadHeader(const std::string &filename)
{
  FILE *fp;
  uint32_t magick;

  ClearIndex();

  if (fFP)
    fclose(fFP);
  fp = fFP = myfopen(widen(filename).c_str(), widen("rb").c_str());

  if (!fp)
  {
    LOG_ERROR("SIFLoader::LoadHeader: failed to open file '{}'", filename);
    return 1;
  }

  if ((magick = fgetl(fp)) != SIF_MAGICK)
  {
    LOG_ERROR("SIFLoader::LoadHeader: magick check failed--this isn't a SIF file or is wrong version?");
    LOG_ERROR(" (expected {:#08x}, got {:#08x})", SIF_MAGICK, magick);
    return 1;
  }

  int nsections = fgetc(fp);
  LOG_DEBUG("SIFLoader::LoadHeader: read index of {} sections", nsections);

  for (int i = 0; i < nsections; i++)
  {
    SIFIndexEntry *entry = new SIFIndexEntry;

    entry->type    = fgetc(fp); // section type
    entry->foffset = fgetl(fp); // absolute offset in file
    entry->length  = fgetl(fp); // length of section data
    entry->data    = NULL;      // we won't load it until asked

    fIndex.push_back(entry);
    // stat("  - Sect%02d @ %04x", entry->type, entry->foffset);
  }

  // ..leave file handle open, its ok
  return 0;
}

// load into memory and return a pointer to the section of type 'type',
// or NULL if the file doesn't have a section of that type.
uint8_t *SIFLoader::FindSection(int type, int *length_out)
{
  // try and find the section in the index
  for (unsigned int i = 0; i < fIndex.size(); i++)
  {
    SIFIndexEntry *entry = (SIFIndexEntry *)fIndex.at(i);
    //		if (!entry) break;

    if (entry->type == type)
    { // got it!

      // haven't loaded it yet? need to fetch it from file?
      if (!entry->data)
      {
        if (!fFP)
        {
          LOG_ERROR("SIFLoader::FindSection: entry found and need to load it, but file handle closed");
          if (length_out)
            *length_out = 0;
          return NULL;
        }

        LOG_DEBUG("Loading SIF section {} from address {:#04x}", type, entry->foffset);

        entry->data = (uint8_t *)malloc(entry->length);
        fseek(fFP, entry->foffset, SEEK_SET);
        fread(entry->data, entry->length, 1, fFP);
      }

      if (length_out)
        *length_out = entry->length;
      return entry->data;
    }
  }

  if (length_out)
    *length_out = 0;
  return NULL;
}
