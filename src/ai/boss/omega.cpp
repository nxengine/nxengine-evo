#include "omega.h"

#include "../stdai.h"
#include "../ai.h"
#include "../sym/smoke.h"
#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../player.h"
#include "../../tsc.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"

#include "../../game.h"
#include "../../screeneffect.h"
#include "../../graphics/sprites.h"
#include "../../graphics/tileset.h"
#include "../../autogen/sprites.h"


#define OMEGA_RISE_HEIGHT			48
#define OMEGA_SINK_DEPTH			60
#define OMEGA_WAIT_TIME				7
#define OMEGA_SPEED					(1 * CSFI)

#define OMG_APPEAR					20  // this MUST be 20 because misery sets this to begin the battle
#define OMG_WAIT					30
#define OMG_MOVE					40
#define OMG_JAWS_OPEN				50
#define OMG_FIRE					60
#define OMG_JAWS_CLOSE				70
#define OMG_UNDERGROUND				80
#define OMG_JUMP					90
#define OMG_EXPLODING				100	// start fancy victory animation
#define OMG_EXPLODED				110 // full-screen flash in progress

#define LEGD_MIN				(25 * CSFI)
#define LEGD_MAX				(46 * CSFI)

#define OMEGA_DAMAGE			20
#define HP_TRIGGER_POINT		280

enum Pieces
{
	LEFTLEG, RIGHTLEG,
	LEFTSTRUT, RIGHTSTRUT,
	
	NUM_PIECES
};

INITFUNC(AIRoutines)
{
	ONDEATH(OBJ_OMEGA_BODY, ondeath_omega_body);
	ONTICK(OBJ_OMEGA_SHOT, ai_omega_shot);
}

/*
void c------------------------------() {}
*/

void OmegaBoss::OnMapEntry(void)
{
	memset(&omg, 0, sizeof(omg));
	memset(pieces, 0, sizeof(pieces));
	
	omg.form = 1;
	omg.firefreq = 3;
	omg.shotxspd = 0x100;
	omg.startfiring = 20;
	omg.stopfiring = 80;
	omg.endfirestate = 200;
	omg.movetime = OMEGA_RISE_HEIGHT;
	
	// *MUST* create in this order so that the z-order is correct
	pieces[LEFTLEG] = CreateObject(0, 0, OBJ_OMEGA_LEG);
	pieces[RIGHTLEG] = CreateObject(0, 0, OBJ_OMEGA_LEG);
	pieces[LEFTSTRUT] = CreateObject(0, 0, OBJ_OMEGA_STRUT);
	pieces[RIGHTSTRUT] = CreateObject(0, 0, OBJ_OMEGA_STRUT);
	
	game.stageboss.object = CreateObject(0, 0, OBJ_OMEGA_BODY);
	game.stageboss.object->hp = omg.lasthp = 400;
	
	game.stageboss.object->flags |= FLAG_SHOW_FLOATTEXT;
	game.stageboss.object->flags &= ~FLAG_SOLID_MUSHY;
	game.stageboss.object->sprite = SPR_OMG_CLOSED;
	
	objprop[OBJ_OMEGA_BODY].shaketime = 0;		// we do our own shaketime
	
	pieces[LEFTLEG]->dir = LEFT;
	pieces[RIGHTLEG]->dir = RIGHT;
	pieces[LEFTSTRUT]->dir = LEFT;
	pieces[RIGHTSTRUT]->dir = RIGHT;
	
	omg.leg_descend = LEGD_MIN;
	pieces[LEFTLEG]->sprite = pieces[RIGHTLEG]->sprite = SPR_OMG_LEG_INAIR;
	
	game.stageboss.object->x = ((217 * TILE_W) + 5) * CSFI;
	game.stageboss.object->y = ((14 * TILE_H) - 5) * CSFI;
	omg.orgx = game.stageboss.object->x;
	omg.orgy = game.stageboss.object->y;
	
	omg.shaketimer = 0;
}

void OmegaBoss::OnMapExit(void)
{
	if (game.stageboss.object)
		game.stageboss.object->Delete();
}

/*
void c------------------------------() {}
*/

void OmegaBoss::Run(void)
{
	Object *&o = game.stageboss.object;
	
	if (omg.defeated)
		return;
	
	switch(o->state)
	{
		case 0:	break;	// waiting for trigger by script
		
		case OMG_WAIT:	// waits for a moment then go to omg.nextstate
		{
			o->state++;
			omg.timer = 0;
		}
		case OMG_WAIT+1:
		{
			if (++omg.timer >= OMEGA_WAIT_TIME)
			{
				omg.timer = 0;
				o->state = omg.nextstate;
			}
		}
		break;
		
		case OMG_APPEAR:
		{
			omg.timer = 0;
			o->frame = 0;
			o->state = OMG_MOVE;
			omg.movedir = -OMEGA_SPEED;
			o->flags |= FLAG_SOLID_MUSHY;
		}
		case OMG_MOVE:	// rising up/going back into ground
		{
			o->frame = 0;
			o->y += omg.movedir;
			
			game.quaketime = 2;
			
			omg.timer++;
			if ((omg.timer & 3) == 0) sound(SND_QUAKE);
			
			if (omg.timer >= omg.movetime)
			{
				if (omg.movedir < 0)
				{	// was rising out of ground
					omg.nextstate = OMG_JAWS_OPEN;
					o->state = OMG_WAIT;
				}
				else
				{	// was going back into ground
					omg.timer = 0;
					o->state = OMG_UNDERGROUND;
					o->flags &= ~(FLAG_SOLID_MUSHY | FLAG_SOLID_BRICK);
				}
			}
		}
		break;
		
		case OMG_JAWS_OPEN:			// jaws opening
		{
			o->state++;
			omg.animtimer = 0;
			sound(SND_JAWS);
			o->sprite = SPR_OMG_OPENED;			// select "open" bounding box
		}
		case OMG_JAWS_OPEN+1:
		{
			omg.animtimer++;
			if (omg.animtimer > 2)
			{
				omg.animtimer = 0;
				o->frame++;
				if (o->frame==3)
				{
					o->state = OMG_FIRE;
					omg.firecounter = 0;
					o->flags |= FLAG_SHOOTABLE;
				}
			}
		}
		break;
		
		case OMG_FIRE:	// throwing out red stuff
		{
			omg.firecounter++;
			
			if (omg.firecounter > omg.startfiring && omg.firecounter < omg.stopfiring)
			{
				if ((omg.firecounter % omg.firefreq)==0)
				{
					Object *shot;
					
					sound(SND_EM_FIRE);
					
					shot = SpawnObjectAtActionPoint(o, OBJ_OMEGA_SHOT);
					shot->xinertia = random(-omg.shotxspd, omg.shotxspd);
					shot->yinertia = -0x333;
					if (omg.form==2 || random(0, 9) < 8)
					{
						shot->sprite = SPR_OMG_BULLET_NORMAL;
						shot->flags = FLAG_SHOOTABLE;
					}
					else
					{
						shot->sprite = SPR_OMG_BULLET_HARD;
						shot->flags = (FLAG_SHOOTABLE | FLAG_INVULNERABLE);
					}
					
					shot->timer = (random(0, 7) >= 4) ? random(300, 400):0;
					shot->damage = 4;
				}
			}
			else if (omg.firecounter >= omg.endfirestate || sound_is_playing(SND_MISSILE_HIT))
			{	// snap jaws shut
				omg.animtimer = 0;
				o->state = OMG_JAWS_CLOSE;
				sound(SND_JAWS);
			}
		}
		break;
		
		case OMG_JAWS_CLOSE:	// jaws closing
		{
			omg.animtimer++;
			if (omg.animtimer > 2)
			{
				omg.animtimer = 0;
				
				o->frame--;
				if (o->frame == 0)
				{
					sound_stop(SND_JAWS);
					sound(SND_BLOCK_DESTROY);
					
					o->sprite = SPR_OMG_CLOSED;		// select "closed" bounding box
					
					o->flags &= ~FLAG_SHOOTABLE;
					o->damage = 0;
					
					if (omg.form == 1)
					{	// form 1: return to sand
						o->state = OMG_WAIT;
						omg.nextstate = OMG_MOVE;
						omg.movedir = OMEGA_SPEED;
						omg.movetime = OMEGA_SINK_DEPTH;
					}
					else
					{	// form 2: jump
						sound(SND_FUNNY_EXPLODE);
						if (o->x < player->x) o->xinertia = 0xC0;
										 else o->xinertia = -0xC0;
						o->state = OMG_JUMP;
						o->yinertia = -0x5FF;
						omg.orgy = o->y;
					}
				}
			}
			
			// hurt player if he was standing in the middle when the jaws shut
			if (player->riding == o)
			{
				hurtplayer(OMEGA_DAMAGE);
			}
		}
		break;
		
		case OMG_UNDERGROUND:		// underground waiting to reappear
		{
			if (++omg.timer >= 120)
			{
				omg.timer = 0;
				o->state = OMG_APPEAR;
				
				o->x = omg.orgx + (random(-64, 64) * CSFI);
				o->y = omg.orgy;
				omg.movetime = OMEGA_RISE_HEIGHT;
				
				// switch to jumping out of ground when we get low on life
				if (omg.form==1 && o->hp <= HP_TRIGGER_POINT)
				{
					o->flags |= FLAG_SOLID_MUSHY;
					
					omg.form = 2;
					omg.firefreq = 5;
					omg.shotxspd = 0x155;
					omg.startfiring = 0;
					omg.stopfiring = 30;
					omg.endfirestate = 50;
					omg.movetime = OMEGA_RISE_HEIGHT+3;
				}
			}
		}
		break;
		
		case OMG_JUMP:	// init for jump
		{
			omg.orgy = o->y;
			o->state++;
			omg.timer = 0;
		}
		case OMG_JUMP+1:	// jumping
		{
			o->yinertia += 0x24;
			if (o->yinertia > 0x5ff) o->yinertia = 0x5ff;
			
			if (o->yinertia > 0)
			{	// coming down
				
				pieces[LEFTLEG]->sprite = pieces[RIGHTLEG]->sprite = SPR_OMG_LEG_ONGROUND;
				
				// retract legs a little when we hit the ground
				if (pieces[LEFTLEG]->blockd || pieces[RIGHTLEG]->blockd)
				{
					o->xinertia = 0;
					omg.leg_descend -= o->yinertia;
					if (++omg.timer >= 3)
					{
						o->yinertia = 0;
						o->state = OMG_JAWS_OPEN;
					}
				}
				
				// --- squash player if we land on him -------------
				// if top of player is higher than bottom of our bounding box
				// but bottom of player's bounding box is not...
				if (player->blockd)
				{
					int omg_bottom = o->y + (sprites[o->sprite].solidbox.y2 * CSFI);
					if (player->y <= omg_bottom)
					{
						if (player->y + (sprites[player->sprite].solidbox.y2 * CSFI) >= omg_bottom)
						{
							if (hitdetect(o, player))	// easy way to verify the X's are lined up
							{	// SQUISH!
								hurtplayer(OMEGA_DAMAGE);
							}
						}
					}
				}
			}
			else
			{	// jumping up; extend legs
				omg.leg_descend = (omg.orgy - o->y) + LEGD_MIN;
				if (omg.leg_descend > LEGD_MAX) omg.leg_descend = LEGD_MAX;
				pieces[LEFTLEG]->sprite = pieces[RIGHTLEG]->sprite = SPR_OMG_LEG_INAIR;
			}
		}
		break;
		
		/// victory
		case OMG_EXPLODING:
		{
			omg.timer = 0;
			o->state++;
		}
		case OMG_EXPLODING+1:
		{
			int x, y;
			
			o->xinertia = o->yinertia = 0;
			
			x = o->CenterX() + (random(-48, 48) * CSFI);
			y = o->CenterY() + (random(-48, 24) * CSFI);
			SmokePuff(x, y);
			effect(x, y, EFFECT_BOOMFLASH);
			
			game.quaketime = 2;
			
			if ((omg.timer % 12)==0) sound(SND_ENEMY_HURT_BIG);
			
			if (++omg.timer > 100)
			{
				omg.timer = 0;
				starflash.Start(o->CenterX(), o->CenterY());
				o->state = OMG_EXPLODED;
			}
			else if (omg.timer==24)
			{
				StartScript(210);
			}
		}
		break;
		
		case OMG_EXPLODED:
		{
			game.quaketime = 40;
			
			if (++omg.timer > 50)
			{
				o->Delete();
				for(int i=0;i<NUM_PIECES;i++)
					pieces[i]->Delete();
				
				omg.defeated = true;
				return;
			}
		}
		break;
	}
	
	// implement shaking when shot
	// we do it manually instead of used the usual shared code
	// because we want all the pieces to shake at once
	if (o->hp != omg.lasthp && !omg.shaketimer)
	{
		omg.shaketimer = 3;
		// why did I write this? anyway, I'm sure it's important
		if (o->x > player->x) o->display_xoff = -1;
						 else o->display_xoff = 1;
		
		omg.lasthp = o->hp;
	}
	if (omg.shaketimer)
	{
		int xoff = -o->display_xoff;
		
		if (!--omg.shaketimer) xoff = 0;
		
		o->display_xoff = xoff;
		pieces[LEFTLEG]->display_xoff = xoff;
		pieces[RIGHTLEG]->display_xoff = xoff;
		pieces[LEFTSTRUT]->display_xoff = xoff;
		pieces[RIGHTSTRUT]->display_xoff = xoff;
	}
	
	if (o->state)
	{
		o->blockl |= pieces[LEFTLEG]->blockl;
		o->blockr |= pieces[RIGHTLEG]->blockr;
		
		pieces[LEFTLEG]->x = o->x - (4 * CSFI); pieces[LEFTLEG]->y = o->y + omg.leg_descend;
		pieces[RIGHTLEG]->x = o->x + (38 * CSFI); pieces[RIGHTLEG]->y = o->y + omg.leg_descend;
		pieces[LEFTSTRUT]->x = o->x + (9 * CSFI); pieces[LEFTSTRUT]->y = o->y + (27 * CSFI);
		pieces[RIGHTSTRUT]->x = o->x + (43 * CSFI); pieces[RIGHTSTRUT]->y = o->y + (27 * CSFI);
	}
}

void ondeath_omega_body(Object *o)
{
	o->flags &= ~FLAG_SHOOTABLE;
	KillObjectsOfType(OBJ_OMEGA_SHOT);
	
	game.stageboss.SetState(OMG_EXPLODING);
}

/*
void c------------------------------() {}
*/

void ai_omega_shot(Object *o)
{
	o->nxflags |= NXFLAG_FOLLOW_SLOPE;
	
	o->yinertia += 5;
	if (o->blockd) o->yinertia = -0x100;
	
	if (o->blockl || o->blockr) o->xinertia = -o->xinertia;
	if (o->blocku) o->yinertia = -o->yinertia;
	
	if (++o->animtimer > 2) { o->frame ^= 1; o->animtimer = 0; }
	
	if (++o->timer > 750)
	{
		effect(o->CenterX(), o->CenterY(), EFFECT_FISHY);
		o->Delete();
	}
}

