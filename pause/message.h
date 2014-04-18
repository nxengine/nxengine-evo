
#ifndef _MESSAGE_H
#define _MESSAGE_H

#include "options.h"
#include "../input.h"
namespace Options {

class Message : public FocusHolder
{
public:
	Message(const char *msg, const char *msg2 = NULL);
	~Message();
	
	void Draw();
	void RunInput();
	
	in_action *rawKeyReturn;
	void (*on_dismiss)(Message *msg);
	
private:
	char *fMsg, *fMsg2;
	int fMsgX, fMsgY;
	int fMsg2X, fMsg2Y;
	
	int fShowDelay;
};


}

#endif
