#include "nx.h"
#include "common/llist.h"
#include "ai/sym/smoke.h"
#include "object.h"
#include "graphics/graphics.h"
using namespace Graphics;
#include "graphics/sprites.h"
#include "autogen/sprites.h"
#include "graphics/tileset.h"
#include "sound/sound.h"
#include "game.h"
#include "player.h"
#include "map.h"
#include "tsc.h"
#include "slope.h"
#include "caret.h"
#include "player.h"
#include "debug.h"
#include "ai/ai.h"

//#define CSF 9

int Object::Width()                      { return (sprites[this->sprite].w * CSFI); }
int Object::Height()                     { return (sprites[this->sprite].h * CSFI); }

int Object::BBoxWidth()          { return (((sprites[this->sprite].bbox.x2 - sprites[this->sprite].bbox.x1) + 1) * CSFI); }
int Object::BBoxHeight()         { return (((sprites[this->sprite].bbox.y2 - sprites[this->sprite].bbox.y1) + 1) * CSFI); }

int Object::CenterX()            { return (this->x + (Width() / 2)) - DrawPointX(); }
int Object::CenterY()            { return (this->y + (Height() / 2)) - DrawPointY(); }

int Object::Left()                       { return (this->x + (sprites[this->sprite].bbox.x1 * CSFI)); }
int Object::Right()                      { return (this->x + (sprites[this->sprite].bbox.x2 * CSFI)); }
int Object::Top()                        { return (this->y + (sprites[this->sprite].bbox.y1 * CSFI)); }
int Object::Bottom()                     { return (this->y + (sprites[this->sprite].bbox.y2 * CSFI)); }

int Object::SolidLeft()          { return (this->x + (sprites[this->sprite].solidbox.x1 * CSFI)); }
int Object::SolidRight()         { return (this->x + (sprites[this->sprite].solidbox.x2 * CSFI)); }
int Object::SolidTop()           { return (this->y + (sprites[this->sprite].solidbox.y1 * CSFI)); }
int Object::SolidBottom()        { return (this->y + (sprites[this->sprite].solidbox.y2 * CSFI)); }

int Object::ActionPointX()       { return (this->x + (sprites[this->sprite].frame[this->frame].dir[this->dir].actionpoint.x * CSFI)); }
int Object::ActionPointY()       { return (this->y + (sprites[this->sprite].frame[this->frame].dir[this->dir].actionpoint.y * CSFI)); }
int Object::ActionPoint2X()      { return (this->x + (sprites[this->sprite].frame[this->frame].dir[this->dir].actionpoint2.x * CSFI)); }
int Object::ActionPoint2Y()      { return (this->y + (sprites[this->sprite].frame[this->frame].dir[this->dir].actionpoint2.y * CSFI)); }

int Object::DrawPointX()         { return (sprites[this->sprite].frame[this->frame].dir[this->dir].drawpoint.x * CSFI); }
int Object::DrawPointY()         { return (sprites[this->sprite].frame[this->frame].dir[this->dir].drawpoint.y * CSFI); }

SIFSprite *Object::Sprite()      { return &sprites[this->sprite]; }


// deletes the specified object, or well, marks it to be deleted.
// it's not actually freed till the end of the tick.
void Object::Delete()
{
Object * const &o = this;

	if (o->deleted)
		return;
	
	// make sure no pointers are pointing at us
	DisconnectGamePointers();
	
	// show any damage waiting to be added NOW instead of later
	if (o->DamageWaiting > 0)
	{
		DamageText->AddQty(o->DamageWaiting);
		o->DamageWaiting = 0;
	}
	
	// set it's id1 flag, required for some scripts
	game.flags[o->id1] = true;
	
	// mark it for deletion at end of loop
	// (can't delete now as it may invalidate pointers--we don't know where we were called from)
	o->deleted = true;
}

void Object::Destroy()
{
Object * const &o = this;

	// make sure no pointers are pointing at us
	DisconnectGamePointers();
	// delete associated floaty text as soon as it's animation is done
	DamageText->ObjectDestroyed = true;
	
	// if any objects are linked to this obj then unlink them
	Object *link;
	for(link = firstobject; link; link = link->next)
	{
		if (link->linkedobject == o)
			link->linkedobject = NULL;
	}
	
	// remove from list and free
	LL_REMOVE(o, prev, next, firstobject, lastobject);
	LL_REMOVE(o, lower, higher, lowestobject, highestobject);
	if (o == player) player = NULL;
	
	delete o;
}

// checks all the games pointers that point to an object
// record and disconnects them if they are pointing at object o.
// used in preparation to delete the object.
// protects against dangling pointers.
void Object::DisconnectGamePointers()
{
Object * const &o = this;

	if (o == player->riding) player->riding = NULL;
	if (o == player->lastriding) player->lastriding = NULL;
	if (o == player->cannotride) player->cannotride = NULL;
	if (o == game.bossbar.object) game.bossbar.object = NULL;	// any enemy with a boss bar
	if (o == game.stageboss.object) game.stageboss.object = NULL;	// the stage boss
	if (o == map.focus.target) map.focus.target = NULL;
	if (o == ID2Lookup[this->id2]) ID2Lookup[this->id2] = NULL;
	if (o == map.waterlevelobject) map.waterlevelobject = NULL;
}

/*
void c------------------------------() {}
*/

void Object::SetType(int type)
{
Object * const &o = this;

	o->type = type;
	o->sprite = objprop[type].sprite;
	o->hp = objprop[type].initial_hp;
	o->damage = objprop[o->type].damage;
	o->frame = 0;
	
	// apply nxflags to new object type!
	// (did this so toroko would handle slopes properly in Gard cutscene)
	o->nxflags = objprop[type].defaultnxflags;
	
	// apply defaultflags to new object type, but NOT ALL defaultflags.
	// otherwise <CNP's _WILL_ get messed up.
	const static int flags_to_keep = \
		(FLAG_SCRIPTONTOUCH | FLAG_SCRIPTONDEATH | FLAG_SCRIPTONACTIVATE | \
		 FLAG_APPEAR_ON_FLAGID | FLAG_DISAPPEAR_ON_FLAGID | \
		 FLAG_FACES_RIGHT);
	
	uint32_t keep = (o->flags & flags_to_keep);
	o->flags = (objprop[type].defaultflags & ~flags_to_keep) | keep;
	
	//stat("new flags: %04x", o->flags);
	
	// setup default clipping extents, in case object turns on clip_enable
	if (!o->clip_enable)
		o->ResetClip();
}

void Object::ChangeType(int type)
{
Object * const &o = this;

	int oldsprite = o->sprite;
	
	o->state = 0;
	o->substate = 0;
	o->frame = 0;
	o->timer = 0;
	o->timer2 = 0;
	o->animtimer = 0;
	
	SetType(type);
	
	// adjust position so spawn points of old object and new object line up
	o->x /= CSFI; o->x *= CSFI;
	o->y /= CSFI; o->y *= CSFI;
	o->x += (sprites[oldsprite].spawn_point.x * CSFI);
	o->y += (sprites[oldsprite].spawn_point.y * CSFI);
	o->x -= (sprites[this->sprite].spawn_point.x * CSFI);
	o->y -= (sprites[this->sprite].spawn_point.y * CSFI);
	
	// added this for when you pick up the puppy in the Deserted House in SZ--
	// makes objects <CNPed during a <PRI initialize immediately instead of waiting
	// for <PRI to be released.
	if (game.frozen)
	{
		OnTick();
		OnAftermove();
	}
	
	// Sprites appearing out of an OBJ_NULL should generally go to the top of the z-order.
	// this was originally added so that the Doctor would appear in front of the core
	// when he teleports in at end of Almond battle (it's since been used in a lot of
	// other places though).
	if (oldsprite == SPR_NULL)
	{
		BringToFront();
	}
	
	OnSpawn();
}

// moves an object to the top of the Z-order,
// so that it is drawn in front of all other objects.
void Object::BringToFront()
{
	LL_REMOVE(this, lower, higher, lowestobject, highestobject);
	LL_ADD_END(this, lower, higher, lowestobject, highestobject);
}

// move an object in the z-order to just below object "behind".
void Object::PushBehind(Object *behind)
{
	if (behind == this)
		return;
	
	LL_REMOVE(this, lower, higher, lowestobject, highestobject);
	LL_INSERT_BEFORE(this, behind, lower, higher, lowestobject, highestobject);
}

void Object::PushBehind(int objtype)
{
	Object *target = Objects::FindByType(objtype);
	if (target)
		PushBehind(target);
	else
		staterr("PushBehind: could not find any objects of type %s", DescribeObjectType(objtype));
}

/*
void c------------------------------() {}
*/

// for each point in pointlist, treats the point as a CSF'ed offset
// within the object's sprite. Then checks the attributes of the tile
// under each point. Returns an attribute mask containing the cumulative
// attributes of all the tiles under each point in the list.
//
// if tile is non-null, it is set to the tile type of the last tile checked.
uint32_t Object::GetAttributes(const Point *pointlist, int npoints, int *tile)
{
int tileno = 0;
uint32_t attr = 0;

	int xoff = (this->x / CSFI);
	int yoff = (this->y / CSFI);
	
	for(int i=0;i<npoints;i++)
	{
		int x = (xoff + pointlist[i].x) / TILE_W;
		int y = (yoff + pointlist[i].y) / TILE_H;
		
		if (x >= 0 && y >= 0 && x < map.xsize && y < map.ysize)
		{
			tileno = map.tiles[x][y];
			attr |= tileattr[tileno];
		}
	}
	
	// also go underwater if we go under the variable waterlevel in Almond
	if (map.waterlevelobject && (this->y + (2 * CSFI)) > map.waterlevelobject->y)
	{
		attr |= TA_WATER;
	}
	
	if (tile) *tile = tileno;
	return attr;
}

// for each point in pointlist, treats the point as a CSF'ed offset
// within the object's sprite. The tile under each position is checked
// to see if it's attributes contain one or more of the attributes
// specified in attrmask.
//
// If any of the points match, returns 1, and optionally returns
// the map coordinates of the first matched tile in tile_x/y.
bool Object::CheckAttribute(const Point *pointlist, int npoints, uint32_t attrmask,
							int *tile_x, int *tile_y)
{
int x, y, xoff, yoff;

	xoff = (this->x / CSFI);
	yoff = (this->y / CSFI);
	
	for(int i=0;i<npoints;i++)
	{
		x = (xoff + pointlist[i].x) / TILE_W;
		y = (yoff + pointlist[i].y) / TILE_H;
		
		if (x >= 0 && y >= 0 && x < map.xsize && y < map.ysize)
		{
			if ((tileattr[map.tiles[x][y]] & attrmask) != 0)
			{
				if (tile_x) *tile_x = x;
				if (tile_y) *tile_y = y;
				return true;
			}
		}
	}
	
	return false;
}

// treats each point in pointlist as an offset within the object, and returns
// true if any of the points intersect with object o2's solidbox.
bool Object::CheckSolidIntersect(Object *other, const Point *pointlist, int npoints)
{
int x, y;
int ox, oy, o2x, o2y;
SIFSprite *s2 = other->Sprite();
	
	ox = (this->x / CSFI);
	oy = (this->y / CSFI);
	o2x = (other->x / CSFI);
	o2y = (other->y / CSFI);
	
	for(int i=0;i<npoints;i++)
	{
		x = ox + pointlist[i].x;
		y = oy + pointlist[i].y;
		
		if (x >= (o2x + s2->solidbox.x1) && x <= (o2x + s2->solidbox.x2))
		{
			if (y >= (o2y + s2->solidbox.y1) && y <= (o2y + s2->solidbox.y2))
			{
				return true;
			}
		}
	}
	
	return false;
}


// update the blocked states of object o.
// updatemask specifies which states are in need of updating.
void Object::UpdateBlockStates(uint8_t updatemask)
{
Object * const &o = this;
SIFSprite *sprite = Sprite();
int mask = GetBlockingType();

	if (updatemask & LEFTMASK)
	{
		o->blockl = CheckAttribute(&sprite->block_l, mask);
		
		// for objects which don't follow slope, have them see the slope as a wall so they
		// won't just go right through it (looks really weird)
		if (!(o->nxflags & NXFLAG_FOLLOW_SLOPE))
		{
			if (!o->blockl)
				o->blockl = IsSlopeAtPointList(o, &sprite->block_l);
		}
	}
	
	if (updatemask & RIGHTMASK)
	{
		o->blockr = CheckAttribute(&sprite->block_r, mask);
		
		// for objects which don't follow slope, have them see the slope as a wall so they
		// won't just go right through it (looks really weird).
		if (!(o->nxflags & NXFLAG_FOLLOW_SLOPE))
		{
			if (!o->blockr)
				o->blockr = IsSlopeAtPointList(o, &sprite->block_r);
		}
	}
	
	if (updatemask & UPMASK)
	{
		o->blocku = CheckAttribute(&sprite->block_u, mask);
		if (!o->blocku) o->blocku = CheckBoppedHeadOnSlope(o) ? 1 : 0;
	}
	
	if (updatemask & DOWNMASK)
	{
		o->blockd = CheckAttribute(&sprite->block_d, mask);
		if (!o->blockd) o->blockd = CheckStandOnSlope(o) ? 1 : 0;
	}
	
	// have player be blocked by objects with FLAG_SOLID_BRICK set
	if (o == player)
		o->SetBlockForSolidBrick(updatemask);
}

// called from UpdateBlockedStates used w/ player.
// sets the object's block/l/r/u/d flags if it is in contact with a SOLID_BRICK object.
void Object::SetBlockForSolidBrick(uint8_t updatemask)
{
SIFSprite *thissprite = this->Sprite();
Object *o;

	// no need to check blockpoints that are already set
	if (this->blockl) updatemask &= ~LEFTMASK;
	if (this->blockr) updatemask &= ~RIGHTMASK;
	if (this->blocku) updatemask &= ~UPMASK;
	if (this->blockd) updatemask &= ~DOWNMASK;
	
	FOREACH_OBJECT(o)
	{
		if (!(o->flags & FLAG_SOLID_BRICK)) continue;
		
		if (updatemask & LEFTMASK)
		{
			if (this->CheckSolidIntersect(o, &thissprite->block_l))
			{
				this->blockl = BLOCKED_OBJECT;	// value of 2 instead of 1
				updatemask &= ~LEFTMASK;		// no need to keep checking
			}
		}
		
		if (updatemask & RIGHTMASK)
		{
			if (this->CheckSolidIntersect(o, &thissprite->block_r))
			{
				this->blockr = BLOCKED_OBJECT;
				updatemask &= ~RIGHTMASK;
			}
		}
		
		if (updatemask & UPMASK)
		{
			if (this->CheckSolidIntersect(o, &thissprite->block_u))
			{
				this->blocku = BLOCKED_OBJECT;
				updatemask &= ~UPMASK;
				
				if (this == player)
					player->bopped_object = o;
			}
		}
		
		if (updatemask & DOWNMASK)
		{
			if (this->CheckSolidIntersect(o, &thissprite->block_d))
			{
				this->blockd = BLOCKED_OBJECT;
				updatemask &= ~DOWNMASK;
				
				if (this == player)
					player->riding = o;
			}
		}
	}
}

/*
void c------------------------------() {}
*/

// given an object, returns which tile attribute affects it's blocked state.
int Object::GetBlockingType()
{
Object * const &o = this;

	if (o == player)
		return TA_SOLID_PLAYER;
	
	if (o->type >= OBJ_SHOTS_START && \
		o->type <= OBJ_SHOTS_END)
	{
		// Bubbler L1 can't pass tile 44.
		if (o->type == OBJ_BUBBLER12_SHOT && o->shot.level == 0)
			return (TA_SOLID_SHOT | TA_SOLID_NPC);
		
		return TA_SOLID_SHOT;
	}
	
	if (o->flags & FLAG_IGNORETILE44)
		return TA_SOLID_PLAYER;
	
	return TA_SOLID_NPC;
}

/*
void c------------------------------() {}
*/

// tries to move the object in the X direction by the given amount.
// returns nonzero if the object was blocked.
bool Object::apply_xinertia(int inertia)
{
Object * const &o = this;

	if (inertia == 0)
		return 0;
	
	if (o->flags & FLAG_IGNORE_SOLID)
	{
		o->x += inertia;
		return 0;
	}
	
	// only apply inertia one pixel at a time so we have
	// proper hit detection--prevents objects traveling at
	// high speed from becoming embedded in walls
	if (inertia > 0)
	{
		while(inertia > (1 * CSFI))
		{
			if (movehandleslope(o, (1 * CSFI))) return 1;
			inertia -= (1 * CSFI);
			
			o->UpdateBlockStates(RIGHTMASK);
		}
	}
	else if (inertia < 0)
	{
		while(inertia < -(1 * CSFI))
		{
			if (movehandleslope(o, -(1 * CSFI))) return 1;
			inertia += (1 * CSFI);
			
			o->UpdateBlockStates(LEFTMASK);
		}
	}
	
	// apply any remaining inertia
	if (inertia)
		movehandleslope(o, inertia);
	
	return 0;
}

// tries to move the object in the Y direction by the given amount.
// returns nonzero if the object was blocked.
bool Object::apply_yinertia(int inertia)
{
Object * const &o = this;

	if (inertia == 0)
		return 0;
	
	if (o->flags & FLAG_IGNORE_SOLID)
	{
		o->y += inertia;
		return 0;
	}
	
	// only apply inertia one pixel at a time so we have
	// proper hit detection--prevents objects traveling at
	// high speed from becoming embedded in walls
	if (inertia > 0)
	{
		if (o->blockd) return 1;
		
		while(inertia > (1 * CSFI))
		{
			o->y += (1 * CSFI);
			inertia -= (1 * CSFI);
			
			o->UpdateBlockStates(DOWNMASK);
			if (o->blockd) return 1;
		}
	}
	else if (inertia < 0)
	{
		if (o->blocku) return 1;
		
		while(inertia < -(1 * CSFI))
		{
			o->y -= (1 * CSFI);
			inertia += (1 * CSFI);
			
			o->UpdateBlockStates(UPMASK);
			if (o->blocku) return 1;
		}
	}
	
	// apply any remaining inertia
	if (inertia)
		o->y += inertia;
	
	return 0;
}


// handles a moving object with "FLAG_SOLID_BRICK" set
// pushing the player as it moves.
void Object::PushPlayerOutOfWay(int xinertia, int yinertia)
{
Object * const &o = this;

	if (xinertia)
	{
		// give a bit of a gap where they must be--i.e. don't push them if they're right
		// at the top or the bottom of the brick: needed when he rides it and falls off, then it
		// turns around and touches him again. in that case what we actually want to do is push him
		// to the top, not push him side-to-side.
		if ((player->SolidBottom() - (2 * CSFI)) > o->SolidTop() &&\
			(player->SolidTop() + (2 * CSFI)) < o->SolidBottom())
		{
			if (xinertia > 0 && player->SolidRight() > o->SolidRight() && solidhitdetect(o, player))
			{	// pushing player right
				if (player->blockr)
				{	// squish!
					hurtplayer(o->smushdamage);
				}
				else
				{
					// align player's blockl grid with our right side
					player->x = o->SolidRight() - (sprites[player->sprite].block_l[0].x * CSFI);
					
					// get player a xinertia equal to our own. You can see this
					// with the moving blocks in Labyrinth H.
					player->xinertia = xinertia;
					player->x += -player->xinertia;
				}
			}
			else if (xinertia < 0 && player->SolidLeft() < o->SolidLeft() && solidhitdetect(o, player))
			{	// pushing player left
				if (player->blockl)
				{	// squish!
					hurtplayer(o->smushdamage);
				}
				else
				{
					// align player's blockr grid with our left side
					player->x = o->SolidLeft() - (sprites[player->sprite].block_r[0].x * CSFI);
					
					// get player a xinertia equal to our own. You can see this
					// with the moving blocks in Labyrinth H.
					player->xinertia = xinertia;
					player->x += -player->xinertia;
				}
			}
		}
	}
	
	if (yinertia < 0)
	{
		if (player->blocku && player->riding == o)	// smushed into ceiling!
			hurtplayer(o->smushdamage);
	}
	else if (yinertia > 0)	// object heading downwards?
	{
		// player riding object down
		if (player->riding == o)
		{
			if (player->yinertia >= 0)		// don't do this if he's trying to jump away
			{
				// align player's blockd grid with our top side so player
				// doesn't perpetually fall.
				player->y = o->SolidTop() - (sprites[player->sprite].block_d[0].y * CSFI);
			}
		}
		else if (player->Top() >= o->CenterY() && solidhitdetect(o, player))	// underneath object
		{
			// push him down if he's underneath us and we're going faster than he is.
			if (yinertia >= player->yinertia)
			{
				if (player->blockd)		// squished into floor!
					hurtplayer(o->smushdamage);
				
				// align his blocku grid with our bottom side
				player->y = o->SolidBottom() - (sprites[player->sprite].block_u[0].y * CSFI);
			}
		}
	}
}

// snap the object down to the nearest solid tile.
// the object must have at least one blockd point for this to work.
void Object::SnapToGround()
{
Object * const &o = this;

	uint32_t flags = o->flags;
	o->flags &= ~FLAG_IGNORE_SOLID;
	
	UpdateBlockStates(DOWNMASK);
	apply_yinertia(SCREEN_HEIGHT * CSFI);
	
	o->flags = flags;
	o->blockd = true;
}

/*
void c------------------------------() {}
*/

// deals the specified amount of damage to the object,
// and kills it if it's hitpoints reach 0.
//
// It is valid to deal 0 damage. The trails of the Spur do this
// to keep the enemy shaking and making noise for as long as
// it's in the beam.
//
// shot is an optional parameter specifying a pointer to
// the shot that hit the object, and is used to spawn
// blood spatter at the correct location.
void Object::DealDamage(int dmg, Object *shot)
{
Object * const &o = this;

	if (o->flags & FLAG_INVULNERABLE)
		return;
	
	o->hp -= dmg;
	
	if (o->flags & FLAG_SHOW_FLOATTEXT)
		o->DamageWaiting += dmg;
	
	if (o->hp > 0)
	{
		if (o->shaketime < objprop[o->type].shaketime - 2)
		{
			o->shaketime = objprop[o->type].shaketime;
			
			if (objprop[o->type].hurt_sound)
				sound(objprop[o->type].hurt_sound);
			
			if (shot)
				effect(shot->CenterX(), shot->CenterY(), EFFECT_BLOODSPLATTER);
		}
	}
	else
	{
		o->Kill();
	}
}

// kills the specified object, performing whatever action is
// applicable to that, such as spawning powerups or running scripts.
void Object::Kill()
{
Object * const &o = this;

	o->hp = 0;
	o->flags &= ~FLAG_SHOOTABLE;
	
	// auto disappear the bossbar if we have just killed a boss
	if (o == game.bossbar.object)
		game.bossbar.defeated = true;
	
	// if a script is set to run on death, run it instead of the usual explosion
	if (o->flags & FLAG_SCRIPTONDEATH)
	{
		o->OnDeath();
		StartScript(o->id2);
	}
	else
	{
		// should spawn the smokeclouds first, for z-order reasons
		SmokeClouds(o, objprop[o->type].death_smoke_amt, 8, 8);
		effect(o->CenterX(), o->CenterY(), EFFECT_BOOMFLASH);
		
		if (objprop[o->type].death_sound)
			sound(objprop[o->type].death_sound);
		
		if (objprop[o->type].ai_routines.ondeath)
		{
			o->OnDeath();
		}
		else
		{
			SpawnPowerups();
			o->Delete();
		}
	}
}


// spawn the powerups you get when you kill an enemy
void Object::SpawnPowerups()
{
Object * const &o = this;
int objectType, bonusType;

	if (!objprop[o->type].xponkill)
		return;
	
	bonusType = random(1, 5);
	if (bonusType >= 3)
	{
		SpawnXP(objprop[o->type].xponkill);
		return;
	}
	
	if (bonusType == 2 && \
		(player->weapons[WPN_MISSILE].hasWeapon || \
		 player->weapons[WPN_SUPER_MISSILE].hasWeapon))
	{
		objectType = OBJ_MISSILE;
	}
	else
	{
		objectType = OBJ_HEART;
	}
	
	// upgrade to big 3-cluster versions of powerups
	// for big enemies.
	if (objprop[o->type].xponkill > 6)
	{
		if (objectType == OBJ_HEART)
		{
			objectType = OBJ_HEART3;
		}
		else
		{
			objectType = OBJ_MISSILE3;
		}
	}
	
	// create the powerup
	Object *powerup = CreateObject(o->CenterX(), o->CenterY(), objectType);
	powerup->x -= (powerup->Width() / 2);
	powerup->y -= (powerup->Height() / 2);
	
	powerup->state = 1;			// make it animate
}


// spawn the given quantity of XP at the center of the object.
// amt indicates the total number of XP points to spawn.
// these will be collated into the appropriate sizes of XP triangles.
void Object::SpawnXP(int amt)
{
Object * const &o = this;

	int x = o->CenterX();
	int y = o->CenterY();
	
	while(amt > 0)
	{
		Object *xp = CreateObject(x, y, OBJ_XP);
		xp->xinertia = random(-0x200, 0x200);
		
		if (amt >= XP_LARGE_AMT)
		{
			xp->sprite = SPR_XP_LARGE;
			amt -= XP_LARGE_AMT;
		}
		else if (amt >= XP_MED_AMT)
		{
			xp->sprite = SPR_XP_MED;
			amt -= XP_MED_AMT;
		}
		else
		{
			xp->sprite = SPR_XP_SMALL;
			amt -= XP_SMALL_AMT;
		}
		
		// center the sprite at the center of the object
		xp->x -= (xp->Width() / 2);
		xp->y -= (xp->Height() / 2);
		
		xp->UpdateBlockStates(ALLDIRMASK);
	}
}

/*
void c------------------------------() {}
*/

void Object::RunAI()
{
Object * const &o = this;

	o->OnTick();
	
	// trigger touch-activated scripts.
	// it actually only triggers once his centerline touches the object.
	// see the passageway between the Throne Room and Kings Table for a
	// clear example of the correct coordinates.
	if (o->flags & FLAG_SCRIPTONTOUCH)
	{
		if (pdistlx(8 * CSFI))
		{
			int y = player->y + (6 * CSFI);
			
			// player->riding check is for fans in Final Cave
			if ((y > o->Top() && y < o->Bottom()) || player->riding == o)
			{
				if (GetCurrentScript() == -1 &&		// no override other scripts
					game.switchstage.mapno == -1)	// no repeat exec after <TRA
				{
					stat("On-touch script %d triggered", o->id2);
					StartScript(o->id2);
				}
			}
		}
	}
}

// deals contact damage to player of o->damage, if applicable.
void Object::DealContactDamage()
{
Object * const &o = this;

	// no contact damage to player while scripts running
	if (GetCurrentScript() != -1 || player->inputs_locked)
		return;
	
	if (!(o->flags & FLAG_NOREARTOPATTACK))
	{
		hurtplayer(o->damage);
		return;
	}
	
	// else, the no rear/top attack flag is set, so only
	// frontal or bottom contact are harmful to the player
	switch(o->GetAttackDirection())
	{
		case -1:	// head-on
			hurtplayer(o->damage);
		break;
		
		case LEFT:	// rear attack, p to left
			if (player->xinertia > -0x100)
				player->xinertia = -0x100;
		break;
		
		case RIGHT:	// rear attack, p to right
			if (player->xinertia < 0x100)
				player->xinertia = 0x100;
		break;
	}
}

// subfunction of HandleContactDamage. On entry, we assume that the player
// is in contact with this object, and that the object is trying to deal
// damage to him.
// returns the type of attack:
//	- UP	a top attack (player hit top of object)
//	- LEFT	rear attack, player to left
//	- RIGHT	rear attack, player to right
//	- -1	head-on or bottom attack
int Object::GetAttackDirection()
{
Object * const &o = this;
const int VARIANCE = (5 * CSFI);

	if (player->riding == o)
		return UP;
	
	if (player->Bottom() <= (o->Top() + VARIANCE))
		return UP;
	
	// (added for X treads) if the object is moving, then the "front"
	// for purposes of this flag is the direction it's moving in.
	// if it's still, the "front" is the actual direction it's facing.
	int rtdir = o->dir;
	if (o->xinertia > 0) rtdir = RIGHT;
	if (o->xinertia < 0) rtdir = LEFT;
	
	if (rtdir == RIGHT)
	{
		if (player->Right() <= (o->Left() + VARIANCE))
			return RIGHT;
	}
	else if (rtdir == LEFT)		// the double check makes sense, what if o->dir was UP or DOWN
	{
		if (player->Left() >= (o->Right() - VARIANCE))
			return LEFT;
	}
	
	return -1;
}

void Object::MoveAtDir(int dir, int speed)
{
	this->xinertia = 0;
	this->yinertia = 0;
	
	switch(dir)
	{
		case LEFT:  this->xinertia = -speed; break;
		case RIGHT: this->xinertia = speed; break;
		case UP:	this->yinertia = -speed; break;
		case DOWN:  this->yinertia = speed; break;
	}
}

/*
void c------------------------------() {}
*/

// animate over a list of frames, where the frames need not be consecutive.
// every speed ticks we will display a new frame from framelist.
// this function requires initilization of animframe and animtimer.
void Object::animate_seq(int speed, const int *framelist, int nframes)
{
Object * const &o = this;

	if (++o->animtimer > speed)
	{
		o->animtimer = 0;
		o->animframe++;
	}
	
	if (o->animframe >= nframes)
		o->animframe = 0;
	
	o->frame = framelist[o->animframe];
}

// used by objects in Maze M, this hints to curly's AI that the object is attacking.
void Object::CurlyTargetHere(int mintime, int maxtime)
{
Object * const &o = this;

	game.curlytarget.x = o->CenterX();
	game.curlytarget.y = o->CenterY();
	game.curlytarget.timeleft = random(mintime, maxtime);
}

// reset the objects clip-extent fields (tp effects etc) to their defaults.
// i.e. such that if clip_enable were to be turned on it would have no immediate effect.
void Object::ResetClip()
{
Object * const &o = this;

	o->clipx1 = o->clipy1 = 0;
	o->clipx2 = sprites[o->sprite].w;
	o->clipy2 = sprites[o->sprite].h;
}
/*
void c------------------------------() {}
*/

void Object::OnTick()
{
	if (objprop[this->type].ai_routines.ontick)
		(*objprop[this->type].ai_routines.ontick)(this);
}

void Object::OnAftermove()
{
	if (objprop[this->type].ai_routines.aftermove)
		(*objprop[this->type].ai_routines.aftermove)(this);
}

void Object::OnSpawn()
{
	if (objprop[this->type].ai_routines.onspawn)
		(*objprop[this->type].ai_routines.onspawn)(this);
}

void Object::OnDeath()
{
	if (objprop[this->type].ai_routines.ondeath)
		(*objprop[this->type].ai_routines.ondeath)(this);
}


