
#include "../nx.h"
#include "pause.h"
#include "../graphics/font.h"
#include "../input.h"


#include "../game.h"
#include "../graphics/graphics.h"
#include "../graphics/sprites.h"
#include "../autogen/sprites.h"
using namespace Graphics;
using namespace Sprites;
#include "dialog.h"
using namespace Options;

Dialog *dlg;
int mm_cursel;


void _resume(ODItem* item, int dir)
{
    lastinputs[F1KEY] = true;
    game.pause(false);
}

void _options(ODItem* item, int dir)
{
    game.pause(GP_OPTIONS);
}

void _reset(ODItem* item, int dir)
{
    lastinputs[F2KEY] = true;
    game.reset();
}

void _exit(ODItem* item, int dir)
{
    lastinputs[ESCKEY] = true;
    game.running = false;
}

bool pause_init(int param)
{
	memset(lastinputs, 1, sizeof(lastinputs));
	dlg = new Dialog();
	dlg->AddItem("Resume", _resume);
	dlg->AddItem("Options", _options);
	dlg->AddItem("Reset", _reset);
	dlg->AddItem("Exit", _exit);
	dlg->SetSelection(mm_cursel);
	dlg->ShowFull();
	dlg->SetSize(100,70);
	return 0;
}

void pause_tick()
{
	dlg->RunInput();
	dlg->Draw();
	
	if (justpushed(ESCKEY))
	{
	    _resume(NULL,0);
		return;
	}
}






