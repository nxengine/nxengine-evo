
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "basics.h"
#include "stat.h"
#include "misc.h"

#define MAXBUFSIZE		1024
char logfilename[64] = { 0 };
void writelog(const char *buf, bool append_cr);


void SetLogFilename(const char *fname)
{
	maxcpy(logfilename, fname, sizeof(logfilename));
	remove(logfilename);
}

void writelog(const char *buf, bool append_cr)
{
FILE *fp;

	fp = fopen(widen(logfilename).c_str(), "a+");
	if (fp)
	{
		fputs(buf, fp);
		if (append_cr) fputc('\n', fp);
		
		fclose(fp);
	}
}

/*
void c------------------------------() {}
*/

void stat(const char *fmt, ...)
{
va_list ar;
char buffer[MAXBUFSIZE];

	va_start(ar, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ar);
	va_end(ar);
	
	puts(buffer);
	fflush(stdout);
	
	if (logfilename[0])
		writelog(buffer, true);
}

void staterr(const char *fmt, ...)
{
va_list ar;
char buffer[MAXBUFSIZE];

	va_start(ar, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ar);
	va_end(ar);
	
	printf(" error << %s >> \n", buffer);
	fflush(stdout);
	
	if (logfilename[0])
	{
		writelog(" error << ", false);
		writelog(buffer, false);
		writelog(" >>\n", false);
	}
}


