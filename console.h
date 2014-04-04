
#ifndef _CONSOLE_H
#define _CONSOLE_H

#define CONSOLE_MAXCMDLEN		48
#define CONSOLE_MAXRESPONSELEN	48

// how many commands to remember in the backbuffer
#define CONSOLE_MAX_BACK		8
#include <vector>
#include <string>

struct CommandEntry
{
	const char *name;
	void (*handler)(std::vector<std::string> *args, int num);
	int minArgs, maxArgs;
};

class DebugConsole
{
public:
	DebugConsole();
	
	void SetVisible(bool newstate);
	bool IsVisible();
	
	bool HandleKey(int key);
	void HandleKeyRelease(int key);
	void Draw();
	
	bool Execute(const char *line);
	void Print(const char *fmt, ...);
	
private:
	void DrawText(const char *text);
	void MatchCommand(const char *cmd, std::vector<void*> *matches);
	char *SplitCommand(const char *line_in, std::vector<std::string> *args);
	void ExpandCommand();
	
	char fLine[CONSOLE_MAXCMDLEN];
	int fLineLen;
	int fKeyDown;
	int fRepeatTimer;
	
	char fLineToExpand[CONSOLE_MAXCMDLEN];
	bool fBrowsingExpansion;
	int fExpandIndex;
	
	char fResponse[CONSOLE_MAXRESPONSELEN];
	int fResponseTimer;
	
	int fCursorTimer;
	bool fVisible;
	
	// up-down last-command buffer
	int fBackIndex;
	std::vector<std::string> fBackBuffer;
};

extern DebugConsole console;

static void __god(std::vector<std::string> *args, int num);
static void __script(std::vector<std::string> *args, int num);
static void __warp(std::vector<std::string> *args, int num);
static void __sound(std::vector<std::string> *args, int num);
static void __music(std::vector<std::string> *args, int num);
static void __giveweapon(std::vector<std::string> *args, int num);
static void __dropweapon(std::vector<std::string> *args, int num);
static void __level(std::vector<std::string> *args, int num);
static void __ammo(std::vector<std::string> *args, int num);
static void __maxammo(std::vector<std::string> *args, int num);
static void __hp(std::vector<std::string> *args, int num);
static void __maxhp(std::vector<std::string> *args, int num);
static void __xp(std::vector<std::string> *args, int num);
static void __spawn(std::vector<std::string> *args, int num);
static void __animate(std::vector<std::string> *args, int num);
static void __infinitedamage(std::vector<std::string> *args, int num);
static void __killall(std::vector<std::string> *args, int num);
static void __movemode(std::vector<std::string> *args, int num);
static void __flag(std::vector<std::string> *args, int num);
static void __clearflags(std::vector<std::string> *args, int num);
static void __equip(std::vector<std::string> *args, int num);
static void __giveitem(std::vector<std::string> *args, int num);
static void __takeitem(std::vector<std::string> *args, int num);
static void __qua(std::vector<std::string> *args, int num);
static void __boa(std::vector<std::string> *args, int num);
static void __cre(std::vector<std::string> *args, int num);
static void __reset(std::vector<std::string> *args, int num);
static void __fps(std::vector<std::string> *args, int num);
static void __set_iquit(std::vector<std::string> *args, int num);
static void __set_noquake(std::vector<std::string> *args, int num);
static void __inhibit_fullscreen(std::vector<std::string> *args, int num);
static void __emulate_bugs(std::vector<std::string> *args, int num);
static void __displayformat(std::vector<std::string> *args, int num);
static void __skip_intro(std::vector<std::string> *args, int num);
static void __hello(std::vector<std::string> *args, int num);
static void __player_hide(std::vector<std::string> *args, int num);
static void __player_inputs_locked(std::vector<std::string> *args, int num);
static void __game_frozen(std::vector<std::string> *args, int num);
static void __textbox_setvisible(std::vector<std::string> *args, int num);



#endif
