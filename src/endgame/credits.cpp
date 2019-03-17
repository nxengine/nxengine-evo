
#include "credits.h"

#include "../ResourceManager.h"
#include "../autogen/sprites.h"
#include "../common/misc.h"
#include "../common/stat.h"
#include "../console.h"
#include "../game.h"
#include "../graphics/Renderer.h"
#include "../map.h"
#include "../nx.h"
#include "../player.h"
#include "../sound/SoundManager.h"
#include "../tsc.h"
using namespace NXE::Graphics;

#define MARGIN 48
#define SCREEN_Y(Y) ((Y) - (scroll_y / CSFI))

Credits *credits = NULL;

/*
void c------------------------------() {}
*/

bool Credits::Init()
{
  if (script.OpenFile())
    return 1;
  if (bigimage.Init())
    return 1;

  spawn_y  = (Renderer::getInstance()->screenHeight + 8);
  scroll_y = 0 * CSFI;

  xoffset      = 0;
  roll_running = true;

  lines_out = lines_vis = 0;

  firstline = NULL;
  lastline  = NULL;

  return 0;
}

Credits::~Credits()
{
  script.CloseFile();
}

/*
void c------------------------------() {}
*/

void Credits::Tick()
{
  /*debug("scroll_y: %d", scroll_y/CSFI);
  debug("spawn_y: %d", spawn_y);
  debug("scr_spawn_y: %d", SCREEN_Y(spawn_y));
  debug("trigger: %d", Renderer::getInstance()->screenHeight+MARGIN);
  debug("");*/
  /*debug("imgno: %d", bigimage.imgno);
  debug("state: %d", bigimage.state);
  debug("imagex: %d", bigimage.imagex);*/

  if (roll_running || SCREEN_Y(spawn_y) >= (Renderer::getInstance()->screenHeight + 8))
  {
    scroll_y += 0x100;
  }

  while (roll_running && SCREEN_Y(spawn_y) < (Renderer::getInstance()->screenHeight + MARGIN))
  {
    RunNextCommand();
  }

  if (player)
  {
    player->hide = true;
    player->dead = true; // should pretty much completely disable HandlePlayer()
  }

  game_tick_normal();
  bigimage.Draw();
  Draw();
}

void Credits::RunNextCommand()
{
  CredCommand cmd;

  if (script.ReadCommand(&cmd))
  {
    console.Print("script.ReadCommand failed: credits terminated");
    roll_running = false;
    return;
  }

  cmd.DumpContents();

  switch (cmd.type)
  {
    case CC_TEXT:
    {
      CredLine *line = NewLine();

      maxcpy(line->text, cmd.text, sizeof(line->text));
      line->image = cmd.parm;
      line->x     = xoffset;
      line->y     = spawn_y;

      // the last line is supposed to be centered--slightly
      // varying font sizes can lead to it being a little bit off
      if (strstr(line->text, "The End"))
      {
        line->x = (Renderer::getInstance()->screenWidth / 2) - (Renderer::getInstance()->font.getWidth(line->text) / 2);
      }

      spawn_y += 1;
      lines_out++;
    }
    break;

    case CC_BLANK_SPACE:
      spawn_y += cmd.parm / 2;
      break;

    case CC_SET_XOFF:
      xoffset = cmd.parm;
      break;

    case CC_FLAGJUMP:
      if (game.flags[cmd.parm])
        Jump(cmd.parm2);
      break;

    case CC_JUMP:
      Jump(cmd.parm);
      break;

    case CC_LABEL:
      break;

    case CC_MUSIC:
      NXE::Sound::SoundManager::getInstance()->music(cmd.parm);
      break;
    case CC_FADE_MUSIC:
      NXE::Sound::SoundManager::getInstance()->fadeMusic();
      break;

    case CC_END:
      roll_running = false;
      break;

    default:
      console.Print("Unhandled command '%c'; credits terminated", cmd.type);
      roll_running = false;
      break;
  }
}

bool Credits::Jump(int label)
{
  CredCommand cmd;
  bool tried_rewind = false;

  stat("- Jump to label %04d", label);

  for (;;)
  {
    if (script.ReadCommand(&cmd) || cmd.type == CC_END)
    { // I think all the jumps in the original credits are forwards,
      // so only try looking back if there's a problem finding the label.
      if (!tried_rewind)
      {
        script.Rewind();
        tried_rewind = true;
      }
      else
      {
        console.Print("Missing label %04d; credits terminated", label);
        roll_running = false;
        return 1;
      }
    }

    if (cmd.type == CC_LABEL && cmd.parm == label)
    {
      return 0;
    }
  }
}

/*
void c------------------------------() {}
*/

bool Credits::DrawLine(CredLine *line)
{
  int x = line->x;
  int y = SCREEN_Y(line->y);
  if (y < -MARGIN)
    return true; // line can be deleted now

  if (line->image)
  {
    Renderer::getInstance()->sprites.drawSprite(x - 24, y - 8, SPR_CASTS, line->image);
    // DrawBox(x, y, x+Renderer::getInstance()->font.getWidth(line->text), y+8,  56, 0, 0);
  }

  // DrawRect(x, y, x+63, y+8, 128, 0, 0);
  Renderer::getInstance()->font.draw(x, y, line->text);

  return false;
}

void Credits::Draw()
{
  CredLine *line, *next;

  line = firstline;
  while (line)
  {
    next = line->next;

    if (DrawLine(line))
    {
      RemoveLine(line);
      delete line;
    }

    line = next;
  }
}

/*
void c------------------------------() {}
*/

CredLine *Credits::NewLine()
{
  return AddLine(new CredLine);
}

CredLine *Credits::AddLine(CredLine *line)
{
  line->prev = NULL;
  line->next = firstline;

  if (firstline)
  {
    firstline->prev = line;
    firstline       = line;
  }
  else
  {
    firstline = lastline = line;
  }

  lines_vis++;
  return line;
}

void Credits::RemoveLine(CredLine *line)
{
  if (line->next)
    line->next->prev = line->prev;
  if (line->prev)
    line->prev->next = line->next;
  if (line == firstline)
    firstline = firstline->next;
  if (line == lastline)
    lastline = lastline->next;
  lines_vis--;
}

/*
void c------------------------------() {}
*/

enum BIStates
{
  BI_CLEAR,
  BI_SLIDE_IN,
  BI_SLIDE_OUT,
  BI_HOLD
};

bool BigImage::Init()
{
  char fname[MAXPATHLEN];

  imagex = 0;
  imgno  = 0;
  state  = BI_CLEAR;
  memset(images, 0, sizeof(images));

  // load any images present
  for (int i = 0; i < MAX_BIGIMAGES; i++)
  {
    if (i > 0 && i < 4 && game.flags[1020])
      sprintf(fname, "endpic/credit%02dm.bmp", i);
    else
      sprintf(fname, "endpic/credit%02d.bmp", i);
    if (ResourceManager::fileExists(ResourceManager::getInstance()->getLocalizedPath(fname)))
    {
      images[i] = Surface::fromFile(ResourceManager::getInstance()->getLocalizedPath(fname), false);
      if (!images[i])
        staterr("BigImage::Init: image '%s' exists but seems corrupt!", fname);
      else
        stat("BigImage: loaded %s ok", fname);
    }
  }

  return 0;
}

BigImage::~BigImage()
{
  for (int i = 0; i < MAX_BIGIMAGES; i++)
  {
    if (images[i])
    {
      staterr("BigImage: freeing image %d", i);
      delete images[i];
      images[i] = NULL;
    }
  }
}

void BigImage::Set(int num)
{
  if (images[num])
  {
    imgno  = num;
    imagex = -images[num]->width();
    state  = BI_SLIDE_IN;
  }
  else
  {
    staterr("BigImage::Set: invalid image number %d", num);
    state = BI_CLEAR;
  }
}

void BigImage::Clear()
{
  state = BI_SLIDE_OUT;
}

void BigImage::Draw()
{
#define IMAGE_SPEED 32

  switch (state)
  {
    case BI_SLIDE_IN:
    {
      imagex += IMAGE_SPEED;
      if (imagex > 0)
      {
        imagex = 0;
        state  = BI_HOLD;
      }
    }
    break;

    case BI_SLIDE_OUT:
    {
      imagex -= IMAGE_SPEED;
      if (imagex < -images[imgno]->width())
        state = BI_CLEAR;
    }
  }

  // take up any unused space with blue
  if (state != BI_HOLD)
    Renderer::getInstance()->fillRect(0, 0, Renderer::getInstance()->screenWidth / 2, Renderer::getInstance()->screenHeight, DK_BLUE);

  if (state != BI_CLEAR)
    Renderer::getInstance()->drawSurface(images[imgno], imagex, 0);
}

/*
void c------------------------------() {}
*/

bool credit_init(int parameter)
{
  credits = new Credits;
  if (credits->Init())
  {
    staterr("Credits initilization failed");
    return 1;
  }

  return 0;
}

void credit_close()
{
  delete credits;
  credits = NULL;
}

void credit_tick()
{
  if (credits)
    credits->Tick();
}

void credit_draw()
{
  if (credits)
  {
    credits->bigimage.Draw();
    credits->Draw();
  }
}

void credit_set_image(int imgno)
{
  if (credits)
    credits->bigimage.Set(imgno);
}

void credit_clear_image()
{
  if (credits)
    credits->bigimage.Clear();
}

/*
void c------------------------------() {}
*/
