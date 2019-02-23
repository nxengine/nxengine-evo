#include "myfnmatch.h"
#include <cstring>
#include <cctype>
#define TRUE 1
#define FALSE 0
#define ISSPACE(x)  (isspace((int)  ((unsigned char)x)))
#define ISDIGIT(x)  (isdigit((int)  ((unsigned char)x)))
#define ISALNUM(x)  (isalnum((int)  ((unsigned char)x)))
#define ISXDIGIT(x) (isxdigit((int) ((unsigned char)x)))
#define ISGRAPH(x)  (isgraph((int)  ((unsigned char)x)))
#define ISALPHA(x)  (isalpha((int)  ((unsigned char)x)))
#define ISPRINT(x)  (isprint((int)  ((unsigned char)x)))
#define ISUPPER(x)  (isupper((int)  ((unsigned char)x)))
#define ISLOWER(x)  (islower((int)  ((unsigned char)x)))
#define ISASCII(x)  (isascii((int)  ((unsigned char)x)))

#define ISBLANK(x)  (int)((((unsigned char)x) == ' ') || \
                          (((unsigned char)x) == '\t'))

#define TOLOWER(x)  (tolower((int)  ((unsigned char)x)))

#define MYFNM_CHARSET_LEN (sizeof(char) * 256)
#define MYFNM_CHSET_SIZE (MYFNM_CHARSET_LEN + 15)

#define MYFNM_NEGATE  MYFNM_CHARSET_LEN

#define MYFNM_ALNUM   (MYFNM_CHARSET_LEN + 1)
#define MYFNM_DIGIT   (MYFNM_CHARSET_LEN + 2)
#define MYFNM_XDIGIT  (MYFNM_CHARSET_LEN + 3)
#define MYFNM_ALPHA   (MYFNM_CHARSET_LEN + 4)
#define MYFNM_PRINT   (MYFNM_CHARSET_LEN + 5)
#define MYFNM_BLANK   (MYFNM_CHARSET_LEN + 6)
#define MYFNM_LOWER   (MYFNM_CHARSET_LEN + 7)
#define MYFNM_GRAPH   (MYFNM_CHARSET_LEN + 8)
#define MYFNM_SPACE   (MYFNM_CHARSET_LEN + 9)
#define MYFNM_UPPER   (MYFNM_CHARSET_LEN + 10)

typedef enum {
  MYFNM_LOOP_DEFAULT = 0,
  MYFNM_LOOP_BACKSLASH
} loop_state;

typedef enum {
  MYFNM_SCHS_DEFAULT = 0,
  MYFNM_SCHS_MAYRANGE,
  MYFNM_SCHS_MAYRANGE2,
  MYFNM_SCHS_RIGHTBR,
  MYFNM_SCHS_RIGHTBRLEFTBR
} setcharset_state;

typedef enum {
  MYFNM_PKW_INIT = 0,
  MYFNM_PKW_DDOT
} parsekey_state;

#define SETCHARSET_OK     1
#define SETCHARSET_FAIL   0

static int parsekeyword(unsigned char **pattern, unsigned char *charset)
{
  parsekey_state state = MYFNM_PKW_INIT;
#define KEYLEN 10
  char keyword[KEYLEN] = { 0 };
  int found = FALSE;
  int i;
  unsigned char *p = *pattern;
  for(i = 0; !found; i++) {
    char c = *p++;
    if(i >= KEYLEN)
      return SETCHARSET_FAIL;
    switch(state) {
    case MYFNM_PKW_INIT:
      if(ISALPHA(c) && ISLOWER(c))
        keyword[i] = c;
      else if(c == ':')
        state = MYFNM_PKW_DDOT;
      else
        return 0;
      break;
    case MYFNM_PKW_DDOT:
      if(c == ']')
        found = TRUE;
      else
        return SETCHARSET_FAIL;
    }
  }
#undef KEYLEN

  *pattern = p; /* move caller's pattern pointer */
  if(strcmp(keyword, "digit") == 0)
    charset[MYFNM_DIGIT] = 1;
  else if(strcmp(keyword, "alnum") == 0)
    charset[MYFNM_ALNUM] = 1;
  else if(strcmp(keyword, "alpha") == 0)
    charset[MYFNM_ALPHA] = 1;
  else if(strcmp(keyword, "xdigit") == 0)
    charset[MYFNM_XDIGIT] = 1;
  else if(strcmp(keyword, "print") == 0)
    charset[MYFNM_PRINT] = 1;
  else if(strcmp(keyword, "graph") == 0)
    charset[MYFNM_GRAPH] = 1;
  else if(strcmp(keyword, "space") == 0)
    charset[MYFNM_SPACE] = 1;
  else if(strcmp(keyword, "blank") == 0)
    charset[MYFNM_BLANK] = 1;
  else if(strcmp(keyword, "upper") == 0)
    charset[MYFNM_UPPER] = 1;
  else if(strcmp(keyword, "lower") == 0)
    charset[MYFNM_LOWER] = 1;
  else
    return SETCHARSET_FAIL;
  return SETCHARSET_OK;
}

/* returns 1 (true) if pattern is OK, 0 if is bad ("p" is pattern pointer) */
static int setcharset(unsigned char **p, unsigned char *charset)
{
  setcharset_state state = MYFNM_SCHS_DEFAULT;
  unsigned char rangestart = 0;
  unsigned char lastchar   = 0;
  int something_found = FALSE;
  unsigned char c;
  for(;;) {
    c = **p;
    switch(state) {
    case MYFNM_SCHS_DEFAULT:
      if(ISALNUM(c)) { /* ASCII value */
        rangestart = c;
        charset[c] = 1;
        (*p)++;
        state = MYFNM_SCHS_MAYRANGE;
        something_found = TRUE;
      }
      else if(c == ']') {
        if(something_found)
          return SETCHARSET_OK;
        else
          something_found = TRUE;
        state = MYFNM_SCHS_RIGHTBR;
        charset[c] = 1;
        (*p)++;
      }
      else if(c == '[') {
        char c2 = *((*p)+1);
        if(c2 == ':') { /* there has to be a keyword */
          (*p) += 2;
          if(parsekeyword(p, charset)) {
            state = MYFNM_SCHS_DEFAULT;
          }
          else
            return SETCHARSET_FAIL;
        }
        else {
          charset[c] = 1;
          (*p)++;
        }
        something_found = TRUE;
      }
      else if(c == '?' || c == '*') {
        something_found = TRUE;
        charset[c] = 1;
        (*p)++;
      }
      else if(c == '^' || c == '!') {
        if(!something_found) {
          if(charset[MYFNM_NEGATE]) {
            charset[c] = 1;
            something_found = TRUE;
          }
          else
            charset[MYFNM_NEGATE] = 1; /* negate charset */
        }
        else
          charset[c] = 1;
        (*p)++;
      }
      else if(c == '\\') {
        c = *(++(*p));
        if(ISPRINT((c))) {
          something_found = TRUE;
          state = MYFNM_SCHS_MAYRANGE;
          charset[c] = 1;
          rangestart = c;
          (*p)++;
        }
        else
          return SETCHARSET_FAIL;
      }
      else if(c == '\0') {
        return SETCHARSET_FAIL;
      }
      else {
        charset[c] = 1;
        (*p)++;
        something_found = TRUE;
      }
      break;
    case MYFNM_SCHS_MAYRANGE:
      if(c == '-') {
        charset[c] = 1;
        (*p)++;
        lastchar = '-';
        state = MYFNM_SCHS_MAYRANGE2;
      }
      else if(c == '[') {
        state = MYFNM_SCHS_DEFAULT;
      }
      else if(ISALNUM(c)) {
        charset[c] = 1;
        (*p)++;
      }
      else if(c == '\\') {
        c = *(++(*p));
        if(ISPRINT(c)) {
          charset[c] = 1;
          (*p)++;
        }
        else
          return SETCHARSET_FAIL;
      }
      else if(c == ']') {
        return SETCHARSET_OK;
      }
      else
        return SETCHARSET_FAIL;
      break;
    case MYFNM_SCHS_MAYRANGE2:
      if(c == '\\') {
        c = *(++(*p));
        if(!ISPRINT(c))
          return SETCHARSET_FAIL;
      }
      if(c == ']') {
        return SETCHARSET_OK;
      }
      else if(c == '\\') {
        c = *(++(*p));
        if(ISPRINT(c)) {
          charset[c] = 1;
          state = MYFNM_SCHS_DEFAULT;
          (*p)++;
        }
        else
          return SETCHARSET_FAIL;
      }
      if(c >= rangestart) {
        if((ISLOWER(c) && ISLOWER(rangestart)) ||
           (ISDIGIT(c) && ISDIGIT(rangestart)) ||
           (ISUPPER(c) && ISUPPER(rangestart))) {
          charset[lastchar] = 0;
          rangestart++;
          while(rangestart++ <= c)
            charset[rangestart-1] = 1;
          (*p)++;
          state = MYFNM_SCHS_DEFAULT;
        }
        else
          return SETCHARSET_FAIL;
      }
      break;
    case MYFNM_SCHS_RIGHTBR:
      if(c == '[') {
        state = MYFNM_SCHS_RIGHTBRLEFTBR;
        charset[c] = 1;
        (*p)++;
      }
      else if(c == ']') {
        return SETCHARSET_OK;
      }
      else if(c == '\0') {
        return SETCHARSET_FAIL;
      }
      else if(ISPRINT(c)) {
        charset[c] = 1;
        (*p)++;
        state = MYFNM_SCHS_DEFAULT;
      }
      else
        /* used 'goto fail' instead of 'return SETCHARSET_FAIL' to avoid a
         * nonsense warning 'statement not reached' at end of the fnc when
         * compiling on Solaris */
        goto fail;
      break;
    case MYFNM_SCHS_RIGHTBRLEFTBR:
      if(c == ']') {
        return SETCHARSET_OK;
      }
      else {
        state  = MYFNM_SCHS_DEFAULT;
        charset[c] = 1;
        (*p)++;
      }
      break;
    }
  }
fail:
  return SETCHARSET_FAIL;
}

static int loop(const unsigned char *pattern, const unsigned char *string)
{
  loop_state state = MYFNM_LOOP_DEFAULT;
  unsigned char *p = (unsigned char *)pattern;
  unsigned char *s = (unsigned char *)string;
  unsigned char charset[MYFNM_CHSET_SIZE] = { 0 };
  int rc = 0;

  for(;;) {
    switch(state) {
    case MYFNM_LOOP_DEFAULT:
      if(*p == '*') {
        while(*(p+1) == '*') /* eliminate multiple stars */
          p++;
        if(*s == '\0' && *(p+1) == '\0')
          return MY_FNMATCH_MATCH;
        rc = loop(p + 1, s); /* *.txt matches .txt <=> .txt matches .txt */
        if(rc == MY_FNMATCH_MATCH)
          return MY_FNMATCH_MATCH;
        if(*s) /* let the star eat up one character */
          s++;
        else
          return MY_FNMATCH_NOMATCH;
      }
      else if(*p == '?') {
        if(ISPRINT(*s)) {
          s++;
          p++;
        }
        else if(*s == '\0')
          return MY_FNMATCH_NOMATCH;
        else
          return MY_FNMATCH_FAIL; /* cannot deal with other character */
      }
      else if(*p == '\0') {
        if(*s == '\0')
          return MY_FNMATCH_MATCH;
        else
          return MY_FNMATCH_NOMATCH;
      }
      else if(*p == '\\') {
        state = MYFNM_LOOP_BACKSLASH;
        p++;
      }
      else if(*p == '[') {
        unsigned char *pp = p+1; /* cannot handle with pointer to register */
        if(setcharset(&pp, charset)) {
          int found = FALSE;
          if(charset[(unsigned int)*s])
            found = TRUE;
          else if(charset[MYFNM_ALNUM])
            found = ISALNUM(*s);
          else if(charset[MYFNM_ALPHA])
            found = ISALPHA(*s);
          else if(charset[MYFNM_DIGIT])
            found = ISDIGIT(*s);
          else if(charset[MYFNM_XDIGIT])
            found = ISXDIGIT(*s);
          else if(charset[MYFNM_PRINT])
            found = ISPRINT(*s);
          else if(charset[MYFNM_SPACE])
            found = ISSPACE(*s);
          else if(charset[MYFNM_UPPER])
            found = ISUPPER(*s);
          else if(charset[MYFNM_LOWER])
            found = ISLOWER(*s);
          else if(charset[MYFNM_BLANK])
            found = ISBLANK(*s);
          else if(charset[MYFNM_GRAPH])
            found = ISGRAPH(*s);

          if(charset[MYFNM_NEGATE])
            found = !found;

          if(found) {
            p = pp+1;
            s++;
            memset(charset, 0, MYFNM_CHSET_SIZE);
          }
          else
            return MY_FNMATCH_NOMATCH;
        }
        else
          return MY_FNMATCH_FAIL;
      }
      else {
        if(*p++ != *s++)
          return MY_FNMATCH_NOMATCH;
      }
      break;
    case MYFNM_LOOP_BACKSLASH:
      if(ISPRINT(*p)) {
        if(*p++ == *s++)
          state = MYFNM_LOOP_DEFAULT;
        else
          return MY_FNMATCH_NOMATCH;
      }
      else
        return MY_FNMATCH_FAIL;
      break;
    }
  }
}

/*
 * @unittest: 1307
 */
int myfnmatch(const char *pattern, const char *string)
{
  if(!pattern || !string) {
    return MY_FNMATCH_FAIL;
  }
  return loop((unsigned char *)pattern, (unsigned char *)string);
}
