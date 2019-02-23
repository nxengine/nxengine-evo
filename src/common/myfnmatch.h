#ifndef HEADER_MY_FNMATCH_H
#define HEADER_MY_FNMATCH_H

#define MY_FNMATCH_MATCH    0
#define MY_FNMATCH_NOMATCH  1
#define MY_FNMATCH_FAIL     2

int myfnmatch(const char *pattern, const char *string);

#endif
