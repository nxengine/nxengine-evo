#include "balrog_common.h"
#include "stdai.h"
#include "ai.h"
#include "../player.h"
#include "../game.h"
#include "../sound/sound.h"


// some routines used by multiple Balrog boss fights or Balrog NPC's
// ---------------------------------------



// grab the player in preparation for running the toss_player_away animation.
void balrog_grab_player(Object *o)
{
	player->disabled = true;
	o->frame = 8;	// face away
	o->substate = 0;
}

// shake and toss the player away. call balrog_grab_player first.
// returns true when complete.
// used in boss battles in Shack and at end of Labyrinth.
bool balrog_toss_player_away(Object *o)
{
	// keep player locked in position while balrog has him
	if (o->substate <= 1)
	{
		player->x = o->CenterX() - (player->Width() / 2);
		player->y = o->y + (4 * CSFI);
		player->xinertia = player->yinertia = 0;
	}
	
	switch(o->substate)
	{
		case 0:		// slowing to a stop
		{
			o->xinertia *= 4;
			o->xinertia /= 5;
			
			if (o->xinertia == 0)
			{
				o->substate = 1;
				o->timer = 0;
				
				o->animtimer = 0;
				o->frame = 15;
			}
		}
		break;
		
		case 1:		// shaking with back turned
		{
			ANIMATE(2, 15, 16);		// shake
			
			// after a moment toss player away
			// I used 101 because that puts it one frame past the end of the
			// player's hurtflash timer; otherwise when you reappear only your
			// gun is visible for the first frame.
			if (++o->timer > 101)
			{
				player->disabled = false;
				
				if (o->dir == LEFT)
				{
					player->x += 0x800;
					player->xinertia = 0x5ff;
					player->dir = RIGHT;
				}
				else
				{
					player->x -= 0x800;
					player->xinertia = -0x5ff;
					player->dir = LEFT;
				}
				
				player->y -= 0x1000;
				player->yinertia = -0x200;
				sound(SND_FUNNY_EXPLODE);
				
				o->dir = player->dir;
				
				o->substate = 2;
				o->frame = 3;	// arms up
				o->timer = 0;
			}
		}
		break;
		
		case 2:		// arms up after throwing player
		{
			if (++o->timer > 50)
				return true;
		}
		break;
	}

	return false;
}

/*
void c------------------------------() {}
*/

