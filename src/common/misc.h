#ifndef __MISC_H_
#define __MISC_H_
#include <cstdint>
#include <cstdio>
#include <string>
#include <algorithm>
#include <cctype>
#include <locale>

enum Directions
{
  RIGHT  = 0,
  LEFT   = 1,
  UP     = 2,
  DOWN   = 3,
  CENTER = 5
};

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
uint32_t getrand();
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

void crtoslashn(char *in, char *out);
bool contains_non_cr(const std::string &str);

#if defined(_WIN32)
std::wstring widen(const std::string &str);
std::string narrow(const std::wstring &str);
#define myfopen _wfopen
#else
std::string widen(const std::string &str);
std::string narrow(const std::string &str);
#define myfopen fopen
#endif

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
}

#endif