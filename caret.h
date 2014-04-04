
#ifndef _CARET_H
#define _CARET_H

enum EffectTypes
{
	EFFECT_NONE = 0,
	
	EFFECT_STARSOLID,				// "shot hit wall" dissipation effect
	EFFECT_STARPOOF,				// "shot exceeded ttl" dissipation effect
	EFFECT_FISHY,					// common enemy shot dissipation effect
	EFFECT_BLOODSPLATTER,			// "blood" spatter from hitting enemies
	EFFECT_BONKPLUS,				// two +'s from player hitting head
	EFFECT_BOOMFLASH,				// big white circle flash from enemies exploding etc
	EFFECT_LEVELUP,
	EFFECT_LEVELDOWN,
	EFFECT_QMARK,					// "?" from pressing down with nothing nearby
	EFFECT_BONUSFLASH,				// small bright "tink" when hearts/missiles disappear
	EFFECT_ZZZZ,					// rising sleeping zzzzz's
	EFFECT_EMPTY,					// rising text that says "empty"
	EFFECT_SMOKETRAIL,				// small smoke puff
	EFFECT_SMOKETRAIL_SLOW,			// small smoke puff (slower animate)
	EFFECT_HEY,						// balloon that says "Hey!" (from grasstown)
	EFFECT_GUNFISH_BUBBLE,			// bubbles spawned during dissipation of Gunfish shots
	EFFECT_LAVA_SPLASH,				// just red version of gunfish bubble, really
	EFFECT_BUBBLE_BURST,			// Bubbler dissipation effect. is NOT CENTERED.
	EFFECT_SPUR_HIT,				// Spur hit wall effect, used in addition to starsolid.
	EFFECT_GHOST_SPARKLE			// rising sparkles from Ballos's dog
};


namespace Carets
{
	bool init(void);
	void close(void);
	
	void DrawAll(void);
	int CountByEffectType(int type);
	int DeleteByEffectType(int type);
	void DestroyAll(void);
};

// synonyms
#define CountEffectsOfType		Carets::CountByEffectType
#define DeleteEffectsOfType		Carets::DeleteByEffectType

struct Caret
{
	void (*OnTick)(Caret *c);
	
	int x, y;
	int xinertia, yinertia;
	int sprite, frame;
	
	int state;
	int effecttype;
	
	int timer, timer2;
	int animtimer;
	
	bool invisible;
	bool deleted;
	
	Caret *next, *prev;
	
// ---------------------------------------
	
	void Delete();
	void Destroy();
	void MoveAtDir(int dir, int speed);
	
	void anim(int speed);
	void animdie(int speed);
};

Caret *CreateCaret(int x, int y, int sprite, void (*ontick)(Caret *c), \
				   int xinertia=0, int yinertia=0);

Caret *effect(int x, int y, int effectno);
void caret_animate1(Caret *c);
void caret_animate2(Caret *c);
void caret_animate3(Caret *c);
void caret_bonkplus(Caret *c);
void caret_fishy(Caret *c);
void caret_spur_hit(Caret *c);
void caret_playertext(Caret *c);
void caret_qmark(Caret *c);
void caret_bonusflash(Caret *c);
void caret_hey(Caret *c);
void caret_gunfish_bubble(Caret *c);
void caret_ghost_sparkle(Caret *c);
void caret_zzzz(Caret *c);


#endif

