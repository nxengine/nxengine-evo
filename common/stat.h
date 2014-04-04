#ifndef __STAT_H_
#define __STAT_H_


void SetLogFilename(const char *fname);
void writelog(const char *buf, bool append_cr);
void stat(const char *fmt, ...);
void staterr(const char *fmt, ...);


#endif