#include "polar_mgun.h"
#include "weapons.h"
#include "../../ObjManager.h"
#include "../../p_arms.h"
#include "../../game.h"
#include "../../sound/sound.h"

// code for both Polar Star _AND_ MGun. The way I handle trails on level 2 & 3 mgun shots
// is a little weird, but it is already written and works ok, so leaving it for now.

// when we hit something, level 2&3 mgun shots enter this state instead of immediately
// destroying themselves so that they act as a "marker" for the trails, so the trails
// know when to delete themselves.
#define STATE_SHOT_HIT				100

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_POLAR_SHOT, ai_polar_shot);
	ONTICK(OBJ_MGUN_L1_SHOT, ai_polar_shot);
	
	ONTICK(OBJ_MGUN_LEADER, ai_polar_shot);
	ONTICK(OBJ_MGUN_TRAIL, ai_mgun_trail);
	
	ONTICK(OBJ_MGUN_SPAWNER, ai_mgun_spawner);
}

/*
void c------------------------------() {}
*/

void ai_polar_shot(Object *o)
{
	// flying/active state (as opposed to mgun leaders being a marker for their trails)
	if (o->state == 0)
	{
		if (damage_enemies(o))
		{
			o->state = STATE_SHOT_HIT;
		}
		else if (IsBlockedInShotDir(o))
		{
			shot_spawn_effect(o, EFFECT_STARSOLID);
			o->state = STATE_SHOT_HIT;
			
			if (!shot_destroy_blocks(o))
			{
				sound(SND_SHOT_HIT);
			}
		}
		else if (--o->shot.ttl < 0)
		{
			shot_spawn_effect(o, EFFECT_STARPOOF);
			o->state = STATE_SHOT_HIT;
		}
	}
	
	// have we hit something? if so, stick around awhile as a marker for
	// any machine-gun layers which may be trailing us.
	if (o->state == STATE_SHOT_HIT)
	{
		// is this a layered machine gun shot? if so, we gotta get fancy
		if (o->type == OBJ_MGUN_LEADER)
		{
			if (o->timer == 0)
			{	// stick around for a frame, visible, at the spot we hit at
				o->xmark = o->xinertia;		// save for next tick
				o->ymark = o->yinertia;
				o->xinertia = o->yinertia = 0;
			}
			else if (o->timer == 1)
			{	// go invisible, and move ourselves one more tick in the direction we
				// were traveling. this ensures that our trails don't disappear in mid-air.
				o->invisible = true;
				
				// I don't know why we need this check. It's finicky and I just
				// discovered it's necessity empirically.
				//if (o->shot.dir == RIGHT || o->shot.dir == DOWN)
				{
				//	o->x += o->xmark;
				//	o->y += o->ymark;
				}
			}
			
			// then wait idle for a moment so any layers trailing us can see where to stop at--
			// this timer value must be greater than the highest possible # of trailing layers
			if (++o->timer > 8)
				o->Delete();
		}
		else
		{
			o->Delete();
		}
	}
}

// the trails of a machine-gun L2 or L3 shot
void ai_mgun_trail(Object *o)
{
	// linkedobject points to the leader
	ASSERT(o->linkedobject != NULL);
	
	// check if the leader has hit something
	if (o->linkedobject->state == STATE_SHOT_HIT)
	{
		bool passed_leader = false;
		
		// dissappear as we pass the point it hit at
		switch(o->shot.dir)
		{
			case RIGHT: passed_leader = ((o->Right() / CSFI) >= (o->linkedobject->Right() / CSFI)); break;
			case LEFT:  passed_leader = ((o->Left() / CSFI) <= (o->linkedobject->Left() / CSFI)); break;
			case UP:    passed_leader = ((o->Top() / CSFI) <= (o->linkedobject->Top() / CSFI)); break;
			case DOWN:  passed_leader = ((o->Bottom() / CSFI) >= (o->linkedobject->Bottom() / CSFI)); break;
		}
		
		if (passed_leader)
			o->Delete();
	}
}

// this invisible object is dropped by the player every time he fires an L2/3 machine-gun shot.
// it spits out the leader, then the trails, one per frame in order, forming the complete
// level 2 or 3 machine-gun blast.
void ai_mgun_spawner(Object *o)
{
Object *shot;
	
	if (!o->timer)
	{	// first layer (leader)
		shot = CreateObject(o->x, o->y, OBJ_MGUN_LEADER);
		o->linkedobject = shot;
	}
	else
	{	// subsequent layers (trail)
		shot = CreateObject(o->x, o->y, OBJ_MGUN_TRAIL);
		shot->linkedobject = o->linkedobject;
	}
	
	// fire next layer
	SetupBullet(shot, o->x, o->y, o->mgun.bultype, o->dir);
	
	// apply the wave
	if (o->dir==UP || o->dir==DOWN)
		shot->xinertia = o->mgun.wave_amt;
	else
		shot->yinertia = o->mgun.wave_amt;
	
	// fire next layer next time
	o->mgun.bultype++;
	if (++o->timer >= o->mgun.nlayers) o->Delete();
}

/*
void c------------------------------() {}
*/


