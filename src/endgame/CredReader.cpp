
#include "CredReader.h"

#include "../ResourceManager.h"
#include "../Utils/Logger.h"
#include "../game.h"
#include "../nx.h"
#include "../tsc.h"

/*
CREDITS FORMAT (credit.tsc)
==============
[T]X	Text T with casts.pbm image X [on left?]
+X		Shift credits X spaces towards the RIGHT
!X		Play music X
-X		Paragraph X lines
fX:Y	Jump to location Y if flag X is set
jX		Jump to location X
lX		Location X
~		Fade music to low volume
/		[end?]
<<<		[comment?]

*/

bool CredReader::ReadCommand(CredCommand *cmd)
{
  int ch;
  unsigned int i;

  memset(cmd, 0, sizeof(CredCommand));
  cmd->type = -1;

  if (data.empty())
  {
    LOG_ERROR("CredReader: ReadNextCommand called but file is not loaded!");
    return 1;
  }

  for (;;)
  {
    ch = get();
    if (ch == '\r' || ch == '\n')
      continue;
    else
      break;
  }

  cmd->type = ch;
  switch (ch)
  {
    case CC_TEXT:
    {
      for (i = 0; i < sizeof(cmd->text) - 1; i++)
      {
        cmd->text[i] = get();
        if (cmd->text[i] == ']' || !cmd->text[i])
          break;
      }

      cmd->text[i] = 0;
    }
    break;

    case CC_SET_XOFF:
    case CC_BLANK_SPACE:
    case CC_JUMP:
    case CC_LABEL:
    case CC_MUSIC:
    case CC_FLAGJUMP:
    case CC_FADE_MUSIC:
    case CC_END:
      break;

    default:
    {
      LOG_ERROR("CredReader: unknown command type '{}'", ch);
      cmd->type = -1;
      return 1;
    }
    break;
  }

  if (isdigit(peek()))
    cmd->parm = ReadNumber();

  if (get() == ':')
    cmd->parm2 = ReadNumber();
  else
    unget();

  return 0;
}

int CredReader::ReadNumber()
{
  int num = atoi(&data[dataindex]);
  while (isdigit(get()))
    ;
  unget();
  return num;
}

/*
struct CredCommand
{
        int type;
        int parm1, parm2;
        char text[80];

        void DumpContents();
};
*/

void CredCommand::DumpContents()
{
  LOG_TRACE("CC '{:c}': [{}]:{:04d}:{:04d}", type, text, parm, parm2);
}

void CredReader::Rewind()
{
  dataindex = 0;
}

/*
void c------------------------------() {}
*/

CredReader::CredReader()
{
  data      = "";
  dataindex = 0;
}

bool CredReader::OpenFile(void)
{
  if (!data.empty())
    CloseFile();

  data = game.tsc->Decrypt(ResourceManager::getInstance()->getLocalizedPath("Credit.tsc"), &datalen);
  if (data.empty())
  {
    LOG_ERROR("CredReader: couldn't open 'Credits.tsc'!");
    return 1;
  }

  dataindex = 0;
  return 0;
}

void CredReader::CloseFile()
{
  if (!data.empty())
  {
    //		free(data);
    data    = "";
    datalen = 0;
  }
}

char CredReader::get()
{
  if (dataindex >= datalen)
    return 0;

  return data[dataindex++];
}

void CredReader::unget()
{
  if (dataindex > 0)
    dataindex--;
}

char CredReader::peek()
{
  if (dataindex >= datalen)
    return 0;

  return data[dataindex];
}

/*
void c------------------------------() {}
*/

/*
bool OpenFile();
        void CloseFile();
        bool ReadNextLine(CredCommand *cmd);

private:
        FILE *fFP;


enum CC
{
        CC_TEXT,
        CC_SET_XOFF,
        CC_BLANK_SPACE,

        CC_JUMP,
        CC_FLAGJUMP,
        CC_LABEL,

        CC_MUSIC,
        CC_FADE_MUSIC,
        CC_END
};

*/
