#include "undead_core.h"

#include "../stdai.h"
#include "../ai.h"
#include "ballos.h"
#include "../sym/smoke.h"
#include "../../game.h"
#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../trig.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"

#include "../../player.h"
#include "../../map.h"
#include "../../graphics/sprites.h"
#include "../../autogen/sprites.h"
#include "../../screeneffect.h"

static struct
{
	SIFPoint offset;	// offset from main object
	SIFRect rect;		// actual bbox rect
}
core_bboxes[] =
{
	{  { 0, -32 },  { -40, -16, 40, 16 }  },	// upper
	{  { 28, 0 },   { -36, -24, 36, 24 }  },	// back/main body
	{  { 4, 32 },   { -44, -8, 44, 8 }    },	// lower
	{  { -28, 4 },  { -20, -20, 20, 20 }  }		// shoot target
};

enum CORE_STATES
{
	CR_FightBegin		= 20,		// scripted
	CR_FaceClosed		= 200,
	CR_FaceSkull		= 210,
	CR_FaceTeeth		= 220,
	CR_FaceDoom			= 230,
	
	CR_Defeated			= 500,		// scripted
	CR_Exploding		= 1000
};

enum FACE_STATES
{
	FC_Closed		= 0,
	FC_Skull		= 1,
	FC_Teeth		= 2,
	FC_Mouth		= 3
};

enum ROTR_STATES
{
	RT_Spin_Closed			= 10,
	RT_Spin_Open			= 20,
	RT_Spin_Slow_Closed		= 30,
	RT_Spin_Fast_Closed		= 40
};

/*
void c------------------------------() {}
*/

INITFUNC(AIRoutines)
{
	ONSPAWN(OBJ_UD_MINICORE_IDLE, onspawn_ud_minicore_idle);
	ONTICK(OBJ_UDMINI_PLATFORM, ai_udmini_platform);
	
	ONTICK(OBJ_UD_PELLET, ai_ud_pellet);
	ONTICK(OBJ_UD_SMOKE, ai_ud_smoke);
	
	ONTICK(OBJ_UD_SPINNER, ai_ud_spinner);
	ONTICK(OBJ_UD_SPINNER_TRAIL, ai_ud_spinner_trail);
	
	ONTICK(OBJ_UD_BLAST, ai_ud_blast);
}

/*
void c------------------------------() {}
*/

/*
	Main core body:
		non-shootable when closed
		invulnerable when open
		starsolid's appear just behind the little dots
		
		when you shoot the face the starsolid hits at just past his nose.
		
		when face is open it does block the face from shooting below,
		but there is still a spot that you can hurt it from below.
		
		four always-dark minicores that spin around it, they also seem
		to be switching their z-order.
*/

void UDCoreBoss::OnMapEntry()
{
Object *o;

	// main object
	o = CreateObject(0, 0, OBJ_UDCORE_MAIN);
	main = o;
	game.stageboss.object = o;
	
	o->sprite = SPR_NULL;//SPR_MARKER;
	objprop[o->type].hurt_sound = SND_CORE_HURT;
	
	o->hp = 700;
	o->x = (592 * CSFI);
	o->y = (120 * CSFI);
	o->id2 = 1000;	// defeated script
	o->flags = (FLAG_SHOW_FLOATTEXT | FLAG_IGNORE_SOLID | FLAG_SCRIPTONDEATH);
	
	// create rear rotators
	rotator[2] = create_rotator(0, 1);
	rotator[3] = create_rotator(0x80, 1);
	
	// create front & back
	front = CreateObject(0, 0, OBJ_UDCORE_FRONT);
	back = CreateObject(0, 0, OBJ_UDCORE_BACK);
	
	// create face
	face = CreateObject(0, 0, OBJ_UDCORE_FACE);
	face->state = FC_Closed;

	// create front rotators
	rotator[0] = create_rotator(0, 0);
	rotator[1] = create_rotator(0x80, 0);
	
	// initilize bboxes
	for(int i=0;i<NUM_BBOXES;i++)
	{
		bbox[i] = CreateObject(0, 0, OBJ_UDMINI_BBOX);
		bbox[i]->sprite = SPR_BBOX_PUPPET_1 + i;
		bbox[i]->hp = 1000;
		
		sprites[bbox[i]->sprite].bbox = core_bboxes[i].rect;
	}
	
	//o->BringToFront();
}

void UDCoreBoss::OnMapExit()
{
	main = NULL;
	game.stageboss.object = NULL;
}

/*
void c------------------------------() {}
*/

static void CreateSpinner(int x, int y)
{
	CreateObject(x, y, OBJ_UD_SPINNER);
	CreateObject(x, y, OBJ_UD_SPINNER)->angle = 0x80;
}


void UDCoreBoss::Run(void)
{
	Object *o = main;
	if (!o) return;

	if (RunDefeated())
		return;
	
	switch(o->state)
	{
		// fight begin (scripted)
		case CR_FightBegin:
		{
			o->state = CR_FaceSkull;
			o->dir = LEFT;
			
			SetRotatorStates(RT_Spin_Slow_Closed);
			SpawnFaceSmoke();
		}
		break;
		
		// face closed
		case CR_FaceClosed:
		{
			o->state++;
			o->timer = 0;
			
			face->state = FC_Closed;
			front->frame = 2;		// closed
			back->frame = 0;		// not orange
			
			set_bbox_shootable(false);
			SetRotatorStates(RT_Spin_Closed);
			SpawnFaceSmoke();
		}
		case CR_FaceClosed+1:
		{
			o->timer++;
			
			if (o->dir == RIGHT || o->frame > 0 || o->hp < 200)
			{
				if (o->timer > 200)
				{
					o->timer2++;
					sound(SND_CORE_THRUST);
					
					// select attack mode
					if (o->hp < 200)
					{
						o->state = CR_FaceDoom;
					}
					else if (o->timer2 > 2)
					{
						o->state = CR_FaceTeeth;
					}
					else
					{
						o->state = CR_FaceSkull;
					}
				}
			}
		}
		break;
		
		// face open/skull
		case CR_FaceSkull:
		{
			o->state++;
			o->timer = 0;
			
			face->state = FC_Skull;
			SpawnFaceSmoke();
			
			o->savedhp = o->hp;
			set_bbox_shootable(true);
		}
		case CR_FaceSkull+1:
		{
			o->timer++;
			RunHurtFlash(o->timer);
			
			if (o->timer < 300)
			{
				if ((o->timer % 120) == 1)
				{
					SpawnPellet(UP);
				}
				
				if ((o->timer % 120) == 61)
				{
					SpawnPellet(DOWN);
				}
			}
			
			if (o->timer > 400 || (o->savedhp - o->hp) > 50)
			{
				o->state = CR_FaceClosed;
			}
		}
		break;
		
		// face open/teeth
		case CR_FaceTeeth:
		{
			o->state++;
			o->timer = 0;
			
			face->state = FC_Teeth;
			SpawnFaceSmoke();
			
			SetRotatorStates(RT_Spin_Open);
			game.quaketime = 100;
			
			o->savedhp = o->hp;
			set_bbox_shootable(true);
		}
		case CR_FaceTeeth+1:
		{
			o->timer++;
			RunHurtFlash(o->timer);
			
			// fire rotators
			if ((o->timer % 40) == 1)
			{
				int i = random(0, 3);
				int x = rotator[i]->x - (16 * CSFI);
				int y = rotator[i]->y;
				
				sound(SND_FUNNY_EXPLODE);
				CreateSpinner(x, y);
			}
			
			if (o->timer > 400 || (o->savedhp - o->hp) > 150 || o->hp < 200)
			{
				o->state = CR_FaceClosed;
			}
		}
		break;
		
		// face open/mouth: blasts of doom
		case CR_FaceDoom:
		{
			o->state++;
			o->timer = 0;
			
			face->state = FC_Mouth;
			SpawnFaceSmoke();
			SetRotatorStates(RT_Spin_Fast_Closed);
			
			sound(SND_FUNNY_EXPLODE);
			
			// spawn a whole bunch of crazy spinners from the face
			CreateSpinner(face->x - (16 * CSFI), face->y);
			CreateSpinner(face->x, face->y - (16 * CSFI));
			CreateSpinner(face->x, face->y + (16 * CSFI));
			
			o->savedhp = o->hp;
			set_bbox_shootable(true);
		}
		case CR_FaceDoom+1:
		{
			o->timer++;
			RunHurtFlash(o->timer);
			
			if ((o->timer % 120) == 1)
				SpawnPellet(UP);
			
			if ((o->timer % 120) == 61)
				SpawnPellet(DOWN);
		}
		break;
	}
	
	// move back and forth
	if (o->state >= CR_FightBegin && o->state < CR_Defeated)
	{
		if (o->x < MAPX(12))
			o->dir = RIGHT;
		
		if (o->x > MAPX(map.xsize - 4))
			o->dir = LEFT;
		
		XACCEL(4);
	}
	
	// spawn minicore platforms
	switch(o->state)
	{
		case CR_FaceClosed+1:
		case CR_FaceTeeth+1:
		case CR_FaceSkull+1:
		case CR_FaceDoom+1:
		{
			// while I don't think there's any way to get her there without
			// a map editor, if you put Curly in the Black Space core room,
			// she WILL fight the core, just as she did the first time.
			if (o->state != 221 && (o->timer % 100) == 1)
				bbox[BB_TARGET]->CurlyTargetHere();
			
			o->timer3++;
			
			// upper platforms
			if (o->timer3 == 75)
			{
				CreateObject(MAPX(map.xsize) + 40, \
							 MAPY(3 + random(-3, 0)), OBJ_UDMINI_PLATFORM);
			}
			
			// lower platforms
			if (o->timer3 == 150)
			{
				o->timer3 = 0;
				CreateObject(MAPX(map.xsize) + 40, \
							 MAPY(10 + random(-1, 3)), OBJ_UDMINI_PLATFORM);
				
				break;
			}
		}
		break;
	}
	
	LIMITX(0x80);
	LIMITY(0x80);
}


void UDCoreBoss::RunAftermove()
{
int i;

	Object *o = main;
	if (!o) return;
	
	run_face(face);
	run_front(front);
	run_back(back);
	
	for(i=0;i<4;i++)
		run_rotator(rotator[i]);
	
	move_bboxes();
}


// spawn smoke puffs from face that come when face opens/closes
void UDCoreBoss::SpawnFaceSmoke()
{
	quake(20);
	
	for(int i=0;i<8;i++)
	{
		int x = face->x + random(-16 * CSFI, 32 * CSFI);
		int y = main->CenterY();
		Object *s = SmokePuff(x, y);
		s->xinertia = random(-0x200, 0x200);
		s->yinertia = random(-0x100, 0x100);
	}
}

// spit a "pellet" shot out of the face. That's what I'm calling the flaming lava-rock
// type things that are thrown out and trail along the ceiling or floor.
void UDCoreBoss::SpawnPellet(int dir)
{
	int y = main->y;
	
	if (dir == UP)
		y -= (16 * CSFI);
	else
		y += (16 * CSFI);
	
	CreateObject(main->x - (32 * CSFI), y, OBJ_UD_PELLET)->dir = dir;
}


void UDCoreBoss::RunHurtFlash(int timer)
{
	if (main->shaketime && (timer & 2))
	{
		front->frame = 1;
		back->frame = 1;
	}
	else
	{
		front->frame = 0;
		back->frame = 0;
	}
}

/*
void c------------------------------() {}
*/

bool UDCoreBoss::RunDefeated()
{
	Object *o = main;
	
	switch(o->state)
	{
		// defeated (descending)
		case CR_Defeated:
		{
			o->state++;
			o->timer = 0;
			o->xinertia = 0;
			o->yinertia = 0;
			
			face->state = FC_Closed;
			front->frame = 0;		// front closed
			back->frame = 0;		// not flashing
			SetRotatorStates(RT_Spin_Slow_Closed);
			
			game.quaketime = 20;
			SmokeXY(o->x, o->y, 100, 128, 64);
			
			KillObjectsOfType(OBJ_UDMINI_PLATFORM);
			set_bbox_shootable(false);
		}
		case CR_Defeated+1:
		{
			SmokeXY(o->x, o->y, 1, 64, 32);
			
			o->xinertia = 0x40;
			o->yinertia = 0x80;
			
			if (++o->timer > 200)
			{
				o->state = CR_Exploding;
				o->xinertia = 0;
				o->yinertia = 0;
				o->timer = 0;
			}
		}
		break;
		
		// defeated (exploding)
		case CR_Exploding:
		{
			quake(100, 0);
			o->timer++;
			
			if ((o->timer % 8) == 0)
				sound(SND_MISSILE_HIT);
			
			int x = o->x + random(-72 * CSFI, 72 * CSFI);
			int y = o->y + random(-64 * CSFI, 64 * CSFI);
			SmokePuff(x, y);
			effect(x, y, EFFECT_BOOMFLASH);
			
			if (o->timer > 100)
			{
				sound(SND_EXPLOSION1);
				starflash.Start(o->x, o->y);
				
				o->state++;
				o->timer = 0;
			}
		}
		break;
		case CR_Exploding+1:
		{
			game.quaketime = 40;
			if (++o->timer > 50)
			{
				KillObjectsOfType(OBJ_MISERY_MISSILE);
				
				front->Delete();
				back->Delete();
				face->Delete();
				for(int i=0;i<NUM_ROTATORS;i++) rotator[i]->Delete();
				for(int i=0;i<NUM_BBOXES;i++) bbox[i]->Delete();
				main->Delete();
				main = NULL;
				
				return 1;
			}
		}
		break;
	}
	
	return 0;
}


/*
void c------------------------------() {}
*/

void UDCoreBoss::run_face(Object *o)
{
	o->sprite = SPR_UD_FACES;
	o->invisible = false;
	
	switch(o->state)
	{
		// to "show" the closed face, we go invisible and the
		// face area of the main core shows through.
		case FC_Closed: o->invisible = true; break;
		case FC_Skull: o->frame = 0; break;
		case FC_Teeth: o->frame = 1; break;
		
		// mouth blasts of doom. Once started, it's perpetual blasting
		// until told otherwise.
		case FC_Mouth:
		{
			o->state++;
			o->timer = 100;
		}
		case FC_Mouth+1:
		{
			if (++o->timer > 300)
				o->timer = 0;
			
			if (o->timer > 250)
			{
				if ((o->timer % 16) == 1)
					sound(SND_QUAKE);
				
				if ((o->timer % 16) == 7)
				{
					CreateObject(o->x, o->y, OBJ_UD_BLAST);
					sound(SND_LIGHTNING_STRIKE);
				}
			}
			
			if (o->timer == 200)
				sound(SND_CORE_CHARGE);
			
			if (o->timer >= 200 && (o->timer & 1))
				o->frame = 3;	// mouth lit
			else
				o->frame = 2;	// mouth norm
		}
		break;
	}
	
	o->x = main->x - (36 * CSFI);
	o->y = main->y - (4 * CSFI);
}


void UDCoreBoss::run_front(Object *o)
{
	// 0 open (should make a face visible at the same time to go into the gap)
	// 1 open/hurt
	// 2 closed
	
	switch(o->state)
	{
		case 0:
		{
			o->sprite = SPR_UD_FRONT;
			o->state = 1;
			o->frame = 2;
		}
		case 1:
		{
			o->x = main->x - (36 * CSFI);
			o->y = main->y;
		}
		break;
	}
}


void UDCoreBoss::run_back(Object *o)
{
	// 0 normal
	// 1 hurt
	
	switch(o->state)
	{
		case 0:
		{
			o->sprite = SPR_UD_BACK;
			o->state = 1;
			o->frame = 0;
		}
		case 1:
		{
			o->x = main->x + (44 * CSFI);
			o->y = main->y;
		}
		break;
	}
}


/*
void c------------------------------() {}
*/

// "front" refers to whether they are doing the front (left) or rear (right)
// half of the arc; the ones marked "front" are actually BEHIND the core.
Object *UDCoreBoss::create_rotator(int angle, int front)
{
	Object *o = CreateObject(0, 0, OBJ_UDMINI_ROTATOR);
	o->angle = angle;
	o->substate = front;
	
	return o;
}

// the rotators are 4 minicores that spin around the main core during the battle
// and have pseudo-3D effects. They also shoot the spinners during the teeth-face phase.
//
// instead of having the cores constantly rearranging their Z-Order as they pass
// in front and behind the core, an optical illusion is used. 2 cores are always
// in front and 2 are always behind. Each set of two cores covers only half the full
// circle. When a core in the front set reaches the top, it warps back to the bottom
// just as a core in the back set reaches the bottom and warps back to the top.
// Thus, they swap places and the core appears to continue around the circle using
// the different z-order of the one that was just swapped-in.
void UDCoreBoss::run_rotator(Object *o)
{
	//debug("rotr s%d", o->state);
	
	switch(o->state)
	{
		case 0:
		{
			o->sprite = SPR_UD_ROTATOR;
			o->flags &= ~FLAG_SHOOTABLE;
			o->hp = 1000;
		}
		break;
		
		case RT_Spin_Closed:
		{
			o->frame = 0;
			o->angle += 2;
		}
		break;
		
		// used when firing spinners in Teeth face
		// (it's easier to coordinate if spinners are actually spawned by core
		// and just positioned next to us)
		case RT_Spin_Open:
		{
			o->frame = 1;
			o->angle += 2;
		}
		break;
		
		case RT_Spin_Slow_Closed:
		{
			o->frame = 0;
			o->angle++;
		}
		break;
		
		case RT_Spin_Fast_Closed:
		{
			o->frame = 0;
			o->angle += 4;
		}
		break;
	}
	
	// each "side" covers half the rotation angle
	int angle = (o->angle / 2);
	
	if (o->substate)
	{	// front (left) half of arc
		angle += 0x40;
	}
	else
	{	// back (right) half of arc
		angle += 0xC0;
	}
	
	o->x = (main->x - (8 * CSFI)) + xinertia_from_angle(angle, (48 * CSFI));
	o->y = main->y + yinertia_from_angle(angle, (80 * CSFI));
}

void UDCoreBoss::SetRotatorStates(int newstate)
{
	for(int i=0;i<NUM_ROTATORS;i++)
		rotator[i]->state = newstate;
}

/*
void c------------------------------() {}
*/

// extra bbox puppets/shoot targets
// only one, located at the face, is shootable, the other 3 are invulnerable shields.
void UDCoreBoss::move_bboxes()
{
	for(int i=0;i<NUM_BBOXES;i++)
	{
		bbox[i]->x = main->x + (core_bboxes[i].offset.x * CSFI);
		bbox[i]->y = main->y + (core_bboxes[i].offset.y * CSFI);
	}
	
	transfer_damage(bbox[BB_TARGET], main);
}

// sets up bboxes for the Core entering shootable or non-shootable mode.
void UDCoreBoss::set_bbox_shootable(bool enable)
{
uint32_t body_flags, target_flags;
int i;

	// in shootable mode target can be hit and shields are up.
	// in non-shootable mode (when face is closed) nothing can be hit.
	if (enable)
	{
		body_flags = FLAG_INVULNERABLE;
		target_flags = FLAG_SHOOTABLE;
	}
	else
	{
		body_flags = 0;
		target_flags = 0;
	}
	
	for(i=0;i<NUM_BBOXES;i++)
	{
		bbox[i]->flags &= ~(FLAG_SHOOTABLE | FLAG_INVULNERABLE);
		
		if (i == BB_TARGET)
			bbox[i]->flags |= target_flags;
		else
			bbox[i]->flags |= body_flags;
	}
}

/*
void c------------------------------() {}
*/

// minicores by entrance seen before fight
void onspawn_ud_minicore_idle(Object *o)
{
	if (o->dir == RIGHT)
		o->flags &= ~FLAG_SOLID_BRICK;
}

// these are the ones you can ride
void ai_udmini_platform(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			o->state = 1;
			o->ymark = o->y;
			
			o->xinertia = -0x200;
			o->yinertia = 0x100;
			if (random(0, 1)) o->yinertia = -o->yinertia;
		}
		case 1:
		{
			if (o->x < -(64 * CSFI))
				o->Delete();
			
			if (o->y > o->ymark) o->yinertia -= 0x10;
			if (o->y < o->ymark) o->yinertia += 0x10;
			LIMITY(0x100);
			
			// when player jumps on them, they open up and start
			// moving their Y to align with the core.
			if (player->riding == o)
			{
				o->ymark = MAPY(9);
				o->frame = 2;
			}
			else if (o->flags & FLAG_SOLID_BRICK)	// don't reset frame if dimmed
			{
				o->frame = 0;
			}
			
			// don't try to squish the player into anything, rather, dim and go non-solid.
			// our bbox is set slightly larger than our solidbox so that we can detect if
			// the player is near.
			if (hitdetect(o, player))
			{
				if ((player->blockl && player->Right() < o->CenterX()) || \
					(o->yinertia > 0 && player->blockd && player->Top() >= o->CenterY() - (1 * CSFI)) || \
					(o->yinertia < 0 && player->blocku && player->Bottom() < o->CenterY()))
				{
					o->flags &= ~FLAG_SOLID_BRICK;
					o->frame = 1;
				}
			}
		}
		break;
	}
}

/*
void c------------------------------() {}
*/

// falling lava-rock thing from Skull face
void ai_ud_pellet(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			o->sprite = SPR_UD_PELLET;
			o->xinertia = -0x200;
			o->state = 1;
		}
		case 1:		// falling
		{
			if (o->dir == UP)
			{
				o->yinertia -= 0x20;
				LIMITY(0x5ff);
				
				if (o->blocku)
					o->state = 2;
			}
			else if (o->dir == DOWN)
			{
				o->yinertia += 0x20;
				LIMITY(0x5ff);
				
				if (o->blockd)
					o->state = 2;
			}
			
			ANIMATE(3, 0, 1);
		}
		break;
		
		case 2:		// hit ground/ceiling
		{
			sound(SND_MISSILE_HIT);
			o->xinertia = (o->x > player->x) ? -0x400 : 0x400;
			o->yinertia = 0;
			
			o->state = 3;
			o->timer = 0;
			o->flags |= FLAG_IGNORE_SOLID;
			
			o->sprite = SPR_UD_BANG;
			o->x -= (4 * CSFI);
			o->y -= (4 * CSFI);
		}
		case 3:
		{
			ANIMATE(0, 0, 2);
			
			if ((++o->timer % 3) == 1)
			{
				Object *smoke = CreateObject(o->CenterX(), o->CenterY(), OBJ_UD_SMOKE);
				
				if (o->dir == UP)
					smoke->yinertia = 0x400;
				else
					smoke->yinertia = -0x400;
				
				smoke->x += o->xinertia;
			}
			
			if (o->CenterX() < (16 * CSFI) || \
				o->CenterX() > (MAPX(map.xsize) - (16 * CSFI)))
			{
				o->Delete();
			}
		}
		break;
	}
}


void ai_ud_smoke(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			o->xinertia = random(-4, 4) * CSFI;
			o->state = 1;
		}
		case 1:
		{
			o->xinertia *= 20; o->xinertia /= 21;
			o->yinertia *= 20; o->yinertia /= 21;
			
			ANIMATE_FWD(1);
			if (o->frame > sprites[o->sprite].nframes)
				o->Delete();
		}
		break;
	}
}

/*
void c------------------------------() {}
*/


// spinny thing shot by rotators during Teeth phase.
// they come in pairs.
void ai_ud_spinner(Object *o)
{
	if (o->x < 0 || o->x > MAPX(map.xsize))
	{
		effect(o->CenterX(), o->CenterY(), EFFECT_BOOMFLASH);
		o->Delete();
		return;
	}
	
	switch(o->state)
	{
		case 0:
		{
			o->xmark = o->x;
			o->ymark = o->y;
			o->state = 1;
		}
		case 1:
		{
			o->angle += 24;
			
			o->speed -= 0x15;
			o->xmark += o->speed;
			
			o->x = o->xmark + xinertia_from_angle(o->angle, (4 * CSFI));
			o->y = o->ymark + yinertia_from_angle(o->angle, (6 * CSFI));
			
			CreateObject(o->x, o->y, OBJ_UD_SPINNER_TRAIL);
			o->BringToFront();
		}
		break;
	}
}

void ai_ud_spinner_trail(Object *o)
{
	o->frame++;
	if (o->frame > 2)
		o->Delete();
}

/*
void c------------------------------() {}
*/

void ai_ud_blast(Object *o)
{
	o->xinertia = -0x1000;
	o->frame ^= 1;

	SmokePuff(o->CenterX() + (random(0, 16) * CSFI), \
			  o->CenterY() + (random(-16, 16) * CSFI));
	
	if (o->x < -0x4000)
		o->Delete();
}






