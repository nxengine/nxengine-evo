
#include "../stdai.h"
#include "smoke.h"
#include "../../ObjManager.h"
#include "../../trig.h"
#include "../../game.h"
#include "../../common/misc.h"
#include "../../graphics/sprites.h"


INITFUNC(AIRoutines)
{
	ONTICK(OBJ_SMOKE_CLOUD, ai_smokecloud);
}

/*
void c------------------------------() {}
*/

// spawn a single smoke puff at x,y and heading off in a random direction
Object *SmokePuff(int x, int y)
{
	Object *o = CreateObject(x, y, OBJ_SMOKE_CLOUD);
	vector_from_angle(random(0,255), random(0x200,0x5ff), &o->xinertia, &o->yinertia);
	return o;
}

// spawn a cloud of smoke centered around object o and starting within "range" distance.
void SmokeClouds(Object *o, int nclouds, int rangex, int rangey, Object *push_behind)
{
	SmokeXY(o->CenterX(), o->CenterY(), nclouds, rangex, rangey, push_behind);
}


void SmokeXY(int x, int y, int nclouds, int rangex, int rangey, Object *push_behind)
{
Object *s;

	for(int i=0;i<nclouds;i++)
	{
		s = SmokePuff(x + (random(-rangex, rangex) << CSF), \
			 	      y + (random(-rangey, rangey) << CSF));
		
		if (push_behind)
			s->PushBehind(push_behind);
	}
}


// spawns smoke from the given side of the object,
// as if it the object had smacked into something.
// for example if an object smashed into ground you could pass DOWN in dir.
void SmokeSide(Object *o, int nclouds, int dir)
{
int xmin, xmax, ymin, ymax;
int xi_min, xi_max, yi_min, yi_max;
static const int SSLOW = 0x155;
static const int SFAST = 0x600;

	switch(dir)
	{
		case UP:
		case DOWN:
		{
			xmin = 0;
			xmax = o->Width();
			
			xi_min = -SSLOW;
			xi_max = SSLOW;
			
			if (dir == UP)
			{
				ymin = ymax = 0;
				yi_min = 0;
				yi_max = SFAST;
			}
			else
			{
				ymin = ymax = (o->Height() - (2<<CSF));
				yi_min = -SFAST;
				yi_max = 0;
			}
		}
		break;
		
		case LEFT:
		case RIGHT:
		{
			ymin = 0;
			ymax = o->Height();
			
			yi_min = -SSLOW;
			yi_max = SSLOW;
			
			if (dir == LEFT)
			{
				xmin = xmax = 0;
				xi_min = 0;
				xi_max = SFAST;
			}
			else
			{
				xmin = xmax = (o->Width() - (2<<CSF));
				xi_min = -SFAST;
				xi_max = 0;
			}
		}
		break;
		
		case CENTER:
		{
			xmin = 0; xmax = o->Width();
			ymin = 0; ymax = o->Height();
			
			xi_min = -SSLOW; xi_max = SSLOW;
			yi_min = -SSLOW; yi_max = 0;
		}
		break;
	}
	
	for(int i=0;i<nclouds;i++)
	{
		CreateObject(o->x + random(xmin, xmax),
					 o->y + random(ymin, ymax),
					 OBJ_SMOKE_CLOUD,
					 random(xi_min, xi_max),
					 random(yi_min, yi_max));
	}
}

void SmokeCloudsSlow(int x, int y, int nclouds)
{
	for(int i=0;i<nclouds;i++)
	{
		CreateObject(x, y, OBJ_SMOKE_CLOUD,
					random(-0x200, 0x200),
					random(-0x200, 0x200));
	}
}

void SmokeBoomUp(Object *o)
{
	for(int i=0;i<8;i++)
	{
		CreateObject(o->CenterX() + random(-16<<CSF, 16<<CSF),
					 o->CenterY() + random(-16<<CSF, 16<<CSF),
					 OBJ_SMOKE_CLOUD,
					 random(-0x155, 0x155),
					 random(-0x600, 0));
	}
}

/*
void c------------------------------() {}
*/

void ai_smokecloud(Object *o)
{
	if (!o->state)
	{
		if (!random(0, 1)) o->frame = 1;
		o->state = 1;
	}
	
	if (++o->animtimer >= 5)
	{
		o->animtimer = 0;
		if (++o->frame >= sprites[o->sprite].nframes)
			o->Delete();
	}
	
	o->xinertia *= 20; o->xinertia /= 21;
	o->yinertia *= 20; o->yinertia /= 21;
}



