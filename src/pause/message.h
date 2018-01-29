
#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <string>
#include "options.h"
#include "../input.h"
namespace Options {

class Message : public FocusHolder
{
public:
	Message(const std::string& msg, const std::string& msg2 = "");
	~Message();
	
	void Draw();
	void RunInput();
	
	in_action *rawKeyReturn;
	void (*on_dismiss)(Message *msg);
	
private:
	std::string fMsg, fMsg2;
	int fMsgX, fMsgY;
	int fMsg2X, fMsg2Y;
	
	int fShowDelay;
	int MESSAGE_X, MESSAGE_Y;
};


}

#endif
