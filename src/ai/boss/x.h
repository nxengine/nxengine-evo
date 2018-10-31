
#ifndef _XBOSS_H
#define _XBOSS_H
#include "../../object.h"
#include "../../stageboss.h"

enum XBDir
{
  UL,
  UR,
  LL,
  LR
};

class XBoss : public StageBoss
{
public:
  void OnMapEntry();
  void OnMapExit();
  void Run();
  void RunAftermove();

private:
  void run_tread(int index);
  void run_body(int index);
  void run_door(int index);
  void run_target(int index);
  void run_fishy_spawner(int index);
  void run_internals();

  bool AllTargetsDestroyed();
  void Init();
  Object *CreateTread(int x, int y, int sprite);
  Object *CreatePiece(int x, int y, int type);

  void SetStates(Object *objects[], int nobjects, int state);
  void SetDirs(Object *objects[], int nobjects, int dir);

  void DeleteMonster();

  Object *mainobject;
  Object *body[4];
  Object *treads[4];
  Object *internals;
  Object *doors[2];
  Object *targets[4];
  Object *fishspawners[4];

  Object *piecelist[24];
  int npieces;

  struct
  {
    bool initilized;
  } X;
};

void ondeath_x_mainobject(Object *internals);
void ondeath_x_target(Object *o);
void ai_x_fishy_missile(Object *o);
void ai_x_defeated(Object *o);

#endif
