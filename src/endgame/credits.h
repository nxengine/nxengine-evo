
#ifndef _CREDITS_H
#define _CREDITS_H

#define MAX_BIGIMAGES 20
#define CRED_MAX_TEXT 80
#include "../graphics/Surface.h"
#include "CredReader.h"

#include <vector>

// a currently displayed line of text
struct CredLine
{
  std::string text = "";
  int image = 0;
  int x, y = 0;
  bool remove = false;
};

class BigImage
{
public:
  bool Init();
  ~BigImage();

  void Set(int num);
  void Clear();
  void Draw();

private:
  int imagex, state;
  int imgno;
  NXE::Graphics::Surface *images[MAX_BIGIMAGES];
};

class Credits
{
public:
  bool Init();
  void Tick();
  void Draw();
  ~Credits();

  BigImage bigimage; // current "SIL" big left-hand image

private:
  void RunNextCommand();
  bool Jump(int label);

  void DrawLine(CredLine& line);

  int spawn_y;  // position of next line relative to top of roll
  int scroll_y; // CSFd roll position

  int xoffset; // x position of next line

  // turns off scrolling and further script execution when "/" command hit at end
  bool roll_running;

  CredReader script;
  std::vector<CredLine> lines;
};

bool credit_init(int parameter);
void credit_tick();
void credit_set_image(int imgno);
void credit_clear_image();
void credit_close();
void credit_draw();

#endif
