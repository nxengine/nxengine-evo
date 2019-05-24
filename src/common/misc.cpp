
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <SDL.h>
#if defined(_WIN32)
#include <windows.h>
#endif

#include "basics.h"
#include "misc.h"
#include "stat.h"
//#include "../game.h"

void stat(const char *fmt, ...);

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
uint16_t fgeti(FILE *fp)
{
  uint16_t value;
  fread(&value, 2, 1, fp);
  return value;
}

uint32_t fgetl(FILE *fp)
{
  uint32_t value;
  fread(&value, 4, 1, fp);
  return value;
}

void fputi(uint16_t word, FILE *fp)
{
  fwrite(&word, 2, 1, fp);
}

void fputl(uint32_t word, FILE *fp)
{
  fwrite(&word, 4, 1, fp);
}
#else
uint16_t fgeti(FILE *fp)
{
  uint16_t a, b;
  a = fgetc(fp);
  b = fgetc(fp);
  return (b << 8) | a;
}

uint32_t fgetl(FILE *fp)
{
  uint32_t a, b, c, d;
  a = fgetc(fp);
  b = fgetc(fp);
  c = fgetc(fp);
  d = fgetc(fp);
  return (d << 24) | (c << 16) | (b << 8) | (a);
}

void fputi(uint16_t word, FILE *fp)
{
  fputc(word, fp);
  fputc(word >> 8, fp);
}

void fputl(uint32_t word, FILE *fp)
{
  fputc(word, fp);
  fputc(word >> 8, fp);
  fputc(word >> 16, fp);
  fputc(word >> 24, fp);
}
#endif // __BYTE_ORDER  == __LITTLE_ENDIAN

double fgetfloat(FILE *fp)
{
  char buf[16];
  double *float_ptr;
  int i;

  for (i = 0; i < 4; i++)
    fgetc(fp);
  for (i = 0; i < 8; i++)
    buf[i] = fgetc(fp);

  float_ptr = (double *)&buf[0];
  return *float_ptr;
}

void fputfloat(double q, FILE *fp)
{
  char *float_ptr;
  int i;

  float_ptr = (char *)&q;

  for (i = 0; i < 4; i++)
    fputc(0, fp);
  for (i = 0; i < 8; i++)
    fputc(float_ptr[i], fp);

  return;
}

// read a string from a file until a null is encountered
void freadstring(FILE *fp, char *buf, int max)
{
  int i;

  --max;
  for (i = 0; i < max; i++)
  {
    buf[i] = fgetc(fp);
    if (!buf[i])
    {
      return;
    }
  }

  buf[i] = 0;
}

// write a string to a file and null-terminate it
void fputstring(const char *buf, FILE *fp)
{
  if (buf[0])
    fprintf(fp, "%s", buf);
  fputc(0, fp);
}

// write a string to a file-- does NOT null-terminate it
void fputstringnonull(const char *buf, FILE *fp)
{
  if (buf[0])
    fprintf(fp, "%s", buf);
}

// reads strlen(str) bytes from file fp, and returns true if they match "str"
bool fverifystring(FILE *fp, const char *str)
{
  int i;
  bool result      = true;
  int stringlength = strlen(str);

  for (i = 0; i < stringlength; i++)
  {
    if (fgetc(fp) != str[i])
      result = false;
  }

  return result;
}

// read data from a file until ',' or CR
void fgetcsv(FILE *fp, char *str, int maxlen)
{
  int i, j;
  char ch;

  maxlen--;
  for (i = j = 0; i < maxlen; i++)
  {
    ch = fgetc(fp);

    if (ch == 13 || ch == ',' || ch == '}' || ch == -1)
    {
      break;
    }

    if (ch != 10)
    {
      str[j++] = ch;
    }
  }

  str[j] = 0;
}

// read a number from a CSV'd list in a file
int fgeticsv(FILE *fp)
{
  char buffer[80];
  fgetcsv(fp, buffer, sizeof(buffer));
  return atoi(buffer);
}

double fgetfcsv(FILE *fp)
{
  char buffer[80];
  fgetcsv(fp, buffer, sizeof(buffer));
  return atof(buffer);
}

// read data from a file until CR
void fgetline(FILE *fp, char *str, int maxlen)
{
  int k;
  str[0] = 0;
  fgets(str, maxlen - 1, fp);

  // trim the CRLF that fgets appends
  for (k = strlen(str) - 1; k >= 0; k--)
  {
    if (str[k] != 13 && str[k] != 10)
      break;
    str[k] = 0;
  }
}

int filesize(FILE *fp)
{
  int cp, sz;

  cp = ftell(fp);
  fseek(fp, 0, SEEK_END);
  sz = ftell(fp);
  fseek(fp, cp, SEEK_SET);

  return sz;
}

bool file_exists(const char *fname)
{
  FILE *fp;

  fp = myfopen(widen(fname).c_str(), widen("rb").c_str());
  if (!fp)
    return 0;
  fclose(fp);
  return 1;
}

char *stprintf(const char *fmt, ...)
{
  va_list ar;
  char *str = GetStaticStr();

  va_start(ar, fmt);
  vsnprintf(str, 255, fmt, ar);
  va_end(ar);

  return str;
}

/*
void c------------------------------() {}
*/

static uint32_t seed = 0;

// return a random number between min and max inclusive
int random(int min, int max)
{
  int range, val;

  if (max < min)
  {
    staterr("random(): warning: max < min [%d, %d]", min, max);
    min ^= max;
    max ^= min;
    min ^= max;
  }

  range = (max - min);

  if (range >= RAND_MAX)
  {
    staterr("random(): range > RAND_MAX", min, max);
    return 0;
  }

  val = getrand() % (range + 1);
  return val + min;
}

uint32_t getrand()
{
  seed = seed * 214013L + 2531011L;
  return (uint32_t)(seed / 65536) % 32768;
}

void seedrand(uint32_t newseed)
{
  seed = newseed;
}

/*
void c------------------------------() {}
*/

bool strbegin(const char *bigstr, const char *smallstr)
{
  int i;

  for (i = 0; smallstr[i]; i++)
    if (bigstr[i] != smallstr[i])
      return false;

  return true;
}

bool strcasebegin(const char *bigstr, const char *smallstr)
{
  int i;

  for (i = 0; smallstr[i]; i++)
    if (toupper(bigstr[i]) != toupper(smallstr[i]))
      return false;

  return true;
}

// returns how many strings are in a null-terminated array of C strings
int count_string_list(const char *list[])
{
  int i;
  for (i = 0; list[i]; i++)
    ;
  return i;
}

char *GetStaticStr(void)
{
  static int counter = 0;
  static struct
  {
    char str[1024];
  } bufs[24];

  if (++counter >= 24)
    counter = 0;
  return bufs[counter].str;
}

// a strncpy that works as you might expect
void maxcpy(char *dst, const char *src, int maxlen)
{
  int len = strlen(src);

  if (len >= maxlen)
  {
    if (maxlen >= 2)
      memcpy(dst, src, maxlen - 2);
    if (maxlen >= 1)
      dst[maxlen - 1] = 0;
  }
  else
  {
    memcpy(dst, src, len + 1);
  }
}

/*
void c------------------------------() {}
*/

static int boolbyte, boolmask_r, boolmask_w;

// prepare for a boolean read operation
void fresetboolean(void)
{
  boolmask_r = 256;
  boolmask_w = 1;
  boolbyte   = 0;
}

// read a boolean value (a single bit) from a file
char fbooleanread(FILE *fp)
{
  char value;

  if (boolmask_r == 256)
  {
    boolbyte   = fgetc(fp);
    boolmask_r = 1;
  }

  value = (boolbyte & boolmask_r) ? 1 : 0;
  boolmask_r <<= 1;
  return value;
}

void fbooleanwrite(char bit, FILE *fp)
{
  if (boolmask_w == 256)
  {
    fputc(boolbyte, fp);
    boolmask_w = 1;
    boolbyte   = 0;
  }

  if (bit)
  {
    boolbyte |= boolmask_w;
  }

  boolmask_w <<= 1;
}

void fbooleanflush(FILE *fp)
{
  fputc(boolbyte, fp);
  boolmask_w = 1;
}

// delimit real newlines in 'in' to "\n"'s.
void crtoslashn(char *in, char *out)
{
  int i, j;

  for (i = j = 0; in[i]; i++)
  {
    if (in[i] == 13)
    {
      out[j++] = '\\';
      out[j++] = 'n';
    }
    else
    {
      out[j++] = in[i];
    }
  }

  out[j] = 0;
}

bool contains_non_cr(const std::string &str)
{
  for (int i = 0; str[i]; i++)
  {
    if (str[i] != '\r' && str[i] != '\n')
      return true;
  }

  return false;
}

int CVTDir(int csdir)
{
  const int cdir_to_nxdir[4] = {LEFT, UP, RIGHT, DOWN};

  if (csdir >= 0 && csdir < 4)
    return cdir_to_nxdir[csdir];

  staterr("CVTDir: invalid direction %d, returning LEFT", csdir);
  return LEFT;
}

#if defined(_WIN32)
std::wstring widen(const std::string &str)
{
  // Convert an ASCII string to a Unicode String
  std::wstring wstrTo;
  wchar_t *wszTo    = new wchar_t[str.length() + 1];
  wszTo[str.size()] = L'\0';
  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wszTo, (int)str.length());
  wstrTo = wszTo;
  delete[] wszTo;
  return wstrTo;
}
std::string narrow(const std::wstring &str)
{
  // Convert an ASCII string to a Unicode String
  std::string wstrTo;
  char *wszTo       = new char[str.length() + 1];
  wszTo[str.size()] = L'\0';
  WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, wszTo, (int)str.length(), NULL, NULL);
  wstrTo = wszTo;
  delete[] wszTo;
  return wstrTo;
}
#else
std::string widen(const std::string &str)
{
  return str;
}
std::string narrow(const std::string &str)
{
  return str;
}
#endif
