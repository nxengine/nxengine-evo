#include "nemesis.h"
#include "weapons.h"
#include "../../ObjManager.h"
#include "../../common/misc.h"
#include "../../game.h"

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_NEMESIS_SHOT, ai_nemesis_shot);
	ONTICK(OBJ_NEMESIS_SHOT_CURLY, ai_nemesis_shot);
}

/*
void c------------------------------() {}
*/

void ai_nemesis_shot(Object *o)
{
	if (run_shot(o, (o->shot.level != 2)))
		return;
	
	// smoke trails on level 1
	if (o->shot.level == 0)
	{
		// observe, first smokecloud is on 3rd frame;
		// it goes BLUE, YELLOW, BLUE--that's when the first cloud appears.
		if ((++o->timer % 4) == 3)
		{
			int x, y, xi, yi;
			x = y = xi = yi = 0;
			
			switch(o->shot.dir)
			{
				case RIGHT:
				{
					x = o->Left();
					y = o->CenterY();
					xi = 0x200;
					yi = random(-0x200, 0x200);
				}
				break;
				
				case LEFT:
				{
					x = o->Right();
					y = o->CenterY();
					xi = -0x200;
					yi = random(-0x200, 0x200);
				}
				break;
				
				case UP:
				{
					x = o->CenterX();
					y = o->Bottom();
					xi = random(-0x200, 0x200);
					yi = -0x200;
				}
				break;
				
				case DOWN:
				{
					x = o->CenterX();
					y = o->Top();
					xi = random(-0x200, 0x200);
					yi = 0x200;
				}
				break;
			}
			
			x += o->xinertia;
			y += o->yinertia;
			
			Object *smoke = CreateObject(x, y, OBJ_SMOKE_CLOUD);
			smoke->xinertia = xi;
			smoke->yinertia = yi;
			smoke->PushBehind(o);
			
			if (o->timer2 == 0)
			{
				smoke->frame = 3;
				o->timer2 = 1;
			}
			else if (random(0, 1))
			{
				smoke->frame = 1;
			}
		}
	}
	
	o->frame ^= 1;
}



