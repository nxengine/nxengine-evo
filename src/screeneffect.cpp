
#include "screeneffect.h"

#include "graphics/Renderer.h"
#include "nx.h"
#include "sound/SoundManager.h"
using namespace NXE::Graphics;
#include "map.h"

SE_FlashScreen flashscreen;
SE_Starflash starflash;
SE_Fade fade;

/*
void c------------------------------() {}
*/

// FlashScreen simply flashes the screen white several times,
// and is used in various places, such as when Misery casts spells.
void SE_FlashScreen::Start(void)
{
  timer        = -1;
  flashes_left = 5;
  flashstate   = true;

  enabled = true;
}

void SE_FlashScreen::Draw(void)
{
  if (++timer >= 2)
  {
    timer = 0;
    flashstate ^= 1;

    if (!flashstate)
    {
      if (--flashes_left <= 0)
        enabled = false;
    }
  }

  if (flashstate)
    Renderer::getInstance()->clearScreen(0xff, 0xff, 0xff);
}

/*
void c------------------------------() {}
*/

// Starflash is a full-screen white explosion in the shape of a '+',
// used when some bosses are defeated.

void SE_Starflash::Start(int x, int y)
{
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EXPLOSION1);
  enabled = true;
  state   = 0;
  centerx = x;
  centery = y;
  size = speed = 0;
}

void SE_Starflash::Draw(void)
{
  SE_Starflash *const &star = this;
  int scr_x1, scr_y1, scr_x2, scr_y2;
  int rel_x, rel_y;

  if (state == 0)
  { // flash getting bigger
    star->speed += (1 * CSFI);
    star->size += star->speed;
  }
  else
  { // flash getting smaller
    star->size -= (star->size >> 3);

    if (star->size < 255)
    {
      enabled = false;
      return;
    }
  }

  // draw the flash
  rel_x = (star->centerx - map.displayed_xscroll);
  rel_y = (star->centery - map.displayed_yscroll);

  // draw a horizontal bar
  scr_y1 = (rel_y - star->size) / CSFI;
  scr_y2 = (rel_y + star->size) / CSFI;
  Renderer::getInstance()->fillRect(0, scr_y1, Renderer::getInstance()->screenWidth, scr_y2, 255, 255, 255);

  if (star->state == 0)
  {
    // draw a vertical bar
    scr_x1 = (rel_x - starflash.size) / CSFI;
    scr_x2 = (rel_x + starflash.size) / CSFI;
    Renderer::getInstance()->fillRect(scr_x1, 0, scr_x2, Renderer::getInstance()->screenHeight, 255, 255, 255);

    // once it's big enough, switch to making it smaller
    if (star->size > (1280 * CSFI))
    {
      star->size  = (Renderer::getInstance()->screenHeight * CSFI);
      star->state = 1;
    }
  }
}

/*
void c------------------------------() {}
*/

#define FADE_LAST_FRAME 15

// Fade is the fade-in/out used on every stage transistion/TRA.
// Unlike other effects, it is drawn underneath the textboxes and Nikumaru counter,
// and so isn't drawn from ScreenEffects::Draw().

SE_Fade::SE_Fade()
{
  state   = FS_NO_FADE;
  enabled = false;
}

// start a fade in or out.
// fadedir: either FADE_IN or FADE_OUT
// sweepdir: direction to "sweep" the fade
// spr: the sprite to use for the fading
void SE_Fade::Start(int fadedir, int sweepdir, int spr)
{
  if (fadedir == FADE_OUT)
  {
    // when fading out, the directions are reversed
    switch (sweepdir)
    {
      case FADE_LEFT:
        sweepdir = FADE_RIGHT;
        break;
      case FADE_RIGHT:
        sweepdir = FADE_LEFT;
        break;
      case FADE_UP:
        sweepdir = FADE_DOWN;
        break;
      case FADE_DOWN:
        sweepdir = FADE_UP;
        break;
    }
  }

  state   = FS_FADING;
  enabled = true;

  fade.sprite = spr;

  fade.fadedir  = fadedir;
  fade.sweepdir = sweepdir;
  fade.curframe = (fadedir == FADE_OUT) ? -FADE_LAST_FRAME : FADE_LAST_FRAME;
}

void SE_Fade::Draw(void)
{
  int x, y;

  if (state == FS_NO_FADE)
  {
    return;
  }
  else if (state == FS_FADED_OUT)
  {
    Renderer::getInstance()->clearScreen(DK_BLUE);
    return;
  }

  int frame = fade.curframe;
  switch (fade.sweepdir)
  {
    case FADE_RIGHT:
      for (x = 0; x < Renderer::getInstance()->screenWidth; x += 16)
      {
        if (frame >= 0)
        {
          if (frame > FADE_LAST_FRAME)
            frame = FADE_LAST_FRAME;

          for (y = 0; y < Renderer::getInstance()->screenHeight; y += 16)
            Renderer::getInstance()->sprites.drawSprite(x, y, fade.sprite, frame);
        }
        frame++;
      }
      break;

    case FADE_LEFT:
      for (x = Renderer::getInstance()->screenWidth - 1; x >= -16; x -= 16)
      {
        if (frame >= 0)
        {
          if (frame > FADE_LAST_FRAME)
            frame = FADE_LAST_FRAME;

          for (y = 0; y < Renderer::getInstance()->screenHeight; y += 16)
            Renderer::getInstance()->sprites.drawSprite(x, y, fade.sprite, frame);
        }
        frame++;
      }
      break;

    case FADE_DOWN:
      for (y = 0; y < Renderer::getInstance()->screenHeight; y += 16)
      {
        if (frame >= 0)
        {
          if (frame > FADE_LAST_FRAME)
            frame = FADE_LAST_FRAME;

          for (x = 0; x < Renderer::getInstance()->screenWidth; x += 16)
            Renderer::getInstance()->sprites.drawSprite(x, y, fade.sprite, frame);
        }
        frame++;
      }
      break;

    case FADE_UP:
      for (y = Renderer::getInstance()->screenHeight - 1; y >= -16; y -= 16)
      {
        if (frame >= 0)
        {
          if (frame > FADE_LAST_FRAME)
            frame = FADE_LAST_FRAME;

          for (x = 0; x < Renderer::getInstance()->screenWidth; x += 16)
            Renderer::getInstance()->sprites.drawSprite(x, y, fade.sprite, frame);
        }
        frame++;
      }
      break;

    case FADE_CENTER:
    {
      int startframe = fade.curframe;
      int centerx    = (Renderer::getInstance()->screenWidth / 2) - 8;
      int centery    = (Renderer::getInstance()->screenHeight / 2) - 8;

      for (x = 0; x < centerx + 16; x += 16)
      {
        frame = startframe;

        for (y = 0; y < centery + 16; y += 16)
        {
          if (frame >= 0)
          {
            if (frame > FADE_LAST_FRAME)
              frame = FADE_LAST_FRAME;

            Renderer::getInstance()->sprites.drawSprite(centerx + x, centery + y, fade.sprite, frame);
            Renderer::getInstance()->sprites.drawSprite(centerx - x, centery + y, fade.sprite, frame);
            Renderer::getInstance()->sprites.drawSprite(centerx + x, centery - y, fade.sprite, frame);
            Renderer::getInstance()->sprites.drawSprite(centerx - x, centery - y, fade.sprite, frame);
          }

          frame++;
        }

        startframe++;
      }
    }
    break;
  }

  if (fade.fadedir == FADE_OUT)
  {
    fade.curframe++;
    if (fade.curframe > FADE_LAST_FRAME)
      state = FS_FADED_OUT;
  }
  else
  { // fading in--terminate fade when done
    fade.curframe--;
    if (fade.curframe < -20)
    {
      state   = FS_NO_FADE;
      enabled = false;
    }
  }
}

void SE_Fade::set_full(int dir)
{
  if (dir == FADE_OUT)
  {
    state = FS_FADED_OUT;

    fade.fadedir  = FADE_OUT;
    fade.sweepdir = FADE_RIGHT; // doesn't matter
    fade.curframe = FADE_LAST_FRAME;
    fade.sprite   = SPR_FADE_DIAMOND;
  }
  else
  {
    state   = FS_NO_FADE;
    enabled = false;
  }
}

int SE_Fade::getstate(void)
{
  return this->state;
}

/*
void c------------------------------() {}
*/

void ScreenEffects::Draw(void)
{
  if (starflash.enabled)
    starflash.Draw();

  if (flashscreen.enabled)
    flashscreen.Draw();
}

void ScreenEffects::Stop()
{
  starflash.enabled   = false;
  flashscreen.enabled = false;
}
