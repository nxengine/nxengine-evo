
#include "console.h"

#include "ObjManager.h"
#include "game.h"
#include "graphics/Renderer.h"
#include "nx.h"
#include "settings.h"
#include "statusbar.h"
#include "tsc.h"

using namespace NXE::Graphics;

#include "common/misc.h"
#include "Utils/Logger.h"
#include "debug.h"
#include "map.h"
#include "p_arms.h"
#include "player.h"
#include "playerstats.h"
#include "sound/SoundManager.h"

#include <cstdarg>
#include <string>
#include <vector>
#include <SDL.h>

#include "sound/SoundManager.h"
#include "player.h"

#define Respond console.Print

#if defined(_WIN32)
#define strcasecmp _stricmp
#endif

static void __god(std::vector<std::string> *args, int num)
{
  game.debug.god = !game.debug.god;
  Respond("God mode:  {}", game.debug.god ? "enabled" : "disabled");
}

static void __script(std::vector<std::string> *args, int num)
{
  // release any focus a current script may have on us
  if (player->movementmode == MOVEMODE_NORMAL)
    map_focus(NULL);

  if (game.tsc->StartScript(num))
  {
    Respond("Script {:04d} started.", num);
  }
  else
  {
    Respond("No such script {:04d}", num);
  }
}

static void __warp(std::vector<std::string> *args, int num)
{
  if (num == 0)
  {
    std::string stagename;
    for (unsigned int i = 0; i < args->size(); i++)
    {
      if (i != 0)
        stagename += ' ';
      stagename += (args->at(i));
    }

    LOG_DEBUG("Looking for '{}'", stagename.c_str());
    for (num = 0;; num++)
    {
      if (num >= num_stages)
      {
        if (!strcasecmp(stagename.c_str(), "village"))
        {
          num = 11;
        }
        else
        {
          Respond("Could determine stage number from your description.");
          return;
        }

        break;
      }

      if (strcasebegin(stages[num].stagename, stagename.c_str()))
        break;
      if (strcasebegin(stages[num].filename, stagename.c_str()))
        break;
    }
  }

  game.switchstage.mapno   = num;
  game.switchstage.playerx = 16;
  game.switchstage.playery = 16;
}

static void __sound(std::vector<std::string> *args, int num)
{
  NXE::Sound::SoundManager::getInstance()->playSfx((NXE::Sound::SFX)num);
  console.SetVisible(true); // keep console up
}

std::vector<std::string> org_names =
{{
    "",
    "egg", "safety", "gameover", "gravity", "grasstown", "meltdown2", "eyesofflame",
    "gestation", "town", "fanfale1", "balrog", "cemetary", "plant", "pulse", "fanfale2",
    "fanfale3", "tyrant", "run", "jenka1", "labyrinth", "access", "oppression", "geothermal",
    "theme", "oside", "heroend", "scorching", "quiet", "lastcave", "balcony", "charge",
    "lastbattle", "credits", "zombie", "breakdown", "hell", "jenka2", "waterway", "seal",
    "toroko", "white", "azarashi", ""
}};

static void __music(std::vector<std::string> *args, int num)
{
    extern std::vector<std::string> org_names;
    bool ok = true;
    std::vector<std::string>::size_type i;

    const char *name = args->at(0).c_str();
    if (num == 0 && strcmp(name, "0") != 0)
    {
      for (i = 1; i < org_names.size(); i++)
      {
        if (strcasebegin(org_names[i].c_str(), name))
        {
          num = i;
          break;
        }
      }

      if (num == 0)
      {
        Respond("Don't know that song.");
        return;
      }
    }

    if (num < 0)
      ok = false;
    else if (num >= (int)org_names.size())
      ok = false;

    if (!ok)
    {
      Respond("track out of range");
      ::NXE::Sound::SoundManager::getInstance()->music(0);
    }
    else
    {
      ::NXE::Sound::SoundManager::getInstance()->music(0);
      ::NXE::Sound::SoundManager::getInstance()->music(num);
      if (num > 0)
        Respond("%s started", org_names[num].c_str());
      else
        Respond("ZERO MUZAK");
    }
}

static void __giveweapon(std::vector<std::string> *args, int num)
{
  if (num >= 0 && num < WPN_COUNT)
  {
    GetWeapon(num, 100);
  }
}

static void __dropweapon(std::vector<std::string> *args, int num)
{
  if (args->size() == 0)
    num = player->curWeapon;

  player->weapons[num].hasWeapon = 0;
  player->weapons[num].maxammo   = 0;
  player->weapons[num].ammo      = 0;

  if (num == player->curWeapon)
    stat_NextWeapon();
}

// set weapon level
static void __level(std::vector<std::string> *args, int num)
{
  num--;
  if (num < 0)
    num = 0;
  if (num > 2)
    num = 2;

  if (player->weapons[player->curWeapon].xp < 5)
    player->weapons[player->curWeapon].xp = 5;

  for (int timeout = 0; timeout < 500; timeout++)
  {
    if (player->weapons[player->curWeapon].level == num)
    {
      return;
    }
    else if (player->weapons[player->curWeapon].level < num)
    {
      AddXP(1);
    }
    else
    {
      SubXP(1);
    }
  }

//  Respond("Timeout");
}

static void __ammo(std::vector<std::string> *args, int num)
{
  player->weapons[player->curWeapon].ammo = num;
  if (player->weapons[player->curWeapon].ammo > player->weapons[player->curWeapon].maxammo)
    player->weapons[player->curWeapon].maxammo = player->weapons[player->curWeapon].ammo;
}

static void __maxammo(std::vector<std::string> *args, int num)
{
  player->weapons[player->curWeapon].maxammo = num;
  if (player->weapons[player->curWeapon].ammo > player->weapons[player->curWeapon].maxammo)
    player->weapons[player->curWeapon].ammo = player->weapons[player->curWeapon].maxammo;
}

static void __hp(std::vector<std::string> *args, int num)
{
  player->hp = num;
  if (player->hp > player->maxHealth)
    player->maxHealth = player->hp;
}

static void __maxhp(std::vector<std::string> *args, int num)
{
  player->maxHealth = num;
  if (player->hp > player->maxHealth)
  {
    player->hp = player->maxHealth;
    // PHealthBar.displayed_value = player->hp;
  }
}

static void __xp(std::vector<std::string> *args, int num)
{
  player->weapons[player->curWeapon].xp = num;
}


static void __spawn(std::vector<std::string> *args, int num)
{
  unsigned int i = 0;

  // if first argument is a number interpret it as a count of
  // objects to spawn.
  unsigned int count;
  if (isdigit(args->at(0).c_str()[0]))
  {
    count = num;
    i++;
  }
  else
  {
    count = 1;
  }

  // reconstitute the arguments into the name of the object
  // to be spawned.
  std::string objName;
  unsigned int starti = i;
  for (;; i++)
  {
    if (i >= args->size())
      break;

    if (i > starti)
      objName += ' ';
    objName += (args->at(i));
  }

  // try and get object type from the provided name
  int type = ObjectNameToType(objName.c_str());
  if (type == -1)
  {
    Respond("Unknown object. See object.h for definitions.");
    return;
  }

  // reset console animate flags on any previously spawned objects
  Object *o;
  FOREACH_OBJECT(o)
  {
    o->nxflags &= ~NXFLAG_CONSOLE_ANIMATE;
  }

  // get starting spawn position and spacing
  int x = player->x + ((player->dir == RIGHT) ? (24 * CSFI) : -(24 * CSFI));
  int y = player->y - (16 * CSFI);
  int w = (Renderer::getInstance()->sprites.sprites[objprop[type].sprite].w + 4) * CSFI;

  // create 'em
  for (i = 0; i < count; i++)
  {
    Object *o = CreateObject(x, y, type);

    o->dir = player->dir;
    o->nxflags |= NXFLAG_CONSOLE_ANIMATE;
    x += w;
  }

  if (count != 1)
    Respond("{} x{}", DescribeObjectType(type), count);
  else
    Respond("{}", DescribeObjectType(type));
}

static void __animate(std::vector<std::string> *args, int num)
{
  Object *o;

  if (args->size() == 2)
  { // specifying explicitly by id2
    o = FindObjectByID2(atoi(args->at(0).c_str()));
    if (o)
      o->state = atoi(args->at(1).c_str());
    else
      Respond("Object not found.");

    return;
  }

  // animating implicitly from last spawn command
  bool found = false;
  FOREACH_OBJECT(o)
  {
    if (o->nxflags & NXFLAG_CONSOLE_ANIMATE)
    {
      o->state = num;
      found    = true;
    }
  }

  if (!found)
    Respond("No objects found.");
}

static void __infinitedamage(std::vector<std::string> *args, int num)
{
  game.debug.infinite_damage = !game.debug.infinite_damage;
  Respond(game.debug.infinite_damage ? "My, oh my..." : "Back to normal.");
}

static void __killall(std::vector<std::string> *args, int num)
{
  for (int i = 0; i < nOnscreenObjects; i++)
  {
    Object *o = onscreen_objects[i];
    if (o->flags & FLAG_SHOOTABLE)
    {
      o->flags &= ~FLAG_INVULNERABLE;
      o->DealDamage(999);
    }
  }
}

static void __movemode(std::vector<std::string> *args, int num)
{
  player->movementmode = num;
}

static void __flag(std::vector<std::string> *args, int num)
{
  game.flags[num] ^= 1;
  Respond("Flag {:04d}: {}", num, game.flags[num] ? "SET" : "CLEARED");
}

static void __clearflags(std::vector<std::string> *args, int num)
{
  memset(game.flags, 0, sizeof(game.flags));
  Respond("Warning- all game flags cleared");
}

static void __equip(std::vector<std::string> *args, int num)
{
  static const char *equiplist[] = {"booster08", "map",        "armsbarrier", "turbocharge", "airtank",
                                    "booster20", "mimigamask", "whimstar",    "nikumaru",    NULL};
  int i, mask;

  const char *item = args->at(0).c_str();
  bool enable      = args->size() > 1 ? atoi(args->at(1).c_str()) : true;

  mask = 0x01;
  for (i = 0; equiplist[i]; i++)
  {
    if (!strcasecmp(equiplist[i], item))
    {
      // allow only booster 08 or booster 20 at a time
      if (mask & (EQUIP_BOOSTER08 | EQUIP_BOOSTER20))
        player->equipmask &= ~(EQUIP_BOOSTER08 | EQUIP_BOOSTER20);

      if (enable)
        player->equipmask |= mask;
      else
        player->equipmask &= ~mask;

      Respond("Item {} ({:#04x}) %sequipped.", equiplist[i], mask, enable ? "" : "un-");
      return;
    }

    mask <<= 1;
  }

  Respond("Unknown item");
}

static void __giveitem(std::vector<std::string> *args, int num)
{
  if (FindInventory(num) == -1)
  {
    AddInventory(num);
    Respond("Added item {} to your inventory.", num);
  }
  else
  {
    Respond("You already have item {} in your inventory.", num);
  }
}

static void __takeitem(std::vector<std::string> *args, int num)
{
  if (FindInventory(num) != -1)
  {
    DelInventory(num);
    Respond("Removed item {} from your inventory.", num);
  }
  else
  {
    Respond("You don't have item {} in your inventory.", num);
  }
}

static void __quake(std::vector<std::string> *args, int num)
{
  if (args->size() > 0)
    megaquake(50);
  else
    quake(50);
}

static void __boa(std::vector<std::string> *args, int num)
{
  game.stageboss.SetState(num);
}

// skip to normal ending sequence
static void __ending_normal(std::vector<std::string> *args, int num)
{
  game.reset();
  game.pause(0);
  game.setmode(GM_NORMAL);

  game.flags[1341]              = true;
  game.switchstage.mapno        = 70;
  game.switchstage.playerx      = 16;
  game.switchstage.playery      = 16;
  game.switchstage.eventonentry = 400;
}

// skip to good ending sequence
static void __ending_good(std::vector<std::string> *args, int num)
{
  game.reset();
  game.pause(0);
  game.setmode(GM_NORMAL);

  game.flags[1341]              = true;
  game.flags[2000]              = true;
  game.switchstage.mapno        = 71;
  game.switchstage.playerx      = 16;
  game.switchstage.playery      = 16;
  game.switchstage.eventonentry = 120;
}

// skip to credits
static void __cre(std::vector<std::string> *args, int num)
{
  game.reset();
  game.pause(0);
  game.setmode(GM_NORMAL);

  game.flags[1341]              = true;
  game.flags[162]              = true;
  game.switchstage.mapno        = 0;
  game.switchstage.playerx      = 16;
  game.switchstage.playery      = 16;
  game.switchstage.eventonentry = 100;
}

static void __cre_good(std::vector<std::string> *args, int num)
{
  game.reset();
  game.pause(0);
  game.setmode(GM_NORMAL);

  game.flags[1341]              = true;
  game.flags[162]              = true;
  game.flags[2000]              = true;
  game.switchstage.mapno        = 0;
  game.switchstage.playerx      = 16;
  game.switchstage.playery      = 16;
  game.switchstage.eventonentry = 100;
}

static void __reset(std::vector<std::string> *args, int num)
{
  game.reset();
}

static void __fps(std::vector<std::string> *args, int num)
{
  extern int fps;

  settings->show_fps ^= 1;
  settings_save();
  fps = 0;
}

/*
void c------------------------------() {}
*/

static void __skip_intro(std::vector<std::string> *args, int num)
{
  settings->skip_intro = !settings->skip_intro;
  settings_save();
  Respond("skip_intro: {}", settings->skip_intro ? "enabled" : "disabled");
}

/*
void c------------------------------() {}
*/

static void __help(std::vector<std::string> *args, int num)
{
    Respond("Available commands:");
    for (auto& command: console.getCommands())
    {
      Respond(command.name + " - " + command.help);
    }
}

static void __quit(std::vector<std::string> *args, int num)
{
  Respond("Bye-bye!");
  lastinputs[ESCKEY] = true;
  game.running       = false;
}

/*
void c------------------------------() {}
*/

static void __hide_player(std::vector<std::string> *args, int num)
{
  player->hide = num;
}

static void __lock_inputs(std::vector<std::string> *args, int num)
{
  player->inputs_locked = num;
}

static void __freeze_game(std::vector<std::string> *args, int num)
{
  game.frozen = num;
}

static void __show_textbox(std::vector<std::string> *args, int num)
{
  textbox.SetVisible(num);
}

DebugConsole::DebugConsole()
{
    commands = {
                {"hide_player",   __hide_player,    1, 1,   "Hide player <0 or 1> (default: 1)"},
                {"lock_inputs", __lock_inputs, 1, 1, "Lock inputs <0 or 1> (default: 1)"},
                {"freeze_game",    __freeze_game,    1, 1,   "Freeze game <0 or 1> (default: 1)"},
                {"show_textbox", __show_textbox, 1, 1, "Show textbox <0 or 1> (default: 1)"},
                {"reset",          __reset,          0, 0,   "Reset game"},
                {"equip",          __equip,          0, 2,   "Equip item <item> <0 or 1> (default: 1)"},
                {"god",            __god,            0, 0,   "Toggle god-mode" },
                {"script",         __script,         1, 1,   "Execute script <num>" },
                {"warp",           __warp,           1, 999, "Warp to level <name|num>" },
                {"sound",          __sound,          1, 1,   "Make a sound <num>" },
                {"music",          __music,          1, 1,   "Play music <name>" },
                {"giveweapon",     __giveweapon,     1, 1,   "Give veapon <num>" },
                {"dropweapon",     __dropweapon,     0, 1,   "Drop weapon [num] (or current, without parameters)" },

                {"level",          __level,          1, 1,   "Set current weapon level to <num>" },
                {"ammo",           __ammo,           1, 1,   "Give ammo <num>" },
                {"maxammo",        __maxammo,        1, 1,   "Set maximum ammo to <num>" },
                {"hp",             __hp,             1, 1,   "Give HP <num>" },
                {"maxhp",          __maxhp,          1, 1,   "Set maximum HP to <num>" },
                {"xp",             __xp,             1, 1,   "Give XP <num>" },
                {"spawn",          __spawn,          1, 999, "Spawn object <num>" },
                {"animate",        __animate,        1, 2,   "Set object <id> state to <num>" },
                {"infinitedamage", __infinitedamage, 0, 0,   "Infinite damage" },
                {"killall",        __killall,        0, 0,   "Kill all on-screen objects" },
                {"movemode",       __movemode,       1, 1,   "Set move mode <num>. 0 - normal, 1 - waterway, 2 - no-clip" },
                {"flag",           __flag,           1, 1,   "Toggle game flag <num>" },
                {"clearflags",     __clearflags,     0, 0,   "Clear all game flags" },

                {"giveitem",       __giveitem,       1, 1,   "Give item <num>" },
                {"takeitem",       __takeitem,       1, 1,   "Remove item <num>" },
                {"boss_state",     __boa,            1, 1,   "Set stageboss state to <num>" },
                {"quake",          __quake,          0, 1,   "Shake the screen. Pass in an argument for a megaquake"},
                {"ending_normal",  __ending_normal,  0, 0,   "Run normal ending" },
                {"ending_good",    __ending_good,    0, 0,   "Run good ending" },
                {"credits_normal", __cre,            0, 0,   "Run credits(normal)" },
                {"credits_good",   __cre_good,       0, 0,   "Run credits(good)" },
                {"fps",            __fps,            0, 0,   "Toggle fps display" },
                {"skip-intro",     __skip_intro,     0, 0,   "Toggle intro skip" },
                {"quit",           __quit,           0, 0,   "Quit game" },

                {"help",          __help,            0, 0,   "Show this help" }
               };
}

/*
void c------------------------------() {}
*/

void DebugConsole::SetVisible(bool newstate)
{
  LOG_DEBUG("DebugConsole::SetVisible({})", newstate ? "true" : "false");

  if (fVisible != newstate)
  {
    fVisible     = newstate;
    fKeyDown     = 0;
    fRepeatTimer = 0;

    if (newstate)
    {
      fLine           = "";
      fBrowsingExpansion = false;
      fBackIndex         = fBackBuffer.size() - 1;

      fResponse.clear();
      fCursorTimer = 0;
    }
  }
}

bool DebugConsole::IsVisible()
{
  return fVisible;
}

bool DebugConsole::HandleKey(int key)
{
  if (!fVisible)
    return 0;
  if (key != 9)
    fBrowsingExpansion = false;

  if (key != fKeyDown)
  {
    fKeyDown     = key;
    fRepeatTimer = 25;
  }
  else
  {
    fRepeatTimer = 1;
  }

  fCursorTimer = 0;

  switch (key)
  {
    case 27:
    case '`':
    {
      SetVisible(false);
    }
    break;

    case 13:
    case 271: // numeric enter
    {
      SetVisible(false);

      Execute(fLine);
    }
    break;

    case 10:
      break;

    case 8:
    {
      if (fLine.size() > 0)
        fLine.pop_back();
    }
    break;

    case 9: // command completion
    {
      ExpandCommand();

      fBrowsingExpansion = true;
      fExpandIndex++;
    }
    break;

    // command backbuffer
    case SDLK_UP:
    case SDLK_DOWN:
    {
      if (fBackBuffer.size() > 0)
      {
        fBackIndex += (key == SDLK_UP) ? -1 : 1;
        if (fBackIndex < 0)
          fBackIndex = (fBackBuffer.size() - 1);
        else
          fBackIndex %= fBackBuffer.size();

        fLine = fBackBuffer.at(fBackIndex);
      }
    }
    break;

    default:
    {
      fLine.append(1,(char)key);
    }
    break;
  }

  return 1;
}

void DebugConsole::HandleKeyRelease(int key)
{
  if (key == fKeyDown)
  {
    fKeyDown     = 0;
    fRepeatTimer = 0;
  }
}

/*
void c------------------------------() {}
*/

void DebugConsole::Draw()
{
  if (fResponse.size() > 0)
  {
    int y = 8 + 8 * fResponse.size();
    for (auto& str: fResponse)
    {
      this->DrawDebugText(str, y);
      y -= 8;
    }

    if (--fResponseTimer <= 0)
    {
      fResponse.erase(fResponse.begin());
      if (fResponse.size() > 0)
      {
        fResponseTimer = 30;
      }
    }
  }
  else if (fVisible)
  {
    // key-repeat
    if (fKeyDown)
    {
      if (--fRepeatTimer < 0)
        HandleKey(fKeyDown);
    }

    std::string buffer;
    buffer = "-> " + fLine + ((fCursorTimer < 20) ? "_" : " ");
    this->DrawDebugText(buffer);

    if (++fCursorTimer > 30)
      fCursorTimer = 0;
  }
}

void DebugConsole::DrawDebugText(const std::string& text, int y)
{
  Renderer::getInstance()->font.drawLTR(4, (Renderer::getInstance()->screenHeight - y), text, 0xFFAAAA, true);
}

bool DebugConsole::Execute(std::string& line)
{
  LOG_DEBUG("DebugConsole::Execute('{}')", line);

  // record command in backbuffer
  if (!fBackBuffer.empty())
    fBackBuffer.erase(fBackBuffer.end() - 1);
  if (fBackBuffer.size() >= CONSOLE_MAX_BACK)
    fBackBuffer.erase(fBackBuffer.begin());

  trim(line);

  fBackBuffer.push_back(line);
  fBackBuffer.push_back(std::string(""));

  // split command into arguments
  std::vector<std::string> args;
  std::string cmd = SplitCommand(line, args);

  if (cmd.size())
  {
    std::vector<CommandEntry> matches;
    MatchCommand(cmd, matches);

    if (matches.size() == 1)
    {
      CommandEntry command = matches.at(0);

      if (args.size() < command.minArgs || args.size() > command.maxArgs)
      {
        if (command.minArgs == command.maxArgs)
        {
          Print("'{}' requires {} argument{}", command.name, command.minArgs, (command.minArgs == 1) ? "" : "s");
        }
        else if (args.size() < command.minArgs)
        {
          Print("'{}' requires at least {} argument{}", command.name, command.minArgs,
                (command.minArgs == 1) ? "" : "s");
        }
        else
        {
          Print("'{}' requires no more than {} arguments", command.name, command.maxArgs);
        }
      }
      else
      {
        void (*handler)(std::vector<std::string> *, int) = command.handler;
        int num                                          = (args.size() > 0) ? atoi(args.at(0).c_str()) : 0;

        (*handler)(&args, num);
        return 1;
      }
    }
    else if (matches.size() == 0)
    {
      Print("I don't understand");
    }
    else
    {
      Print("Ambiguous command");
    }
  }

  return 0;
}

void DebugConsole::MatchCommand(const std::string& cmd, std::vector<CommandEntry>& matches)
{
  for (auto& command: commands)
  {
    size_t pos = command.name.find(cmd);
    if (pos == 0 && pos != std::string::npos)
      matches.push_back(command);
  }
}

// split an input line into command and arguments
// returns the command portion of the line

std::string DebugConsole::SplitCommand(const std::string& line_in, std::vector<std::string>& args)
{
    std::size_t start = 0;
    std::string first;
    std::size_t idx = 0;

    std::size_t end = line_in.find_first_of(" \t");

    while (end != std::string::npos) {
        if (idx == 0)
          first = line_in.substr(start, end - start);
        else
          args.push_back(line_in.substr(start, end - start));
        start = end + 1;
        end = line_in.find_first_of(" \t", start);
        idx++;
    }

    if (idx == 0)
      first = line_in.substr(start, end - start);
    else
      args.push_back(line_in.substr(start));

    return first;
}

// tab-expand the current command
void DebugConsole::ExpandCommand()
{
  std::vector<std::string> args;
  std::vector<CommandEntry> matches;

  if (!fBrowsingExpansion)
  {
    trim(fLine);
    fLineToExpand = fLine;
    fExpandIndex = 0;
  }

  std::string cmd = SplitCommand(fLineToExpand, args);
  if (cmd.size())
  {
    MatchCommand(cmd, matches);

    if (matches.size() > 0)
    {
      if (fExpandIndex >= matches.size())
        fExpandIndex = 0;

      CommandEntry command = matches.at(fExpandIndex);
      std::string newCommand(command.name);

      for (unsigned int i = 0; i < args.size(); i++)
      {
        newCommand.append(" ");
        newCommand.append(args.at(i));
      }

      if (args.size() < command.minArgs)
        newCommand.append(" ");

      fLine = newCommand;
    }
  }

  if (matches.size() != 1)
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_TINK);
}
