
#ifndef _SISTERS_H
#define _SISTERS_H
#include "../../object.h"
#include "../../stageboss.h"

// although you will need to add additional copies of the head sprites
// for it to work properly, just try setting this number to something
// like 10 and running the fight!
#define NUM_SISTERS 2

class SistersBoss : public StageBoss
{
public:
  void OnMapEntry();
  void OnMapExit();
  void Run();

private:
  void run_head(int index);
  void head_set_bbox(int index);
  void run_body(int index);

  void SetHeadStates(int newstate);
  void SetBodyStates(int newstate);

  void SpawnScreenSmoke(int count);

  int mainangle;

  Object *main;
  Object *head[NUM_SISTERS];
  Object *body[NUM_SISTERS];
};

#endif
