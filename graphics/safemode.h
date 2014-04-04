
#ifndef _SAFEMODE_H
#define _SAFEMODE_H

enum		// special Y positioning arguments for moveto
{
	SM_UPPER_THIRD = -1,
	SM_CENTER = -2,
	SM_LOWER_THIRD = -3,
	SM_NONE = -4,
	SM_MIDUPPER_Y = -5
};

namespace safemode
{
	bool init();
	void close();
	
	void moveto(int y);
	bool print(const char *fmt, ...);
	void clear();
	
	void status(const char *fmt, ...);
	void clearstatus();
	
	int run_until_key(bool delay=true);
};

#endif
