
#ifndef _SCREENEFFECT_H
#define _SCREENEFFECT_H
#include "autogen/sprites.h"

// screeneffects are a simple draw overlay used w/ things such as flashes and such.
class ScreenEffect
{
public:
	ScreenEffect() { enabled = false; }
	virtual ~ScreenEffect() { }
	virtual void Draw() = 0;
	
	bool enabled;
	
protected:
	int state;
	int timer;
};


// FlashScreen simply flashes the screen white several times,
// and is used in various places such as when Misery casts spells.
struct SE_FlashScreen : public ScreenEffect
{
	void Start();
	void Draw();
	
	int flashes_left;
	bool flashstate;
};


// Starflash is a full-screen white explosion in the shape of a '+',
// used when some bosses are defeated.
struct SE_Starflash : public ScreenEffect
{
	void Start(int x, int y);
	void Draw();
	
	int centerx, centery;
	int size, speed;
};


// Fade is the fade-in/out used on every stage transistion/TRA.
struct SE_Fade : public ScreenEffect
{
	SE_Fade();
	
	void Start(int fadedir, int dir, int spr=SPR_FADE_DIAMOND);
	void Draw(void);
	void set_full(int dir);
	int getstate(void);

	struct
	{
		int fadedir;
		int sweepdir;
		int curframe;
		int sprite;
	} fade;
};

#define FADE_IN			0
#define FADE_OUT		1

// these directions correspond to the FAI/FAO parameters.
#define FADE_LEFT		0
#define FADE_UP			1
#define FADE_RIGHT		2
#define FADE_DOWN		3
#define FADE_CENTER		4

#define FS_NO_FADE		0		// no fade is active
#define FS_FADING		1		// currently fading in or out
#define FS_FADED_OUT	2		// completely faded out


namespace ScreenEffects
{
	void Draw(void);
	void Stop();
};


extern SE_FlashScreen flashscreen;
extern SE_Starflash starflash;
extern SE_Fade fade;

#endif

