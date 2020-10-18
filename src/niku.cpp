
#include "niku.h"

#include "common/misc.h"
#include "Utils/Logger.h"
#include "ResourceManager.h"

#include <SDL.h>
#include <cstdio>
#include <string>

/*
        290.rec contains the tick value 4 times, followed by the 4 byte key
        to decrypt each instance, for a total of 20 bytes.
*/

// load the contents of 290.rec and store in value_out. Returns 0 on success.
// If there is no such file or an error occurs, writes 0 to value_out.
uint32_t niku_load()
{
  FILE *fp;
  uint8_t buffer[20];
  uint32_t *result = (uint32_t *)buffer;
  int i, j;

  std::string fname = ResourceManager::getInstance()->getPrefPath("290.rec");

  fp = myfopen(widen(fname).c_str(), widen("rb").c_str());
  if (!fp)
  {
    LOG_DEBUG("niku_load: couldn't open file '{}'", fname);
    return 0xFFFFFFFF;
  }

  fread(buffer, 20, 1, fp);
  fclose(fp);

  for (i = 0; i < 4; i++)
  {
    uint8_t key = buffer[i + 16];

    j = i * 4;
    buffer[j] -= key;
    buffer[j + 1] -= key;
    buffer[j + 2] -= key;
    buffer[j + 3] -= (key / 2);
  }

  if ((result[0] != result[1]) || (result[0] != result[2]) || (result[0] != result[3]))
  {
    LOG_ERROR("niku_load: value mismatch; '{}' corrupt", fname);
    return 0xFFFFFFFF;
  }
  else
  {
    LOG_DEBUG("niku_load: loaded value {:#08x} from {}", *result, fname);
    return *result;
  }
  return 0xFFFFFFFF;
}

// save the timestamp in value to 290.rec.
bool niku_save(uint32_t value)
{
  uint32_t old_time = niku_load();
  if (old_time < value)
  {
    return false;
  }

  uint8_t buf_byte[20];
  uint32_t *buf_dword = (uint32_t *)buf_byte;

  std::string fname = ResourceManager::getInstance()->getPrefPath("290.rec");

  // encode each copy
  for (int i = 0; i < 4; i++)
  {
    buf_dword[i] = value;
    buf_byte[i+16] = random(0, 250) + i;
    uint8_t p[4];

    p[0] = (uint8_t)(buf_dword[i] + buf_byte[i+16]);
    p[1] = (uint8_t)((buf_dword[i] >> 8) + buf_byte[i+16]);
    p[2] = (uint8_t)((buf_dword[i] >> 16) + buf_byte[i+16]);
    p[3] = (uint8_t)((buf_dword[i] >> 24) + buf_byte[i+16] / 2);

    buf_dword[i] = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
  }

  FILE *fp = myfopen(widen(fname).c_str(), widen("wb").c_str());
  if (!fp)
  {
    LOG_ERROR("niku_save: failed to open '{}'", fname);
    return false;
  }

  fwrite(buf_byte, 20, 1, fp);
  fclose(fp);

  LOG_DEBUG("niku_save: wrote value {:#08x}", value);
  return true;
}
