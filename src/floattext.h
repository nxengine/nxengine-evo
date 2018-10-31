#ifndef _FLOATTEXT_H
#define _FLOATTEXT_H
class Object;
#include <SDL.h>
#define FT_Y_START -4 // this starts it exactly centered, since the font is 8px tall
#define FT_Y_HOLD -19
#define FT_Y_RISEAWAY (FT_Y_HOLD - 8)

enum FloatTextStates
{
  FT_IDLE,
  FT_RISE,
  FT_HOLD,
  FT_SCROLL_AWAY,
};

class FloatText
{
public:
  FloatText(int sprite);
  ~FloatText();
  void Reset();

  void AddQty(int amt);
  bool IsScrollingAway();

  void UpdatePos(Object *assoc_object);

  static void DrawAll();
  static void DeleteAll();
  static void ResetAll(void);

  bool ObjectDestroyed;

private:
  void Draw();

  uint8_t state;

  int yoff; // how much we've risen
  int shownAmount;
  int sprite; // allows selecting font
  int timer;

  int objX, objY; // the center pixel of the associated object (de-CSFd)

  FloatText *next, *prev;
  static FloatText *first, *last;
};

#endif
