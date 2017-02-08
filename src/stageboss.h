
#ifndef _STAGEBOSS_H
#define _STAGEBOSS_H

#include "object.h"

// Stage Bosses are "big" boss enemies used in some stages.
//
// The stage boss class runs at a conceptual level "above"
// normal AI routines, more like at the level of the game loop.
//
// This gives these complex bosses a good way to control more
// things than just a single object (some bosses for example have
// multiple parts which need to be coordinated, such as Omega).
//
// A pointer to the "main" object of a stage boss is stored by
// the derived class at game.stageboss.object.
//
// The script command <BSL0000 refers to opening a boss bar
// against game.stageboss.object.

enum BossType
{
	BOSS_NONE	= 0x00,
	
	BOSS_OMEGA,
	BOSS_BALFROG,
	BOSS_MONSTER_X,
	BOSS_CORE,
	BOSS_IRONH,
	BOSS_SISTERS,
	BOSS_UNDEAD_CORE,
	BOSS_HEAVY_PRESS,
	BOSS_BALLOS
};


class StageBoss
{
public:
    StageBoss() {}
    virtual ~StageBoss() {}
	virtual void OnMapEntry() { }
	virtual void OnMapExit() { }
	
	// called every tick (for logic only, please don't draw from here)
	virtual void Run() { }
	virtual void RunAftermove() { }
	
	virtual void SetState(int newstate);
};


class StageBossManager
{
public:
	StageBossManager();
	
	bool SetType(int newtype);
	int Type();
	
	// safe interface to the current StageBoss instance
	// (they safely do nothing if called in a stage without a stage boss)
	void OnMapEntry();
	void OnMapExit();
	void Run();
	void RunAftermove();
	
	void SetState(int newstate);
	
	// pointer to the "main object" of a stage boss
	// (the one to show the boss bar for)
	// this is set by the derived class, and is cleared in OnMapExit
	// of the derived class and on a SetType().
	Object *object;
	
private:
	StageBoss *fBoss;
	int fBossType;
};




#endif
