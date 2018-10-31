
#ifndef _BALLOS_H
#define _BALLOS_H
#include "../../object.h"
#include "../../stageboss.h"

#define NUM_EYES 2
enum TRIGDIR
{
  LESS_THAN    = 0,
  GREATER_THAN = 1
};

class BallosBoss : public StageBoss
{
public:
  void OnMapEntry();
  void Run();
  void RunAftermove();

private:
  void RunForm1(Object *o);
  void RunForm2(Object *o);
  void RunForm3(Object *o);
  void RunComeDown(Object *o);
  void RunDefeated(Object *o);

  void run_eye(int index);
  void place_eye(int index);
  void SetEyeStates(int newstate);

  bool passed_xcoord(bool ltgt, int xcoord, bool reset = false);
  bool passed_ycoord(bool ltgt, int ycoord, bool reset = false);

  Object *main;
  Object *body;
  Object *eye[NUM_EYES];
  Object *shield; // top shield to cover eyes from above
};

void ondeath_ballos(Object *o);
void ai_ballos_rotator(Object *o);
void aftermove_ballos_rotator(Object *o);
void ai_ballos_platform(Object *o);

#endif
