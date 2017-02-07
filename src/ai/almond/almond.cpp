
#include "../stdai.h"
#include "../ai.h"
#include "../sym/smoke.h"
#include "../../sound/sound.h"
#include "../../common/stat.h"
#include "almond.h"

#include "../../game.h"
#include "../../map.h"

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_WATERLEVEL, ai_waterlevel);
	
	ONTICK(OBJ_SHUTTER, ai_shutter);
	ONTICK(OBJ_SHUTTER_BIG, ai_shutter);
	ONTICK(OBJ_ALMOND_LIFT, ai_shutter);
	
	ONTICK(OBJ_SHUTTER_STUCK, ai_shutter_stuck);
	ONTICK(OBJ_ALMOND_ROBOT, ai_almond_robot);
}

/*
void c------------------------------() {}
*/

void ai_waterlevel(Object *o)
{
/*	debug("WL State: %d", o->state);
	debug("WL Y: %d", o->y>>CSF);
	debug("WL Timer: %d", o->timer);
	debug("WLForceUp: %d", map.wlforceup);*/
	
	if (map.wlforcestate)
	{
		stat("Forced WL state to %d", map.wlforcestate);
		o->state = map.wlforcestate;
		map.wlforcestate = 0;
	}
	
	switch(o->state)
	{
		case 0:
			map.waterlevelobject = o;
			o->state = WL_CALM;
			o->y += (8<<CSF);
			o->ymark = o->y;
			o->yinertia = 0x200;
		case WL_CALM:	// calm waves around set point
			o->yinertia += (o->y < o->ymark) ? 4 : -4;
			LIMITY(0x100);
		break;
		
		case WL_CYCLE:			// wait 1000 ticks, then rise all the way to top come down and repeat
			o->state = WL_DOWN; o->timer = 0;
		case WL_DOWN:
			o->yinertia += (o->y < o->ymark) ? 4 : -4;
			LIMITY(0x200);
			if (++o->timer > 1000)
			{
				o->state = WL_UP;
			}
		break;
		case WL_UP:			// rise all the way to top then come back down
			o->yinertia += (o->y > 0) ? -4 : 4;
			LIMITY(0x200);
			
			// when we reach the top return to normal level
			if (o->y < (64<<CSF))
			{
				o->state = WL_CYCLE;
			}
		break;
		
		case WL_STAY_UP:	// rise quickly all the way to top and stay there
			o->yinertia += (o->y > 0) ? -4 : 4;
			if (o->yinertia < -0x200) o->yinertia = -0x200;
			if (o->yinertia > 0x100) o->yinertia = 0x100;
		break;
	}
	
	map.wlstate = o->state;
}


/// common code to both Shutter AND Lift
void ai_shutter(Object *o)
{
	if (o->state == 10)
	{
		// allow hitting the stuck shutter no. 4
		o->flags &= ~(FLAG_SHOOTABLE | FLAG_INVULNERABLE);
		
		switch(o->dir)
		{
			case LEFT:  o->x -= 0x80; break;
			case RIGHT: o->x += 0x80; break;
			case UP:    o->y -= 0x80; break;
			case DOWN:  o->y += 0x80; break;
		}
		
		// animate Almond_Lift
		if (o->type==OBJ_ALMOND_LIFT)
		{
			ai_animate3(o);
		}
		else if (o->type==OBJ_SHUTTER_BIG)
		{
			if (!o->timer)
			{
				game.quaketime = 20;
				sound(SND_QUAKE);
				
				o->timer = 6;
			} else o->timer--;
		}
	}
	else if (o->state == 20)	// tripped by script when Shutter_Big closes fully
	{
		SmokeSide(o, 4, DOWN);
		o->state = 21;
	}
	
	if (o->type == OBJ_SHUTTER_BIG)
	{
		ANIMATE(10, 0, 3);
	}
}

void ai_shutter_stuck(Object *o)
{
	// when you shoot shutter 4, you're actually shooting us, but we want them
	// to think they're shooting the regular shutter object, so go invisible
	o->invisible = 1;
}

/*
void c------------------------------() {}
*/

// the damaged robot which wakes up right before the Almond battle
void ai_almond_robot(Object *o)
{
	switch(o->state)
	{
		case 0:
			o->frame = 0;
		break;
		
		case 10:	// blows up
			sound(SND_BIG_CRASH);
			SmokeClouds(o, 8, 3, 3);
			o->Delete();
		break;
		
		case 20:	// flashes
			ANIMATE(10, 0, 1);
		break;
	}
}





