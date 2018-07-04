
#include "weapons.h"
#include "whimstar.h"
#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../sound/sound.h"

#include "../../player.h"
#include "../../map.h"
#include "../../game.h"
#include "../../graphics/sprites.h"
#include "../../autogen/sprites.h"
using namespace Sprites;


INITFUNC(AIRoutines)
{
	ONTICK(OBJ_WHIMSICAL_STAR, ai_whimsical_star);
}

/*
void c------------------------------() {}
*/

void init_whimstar(WhimsicalStar *wh)
{
	// this offset kickstarts them into orbiting at a sufficient distance
	int px = player->Right();// + 0x1000;
	int py = player->Bottom();// + 0x1000;
	
	for(int i=0;i<MAX_WHIMSTARS;i++)
	{
		wh->stars[i].x = px;
		wh->stars[i].y = py;
	}
}

void add_whimstar(WhimsicalStar *wh)
{
	if (wh->nstars < MAX_WHIMSTARS)
		wh->nstars++;
}

void remove_whimstar(WhimsicalStar *wh)
{
	if (wh->nstars > 0)
		wh->nstars--;
}

void run_whimstar(WhimsicalStar *wh)
{
Whimstar *stars = wh->stars;
int i;

	if (++wh->stariter > 2)
		wh->stariter = 0;
	
	for(i=0;i<MAX_WHIMSTARS;i++)
	{
		int tgtx, tgty;
		
		// first star follows player,
		// subsequent stars follow the previous star
		if (i == 0)
		{
			tgtx = player->CenterX();
			tgty = player->CenterY();
		}
		else
		{
			tgtx = stars[i - 1].x;
			tgty = stars[i - 1].y;
		}
		
		stars[i].xinertia += (stars[i].x < tgtx) ? 0x80 : -0x80;
		stars[i].yinertia += (stars[i].y < tgty) ? 0xAA : -0xAA;
		
		if (stars[i].xinertia > 0xA00)  stars[i].xinertia = 0xA00;
		if (stars[i].xinertia < -0xA00) stars[i].xinertia = -0xA00;
		if (stars[i].yinertia > 0xA00)  stars[i].yinertia = 0xA00;
		if (stars[i].yinertia < -0xA00) stars[i].yinertia = -0xA00;
		
		stars[i].x += stars[i].xinertia;
		stars[i].y += stars[i].yinertia;
		
		if (player->equipmask & EQUIP_WHIMSTAR)
		{
			if (i < wh->nstars && i == wh->stariter && !player->inputs_locked)
			{
				CreateObject(stars[i].x, stars[i].y, OBJ_WHIMSICAL_STAR);
			}
		}
	}
}

void draw_whimstars(WhimsicalStar *wh)
{
Whimstar *stars = wh->stars;
int i;

	for(i=0;i<wh->nstars;i++)
	{
		int scr_x = (stars[i].x / CSFI) - (map.displayed_xscroll / CSFI);
		int scr_y = (stars[i].y / CSFI) - (map.displayed_yscroll / CSFI);
		
		draw_sprite_at_dp(scr_x, scr_y, SPR_WHIMSICAL_STAR, i);
	}
}

/*
void c------------------------------() {}
*/


// invisible trail object left by whimsical star which damages enemies
void ai_whimsical_star(Object *o)
{
	Object *enemy = check_hit_enemy(o);
	if (enemy)
	{
		if (!(enemy->flags & FLAG_INVULNERABLE))
		{
			enemy->DealDelayedDamage(1);
		}
		else
		{
			effect(o->x, o->y, EFFECT_STARSOLID);
			sound(SND_TINK);
		}
		
		o->Delete();
	}
	else if (o->state == 1)
	{
		o->Delete();
	}
	else
	{
		o->invisible = true;
		o->state = 1;
	}
}








