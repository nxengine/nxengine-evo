
#ifndef _STATUSBAR_H
#define _STATUSBAR_H

struct PercentBar
{
	int displayed_value;
	int dectimer;
};

struct StatusBar
{
	int xpflashcount;
	int xpflashstate;
};

extern StatusBar statusbar;
void niku_draw(int value, bool force_white=false);

void stat_PrevWeapon(bool quiet=false);
void stat_NextWeapon(bool quiet=false);

bool statusbar_init(void);
void DrawStatusBar(void);
void DrawAirLeft(int x, int y);
void DrawWeaponAmmo(int x, int y, int wpn);
void DrawWeaponLevel(int x, int y, int wpn);
static void RunStatusBar(void);
void weapon_slide(int dir, int newwpn);
void weapon_introslide();
void InitPercentBar(PercentBar *bar, int starting_value);
void RunPercentBar(PercentBar *bar, int current_value);
void DrawPercentBar(PercentBar *bar, int x, int y, int curvalue, int maxvalue, int width);
void DrawNumber(int x, int y, int num);
void DrawPercentage(int x, int y, int fill_sprite, int fsframe, int curvalue, int maxvalue, int width_at_max);
void DrawNumberRAlign(int x, int y, int s, int num);
void DrawTwoDigitNumber(int x, int y, int num);
void DrawDigit(int x, int y, int digit);
void niku_run();


#endif
