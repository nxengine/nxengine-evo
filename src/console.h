
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
	unsigned int minArgs, maxArgs;
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
	unsigned int fLineLen;
	int fKeyDown;
	int fRepeatTimer;
	
	char fLineToExpand[CONSOLE_MAXCMDLEN];
	bool fBrowsingExpansion;
	unsigned int fExpandIndex;
	
	char fResponse[CONSOLE_MAXRESPONSELEN];
	int fResponseTimer;
	
	int fCursorTimer;
	bool fVisible;
	
	// up-down last-command buffer
	int fBackIndex;
	std::vector<std::string> fBackBuffer;
};

extern DebugConsole console;


#endif
