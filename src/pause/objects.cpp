
// A miniature implementation of the object manager.
// It runs the little "mascots" that come on the screen after a while in the options menu.
#include "../nx.h"
#include "../common/llist.h"
#include "options.h"
#include "../ai/ai.h"
#include "objects.h"
#include "../common/misc.h"

#include "../game.h"
#include "../graphics/graphics.h"
#include "../graphics/sprites.h"
#include "../autogen/sprites.h"
using namespace Graphics;
using namespace Sprites;


using namespace Options;

Object *firstobj, *lastobj;

#define OC_CONTROLLER		0
#define OC_QUOTE			1
#define OC_IKACHAN			2


/*
void c------------------------------() {}
*/

static void ai_oc_controller(Object *o)
{
	//AIDEBUG;
	
	switch(o->state)
	{
		case 0:		// init
		{
			o->timer = 400;
			o->state = 1;
		}
		break;
		case 1:		// delay before next event
		{
			if (--o->timer <= 0)
			{
				o->state = (++o->timer2 * 10);
				o->timer = 0;
				
				if (o->timer2 >= 2)
					o->timer2 = 0;
			}
		}
		break;
		
		case 10:	// quote
		{
			create_object(0, 0, OC_QUOTE);
			o->timer = 1100;
			o->state = 1;
		}
		break;
		
		case 20:	// ikachans
		{
			o->timer++;
			
			// current
			/*if (o->timer < 175)
			{
				if ((o->timer % 6) == 1)
					create_object(-16 * CSFI, random(-16, SCREEN_HEIGHT) * CSFI, OC_CURRENT);
			}*/
			
			if (o->timer <= 150)
			{
				if ((o->timer % 10) == 1)
					create_object(-16 * CSFI, random(-16, SCREEN_HEIGHT) * CSFI, OC_IKACHAN);
			}
			
			if (o->timer > 300)
				o->state = 0;
		}
		break;
	}
}


static void ai_oc_quote(Object *o)
{
	//AIDEBUG;
	
	switch(o->state)
	{
		case 0:
		{
			o->xmark  = (SCREEN_WIDTH - 50) * CSFI;
			o->xmark2 = (SCREEN_WIDTH + 10) * CSFI;
			
			o->x = o->xmark2;
			o->y = (SCREEN_HEIGHT - sprites[o->sprite].h - 8) * CSFI;
			o->dir = LEFT;
			
			o->sprite = SPR_OC_QUOTE;
			o->state = 20;
		}
		break;
		
		case 20:
		{
			o->dir = LEFT;
			o->timer = 0;
			o->animtimer = 2;
			o->state = 21;
		}
		case 21:
		{
			if (o->x > o->xmark)
			{
				ANIMATE(8, 0, 3);
				XMOVE(0x100);
			}
			else
			{
				o->frame = 0;
				o->xinertia = 0;
				
				if (++o->timer > 20)
				{
					o->state = 22;
					o->timer = 0;
					o->frame = 4;
				}
			}
		}
		break;
		case 22:
		{
			o->timer++;
			if (o->timer == 100) o->frame = 0;
			if (o->timer > 130)
			{
				o->state = 30;
				o->timer = 0;
				o->dir = RIGHT;
			}
		}
		break;
		
		case 30:
		{
			ANIMATE(8, 0, 3);
			XMOVE(0x100);
			
			if (o->x > o->xmark2)
				o->deleted = true;
		}
		break;
	}
}


static void ai_oc_ikachan(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			o->state = 1;
			o->timer = random(3, 20);
			o->sprite = SPR_IKACHAN;
		}
		case 1:		// he pushes ahead
		{
			if (--o->timer <= 0)
			{
				o->state = 2;
				o->timer = random(10, 50);
				o->frame = 1;
				o->xinertia = 0x600;
			}
		}
		break;
		
		case 2:		// after a short time his tentacles look less whooshed-back
		{
			if (--o->timer <= 0)
			{
				o->state = 3;
				o->timer = random(40, 50);
				o->frame = 2;
				o->yinertia = random(-0x100, 0x100);
			}
		}
		break;
		
		case 3:		// gliding
		{
			if (--o->timer <= 0)
			{
				o->state = 1;
				o->timer = 0;
				o->frame = 0;
			}
			
			o->xinertia -= 0x10;
		}
		break;
	}
	
	if (o->x > SCREEN_WIDTH * CSFI)
		o->deleted = true;
}

/*
static void ai_oc_current(Object *o)
{
	o->sprite = SPR_WATER_DROPLET;
	o->frame = random(0, 4);
	
	o->xinertia = 0x400;
	
	if (o->x > SCREEN_WIDTH * CSFI)
		o->deleted = true;
}
*/


void Options::init_objects()
{
	firstobj = NULL;
	lastobj = NULL;
	create_object(0, 0, OC_CONTROLLER);
}

void Options::close_objects()
{
	Object *o = firstobj;
	while(o)
	{
		Object *next = o->next;
		delete o;
		o = next;
	}
	
	firstobj = lastobj = NULL;
}

void Options::run_and_draw_objects(void)
{
void (*ai_routine[])(Object *) = {
	ai_oc_controller,
	ai_oc_quote,
	ai_oc_ikachan
};

	// draw character
	Object *o = firstobj;
	while(o)
	{
		(*ai_routine[o->type])(o);
		Object *next = o->next;
		
		// cull deleted
		if (o->deleted)
		{
			LL_REMOVE(o, prev, next, firstobj, lastobj);
			delete o;
		}
		else if (o->sprite != SPR_NULL)
		{
			o->x += o->xinertia;
			o->y += o->yinertia;
			
			draw_sprite(o->x / CSFI, o->y / CSFI, o->sprite, o->frame, o->dir);
		}
		
		o = next;
	}
}

Object *Options::create_object(int x, int y, int type)
{
static Object ZERO;

	Object *o = new Object;
	*o = ZERO;
	
	o->x = x;
	o->y = y;
	o->type = type;
	LL_ADD_END(o, prev, next, firstobj, lastobj);
	
	return o;
}




