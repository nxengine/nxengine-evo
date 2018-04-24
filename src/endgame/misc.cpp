#include "misc.h"
#include "../ai/stdai.h"
#include "../ai/ai.h"
#include "../ObjManager.h"
#include "../common/misc.h"

#include "../game.h"
#include "../player.h"
#include "../map.h"
#include "../autogen/sprites.h"



INITFUNC(AIRoutines)
{
	ONTICK(OBJ_CLOUD_SPAWNER, ai_cloud_spawner);
	ONTICK(OBJ_CLOUD, ai_cloud);
	
	ONTICK(OBJ_BALROG_FLYING, ai_balrog_flying);
	AFTERMOVE(OBJ_BALROG_PASSENGER, aftermove_balrog_passenger);

	ONTICK(OBJ_BALROG_MEDIC, ai_balrog_medic);
	ONTICK(OBJ_GAUDI_PATIENT, ai_gaudi_patient);
	
	ONTICK(OBJ_BABY_PUPPY, ai_baby_puppy);
	
	ONTICK(OBJ_TURNING_HUMAN, ai_turning_human);
	ONTICK(OBJ_BUILDING_FAN, ai_animate2);
	ONTICK(OBJ_AHCHOO, ai_ahchoo);

	ONTICK(OBJ_MISERY_WIND, ai_misery_wind);
	ONTICK(OBJ_THE_CAST, ai_the_cast);
}

/*
void c------------------------------() {}
*/

// clouds should be layered parallax, according to their size & speed.
// the clouds are behind all the other objects (player, etc).
static int cloud_sprites[] = { SPR_CLOUD_1, SPR_CLOUD_2, SPR_CLOUD_3, SPR_CLOUD_4 };


// makes the clouds from the falling scene (good ending)
void ai_cloud_spawner(Object *o)
{
Object *cloud;
int type;

	if (o->state == 0)
	{
		for(int i=0;i<4;i++)
		{
			o->cloud.layers[i] = CreateObject(0, 0, OBJ_NULL);
			o->cloud.layers[i]->PushBehind(lowestobject);
		}
		
		o->state = 1;
	}
	
	if (--o->timer < 0)
	{
		o->timer = random(0, 16);
		type = random(0, 3);
		
		cloud = CreateObject(0, 0, OBJ_CLOUD);
		cloud->sprite = cloud_sprites[type];
		
		if (o->dir == LEFT)		// vertical clouds (falling)
		{
			cloud->x = o->x + MAPY(random(-10, 10));
			cloud->y = o->y;
			cloud->yinertia = -(0x1000 >> type);	// each type half as fast as the last
		}
		else					// horizontal clouds (flying with Kazuma)
		{
			cloud->x = o->x;
			cloud->y = o->y + MAPY(random(-7, 7));
			if (widescreen) cloud->y+=(32 * CSFI);
			cloud->xinertia = -(0x400 >> type);
		}
		
		// cut down on the amount of time Kazuma is flying
		// against plain blue when he appears in the credits
		if (game.mode == GM_CREDITS && o->state < 10)
		{
			cloud->x -= (128 * CSFI);
			o->state++;
		}
		
		cloud->PushBehind(o->cloud.layers[type]);
	}

}

void ai_cloud(Object *o)
{
	if (o->x < -o->Width() || o->y < -o->Height())
		o->Delete();
}

/*
void c------------------------------() {}
*/

// Balrog flying in clouds with player and Curly in best-ending.
void ai_balrog_flying(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			o->state = 1;
			o->animtimer = 0;
			
			o->ymark = o->y - 0x2000;
			o->xmark = o->x - 0xC00;
			o->yinertia = 0;
			
			CreateObject(0, 0, OBJ_BALROG_PASSENGER, 0, 0, LEFT, o)->state = 1;
			CreateObject(0, 0, OBJ_BALROG_PASSENGER, 0, 0, RIGHT, o)->state = 1;
		}
		case 1:
		{
			ANIMATE(4, 0, 1);
			
			o->xinertia += (o->x < o->xmark) ? 0x08 : -0x08;
			o->yinertia += (o->y < o->ymark) ? 0x08 : -0x08;
		}
		break;
		
		case 20:	// fly away
		{
			o->state = 21;
			o->xinertia = -0x400;
			o->yinertia = 0x200;
		}
		case 21:
		{
			ANIMATE(2, 0, 1);
			
			o->xinertia += 0x10;
			o->yinertia -= 0x08;
			
			if (o->x > 0x78000)
			{
				o->xinertia = 0;
				o->yinertia = 0;
				o->state = 22;
			}
		}
		break;
	}
}



// player/curly when rescued by Balrog during best-ending
void aftermove_balrog_passenger(Object *o)
{
	if (!o->linkedobject)
	{
		o->Delete();
		return;
	}

	switch(o->state)
	{
		case 0:		// being rescued from Seal Chamber
		{
			if (o->dir == RIGHT)
			{
				o->sprite = player->sprite;
				o->frame = 4;
				
				o->x = o->linkedobject->x - (2 * CSFI);
			}
			else
			{
				o->sprite = SPR_CURLY;
				o->frame = 7;
				
				o->x = o->linkedobject->x + (26 * CSFI);
			}
			
			o->y = o->linkedobject->y + (14 * CSFI);
		}
		break;
		
		case 1:		// flying in clouds
		{
			if (o->dir == RIGHT)
			{
				o->sprite = SPR_MYCHAR;
				o->frame = 12;
				
				o->x = o->linkedobject->x - (15 * CSFI);
			}
			else
			{
				o->sprite = SPR_CURLY;
				o->frame = 18;
				
				o->x = o->linkedobject->x - (4 * CSFI);
			}
			
			o->y = o->linkedobject->y - (23 * CSFI);
		}
		break;
	}
}


/*
void c------------------------------() {}
*/

// seen in credits
void ai_balrog_medic(Object *o)
{
	o->frame = 0;
	randblink(o, 1, 12);
}

void ai_gaudi_patient(Object *o)
{
	switch(o->state)
	{
		case 0:		// sitting
		{
			o->frame = 0;
		}
		break;
		
		case 10:	// lying
		{
			o->frame = 1;
		}
		break;
		
		case 20:	// hurting
		{
			o->state = 21;
			o->frame = 2;
		}
		case 21:
		{
			ANIMATE(10, 2, 3);
		}
		break;
	}
}

/*
void c------------------------------() {}
*/

void ai_baby_puppy(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			o->animtimer = random(0, 6);	// desync with other puppies
			o->state = 1;
		}
		case 1:
		{
			ANIMATE(6, 0, 1);
		}
		break;
	}
}

/*
void c------------------------------() {}
*/

void ai_turning_human(Object *o)
{
Object *ahchoo;
	
	// LEFT = Itoh
	// RIGHT = Sue
	switch(o->state)
	{
		case 0:
		{
			o->x += (16 * CSFI);
			o->y -= (16 * CSFI);
			o->state = 1;
		}
		case 1:
		{
			if (++o->timer > 80)
			{
				o->state = 10;
				o->timer = 0;
			}
			
			// before machine turns on they both blink,
			// at slightly different times
			if (o->dir == LEFT)
			{
				if (o->timer == 30) o->frame = 1;
				if (o->timer == 40) o->frame = 0;
			}
			else
			{
				if (o->timer == 50) o->frame = 1;
				if (o->timer == 60) o->frame = 0;
			}
		}
		break;
		
		case 10:	// being transformed
		{
			o->timer++;
			o->frame = (o->timer & 2) ? 2 : 3;
			
			if (o->timer > 50)
			{
				o->state = 20;
				o->frame = 4;
				
				// wait for slightly different times before falling
				o->timer = (o->dir == LEFT) ? 40 : 60;
			}
		}
		break;
		
		case 20:	// waiting after transformation
		{
			if (--o->timer < 0)
			{
				o->timer = 0;
				o->state = 30;
			}
		}
		break;
		
		case 30:	// falling
		{
			o->yinertia += 0x40;
			LIMITY(0x5ff);
			
			if (++o->timer > 50)	// sneeze
			{
				o->state = 40;
				o->timer = 0;
				o->frame = 6;	// head-back to sneeze
				
				// create sneeze. Itoh is taller.
				int yoffs = (o->dir == LEFT) ? (16 * CSFI) : (8 * CSFI);
				ahchoo = CreateObject(o->x, o->y - yoffs, OBJ_AHCHOO);
				ahchoo->linkedobject = o;
			}
		}
		break;
		
		case 40:	// sneezing
		{
			o->timer++;
			
			if (o->timer == 30)
				o->frame = 7;
			
			if (o->timer > 40)
				o->state = 50;
		}
		break;
		
		case 50:	// turn back to mimiga...
		{
			o->state = 51;
			o->timer = 0;
			o->frame = 0;
		}
		case 51:	// ..and blink
		{
			o->timer++;
			
			if (o->timer == 30) o->frame = 1;
			if (o->timer == 40) o->frame = 0;
		}
		break;
	}
}


void ai_ahchoo(Object *o)
{
	if (!o->linkedobject)
	{
		o->Delete();
		return;
	}
	
	o->timer++;
	
	switch(o->state)
	{
		case 0:		// rise up "ah..."
		{
			if (o->timer < 4)
				o->y -= (2 * CSFI);
			
			if (o->linkedobject->frame == 7)	// sneezing frame
			{
				o->frame = 1;	// "choo!"
				o->state = 1;
				o->xmark = o->x;
				o->ymark = o->y;
			}
		}
		break;
		
		case 1:		// shaking "choo!"
		{
			if (o->timer < 48)
			{	// shake
				o->x = o->xmark + (random(-1, 1) * CSFI);
				o->y = o->ymark + (random(-1, 1) * CSFI);
			}
			else
			{	// return to original pos
				o->xmark = o->x;
				o->ymark = o->y;
			}
			
			if (o->timer > 70)
				o->Delete();
		}
		break;
	}
}

/*
void c------------------------------() {}
*/

void ai_misery_wind(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			ANIMATE(6, 0, 1);
		}
		break;
		
		case 10:	// look at screen
		{
			ANIMATE(6, 3, 4);
		}
		break;
	}
}


/*
void c------------------------------() {}
*/

// 000 - King
// 100 - Toroko
// 200 - Kazuma
// 300 - Sue
// 400 - Momorin
// 500 - Booster
// 600 - Jenka
//
// 700 - Nurse Hasumi
// 800 - Dr Gero
// 900 - Balrog
// 1000 - Curly
// 1100 - Misery
// 1200 - Malco
// 1300 - Hermit Gunsmith
void ai_the_cast(Object *o)
{
const static struct
{
	int sprite;
	int fallframe, standframe;
	int dir;
	bool tall;
}
cast_data[] =
{
	{SPR_KING,		6, 0, RIGHT, false},
	{SPR_TOROKO,		2, 0, RIGHT, false},
	{SPR_KAZUMA,		4, 0, RIGHT, true},
	{SPR_SUE, 		4, 0, RIGHT, false},
	{SPR_MOMORIN, 	2, 0, LEFT, true},
	{SPR_PROFESSOR_BOOSTER, 8, 0, LEFT, false},
	{SPR_JENKA,		0, 0, LEFT, false},
	
	{SPR_NURSE_HASUMI,	 1, 0, RIGHT, false},
	{SPR_DR_GERO,		 1, 0, RIGHT, false},
	{SPR_BALROG_CAST,	 1, 0, RIGHT, true},
	{SPR_CURLY,			 1, 0, RIGHT, false},
	{SPR_MISERY,			 0, 2, LEFT, false},
	{SPR_MALCO_BROKEN,	 1, 0, LEFT, true},
	{SPR_HERMIT_GUNSMITH, 1, 0, LEFT, true}
};

	switch(o->state)
	{
		case 0:
		{
			o->dirparam /= 100;
			if (o->dirparam >= 14) o->dirparam = 0;
			
			o->sprite = cast_data[o->dirparam].sprite;
			o->frame = cast_data[o->dirparam].fallframe;
			o->dir = cast_data[o->dirparam].dir;
			
			if (cast_data[o->dirparam].tall)
				o->y -= (4 * CSFI);
			
			// create King's sword
			if (o->sprite == SPR_KING)
			{
				Object *sword;
				sword = CreateObject(o->x, o->y, OBJ_KINGS_SWORD);
				sword->linkedobject = o;
				sword->carry.flip = true;
			}
			
			// Balrog goes behind Curly
			if (o->sprite == SPR_BALROG_CAST)
			{
				o->PushBehind(lowestobject);
			}
			
			o->state = 1;
		}
		case 1:
		{
			o->yinertia += 0x40;
			LIMITY(0x5ff);
			
			if (o->blockd)
			{
				o->frame = cast_data[o->dirparam].standframe;
				o->state = 2;
			}
		}
		break;
	}
}





























