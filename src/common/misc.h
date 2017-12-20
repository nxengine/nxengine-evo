#ifndef __MISC_H_
#define __MISC_H_
#include <cstdio>
#include <cstdint>
#include <string>

enum Directions
{
        RIGHT   = 0,
        LEFT    = 1,
        UP              = 2,
        DOWN    = 3,
        CENTER  = 5
};


uint16_t fgeti(FILE *fp);
uint32_t fgetl(FILE *fp);
void fputi(uint16_t word, FILE *fp);
void fputl(uint32_t word, FILE *fp);
uint16_t fgeti(FILE *fp);
uint32_t fgetl(FILE *fp);
void fputi(uint16_t word, FILE *fp);
void fputl(uint32_t word, FILE *fp);
double fgetfloat(FILE *fp);
void fputfloat(double q, FILE *fp);
void freadstring(FILE *fp, char *buf, int max);
void fputstring(const char *buf, FILE *fp);
void fputstringnonull(const char *buf, FILE *fp);
bool fverifystring(FILE *fp, const char *str);
void fgetcsv(FILE *fp, char *str, int maxlen);
int fgeticsv(FILE *fp);
double fgetfcsv(FILE *fp);
void fgetline(FILE *fp, char *str, int maxlen);
int filesize(FILE *fp);
bool file_exists(const char *fname);
char *stprintf(const char *fmt, ...);
int random(int min, int max);
void seedrand(uint32_t newseed);
bool strbegin(const char *bigstr, const char *smallstr);
bool strcasebegin(const char *bigstr, const char *smallstr);
int count_string_list(const char *list[]);
char *GetStaticStr(void);
void maxcpy(char *dst, const char *src, int maxlen);
void fresetboolean(void);
char fbooleanread(FILE *fp);
void fbooleanwrite(char bit, FILE *fp);
void fbooleanflush(FILE *fp);


int CVTDir(int csdir);

void crtoslashn(char* in, char* out);
bool contains_non_cr(const std::string& str);

#if defined(_WIN32)
std::wstring widen(const std::string &str);
std::string narrow(const std::wstring &str);
#define myfopen _wfopen
#else
std::string widen(const std::string &str);
std::string narrow(const std::string &str);
#define myfopen fopen
#endif

#endif