
#include "../nx.h"
#include "message.h"
#include "../graphics/font.h"
#include <vector>

#include "../graphics/graphics.h"
using namespace Graphics;
#include "../TextBox/TextBox.h"
#include "../input.h"
#include "../game.h"


using namespace Options;
extern std::vector<void*> optionstack;

#define MESSAGE_W		244
#define MESSAGE_H		48

Message::Message(const std::string& msg, const std::string& msg2)
{
    MESSAGE_X = ((SCREEN_WIDTH / 2) - 112);
    MESSAGE_Y = ((SCREEN_HEIGHT / 2) - 30);

	rawKeyReturn = NULL;
	on_dismiss = NULL;
	last_sdl_action.key = -1;
	last_sdl_action.jbut = -1;
	last_sdl_action.jhat = -1;
	last_sdl_action.jaxis = -1;
	
	fMsg = msg;
	fMsg2 = msg2;
	
	fMsgX = MESSAGE_X + ((MESSAGE_W / 2) - (GetFontWidth(fMsg) / 2));
	
	if (fMsg2[0])
	{
		fMsgY = MESSAGE_Y + 10;
		fMsg2X = MESSAGE_X + ((MESSAGE_W / 2) - (GetFontWidth(fMsg2) / 2));
		fMsg2Y = fMsgY + GetFontHeight() + 4;
		fShowDelay = 0;
	}
	else
	{
		fMsgY = (MESSAGE_Y + ((MESSAGE_H / 2) - (GetFontHeight() / 2))) - 1;
		fShowDelay = 4;
	}
	optionstack.push_back(this);
}

Message::~Message()
{
    for (std::vector<void*>::iterator it = optionstack.begin() ; it != optionstack.end(); ++it)
    {
        if (*it!=NULL && *it==this)
        {
            optionstack.erase(it);
            break;
        }
    }
}

/*
void c------------------------------() {}
*/

void Message::Draw()
{
	TextBox::DrawFrame(MESSAGE_X, MESSAGE_Y, MESSAGE_W, MESSAGE_H);
	
	if (fShowDelay > 0)
	{
		fShowDelay--;
		return;
	}
	
	font_draw(fMsgX, fMsgY, _(fMsg));
	
	if (fMsg2[0])
		font_draw(fMsg2X, fMsg2Y, _(fMsg2));
}


void Message::RunInput()
{
	if ( (last_sdl_action.key != -1) || (last_sdl_action.jbut != -1) || (last_sdl_action.jhat != -1) || (last_sdl_action.jaxis != -1) )
	{
		if (rawKeyReturn) *rawKeyReturn = last_sdl_action;
		if (on_dismiss)   (*on_dismiss)(this);
		delete this;
	}
}




