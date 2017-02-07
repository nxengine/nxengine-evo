
// frenzied mimiga subboss seen in Grasstown Hut
#include "frenzied_mimiga.h"

#include "../stdai.h"
#include "../ai.h"
#include "../sym/smoke.h"
#include "../../sound/sound.h"

#include "../../game.h"
#include "../../player.h"


INITFUNC(AIRoutines)
{
	ONTICK(OBJ_FRENZIED_MIMIGA, ai_frenzied_mimiga);
}

/*
void c------------------------------() {}
*/

void ai_frenzied_mimiga(Object *o)
{
	/*debug("state %d", o->state);
	debug("timer %d", o->timer);
	debug("xi %d", o->xinertia);
	debug("yi %d", o->yinertia);*/
	
	switch(o->state)
	{
		case 0:
			o->xinertia = 0;
			o->state = 1;
			o->timer = 0;
			o->timer2 = 0;
		case 1:
		{	// waiting-- attack once player gets too close or shoots us
			if (++o->timer > 40)
			{
				if (pdistlx(96<<CSF) && pdistly2(96<<CSF, 32<<CSF))
				{
					o->state = 10;
					o->timer = 0;
				}
				
				if (o->shaketime)
				{
					o->state = 10;
					o->timer = 0;
				}
			}
		}
		break;
		
		case 10:
		{	// woken up-- preparing to attack
			FACEPLAYER;
			o->frame = 1;
			
			if (++o->timer > 20)
			{
				o->timer = 0;
				o->state = 20;
			}
		}
		break;
		
		case 20:
		{	// hop, hop, lunge...
			o->damage = 0;
			o->xinertia = 0;
			
			ANIMATE_FWD(2);
			if (o->frame >= 3)
			{
				FACEPLAYER;
				XMOVE(0x200);
				
				if (++o->timer2 >= 3)	// lunge/bite
				{
					o->timer2 = 0;
					
					sound(SND_JAWS);
					o->frame = 4;
					o->damage = 5;
					o->xinertia *= 2;
				}
				else
				{
					sound(SND_ENEMY_JUMP);
				}
				
				o->state = 21;
				o->yinertia = -0x400;
			}
		}
		break;
		case 21:	// doing jump or lunge
		{
			if (o->blockd && o->yinertia >= 0)
			{
				sound(SND_THUD);
				
				o->state = 20;
				o->frame = 1;
				o->animtimer = 0;
				o->damage = 0;
				
				// if player too far away return to wait state
				if (!pdistlx(144<<CSF) || !pdistly2(144<<CSF, 72<<CSF))
				{
					o->state = 0;
				}
			}
		}
		break;
		
		case 30:		// jumping out of fireplace (set by script)
		{
			SmokeClouds(o, 8, 16, 16);
			o->frame = 0;
			o->state = 0;
		}
		break;
		
		case 50:		// killed (as boss, in Grasstown Hut) (set by script)
		{
			sound(SND_ENEMY_HURT);
			o->frame = 4;
			o->damage = 0;
			o->flags &= ~(FLAG_SHOOTABLE | FLAG_SOLID_MUSHY);
			
			o->state = 51;
			o->yinertia = -0x200;
		}
		case 51:
		{
			if (o->blockd && o->yinertia >= 0)
			{
				o->frame = 5;
				o->xinertia = 0;
				sound(SND_THUD);
				
				o->state = 52;	// falls slower
			}
		}
		break;
	}
	
	if (o->state == 52)
		o->yinertia += 0x20;
	else
		o->yinertia += 0x40;
	
	LIMITY(0x5ff);
}



