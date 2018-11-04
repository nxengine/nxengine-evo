#include "sym.h"

#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../common/misc.h"
#include "../../common/stat.h"
#include "../../debug.h"
#include "../../game.h"
#include "../../graphics/graphics.h"
#include "../../map.h"
#include "../../player.h"
#include "../../playerstats.h"
#include "../../sound/SoundManager.h"
#include "../../tsc.h"
#include "../ai.h"
#include "../stdai.h"
#include "../sym/smoke.h"
using namespace Graphics;
#include "../../autogen/sprites.h"
#include "../../graphics/sprites.h"
#include "../../screeneffect.h"
#include "../../settings.h"

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_NULL, ai_null);
  ONTICK(OBJ_HVTRIGGER, ai_hvtrigger);

  ONTICK(OBJ_XP, ai_xp);
  ONTICK(OBJ_HEART, ai_powerup);
  ONTICK(OBJ_HEART3, ai_powerup);
  ONTICK(OBJ_MISSILE, ai_powerup);
  ONTICK(OBJ_MISSILE3, ai_powerup);

  ONTICK(OBJ_HIDDEN_POWERUP, ai_hidden_powerup);

  ONTICK(OBJ_DOOR, ai_door);
  ONTICK(OBJ_LARGEDOOR, ai_largedoor);

  ONTICK(OBJ_SAVE_POINT, ai_save_point);
  ONTICK(OBJ_RECHARGE, ai_recharge);

  ONTICK(OBJ_CHEST_CLOSED, ai_chest_closed);
  ONTICK(OBJ_CHEST_OPEN, ai_chest_open);

  ONTICK(OBJ_TELEPORTER, ai_teleporter);
  ONTICK(OBJ_TELEPORTER_LIGHTS, ai_animate2);

  ONTICK(OBJ_COMPUTER, ai_animate4);
  ONTICK(OBJ_TERMINAL, ai_terminal);

  ONTICK(OBJ_LIFE_CAPSULE, ai_animate4);
  ONTICK(OBJ_XP_CAPSULE, ai_xp_capsule);

  ONTICK(OBJ_SPRINKLER, ai_sprinkler);
  ONTICK(OBJ_WATER_DROPLET, ai_water_droplet);
  ONTICK(OBJ_LAVA_DROPLET, ai_water_droplet);
  ONTICK(OBJ_DROPLET_SPAWNER, ai_droplet_spawner);

  ONTICK(OBJ_FAN_UP, ai_fan_vert);
  ONTICK(OBJ_FAN_DOWN, ai_fan_vert);
  ONTICK(OBJ_FAN_LEFT, ai_fan_hoz);
  ONTICK(OBJ_FAN_RIGHT, ai_fan_hoz);
  ONTICK(OBJ_FAN_DROPLET, ai_fan_droplet);

  ONTICK(OBJ_PRESS, ai_press);
  ONTICK(OBJ_HIDDEN_SPARKLE, ai_animate4);
  ONTICK(OBJ_LIGHTNING, ai_lightning);

  ONTICK(OBJ_STRAINING, ai_straining);

  ONTICK(OBJ_BUBBLE_SPAWNER, ai_bubble_spawner);

  ONTICK(OBJ_CHINFISH, ai_chinfish);
  ONTICK(OBJ_FIREPLACE, ai_fireplace);

  ONTICK(OBJ_SMOKE_DROPPER, ai_smoke_dropper);

  ONSPAWN(OBJ_SPIKE_SMALL, onspawn_spike_small);

  ONTICK(OBJ_SCROLL_CONTROLLER, ai_scroll_controller);
  ONTICK(OBJ_QUAKE, ai_quake);
}

/*
void c------------------------------() {}
*/

// ai routine for OBJ_NULL
void ai_null(Object *o)
{
  if (o->state == 0)
  {
    o->state = 1;

    // FLAG_FACES_RIGHT is causes the object to drop down one tile when actually
    // seen in the game. I think this was used to make map editing easier in
    // places where things were getting really crouded with entities.
    if (o->dir == RIGHT)
    {
      o->y += (TILE_H * CSFI);

      // precedence hack for Boulder Chamber
      if (game.curmap == STAGE_BOULDER_CHAMBER)
      {
        Object *boulder = Objects::FindByType(OBJ_BOULDER);
        if (boulder)
          o->PushBehind(boulder);
      }
    }
  }
}

// project the Option 1 beam and set the hvtrigger's y1/y2 or x1/x2
static void hv_project_beam(Object *o)
{
  int tilex = (o->x / CSFI) / TILE_W;
  int tiley = (o->y / CSFI) / TILE_H;
  int x, y, t;

  if (!o->hvt.is_horizontal)
  {
    for (y = tiley; y >= 0; y--)
    {
      t = map.tiles[tilex][y];
      if (tileattr[t] & TA_SOLID)
      {
        y++;
        break;
      }
    }

    o->hvt.y1 = (y * TILE_H) * CSFI;

    for (y = tiley; y < map.ysize; y++)
    {
      t = map.tiles[tilex][y];
      if (tileattr[t] & TA_SOLID)
      {
        y--;
        break;
      }
    }

    o->hvt.y2 = ((y * TILE_H) + (TILE_H - 1)) * CSFI;
  }
  else
  {
    for (x = tilex; x >= 0; x--)
    {
      t = map.tiles[x][tiley];
      if (tileattr[t] & TA_SOLID)
      {
        x++;
        break;
      }
    }

    o->hvt.x1 = (x * TILE_W) * CSFI;

    for (x = tilex; x < map.xsize; x++)
    {
      t = map.tiles[x][tiley];
      if (tileattr[t] & TA_SOLID)
      {
        x--;
        break;
      }
    }

    o->hvt.x2 = ((x * TILE_W) + (TILE_W - 1)) * CSFI;
  }
}

// H/V Trigger
//
// By default, triggers on vertical axis.
// If set to face right, it triggers on horizontal axis instead.
//
// If FLAG_SCRIPTONTOUCH is set, projects an invisible horizontal or vertical "Beam"
// which is blocked by solid bricks, and only by intersecting the beam
// can the player trip the trigger.
//
void ai_hvtrigger(Object *o)
{
  // init: find bounding box within which we will trigger
  if (o->state == 0)
  {
    o->state             = 1;
    o->hvt.is_horizontal = (o->dir == LEFT);

    // start with a trigger box around object which
    // encloses only the object's tile itself.
    o->hvt.x1 = o->x;
    o->hvt.y1 = o->y;
    o->hvt.x2 = o->x + ((TILE_W - 1) * CSFI);
    o->hvt.y2 = o->y + ((TILE_H - 1) * CSFI);

    // now expand the trigger box as appropriate
    if (o->flags & FLAG_SCRIPTONTOUCH)
    {
      o->flags &= ~FLAG_SCRIPTONTOUCH;
      hv_project_beam(o);
    }
    else if (o->hvt.is_horizontal)
    {
      o->hvt.x1 = 0;
      o->hvt.x2 = (map.xsize * TILE_W) * CSFI;
    }
    else
    {
      o->hvt.y1 = 0;
      o->hvt.y2 = (map.ysize * TILE_H) * CSFI;
    }
  }

  if (game.debug.DrawBoundingBoxes)
    AddDebugMark(o->hvt.x1, o->hvt.y1, o->hvt.x2, o->hvt.y2, DM_BOX, 255, 0, 255);

  int px = player->CenterX();
  if (px > o->hvt.x2)
    return;
  if (px < o->hvt.x1)
    return;

  int py = player->CenterY();
  if (py > o->hvt.y2)
    return;
  if (py < o->hvt.y1)
    return;

  if (game.debug.DrawBoundingBoxes)
    AddDebugMark(o->hvt.x1, o->hvt.y1, o->hvt.x2, o->hvt.y2, DM_BOX, 0, 255, 0);

  // ok then, we can trigger, except for:
  if (game.tsc->GetCurrentScript() == -1 && // no override other scripts
      game.switchstage.mapno == -1)         // no repeat exec after <TRA
  {
#ifdef TRACE_SCRIPT
    stat("HVTrigger %04d (%08x) activated", o->id2, o);
#endif
    game.tsc->StartScript(o->id2);
  }
}

/*
void c------------------------------() {}
*/

void ai_xp(Object *o)
{
  if (o->state == 0)
  {
    o->yinertia = random(-1024, 0);
    o->xinertia = random(-512, 512);
    o->frame    = random(0, 4);
    if (random(0, 1))
      o->dir = CVTDir(0);
    else
      o->dir = CVTDir(2);
    o->state = 1;
  }

  if (Handle_Falling_Left(o))
  {                                           // left-falling bouncing
    if (map.scrolltype == BK_FASTLEFT_LAYERS) // as opposed to Ironhead (BK_FASTLEFT)
    {
      if (o->blockl)
      {
        if (o->onscreen || pdistly((SCREEN_HEIGHT - (SCREEN_HEIGHT / 3)) * CSFI))
          NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_XP_BOUNCE);

        o->xinertia = 0x100;
        o->yinertia *= 2;
        o->yinertia /= 3;
      }

      if (o->blocku || o->blockd)
      {
        o->yinertia = -o->yinertia;
      }
    }
  }
  else
  { // normal bouncing
    o->yinertia += 42;
    if (o->blockd)
    {
      // disappear if we were spawned embedded in ground
      // added for XP spawned by sandcrocs
      if (o->blocku || (o->blockl && o->blockr))
      {
        o->Delete();
        return;
      }

      if (o->onscreen || pdistlx((SCREEN_WIDTH - (SCREEN_WIDTH / 3)) * CSFI))
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_XP_BOUNCE);

      o->yinertia = -0x280;
      o->xinertia *= 2;
      o->xinertia /= 3;
    }
    if (o->blocku)
    {
      if (o->blockd || (o->blockl && o->blockr))
      {
        o->Delete();
        return;
      }
      o->yinertia = -o->yinertia;
      o->y++;
    }

    if (o->blockl || o->blockr)
    {
      o->xinertia = -o->xinertia;
    }
  }

  if (++o->animtimer >= 3)
  {
    o->animtimer = 0;
    if (++o->frame > 5)
      o->frame = 0;
  }

  if (++o->timer > 0x1f4)
  {
    o->Delete();
    return;
  }
  else if (o->timer > 0x1f2)
  { // twinkle before disappearing
    o->frame     = 0;
    o->invisible = 0;
    return;
  }
  else if (o->timer > 0x190)
  {
    o->invisible = (o->timer & 2);
  }

  // let player get it!
  if (hitdetect(o, player))
  {
    switch (o->sprite)
    {
      case SPR_XP_SMALL:
        AddXP(XP_SMALL_AMT);
        break;
      case SPR_XP_MED:
        AddXP(XP_MED_AMT);
        break;
      case SPR_XP_LARGE:
        AddXP(XP_LARGE_AMT);
        break;
    }

    o->Delete();
  }
}

// Hearts and Missiles
void ai_powerup(Object *o)
{
  // if o->state == 0, then was present in map; not dropped by an enemy...lasts forever
  if (o->state > 0)
  {
    Handle_Falling_Left(o);

    switch (o->state)
    {
      case 1:   // animating
      case 101: // animating (in left-fall mode)
        if (++o->timer >= 256)
        {
          o->timer = 0;
          o->state++;
        }
        else
          break;
      case 2:   // start blinking--we're about to go away!!
      case 102: // blinking (in left-fall mode)
        if (++o->timer > 48)
        {
          effect(o->CenterX() - (1 * CSFI), o->CenterY() - (1 * CSFI), EFFECT_BONUSFLASH);
          o->Delete();
          return;
        }

        o->invisible = (o->timer2 & 2);
        o->timer2++;
        break;
    }

    // animation/frame selection
    if (++o->animtimer > 2)
    {
      o->animtimer = 0;
      o->frame ^= 1;
    }
  }
  else if (!o->state)
  { // adjust position of map-spawned missiles
    if (o->type == OBJ_MISSILE)
    {
      o->x += (3 * CSFI);
      o->y += (4 * CSFI);
    }
    o->state = -1;
  }

  // hand over the powerup if player touches it
  if (hitdetect(o, player))
  {
    switch (o->type)
    {
      case OBJ_HEART:
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_HEALTH_REFILL);
        AddHealth(2);
        break;
      case OBJ_HEART3:
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_HEALTH_REFILL);
        AddHealth(6);
        break;

      case OBJ_MISSILE:
      case OBJ_MISSILE3:
      {
        int amt = (o->type == OBJ_MISSILE3) ? 3 : 1;
        int wpn = (player->weapons[WPN_SUPER_MISSILE].hasWeapon) ? WPN_SUPER_MISSILE : WPN_MISSILE;

        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_GET_MISSILE);
        AddAmmo(wpn, amt);
      }
      break;
    }

    o->Delete();
  }
}

bool Handle_Falling_Left(Object *o)
{
  if (map.scrolltype == BK_FASTLEFT || map.scrolltype == BK_FASTLEFT_LAYERS)
  {
    if (o->state < 100) // initilize
    {
      o->state += 100;
      o->yinertia = random(-0x20, 0x20);
      o->xinertia = random(127, 256);
      // o->nxflags |= NXFLAG_FOLLOW_SLOPE;
    }

    o->xinertia -= 0x08;
    if (o->xinertia < -0x600)
      o->xinertia = -0x600;

    if (map.scrolltype == BK_FASTLEFT)
    {
      if (o->x < ((5 * TILE_W) * CSFI))
        o->Delete(); // went off screen in IronH
    }

    if (o->blockl && o->xinertia <= 0)
      o->xinertia = 0x40;
    if (o->blocku && o->yinertia <= 0)
      o->yinertia = 0x40;
    if (o->blockd && o->yinertia >= 0)
      o->yinertia = -0x40;

    return 1;
  }

  return 0;
}

// hidden heart/missile
void ai_hidden_powerup(Object *o)
{
  if (o->hp < 990)
  {
    SmokeClouds(o, 8, 8, 8, NULL);
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EXPL_SMALL);

    o->ChangeType((o->dir == LEFT) ? OBJ_HEART : OBJ_MISSILE);
    if (o->type == OBJ_HEART)
    {
      o->x += (3 * CSFI);
      o->y += (4 * CSFI);
    }
  }
}

void ai_xp_capsule(Object *o)
{
  ANIMATE(4, 0, 1);

  if (o->hp < 100)
  {
    o->SpawnXP(o->id1);
    SmokeClouds(o, 8, 2, 2);
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_FUNNY_EXPLODE);

    o->Delete();
  }
}

/*
void c------------------------------() {}
*/

// save points, recharges, and chests make smoke and jump in air for a moment
// when spawned as a bonus item (e.g. after a boss fight). That they should do
// this is indicated by the script setting it's direction to RIGHT.
static void smoke_if_bonus_item(Object *o)
{
  if (o->dir == RIGHT)
  {
    SmokeClouds(o, 4, 8, 8);
    o->yinertia = -0x200;
  }
}

void ai_save_point(Object *o)
{
  if (o->state == 0)
  {
    smoke_if_bonus_item(o);
    o->flags |= FLAG_SCRIPTONACTIVATE; // needed for SP after Sisters fight
    o->state = 1;
  }

  ai_animate3(o);

  o->yinertia += 0x40;
  LIMITY(0x5ff);
}

void ai_recharge(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      smoke_if_bonus_item(o);
      o->state = 1;
    }
    case 1: // flickery animation
    {
      int x = random(0, 30);

      if (x < 10)
        o->state = 2;
      else if (x < 25)
        o->state = 3;
      else
        o->state = 4;

      o->timer     = random(16, 64);
      o->animtimer = 0;
    }
    break;

    case 2:
      o->frame = 0;
      break; // solid on

    case 3: // flickery
      o->animtimer++;
      o->frame = (o->animtimer & 1);
      break;

    case 4:
      o->frame = 1;
      break; // solid off
  }

  if (--o->timer <= 0)
    o->state = 1;

  o->yinertia += 0x40;
  LIMITY(0x5ff);
}

void ai_chest_closed(Object *o)
{
  if (o->state == 0)
  {
    smoke_if_bonus_item(o);
    o->flags |= FLAG_SCRIPTONACTIVATE; // isn't always set
    o->state = 1;

    // stop motion of chest spawned from Kulala
    //		if (settings->emulate_bugs)
    o->xinertia = 0;
  }

  // gleaming animation
  if (++o->timer < 3)
    o->frame = 1;
  else if (o->timer < 6)
    o->frame = 2;
  else
  {
    o->frame = 0;
    if (o->timer >= 80)
      o->timer = 0;
  }

  // need this for the big jelly that drops a chest in Weed
  if (o->blockd)
    o->xinertia = 0;

  // fall
  o->yinertia += 0x40;
  LIMITY(0x5ff);
}

void ai_chest_open(Object *o)
{
  o->flags |= FLAG_SCRIPTONACTIVATE;
}

/*
void c------------------------------() {}
*/

void ai_lightning(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->invisible = true;
      o->state     = 1;

      if (o->dir == RIGHT)
        flashscreen.Start();
    }
    case 1:
    {
      if (++o->timer > 10)
      {
        o->state     = 2;
        o->invisible = false;
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_LIGHTNING_STRIKE);
      }
    }
    break;

    case 2:
    {
      if (++o->animtimer > 2)
      {
        o->animtimer = 0;

        if (o->frame >= 3)
        {
          for (int i = 0; i < 5; i++)
            SmokePuff(o->ActionPointX(), o->ActionPointY());

          effect(o->ActionPointX(), o->ActionPointY(), EFFECT_BOOMFLASH);
          o->Delete();
        }
        else
        {
          if (++o->frame == 2)
            o->damage = 10;
        }
      }
    }
    break;
  }
}

void ai_teleporter(Object *o)
{
  if (o->state == 0)
  {
    o->frame = 0;
  }
  else
  {
    o->frame ^= 1;
  }
}

void ai_door(Object *o)
{
  switch (o->state)
  {
    case 0:
      o->frame = (o->dir == RIGHT) ? 1 : 0;
      break;

    case 1: // being banged on
      o->shaketime = 20;
      o->state     = 0;
      break;
  }
}

void ai_largedoor(Object *o)
{
  switch (o->state)
  {
    case 0:
      o->state = 1;
      if (o->dir == RIGHT)
      {
        // o->frame = 1;
        o->x -= (TILE_W * CSFI);
      }
      o->xmark = o->x;
      break;

    case 10: // door opens into frame
      o->state = 11;
      o->timer = 0;
      o->flags |= FLAG_IGNORE_SOLID;
      o->clip_enable = 1;
    case 11:
    {
      int px;
      o->timer++;
      if ((o->timer & 7) == 0)
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_QUAKE);
      }

      px = (o->timer >> 3);
      if (o->dir == LEFT)
      {
        o->x      = o->xmark - (px * CSFI);
        o->clipx1 = px;
      }
      else
      {
        o->x      = o->xmark + (px * CSFI);
        o->clipx2 = (16 - px);
      }

      if (o->timer == 104)
        o->Delete();
    }
  }
}

void ai_press(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      if (!o->blockd)
      {
        o->state = 10;
        o->frame = 1;
      }
    }
    break;

    case 10: // fall
    {
      if (o->frame < 2)
        ANIMATE_FWD(2);

      o->yinertia += 0x20;
      LIMITY(0x5ff);

      if (o->y < player->y)
      {
        o->flags &= ~FLAG_SOLID_BRICK;
        o->damage = 127;
      }
      else
      {
        o->flags |= FLAG_SOLID_BRICK;
        o->damage = 0;
      }

      if (o->blockd)
      {
        SmokeSide(o, 4, DOWN);
        quake(10);

        o->state  = 11;
        o->frame  = 0;
        o->damage = 0;
        o->flags |= FLAG_SOLID_BRICK;
      }
    }
    break;
  }
}

void ai_terminal(Object *o)
{
  switch (o->state)
  {
    case 0:
    case 1:
      o->frame = 0;
      if (pdistlx(8 * CSFI) && pdistly2(16 * CSFI, 8 * CSFI))
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_COMPUTER_BEEP);
        o->frame = 1;
        o->state = 10;
      }
      break;

    case 10:
      ANIMATE(0, 1, 2);
      break;
  }
}

/*
void c------------------------------() {}
*/

#define FAN_BLOW_FORCE 0x88

void ai_fan_vert(Object *o)
{
  if (o->dir == LEFT)
  { // fan off
    o->frame = 0;
    return;
  }

  int blowdir = (o->type == OBJ_FAN_UP) ? UP : DOWN;
  ANIMATE(0, 0, 2);

  // spawn droplet effects
  if (pdistlx(SCREEN_WIDTH * CSFI) && pdistly(SCREEN_HEIGHT * CSFI))
  {
    if (!random(0, 5))
    {
      int x = o->x + (random(4, 12) * CSFI);
      int y = (blowdir == DOWN) ? o->Bottom() : o->y;

      Object *drop = CreateObject(x, y, OBJ_FAN_DROPLET);
      drop->dir    = blowdir;
    }
  }

  // blow player
  if (pdistlx(8 * CSFI) && pdistly(96 * CSFI))
  {
    if (blowdir == UP && player->y < o->y)
      player->yinertia -= FAN_BLOW_FORCE;

    if (blowdir == DOWN && player->y > o->Bottom())
      player->yinertia += FAN_BLOW_FORCE;
  }
}

void ai_fan_hoz(Object *o)
{
  if (o->dir == LEFT)
  { // fan off
    o->frame = 0;
    return;
  }

  int blowdir = (o->type == OBJ_FAN_LEFT) ? LEFT : RIGHT;
  ANIMATE(0, 0, 2);

  // spawn droplet effects
  if (pdistlx(SCREEN_WIDTH * CSFI) && pdistly(SCREEN_HEIGHT * CSFI))
  {
    if (!random(0, 5))
    {
      int x = (blowdir == LEFT) ? o->x : o->Right();
      int y = o->y + (random(4, 12) * CSFI);

      Object *drop = CreateObject(x, y, OBJ_FAN_DROPLET);
      drop->dir    = blowdir;
    }
  }

  // blow player
  if (pdistlx(96 * CSFI) && pdistly(8 * CSFI))
  {
    if (blowdir == LEFT && player->x < o->x)
      player->xinertia -= FAN_BLOW_FORCE;

    if (blowdir == RIGHT && player->x > o->Right())
      player->xinertia += FAN_BLOW_FORCE;
  }
}

// the visible moving air current from fans
void ai_fan_droplet(Object *o)
{
  switch (o->state)
  {
    case 0:
      o->state = 1;
      o->frame = random(0, 2);

      o->xinertia = o->yinertia = 0;

      switch (o->dir)
      {
        case LEFT:
          o->xinertia = -1;
          break;
        case RIGHT:
          o->xinertia = 1;
          break;
        case UP:
          o->yinertia = -1;
          break;
        case DOWN:
          o->yinertia = 1;
          break;
      }

      o->dir = RIGHT; // so frame is correct

      o->xinertia *= random((2 * CSFI), (4 * CSFI));
      o->yinertia *= random((2 * CSFI), (4 * CSFI));
    case 1:
      ANIMATE_FWD(6);
      if (o->frame > 4)
        o->Delete();
      break;
  }
}

/*
void c------------------------------() {}
*/

// Yamashita Farm and Plantation water-droplet emitter
// throws out oodles of water splash droplets
void ai_sprinkler(Object *o)
{
  if (o->dir == RIGHT)
    return;

  if (++o->animtimer & 1)
    o->frame ^= 1;

  if (pdistlx(0x28000) && pdistly(0x1E000))
  {
    Object *drop;

    drop = CreateObject(o->CenterX() + (1 * CSFI), o->CenterY() + (1 * CSFI), OBJ_WATER_DROPLET);

    drop->xinertia = random(-(2 * CSFI), (2 * CSFI));
    drop->yinertia = random(-(3 * CSFI), 384);
  }
}

// generates small splash water droplets
void ai_droplet_spawner(Object *o)
{
  if (pdistlx(SCREEN_WIDTH * CSFI) && pdistly(SCREEN_HEIGHT * CSFI))
  {
    if (!random(0, 80))
    {
      CreateObject(o->x + (random(2, (TILE_W - 2)) * CSFI), o->y, OBJ_WATER_DROPLET);
    }
  }
}

// small flying water droplet from splashes or sprinklers
void ai_water_droplet(Object *o)
{
  static const Point waterattrpoint[] = {{0, -3}};

  o->flags &= ~FLAG_IGNORE_SOLID;

  o->yinertia += 0x20;
  if (o->yinertia > 0x5ff)
    o->yinertia = 0x5ff;

  o->frame = random(0, 4);

  if (++o->timer > 10)
  {
    if (o->blockl || o->blockr || o->blockd || (o->GetAttributes(waterattrpoint, 1, NULL) & TA_WATER))
    {
      o->Delete();
    }
  }
}

/*
void c------------------------------() {}
*/

void ai_bubble_spawner(Object *o)
{
  Object *ko;

  if (o->dir == LEFT)
  { // spawn smoke (broken motorcycle in Grass)
    if (!random(0, 40))
    {
      ko           = CreateObject(o->x + (random(-20, 20) * CSFI), o->y, OBJ_SMOKE_CLOUD);
      ko->xinertia = 0x100;
      ko->yinertia = -0x200;
    }
  }
  else
  { // spawn "bubbles" (IronH battle)
    ko = CreateObject(o->x + (random(-160, 160) * CSFI), o->y + (random(-128, 128) * CSFI), OBJ_FAN_DROPLET);

    ko->dir      = RIGHT;
    ko->xinertia = 0x100;
  }
}

void ai_chinfish(Object *o)
{
  switch (o->state)
  {
    case 0:
      o->state    = 1;
      o->xmark    = o->x;
      o->ymark    = o->y;
      o->yinertia = 0x88;
    case 1:
      o->yinertia += (o->y > o->ymark) ? -8 : 8;
      LIMITY(0x100);
      ANIMATE(4, 0, 1);
      if (o->shaketime)
        o->frame = 2;
      break;
  }
}

void ai_fireplace(Object *o)
{
  switch (o->state)
  {
    case 0: // burn
      o->frame     = 0;
      o->state     = 1;
      o->invisible = 0;
    case 1:
      ai_animate4(o);
      break;

    case 10: // extinguished by Jellyfish Juice
      o->state = 11;
      effect(o->CenterX(), o->CenterY(), EFFECT_BOOMFLASH);
      SmokeClouds(o, 8, 16, 16);
    case 11:
      o->invisible = 1;
      break;
  }
}

// straining effect from Boulder Chamber
void ai_straining(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      if (o->dir == LEFT)
      { // curly's straining
        o->x += (14 * CSFI);
        o->y -= (18 * CSFI);
      }
      else
      { // player's straining
        o->x = player->x - (6 * CSFI);
        o->y = player->y - (2 * CSFI);
      }

      o->state = 1;
    }
    case 1:
    {
      if (++o->animtimer > 8)
      {
        o->animtimer = 0;
        o->frame ^= 1;

        if (++o->timer2 >= 8)
          o->Delete();
      }
    }
    break;
  }
}

// used by CMP to delay smoke created during a PRI until the PRI is released.
// See the description in tsc.cpp.
void ai_smoke_dropper(Object *o)
{
  SmokeXY(o->x, o->y, o->timer2, TILE_W / 2, TILE_H / 2);
  o->Delete();
}

/*
void c------------------------------() {}
*/

// this object is used in a few places, such as during the Red Demon fight (last cave),
// and during some of the end sequences. It seems to be primarily involved with providing
// "extra" map scrolling modes. Generally you'll <FON on it, then set the mode you desire.
void ai_scroll_controller(Object *o)
{
  // debug("scrollctrl: state %d; dp %d; linked %08x", o->state, o->dirparam, o->linkedobject);

  switch (o->state)
  {
    // stay above player's head. This is used during the "mad run" Balcony2 stage;
    // you'll notice there is not normal scrolling during this part.
    case 10:
    {
      o->x = player->x;
      o->y = player->y - (32 * CSFI);
    }
    break;

    // pan in the specified direction. used when you get the good ending
    // to pan over all the scenes from the island just before it crashes.
    case 20:
    {
      switch (o->dir)
      {
        case LEFT:
          o->x -= (2 * CSFI);
          break;
        case UP:
          o->y -= (2 * CSFI);
          break;
        case RIGHT:
          o->x += (2 * CSFI);
          break;
        case DOWN:
          o->y += (2 * CSFI);
          break;
      }

      // player is invisible during this part. dragging him along is
      // what makes all the monsters, falling spikes etc react.
      player->x = o->x;
      player->y = o->y;
    }
    break;

    // stay below player.
    case 30:
    {
      o->x = player->x;
      o->y = player->y + (80 * CSFI);
    }
    break;

    // stay mid-way between player and the specified object.
    // used during the Red Demon fight in Last Cave (hidden).
    case 100:
    {
      o->state = 101;

      if (o->dirparam == 0)
      {
        o->linkedobject = game.stageboss.object;

        if (!o->linkedobject)
        {
          staterr("sctrl: no stageboss object!");
          o->Delete();
        }
      }
      else
      {
        o->linkedobject = FindObjectByID2(o->dirparam);

        if (o->linkedobject)
        {
          staterr("sctrl: successfully linked to object %08x", o->linkedobject);
        }
        else
        {
          staterr("sctrl: failed to link to id2 %d: object not found", o->id2);
          o->Delete();
        }
      }
    }
    case 101:
    {
      if (o->linkedobject)
      {
        o->x = (player->x + o->linkedobject->x) / 2;
        o->y = (player->y + o->linkedobject->y) / 2;
      }
    }
    break;
  }
}

// makes a perpetual quake. used during end-run and Hell.
void ai_quake(Object *o)
{
  if ((game.curmap != STAGE_HELL1 && game.curmap != STAGE_HELL2 && game.curmap != STAGE_HELL3
       && game.curmap != STAGE_HELL4 && game.curmap != STAGE_HELL42 && game.curmap != STAGE_STATUE_CHAMBER
       && game.curmap != STAGE_CORRIDOR && game.curmap != STAGE_SEAL_CHAMBER))
  {
    // game.quaketime = 10;
    quake(10, NXE::Sound::SFX::SND_NULL);
  }
}

/*
void c------------------------------() {}
*/

void ai_generic_angled_shot(Object *o)
{
  // enemies can set the shot's ttl by setting an initial timer value
  // after they spawn us. Or, they can leave it at zero and we
  // will set a default.
  if (o->state == 0)
  {
    o->state = 1;

    if (o->timer == 0)
      o->timer = 200;
  }

  if (o->sprite == SPR_GAUDI_FLYING_SHOT)
  {
    ANIMATE(0, 0, sprites[o->sprite].nframes - 1);
  }
  else
  {
    ANIMATE(2, 0, sprites[o->sprite].nframes - 1);
  }

  if (o->blockl && o->xinertia < 0)
    goto del;
  if (o->blockr && o->xinertia > 0)
    goto del;
  if (o->blocku && o->yinertia < 0)
    goto del;
  if (o->blockd && o->yinertia > 0)
    goto del;

  if (--o->timer < 0)
  {
  del:;

    effect(o->CenterX(), o->CenterY(), EFFECT_FISHY);
    o->Delete();
  }
}

/*
void c------------------------------() {}
*/

void onspawn_spike_small(Object *o)
{
  o->frame = o->id2;

  // hack to remove 2 spikes in First Cave which are
  // extraneous and invisible because they are embedded
  // in the wall, but due to slight engine differences
  // you can still sometimes get hurt by them in our engine.
  int tile = map.tiles[(o->CenterX() / CSFI) / TILE_W][(o->CenterY() / CSFI) / TILE_H];
  if (tileattr[tile] & TA_SOLID)
  {
    stat("onspawn_spike_small: spike %08x embedded in wall, deleting", o);
    o->Delete();
  }
}
