
#include "nx.h"
#include <cstdarg>
#include "console.h"
#include "game.h"
#include "ObjManager.h"
#include "settings.h"
#include "statusbar.h"
#include "tsc.h"
#include "graphics/graphics.h"
using namespace Graphics;
#include "graphics/font.h"
#include "graphics/sprites.h"
#include "sound/sound.h"
#include "common/stat.h"
#include "common/misc.h"
#include "map.h"
#include "debug.h"
#include "player.h"
#include "p_arms.h"

#include "playerstats.h"
#include <vector>
#include <string>

#define Respond		console.Print

#if defined(_WIN32)
#define strcasecmp _stricmp
#endif


static void __god(std::vector<std::string> *args, int num)
{
	game.debug.god = !game.debug.god;
	Respond("God mode:  %s", game.debug.god ? "enabled" : "disabled");
}

static void __script(std::vector<std::string> *args, int num)
{
	// release any focus a current script may have on us
	if (player->movementmode == MOVEMODE_NORMAL)
		map_focus(NULL);
	
	if (game.tsc->StartScript(num))
	{
		Respond("Script %04d started.", num);
	}
	else
	{
		Respond("No such script %04d", num);
	}
}

static void __warp(std::vector<std::string> *args, int num)
{
	if (num == 0)
	{
		std::string stagename;
		for(unsigned int i=0;i<args->size();i++)
		{
			if (i != 0) stagename+=' ';
			stagename+=(args->at(i));
		}
		
		stat("Looking for '%s'", stagename.c_str());
		for(num=0;;num++)
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
	
	game.switchstage.mapno = num;
	game.switchstage.playerx = 16;
	game.switchstage.playery = 16;
}

static void __sound(std::vector<std::string> *args, int num)
{
	sound(num);
	console.SetVisible(true);	// keep console up
}

static void __music(std::vector<std::string> *args, int num)
{
	extern const char *org_names[];
	bool ok = true;
	int i;
	
	const char *name = args->at(0).c_str();
	if (num == 0 && strcmp(name, "0") != 0)
	{
		for(i=1;;i++)
		{
			if (!org_names[i]) break;
			
			if (strcasebegin(org_names[i], name))
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
	
	if (num < 0) ok = false;
	else
	{
		for(i=1;i<=num;i++)
		{
			if (!org_names[i])
			{
				ok = false;
				break;
			}
		}
	}
	
	if (!ok)
	{
		Respond("track out of range");
		music(0);
	}
	else
	{
		music(0);
		music(num);
		if (org_names[num])
			Respond("%s started", org_names[num]);
	}
}

static void __giveweapon(std::vector<std::string> *args, int num)
{
	if (num >= 0 && num < WPN_COUNT)
	{
		player->weapons[num].hasWeapon = 1;
		player->weapons[num].maxammo = 0;		// gives it unlimited ammo
		player->weapons[num].ammo = 0;
		player->curWeapon = num;
	}
}

static void __dropweapon(std::vector<std::string> *args, int num)
{
	if (args->size() == 0)
		num = player->curWeapon;
	
	player->weapons[num].hasWeapon = 0;
	player->weapons[num].maxammo = 0;
	player->weapons[num].ammo = 0;
	
	if (num == player->curWeapon)
		stat_NextWeapon();
}

// set weapon level
static void __level(std::vector<std::string> *args, int num)
{
	num--;
	if (num < 0) num = 0;
	if (num > 2) num = 2;
	
	if (player->weapons[player->curWeapon].xp < 5)
		player->weapons[player->curWeapon].xp = 5;
	
	for(int timeout=0;timeout<500;timeout++)
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
	
	Respond("Timeout");
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
		//PHealthBar.displayed_value = player->hp;
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
	for(;;i++)
	{
		if (i>=args->size()) break;
		
		if (i > starti) objName+=' ';
		objName+=(args->at(i));
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
	int x = player->x + ((player->dir==RIGHT) ? (24 * CSFI) : -(24 * CSFI));
	int y = player->y - (16 * CSFI);
	int w = (sprites[objprop[type].sprite].w + 4) * CSFI;
	
	// create 'em
	for(i=0;i<count;i++)
	{
		Object *o = CreateObject(x, y, type);
		
		o->dir = player->dir;
		o->nxflags |= NXFLAG_CONSOLE_ANIMATE;
		x += w;
	}
	
	if (count != 1)
		Respond("%s x%d", DescribeObjectType(type), count);
	else
		Respond("%s", DescribeObjectType(type));
}

static void __animate(std::vector<std::string> *args, int num)
{
	Object *o;
	
	if (args->size() == 2)
	{	// specifying explicitly by id2
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
			found = true;
		}
	}
	
	if (!found)
		Respond("No objects found.");
}

static void __infinitedamage(std::vector<std::string> *args, int num)
{
	if (args->size() > 0)
		game.debug.infinite_damage = num;
	else
		game.debug.infinite_damage ^= 1;
	
	Respond(game.debug.infinite_damage ? "My, oh my..." : "Back to normal.");
}

static void __killall(std::vector<std::string> *args, int num)
{
	for(int i=0;i<nOnscreenObjects;i++)
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
	Respond("Flag %04d: %s", num, game.flags[num] ? "SET":"CLEARED");
}

static void __clearflags(std::vector<std::string> *args, int num)
{
	memset(game.flags, 0, sizeof(game.flags));
	Respond("Warning- all game flags cleared");
}

static void __equip(std::vector<std::string> *args, int num)
{
static const char *equiplist[] =
{
	"booster08",
	"map",
	"armsbarrier",
	"turbocharge",
	"airtank",
	"booster20",
	"mimigamask",
	"whimstar",
	"nikumaru",
	NULL
};
int i, mask;

	const char *item = args->at(0).c_str();
	bool enable = args->size()>1 ? atoi(args->at(1).c_str()) : true;
	
	mask = 0x01;
	for(i=0;equiplist[i];i++)
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
			
			Respond("Item %s (0x%04x) %sequipped.",
				equiplist[i], mask, enable ? "" : "un-");
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
		Respond("Added item %d to your inventory.", num);
	}
	else
	{
		Respond("You already have item %d in your inventory.", num);
	}
}

static void __takeitem(std::vector<std::string> *args, int num)
{
	if (FindInventory(num) != -1)
	{
		DelInventory(num);
		Respond("Removed item %d from your inventory.", num);
	}
	else
	{
		Respond("You don't have item %d in your inventory.", num);
	}
}

static void __qua(std::vector<std::string> *args, int num)
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

// skip to good ending sequence
static void __cre(std::vector<std::string> *args, int num)
{
	game.reset();
	game.pause(0);
	game.setmode(GM_NORMAL);
	
	game.flags[1341] = true;
	game.switchstage.mapno = 70;
	game.switchstage.playerx = 16;
	game.switchstage.playery = 16;
	game.switchstage.eventonentry = 400;
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
	settings->skip_intro = num;
	settings_save();
	Respond("skip_intro: %s", settings->skip_intro ? "enabled":"disabled");
}

/*
void c------------------------------() {}
*/

static void __hello(std::vector<std::string> *args, int num)
{
	Respond("I'm a computer, you ninny. Go get a real friend.");
}

/*
void c------------------------------() {}
*/

static void __player_hide(std::vector<std::string> *args, int num)
{
	player->hide = num;
}

static void __player_inputs_locked(std::vector<std::string> *args, int num)
{
	player->inputs_locked = num;
}

static void __game_frozen(std::vector<std::string> *args, int num)
{
	game.frozen = num;
}

static void __textbox_setvisible(std::vector<std::string> *args, int num)
{
	textbox.SetVisible(num);
}

static CommandEntry commands[] =
{
	{"god", __god, 0, 1},
	{"script", __script, 1, 1},
	{"warp", __warp, 1, 999},
	{"sound", __sound, 1, 1},
	{"music", __music, 1, 1},
	{"giveweapon", __giveweapon, 1, 1},
	{"dropweapon", __dropweapon, 0, 1},
	{"level", __level, 1, 1},
	{"ammo", __ammo, 1, 1},
	{"maxammo", __maxammo, 1, 1},
	{"hp", __hp, 1, 1},
	{"maxhp", __maxhp, 1, 1},
	{"xp", __xp, 1, 1},
	{"spawn", __spawn, 1, 999},
	{"animate", __animate, 1, 2},
	{"infinitedamage", __infinitedamage, 0, 1},
	{"killall", __killall, 0, 0},
	{"movemode", __movemode, 1, 1},
	{"flag", __flag, 1, 1},
	{"clearflags", __clearflags, 0, 0},
	{"equip", __equip, 1, 2},
	{"giveitem", __giveitem, 1, 1},
	{"takeitem", __takeitem, 1, 1},
	{"qua", __qua, 0, 1},
	{"boa", __boa, 1, 1},
	{"cre", __cre, 0, 0},
	{"reset", __reset, 0, 0},
	{"fps", __fps, 0, 1},
	
	{"skip-intro", __skip_intro, 1, 1},
	
	{"player->hide", __player_hide, 1, 1},
	{"player->inputs_locked", __player_inputs_locked, 1, 1},
	{"game.frozen", __game_frozen, 1, 1},
	{"textbox.SetVisible", __textbox_setvisible, 1, 1},
	
	{"hello", __hello, 0, 0},
	{"hi", __hello, 0, 0},
	
	{NULL, NULL, 0, 0}
};


DebugConsole::DebugConsole()
{
	fVisible = false;
	fLineLen = 0;
	fCursorTimer = 0;
	fResponseTimer = 0;
	
	fLine[0] = 0;
	fResponse[0] = 0;
	
	fKeyDown = 0;
	fRepeatTimer = 0;
}

/*
void c------------------------------() {}
*/

void DebugConsole::SetVisible(bool newstate)
{
	//stat("DebugConsole::SetVisible(%s)", newstate?"true":"false");
	
	if (fVisible != newstate)
	{
		fVisible = newstate;
		fKeyDown = 0;
		fRepeatTimer = 0;
		
		if (newstate)
		{
			fLine[0] = 0;
			fLineLen = 0;
			fBrowsingExpansion = false;
			fBackIndex = fBackBuffer.size()-1;
			
			fResponse[0] = 0;
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
	if (!fVisible) return 0;
	if (key != 9) fBrowsingExpansion = false;
	
	if (key != fKeyDown)
	{
		fKeyDown = key;
		fRepeatTimer = 25;
	}
	else
	{
		fRepeatTimer = 1;
	}
	
	fCursorTimer = 0;

	switch(key)
	{
		case 27:
		case '`':
		{
			SetVisible(false);
		}
		break;
		
		case 13:
		case 271:	// numeric enter
		{
			SetVisible(false);
			
			fLine[fLineLen] = 0;
			Execute(fLine);
			fLineLen = 0;
		}
		break;
		
		case 10: break;
		
		case 8:
		{
			if (fLineLen > 0)
				fLineLen--;
		}
		break;
		
		case 9:		// command completion
		{
			fLine[fLineLen] = 0;
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
				if (fBackIndex < 0) fBackIndex = (fBackBuffer.size() - 1);
				else fBackIndex %= fBackBuffer.size();
				
				const char *str = fBackBuffer.at(fBackIndex).c_str();
				
				maxcpy(fLine, str, sizeof(fLine) - 1);
				fLineLen = strlen(str);
			}
		}
		break;
		
		default:
		{
			if (fLineLen < (sizeof(fLine) - 1))
				fLine[fLineLen++] = key;
		}
		break;
	}
	
	return 1;
}

void DebugConsole::HandleKeyRelease(int key)
{
	if (key == fKeyDown)
	{
		fKeyDown = 0;
		fRepeatTimer = 0;
	}
}

/*
void c------------------------------() {}
*/

void DebugConsole::Draw()
{
	if (fResponse[0])
	{
		this->DrawText(fResponse);
		
		if (--fResponseTimer <= 0)
			fResponse[0] = 0;
	}
	else if (fVisible)
	{
		// key-repeat
		if (fKeyDown)
		{
			if (--fRepeatTimer < 0)
				HandleKey(fKeyDown);
		}
		
		char buffer[CONSOLE_MAXCMDLEN + 10];
		fLine[fLineLen] = 0;
		
		sprintf(buffer, "-> %s%c",
			fLine, (fCursorTimer < 20) ? '_' : ' ');
		
		this->DrawText(buffer);
		
		if (++fCursorTimer > 30)
			fCursorTimer = 0;
	}
}

void DebugConsole::DrawText(const char *text)
{
	font_draw(4, (SCREEN_HEIGHT - 16), text, 0xFFAAAA, true);
}

/*
void c------------------------------() {}
*/

void DebugConsole::Print(const char *fmt, ...)
{
va_list ar;

	va_start(ar, fmt);
	vsnprintf(fResponse, sizeof(fResponse), fmt, ar);
	va_end(ar);
	
	stat("%s", fResponse);
	fResponseTimer = 60;
}

/*
void c------------------------------() {}
*/

bool DebugConsole::Execute(const char *line)
{
	stat("DebugConsole::Execute('%s')", line);
	
	// record command in backbuffer
	if (fBackBuffer.size() >= CONSOLE_MAX_BACK)
		fBackBuffer.erase(fBackBuffer.begin());
	fBackBuffer.push_back(std::string(line));
	
	// split command into arguments
	std::vector<std::string> args;
	char *cmd = SplitCommand(line, &args);
	
	if (cmd)
	{
		std::vector<void*> matches;
		MatchCommand(cmd, &matches);
		free(cmd);
		
		if (matches.size() == 1)
		{
			CommandEntry *command = (CommandEntry *)matches.at(0);
			
			if (args.size() < command->minArgs || \
				args.size() > command->maxArgs)
			{
				if (command->minArgs == command->maxArgs)
				{
					Print("'%s' requires %d argument%s", \
						command->name, command->minArgs, \
						(command->minArgs == 1) ? "":"s");
				}
				else if (args.size() < command->minArgs)
				{
					Print("'%s' requires at least %d argument%s",
						command->name, command->minArgs,
						(command->minArgs == 1) ? "":"s");
				}
				else
				{
					Print("'%s' requires no more than %d arguments",
						command->name, command->maxArgs);
				}
			}
			else
			{
				void (*handler)(std::vector<std::string> *, int) = command->handler;
				int num = (args.size() > 0) ? atoi(args.at(0).c_str()) : 0;
				
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

void DebugConsole::MatchCommand(const char *cmd, std::vector<void*> *matches)
{
	for(int i=0; commands[i].name; i++)
	{
		if (strcasebegin(commands[i].name, cmd))
			matches->push_back(&commands[i]);
	}
}

// split an input line into command and arguments
// returns the command portion of the line. you must free this buffer.
char *DebugConsole::SplitCommand(const char *line_in, std::vector<std::string> *args)
{
	while(*line_in == ' ' || *line_in == '\t') line_in++;
	char *line = strdup(line_in);
	
	char *cmd = strtok(line, " \t");
	if (cmd && cmd[0])
	{
		while(const char *arg = strtok(NULL, " \t"))
		{
			args->push_back(std::string(arg));
		}
		
		return line;
	}
	
	free(line);
	return NULL;
}

// tab-expand the current command
void DebugConsole::ExpandCommand()
{
std::vector<std::string> args;
std::vector<void*> matches;
char *cmd;

	fLine[fLineLen] = 0;
	
	if (!fBrowsingExpansion)
	{
		maxcpy(fLineToExpand, fLine, sizeof(fLineToExpand));
		fExpandIndex = 0;
	}
	
	cmd = SplitCommand(fLineToExpand, &args);
	if (cmd)
	{
		MatchCommand(cmd, &matches);
		free(cmd);
		
		if (matches.size() > 0)
		{
			if (fExpandIndex >= matches.size())
				fExpandIndex = 0;
			
			CommandEntry *command = (CommandEntry *)matches.at(fExpandIndex);
			std::string newCommand(command->name);
			
			for(unsigned int i=0;i<args.size();i++)
			{
				const char *arg = args.at(i).c_str();
				
				newCommand.append(" ");
				newCommand.append(arg);
			}
			
			if (args.size() < command->minArgs)
				newCommand.append(" ");
			
			maxcpy(fLine, newCommand.c_str(), sizeof(fLine));
			fLineLen = strlen(fLine);
		}
	}
	
	if (matches.size() != 1)
		sound(SND_TINK);
}

/*
void c------------------------------() {}
*/


