#include "sand.h"

#include "../stdai.h"
#include "../ai.h"
#include "../sym/smoke.h"
#include "../sand/puppy.h"
#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../trig.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"

#include "../../game.h"
#include "../../player.h"
#include "../../map.h"
#include "../../graphics/sprites.h"

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_BEETLE_BROWN, ai_beetle_horizwait);
	
	ONTICK(OBJ_POLISH, ai_polish);
	ONDEATH(OBJ_POLISH, ondeath_polish);
	
	ONTICK(OBJ_POLISHBABY, ai_polishbaby);
	
	ONTICK(OBJ_SANDCROC, ai_sandcroc);
	
	ONTICK(OBJ_MIMIGAC1, ai_curlys_mimigas);
	ONTICK(OBJ_MIMIGAC2, ai_curlys_mimigas);
	ONTICK(OBJ_MIMIGAC_ENEMY, ai_curlys_mimigas);
	
	ONTICK(OBJ_SUNSTONE, ai_sunstone);
	
	ONTICK(OBJ_ARMADILLO, ai_armadillo);
	
	ONTICK(OBJ_CROW, ai_crow);
	ONTICK(OBJ_CROWWITHSKULL, ai_crowwithskull);
	
	ONTICK(OBJ_SKULLHEAD, ai_skullhead);
	ONTICK(OBJ_SKULLHEAD_CARRIED, ai_skullhead_carried);
	AFTERMOVE(OBJ_SKULLHEAD_CARRIED, aftermove_skullhead_carried);
	
	ONTICK(OBJ_SKULLSTEP, ai_skullstep);
	ONTICK(OBJ_SKULLSTEP_FOOT, ai_skullstep_foot);
	
	ONTICK(OBJ_SKELETON, ai_skeleton);
	ONTICK(OBJ_SKELETON_SHOT, ai_skeleton_shot);
}

/*
void c------------------------------() {}
*/

void ai_polish(Object *o)
{
	#define POLISH_ACCEL	0x20
	#define POLISH_SPEED	0x200
	#define POLISH_BOUNCE	0x100
	
	#define POLISH_CCW_LEFT		1
	#define POLISH_CCW_UP		2
	#define POLISH_CCW_RIGHT	3
	#define POLISH_CCW_DOWN		4
	
	#define POLISH_CW_LEFT		5
	#define POLISH_CW_UP		6
	#define POLISH_CW_RIGHT		7
	#define POLISH_CW_DOWN		8
	
	switch(o->state)
	{
		case 0:		// initilization
		{
			o->state = (o->dir == LEFT) ? POLISH_CW_RIGHT : POLISH_CCW_LEFT;
			
			// reprocess first frame
			ai_polish(o);
		}
		break;
		
		// -------------- Traveling around counter-clockwise --------------
		
		case POLISH_CCW_LEFT:	// traveling left on ceiling
		{
			o->yinertia -= POLISH_ACCEL;
			if (o->yinertia < 0 && o->blocku)
			{
				o->yinertia = POLISH_BOUNCE;
				o->xinertia -= POLISH_BOUNCE;
			}
			
			if (o->blockl) o->state = POLISH_CCW_DOWN;
		}
		break;
		
		case POLISH_CCW_UP:	// traveling up right wall
		{
			o->xinertia += POLISH_ACCEL;
			if (o->xinertia > 0 && o->blockr)
			{
				o->xinertia = -POLISH_BOUNCE;
				o->yinertia -= POLISH_BOUNCE;
			}
			
			if (o->blocku) o->state = POLISH_CCW_LEFT;
		}
		break;
		
		case POLISH_CCW_RIGHT:	// traveling right on floor
		{
			o->yinertia += POLISH_ACCEL;
			if (o->yinertia > 0 && o->blockd)
			{
				o->yinertia = -POLISH_BOUNCE;
				o->xinertia += POLISH_BOUNCE;
			}
			
			if (o->blockr) o->state = POLISH_CCW_UP;
		}
		break;
		
		case POLISH_CCW_DOWN:	// traveling down left wall
		{
			o->xinertia -= POLISH_ACCEL;
			if (o->xinertia < 0 && o->blockl)
			{
				o->xinertia = POLISH_BOUNCE;
				o->yinertia += POLISH_BOUNCE;
			}
			
			if (o->blockd) o->state = POLISH_CCW_RIGHT;
		}
		break;
		
		// -------------- Traveling around clockwise --------------
		
		case POLISH_CW_LEFT:		// traveling left on floor
		{
			o->yinertia += POLISH_ACCEL;
			if (o->yinertia > 0 && o->blockd)
			{
				o->yinertia = -POLISH_BOUNCE;
				o->xinertia -= POLISH_BOUNCE;
			}
			
			if (o->blockl) o->state = POLISH_CW_UP;
		}
		break;
		
		case POLISH_CW_UP:		// traveling up left wall
		{
			o->xinertia -= POLISH_ACCEL;
			if (o->xinertia < 0 && o->blockl)
			{
				o->xinertia = POLISH_BOUNCE;
				o->yinertia -= POLISH_BOUNCE;
			}
			
			if (o->blocku) o->state = POLISH_CW_RIGHT;
		}
		break;
		
		case POLISH_CW_RIGHT:		// traveling right on ceiling
		{
			o->yinertia -= POLISH_ACCEL;
			if (o->yinertia < 0 && o->blocku)
			{
				o->yinertia = POLISH_BOUNCE;
				o->xinertia += POLISH_BOUNCE;
			}
			
			if (o->blockr) o->state = POLISH_CW_DOWN;
		}
		break;
		
		case POLISH_CW_DOWN:		// traveling down right wall
		{
			o->xinertia += POLISH_ACCEL;
			if (o->xinertia > 0 && o->blockr)
			{
				o->xinertia = -POLISH_BOUNCE;
				o->yinertia += POLISH_BOUNCE;
			}
			
			if (o->blockd) o->state = POLISH_CW_LEFT;
		}
		break;
	}
	
	LIMITX(POLISH_SPEED);
	LIMITY(POLISH_SPEED);
	
	// animation
	o->animframe ^= 1;
	o->frame = o->animframe + 2;
	//if (o->state==P_DOWNRIGHT || o->state==P_UPRIGHT) o->frame += 2;
	o->dir = RIGHT;
}

void ondeath_polish(Object *o)
{
int i;
int x, y;
	x = o->CenterX();
	y = o->CenterY();
	for(i=0;i<10;i++)
	{
		CreateObject(x, y, OBJ_POLISHBABY);
	}
	o->Delete();
}

void ai_polishbaby(Object *o)
{
	if (!o->state)
	{
		o->state = 1;
		if (!random(0, 1)) o->xinertia = random(0x100, 0x200);
				  else o->xinertia = random(-0x200, -0x100);
		
		if (!random(0, 1)) o->yinertia = random(0x100, 0x200);
				  else o->yinertia = random(-0x200, -0x100);
	}
	
	if (o->xinertia > 0 && o->blockr) o->xinertia = -o->xinertia;
	if (o->xinertia < 0 && o->blockl) o->xinertia = -o->xinertia;
	if (o->yinertia > 0 && o->blockd) o->yinertia = -o->yinertia;
	if (o->yinertia < 0 && o->blocku) o->yinertia = -o->yinertia;
	
	o->frame ^= 1;
}

/*
void c------------------------------() {}
*/

void ai_sandcroc(Object *o)
{
int pbottom, crocbottom;
	
	switch(o->state)
	{
		case 0:
			o->state = 1;
			o->timer = 0;
			o->ymark = o->y;
			o->flags &= ~(FLAG_SOLID_MUSHY | FLAG_SHOOTABLE | FLAG_INVULNERABLE | FLAG_IGNORE_SOLID);
		case 1:
			// track player invisibly underground
			o->xinertia = (o->CenterX() < player->CenterX()) ? 0x400:-0x400;
			
			if (pdistlx(19 * CSFI))
			{
				// check if bottoms of player and croc are near
				pbottom = player->y + (sprites[player->sprite].bbox.y2 * CSFI);
				crocbottom = o->y + (sprites[o->sprite].bbox.y2 * CSFI) + 0x600;
				
				if (pbottom <= crocbottom && crocbottom - pbottom < (12 * CSFI))
				{	// attack!!
					o->xinertia = 0;
					o->state = 2;
					o->timer = 0;
					sound(SND_JAWS);
					o->frame = 0;
				}
			}
		break;
		
		case 2:		// attacking
			if (++o->animtimer > 3)
			{
				o->animtimer = 0;
				o->frame++;
			}
			
			if (o->frame==3) o->damage = (o->type==OBJ_SANDCROC_OSIDE) ? 15:10;
			else if (o->frame==4)
			{
				o->flags |= FLAG_SHOOTABLE;
				o->state = 3;
				o->timer = 0;
			}
		break;
		
		case 3:
			o->flags |= FLAG_SOLID_MUSHY;
			o->damage = 0;
			o->timer++;
			
			if (o->shaketime)
			{
				o->state = 4;
				o->timer = 0;
				o->yinertia = 0;
				o->shaketime += 25;		// delay floattext until after we're underground
			}
		break;
		
		case 4:		// retreat
			o->y += 0x280;
			
			if (++o->timer == 30)
			{
				o->flags &= ~(FLAG_SOLID_MUSHY | FLAG_SHOOTABLE);
				o->state = 5;
				o->timer = 0;
			}
		break;
		
		case 5:
			o->frame = 0;
			o->y = o->ymark;
			
			if (o->timer < 100)
			{
				o->timer++;
				// have to wait before moving: till floattext goes away
				// else they can see us jump
				if (o->timer==98)
				{
					o->xinertia = (player->CenterX() - o->CenterX());
				}
				else o->xinertia = 0;
			}
			else
			{
				o->state = 0;
			}
		break;
	}
	LIMITY(0x100);
	
	// these guys (from oside) don't track
	if (o->type == OBJ_SANDCROC_OSIDE) o->xinertia = 0;
}


void ai_sunstone(Object *o)
{
	switch(o->state)
	{
		case 0:
			o->flags |= FLAG_IGNORE_SOLID;
			o->state = 1;
		break;
		
		case 10:	// triggered to move by hvtrigger script
			o->frame = 1;
			o->timer = 0;
			o->state++;
		case 11:
			
			if (o->dir==LEFT) o->x -= 0x80; else o->x += 0x80;
			
			if ((o->timer & 7) == 0) sound(SND_QUAKE);
			o->timer++;
				
			game.quaketime = 20;
		break;
	}
}


void ai_crow(Object *o)
{
	// for crows carrying skullheads
	if (o->state >= 100)
	{
		// if our skullhead dies, go into attack mode
		if (!o->linkedobject)
		{
			o->state = 2;
		}
		else
		{
			// link our blockd with it's blockd so we don't fly it through the floor
			o->blockd |= o->linkedobject->blockd;
		}
	}
	
	switch(o->state)
	{
		case 0:
		{
			uint8_t angle = random(0, 255);
			vector_from_angle(angle, (1 * CSFI), &o->xinertia, &o->yinertia);
			
			o->xmark = o->x + (o->xinertia * 8);
			o->ymark = o->y + (o->yinertia * 8);
			
			o->state = 1;
			o->nxflags |= NXFLAG_FOLLOW_SLOPE;
			
			o->animframe = random(0, 1);
			o->animtimer = random(0, 4);
		}
		case 1:
		case 101:
		{
			if (o->x > o->xmark) o->xinertia -= 16;
			else if (o->x < o->xmark) o->xinertia += 16;
			
			if (o->y > o->ymark) o->yinertia -= 16;
			else if (o->y < o->ymark) o->yinertia += 16;
			
			FACEPLAYER;
			LIMITX(0x200);
			LIMITY(0x200);
			
			if (o->shaketime)
			{
				o->state++;		// state 2/102
				o->timer = 0;
				
				if (!o->linkedobject)
				{
					if (o->dir==RIGHT) o->xinertia = -0x200;
								  else o->xinertia = 0x200;
				}
				
				o->yinertia = 0;
			}
		}
		break;
		
		case 2:
		case 102:
		{
			FACEPLAYER;
			
			if (o->shaketime)
			{	// fall while hurt
				o->yinertia += 0x20;
				o->xinertia = 0;
			}
			else
			{
				if (!o->linkedobject)
				{	// move towards player
					if (o->x < player->x) o->xinertia += 0x10;
									 else o->xinertia -= 0x10;
					
					if (o->y < player->y) o->yinertia += 0x10;
									 else o->yinertia -= 0x10;
				}
				else
				{	// carrying a skull; don't chase him
					o->state--;		// state 1/101
				}
			}
			
			// bounce off walls
			if (o->xinertia < 0 && o->blockl) o->xinertia = 0x200;
			if (o->xinertia > 0 && o->blockr) o->xinertia = -0x200;
			
			if (o->yinertia < 0 && o->blocku) o->yinertia = 0x200;
			if (o->yinertia > 0 && o->blockd) o->yinertia = -0x200;
			
			LIMITX(0x5ff);
			LIMITY(0x5ff);
		}
		break;
	}
	
	if (++o->animtimer >= 2)
	{
		o->animtimer = 0;
		o->animframe ^= 1;
	}
	
	o->frame = o->animframe;
	if (o->shaketime) o->frame = 4;
}


void ai_crowwithskull(Object *o)
{
Object *skull;

	// create the skullhead we're carrying
	skull = CreateObject(0, 0, OBJ_SKULLHEAD_CARRIED);
	skull->linkedobject = o;
	skull->timer = random(0, 50);
	o->linkedobject = skull;
	
	// switch over to the main crow AI, but only move up & down
	o->yinertia = random(-0x200, -0x100);
	o->xmark = o->x;
	o->ymark = o->y + random(-(28 * CSFI), (10 * CSFI));
	o->animframe = random(0, 1);
	o->animtimer = random(0, 4);
	o->state = 101;
	o->type = OBJ_CROW;
	
	// run the ai for the normal crow for this first frame
	ai_crow(o);
}

/*
void c------------------------------() {}
*/


void ai_skullhead(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			o->speed = 0x100;	// skullhead_carried shares ai but moves faster
			o->state = 1;
			o->timer = random(-5, 0);
		}
		case 1:
		{
			if (++o->timer > 3)
			{
				o->yinertia = -0x350;
				o->state = 2;
				o->frame = 1;
				XMOVE(o->speed);
			}
			else break;
		}
		case 2:
		{
			if (o->blockd)
			{
				o->xinertia = 0;
				o->state = 1;
				o->timer = 0;
				o->frame = 0;
			}
			
			if (o->blocku && o->yinertia < 0) o->yinertia = 0;
			if (o->yinertia > 0) o->frame = 0; else o->frame = 1;
		}
		break;
	}
	
	if (o->xinertia)
	{
		if (o->blockl) { o->dir = RIGHT; o->xinertia = o->speed; }
		if (o->blockr) { o->dir = LEFT; o->xinertia = -o->speed; }
	}
	
	o->yinertia += 0x40;
	LIMITY(0x5ff);
}

void ai_skullhead_carried(Object *o)
{
	// if our crow dies, change into a regular skullhead
	if (!o->linkedobject)
	{
		o->type = OBJ_SKULLHEAD;
		o->state = 2;	// falling
		o->speed = 0x200;
		XMOVE(o->speed);
		ai_skullhead(o);
		return;
	}
	
	switch(o->state)
	{
		case 0:
		case 1:			// mouth closed
		{
			o->frame = 0;
			
			// shoot only when player near
			if ((abs(player->x - o->x) < (130 * CSFI)) &&
				(abs(player->y - o->y) < (100 * CSFI)))
			{
				o->timer++;
			}
			else o->timer = 49;
			
			if (o->timer >= 50)
			{
				o->frame = 1;
				o->state = 2;
				o->timer = 0;
			}
		}
		break;
		
		case 2:			// mouth opened
		{
			o->frame = 1;
			o->timer++;
			
			if ((o->timer==30 || o->timer==35) && o->onscreen)
			{
				EmFireAngledShot(o, OBJ_SKELETON_SHOT, 2, 0x300);
				sound(SND_EM_FIRE);
			}
			else if (o->timer > 50)
			{
				o->frame = 0;
				o->state = 1;
				o->timer = 0;
			}
		}
		break;
	}
}

void aftermove_skullhead_carried(Object *o)
{
Object *crow;

	// keep us in the right spot relative to our crow
	if (o->linkedobject)
	{
		crow = o->linkedobject;
		o->x = crow->x + (1 * CSFI);
		o->y = crow->y + (21 * CSFI);
		o->dir = crow->dir;
	}
}

/*
void c------------------------------() {}
*/

void ai_skeleton_shot(Object *o)
{
	ai_animate2(o);				// animate at 1 frame per 2 ticks
	
	// bounce off walls
	if ((o->blockl && o->xinertia < 0) || (o->blockr && o->xinertia > 0))
	{
		o->xinertia = -o->xinertia;
		o->timer += 5;
	}
	
	// bounce off ceiling
	if ((o->blocku && o->yinertia < 0))
	{
		o->yinertia = -o->yinertia;
		o->timer += 5;
	}
	
	// if hit floor bounce along it...
	if (o->blockd)
	{
		o->yinertia = -0x180;
		o->state = 1;	// begin falling
		o->timer += 4;
	}
	
	if (o->state == 1)
	{
		o->yinertia += 0x10;
		LIMITY(0x5ff);
	}
	
	if (o->timer >= 10)
	{
		effect(o->CenterX(), o->CenterY(), EFFECT_FISHY);
		o->Delete();
	}
}


void ai_armadillo(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			FACEPLAYER;
			o->state = 1;
		}
		case 1:
		{
			if (o->blockl && o->dir == LEFT) o->dir = RIGHT;
			if (o->blockr && o->dir == RIGHT) o->dir = LEFT;
			XMOVE(0x100);
			
			ANIMATE(4, 0, 1);
		}
		break;
	}
	
	o->yinertia += 0x40;
	LIMITY(0x5ff);
}

/*
void c------------------------------() {}
*/

void ai_skullstep(Object *o)
{
Object *foot;
	
	switch(o->state)
	{
		case 0:
		{
			o->y += (7 * CSFI);
			
			// spawn the feet
			foot = CreateObject(0, 0, OBJ_SKULLSTEP_FOOT);
			foot->linkedobject = o;
			
			foot = CreateObject(0, 0, OBJ_SKULLSTEP_FOOT);
			foot->linkedobject = o;
			foot->angleoffset = 128;
			
			o->state = 1;
			o->angle = 200;
		}
		case 1:
		{
			// spin the feet
			o->angle += (o->dir==RIGHT) ? 6 : -6;
			
			// if we hit the ground, slow down. if we have been there for a while,
			// our feet must have gone away, so go to explode state
			if (o->blockd)
			{
				o->xinertia *= 3;
				o->xinertia /= 4;
				if (++o->timer > 60)
				{
					o->state = 2;
					o->timer = 0;
				}
			}
			else o->timer = 0;
			
			// turn around if we run into a wall
			if ((o->dir==RIGHT && o->blockr) || (o->dir==LEFT && o->blockl))
			{
				if (++o->timer2 > 8)
				{
					o->dir ^= 1;
				}
			}
			else o->timer2 = 0;
		}
		break;
		
		case 2:
		{
			o->timer++;
			o->shaketime = o->timer;	// have to set it to timer and not a constant so it alternates
			if (o->timer > 50)
			{
				SmokeClouds(o, 8, 8, 8);
				sound(SND_FUNNY_EXPLODE);
				o->Delete();
			}
		}
		break;
	}
	
	o->yinertia += 0x80;
	if (o->blockd) o->yinertia = 0;
	LIMITX(0x2ff);
	LIMITY(0x2ff);
}


void ai_skullstep_foot(Object *o)
{
Object *skull = o->linkedobject;
uint8_t angle;
int circle_x, circle_y;
	
	// skull was destroyed before us?
	if (!skull || skull->state==2)
	{
		o->Delete();
		return;
	}
	
	angle = skull->angle;
	angle += o->angleoffset;
	
	// handle moving up when stepping on the ground
	if (o->blockd)
		skullstep_do_step(o, skull, angle);
	
	o->dir = skull->dir;
	
	// spin around
	vector_from_angle(angle, (10 * CSFI), &circle_x, &circle_y);
	o->x = skull->x + (3 * CSFI) + circle_x;
	o->y = skull->y + (8 * CSFI) + circle_y;
	
	o->frame = (angle >= 20 && angle <= 108) ? 0:1;
}


// prevent climbing the walls. this didn't seem to be a problem in the original
// game but to me, the feet tend to spin into walls when they're near them and
// "falsely" set blockd resulting in skipping up the wall.
void skullstep_do_step(Object *o, Object *skull, int angle)
{
int x, y;
int i;

	// get the coordinates of the tile that's blocking us
	// we already KNOW we're blockd, so if it returns false, we're standing on a slope
	if (o->CheckAttribute(&sprites[o->sprite].block_d, TA_SOLID_NPC, &x, &y))
	{
		// if the tile above it is also solid, it can't be a floor, it's a wall!
		if (tileattr[map.tiles[x][y-1]] & TA_SOLID_NPC)
		{
			return;
		}
		else
		{
			// it's also a wall if the tile below is solid and neither of the tiles to
			// the left or right are solid (top of a wall)
			if (tileattr[map.tiles[x][y+1]] & TA_SOLID_NPC)
			{
				// we have to check TWO tiles to the right and see if EITHER is nonsolid because
				// of the two-tile wall on the right-lower "arena" slopey part--kind of a hack,
				// i hate to have to put map-specific code in
				if (!(tileattr[map.tiles[x+1][y]] & TA_SOLID_NPC) || \
					!(tileattr[map.tiles[x+2][y]] & TA_SOLID_NPC))
				{
					if (!(tileattr[map.tiles[x-1][y]] & TA_SOLID_NPC))
					{
						return;
					}
				}
			}
		}
	}
	
	// move us up until we're no longer in the ground
	for(i=0;i<10;i++)
	{
		o->y -= (1 * CSFI);
		skull->y -= (1 * CSFI);
		o->UpdateBlockStates(DOWNMASK);
		if (!o->blockd) break;
	}
	
	// now move us down so we're just touching the ground
	o->y += (1 * CSFI);
	skull->y += (1 * CSFI);
	
	// move us in the direction we're facing
	skull->xinertia += (o->dir==RIGHT) ? 0x80 : -0x80;
}

/*
void c------------------------------() {}
*/

void ai_skeleton(Object *o)
{
uint8_t pnear;
#define SKNEAR_X		(352 * CSFI)
#define SKNEAR_BELOW	(160 * CSFI)
#define SKNEAR_ABOVE	(64 * CSFI)
	
	pnear = (pdistlx(SKNEAR_X) && pdistly2(SKNEAR_ABOVE, SKNEAR_BELOW));
	
	switch(o->state)
	{
		case 0:
		{
			o->frame = o->blockd ? 0:1;
			o->xinertia = 0;
			
			// if player comes near, enter attack state
			// also if he shoots us while we're inactive, don't just stand there
			// like a ninny, jump around!
			if (o->shaketime) o->state = 20;
			if (pnear) o->state = 10;
		}
		break;
		
		case 10:	// prepare for jump
			o->timer = o->timer2 = o->frame = o->xinertia = 0;
			o->state = 11;
		case 11:
			if (++o->timer > 5 && o->blockd)
			{
				// jump if player near, else go to idle
				o->state = pnear ? 20:0;
			}
		if (o->state != 20) break;
		
		case 20:
		{
			o->state = 21;
			o->frame = 1;
			o->timer2 = 0;
			o->yinertia = -(random(1, 3) * CSFI);
			
			// jump towards player, unless we've been hurt; in that case jump away
			if (!o->shaketime)
			{
				o->xinertia += (o->x > player->x) ? -0x100:0x100;
			}
			else
			{
				o->xinertia += (o->x > player->x) ? 0x100:-0x100;
			}
		}
		case 21:
		{
			if (o->yinertia > 0)
			{
				if (o->timer2 == 0)
				{
					o->timer2++;
					
					EmFireAngledShot(o, OBJ_SKELETON_SHOT, 0, 0x300);
					if (o->onscreen) sound(SND_EM_FIRE);
				}
				
				if (o->blockd)
				{
					o->state = 10;
					o->frame = 0;
				}
			}
			else
			{
				if (o->blocku) o->yinertia = 0;
			}
		}
		break;
	}
	
	if (o->state >= 10) FACEPLAYER;
	o->yinertia += 0x33;
	LIMITX(0x5ff);
	LIMITY(0x5ff);
}


// curly's mimiga's
void ai_curlys_mimigas(Object *o)
{
static const uint8_t mimiga_walk_frames[5] = { 0, 2, 0, 3 };

	switch(o->state)
	{
		case 0:		// init/set initial anim state
			o->frame = 0;
			
			if (o->type==OBJ_MIMIGAC1) o->state = 100;
			else
			{	// OBJ_MIMIGAC2 & OBJ_MIMIGA_ENEMY
				o->state = 2;			// stand and blink
				
				if (o->type==OBJ_MIMIGAC2)
				{
					if (o->dir == RIGHT)
						o->state = 110;		// sleeping
				}
			}
			
			ai_curlys_mimigas(o);		// re-process again with correct state
		break;
		
		case 2:		// init stand and blink
			o->state = 3;
		case 3:		// stand and blink
			o->frame = 0;
			randblink(o, 1, 8);
		break;
		
		// sitting mimiga (when facing right)
		// facing away mimiga (when facing left)
		case 100:
			o->frame = 4;
		break;
		
		case 110:	// sleeping facing left mimiga
			o->frame = 7;
			ai_zzzz_spawner(o);
		break;
		
		/// ******************** Fighting Mimiga Code ********************
		case 10:
			o->flags |= FLAG_SHOOTABLE;
			
			o->hp = 1000;
			o->state = 11;
			
			o->timer = random(0, 50);
			o->frame = 0;
		//fall thru to state 11
		case 11:
			if (o->timer) o->timer--;
			else o->state = 13;
		break;
		
		case 13:
			o->state = 14;
			o->timer = random(0, 50);
			o->animframe = 0;
			
			if (o->x <= player->x) o->dir = RIGHT;
					          else o->dir = LEFT;
			// fall thru
		case 14:
			if (++o->animtimer > 2)
			{
				o->animtimer = 0;
				if (++o->animframe > 3) o->animframe = 0;
			}
			o->frame = mimiga_walk_frames[o->animframe];
			
			if (o->dir==LEFT) o->xinertia -= 0x40;
						 else o->xinertia += 0x40;
			
			if (o->timer)
			{
				o->timer--;
			}
			else
			{	// enter hop state
				o->state = 15;
				o->frame = 2;
				o->yinertia = -0x200;
				o->damage = 2;
			}
		break;
		
		case 15:	// hopping
			if (o->blockd)
			{	// landed
				o->xinertia = 0;
				o->state = 10;
				o->damage = 0;
			}
		break;
		
		case 20:	// aiiie! got shot!!
			if (o->blockd)
			{
				o->xinertia = 0;
				o->state = 21;
				if (o->frame==6) o->frame = 7;
							else o->frame = 8;
				
				o->timer = random(300, 400);
			}
		break;
		
		case 21:	// lying on ground knocked out
			if (o->timer)
			{
				o->timer--;
				break;
			}
			o->flags |= FLAG_SHOOTABLE;
			o->hp = 1000;
			o->state = 11;
			o->timer = random(0, 50);
			o->frame = 0;
		break;
	}
	
	if (o->state > 10 && o->state < 20 && o->hp != 1000)
	{	// got shot by player
		o->state = 20;
		o->yinertia = -0x200;
		o->frame = random(5, 6);
		
		o->damage = 0;
		o->flags &= ~FLAG_SHOOTABLE;
	}
	
	o->yinertia += 0x20;
	LIMITY(0x5ff);
	LIMITX(0x1ff);
}

/*
void c------------------------------() {}
*/

void ai_beetle_horizwait(Object *o)
{
	enum { FLYING = 0, ON_WALL = 1 };
	
	if (o->state == FLYING)
	{
		if (++o->animtimer == 2)
		{
			o->animtimer = 0;
			o->animframe ^= 1;
		}
		
		o->frame = (o->animframe + 1);
		
		if (o->dir == RIGHT)
		{
			o->xinertia += 0x50;
			if (o->xinertia > 0x32c) o->xinertia = 0x32c;
			
			if (o->blockr)
			{
				o->dir = LEFT;
				o->state = ON_WALL;
				o->frame = 0;
				o->xinertia = 0;
			}
		}
		else
		{
			o->xinertia -= 0x50;
			if (o->xinertia < -0x32c) o->xinertia = -0x32c;
			
			if (o->blockl)
			{
				o->dir = RIGHT;
				o->state = ON_WALL;
				o->frame = 0;
				o->xinertia = 0;
			}
		}
	}
	else
	{	// waiting on wall
		if (abs(o->y - player->y) < (12 * CSFI))
		{
			if ((o->dir == RIGHT && (player->x > o->x)) || \
				(o->dir == LEFT && (player->x < o->x)))
			{
				o->animframe = 0;
				o->state = FLYING;
			}
		}
	}
}

