
#ifndef _INITLIST_H
#define _INITLIST_H

#define MAX_INIT_RECORDS 100

class InitList
{
public:
  void AddFunction(void (*func)(void));
  void AddFunction(bool (*func)(void));
  void AddFunction(void *func);
  bool CallFunctions();

private:
  void *fFunctions[MAX_INIT_RECORDS];
  int fCount; // counting on behavior of auto-initilization to 0
};

class InitAdder
{
public:
  InitAdder(InitList *initlist, void (*func)(void))
  {
    initlist->AddFunction(func);
  }
  InitAdder(InitList *initlist, bool (*func)(void))
  {
    initlist->AddFunction(func);
  }
  InitAdder(InitList &initlist, void (*func)(void))
  {
    initlist.AddFunction(func);
  }
  InitAdder(InitList &initlist, bool (*func)(void))
  {
    initlist.AddFunction(func);
  }
};

#define INITFUNC(TARGET)                                                                                               \
  static void __InitFunc(void);                                                                                        \
  static InitAdder _ia(TARGET, __InitFunc);                                                                            \
  static void __InitFunc(void)

#endif
