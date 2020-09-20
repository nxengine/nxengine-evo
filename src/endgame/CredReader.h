
#ifndef _CREDITS_READER_H
#define _CREDITS_READER_H

#include <string>

enum CC
{
  CC_TEXT        = '[',
  CC_SET_XOFF    = '+',
  CC_BLANK_SPACE = '-',

  CC_JUMP     = 'j',
  CC_FLAGJUMP = 'f',
  CC_LABEL    = 'l',

  CC_MUSIC      = '!',
  CC_FADE_MUSIC = '~',
  CC_END        = '/'
};

// represents a command from the .tsc
struct CredCommand
{
  char type = 0;
  int parm, parm2 = 0;
  std::string text = "";

  void DumpContents();
};

class CredReader
{
public:
  CredReader();
  bool OpenFile();
  void CloseFile();
  bool ReadCommand(CredCommand *cmd);
  void Rewind();

private:
  int ReadNumber();
  char get();
  void unget();
  char peek();

  std::string data;
  int dataindex, datalen;
};

#endif
