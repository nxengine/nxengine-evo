
#ifndef _CONSOLE_H
#define _CONSOLE_H

#define CONSOLE_MAXCMDLEN 48
#define CONSOLE_MAXRESPONSELEN 48

// how many commands to remember in the backbuffer
#define CONSOLE_MAX_BACK 8
#include <string>
#include <vector>
#include <spdlog/fmt/fmt.h>

struct CommandEntry
{
  std::string name;
  void (*handler)(std::vector<std::string> *args, int num);
  unsigned int minArgs, maxArgs;
  std::string help;
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

  bool Execute(std::string& line);

  template<typename... Args> void Print(const std::string& format, Args... args)
  {
    fResponse.push_back(fmt::format(format, args...));
    fResponseTimer = 60;
  }

  std::vector<CommandEntry>& getCommands()
  {
    return commands;
  }

private:
  void DrawDebugText(const std::string& text, int y = 16);
  void MatchCommand(const std::string& cmd, std::vector<CommandEntry>& matches);
  std::string SplitCommand(const std::string& line_in, std::vector<std::string>& args);
  void ExpandCommand();

  std::string fLine = "";
  int fKeyDown = 0;
  int fRepeatTimer = 0;

  std::string fLineToExpand = "";
  bool fBrowsingExpansion;
  unsigned int fExpandIndex;

  std::vector<std::string> fResponse;
  int fResponseTimer = 0;

  int fCursorTimer = 0;
  bool fVisible = false;

  // up-down last-command buffer
  int fBackIndex;
  std::vector<std::string> fBackBuffer;
  std::vector<CommandEntry> commands;
};

extern DebugConsole console;

#endif
