
#include <math.h>
#include "nx.h"
#include "trig.h"
#include "common/misc.h"

#include "ai/ai.h"
#include "player.h"

signed int sin_table[256];
signed int tan_table[64];


char trig_init(void)
{
int degrees;

// converts from 0-256 scale to 0-360 scale, then from degrees to radians
	#define PIBT 	((360.00f / 256.00f) * (3.14159265f / 180.00f))
	
	for(degrees=0;degrees<256;degrees++)
	{
		sin_table[degrees] = (int)(sin((double)degrees * PIBT) * (1 * CSFI));
	}
	
	for(degrees=0;degrees<64;degrees++)
	{
		tan_table[degrees] = (int)(tan((double)degrees * PIBT) * (1 << 13));
	}
	
	//SetFullscreen(1);
	return 0;
}

/*void trig_test(void)
{
int x, y;
int x1, y1, x2, y2;
int xdist, ydist;
char buf[80];
int angle;
double fratio;
int ratio;
char k = 0;
static int avx, avy;
static int avtimer=0;
static int avsx=80 * CSFI, avsy=60 * CSFI;
static int avectx, avecty;
static int lx=-1,ly=-1;
static char firsttime = 1;

	if (firsttime)
	{
		trig_init();
		SDL_WarpMouse(240<<1,64<<1);
		firsttime = 0;
	}
	
	ClearScreen();
	
	x1 = mouse.x;
	y1 = mouse.y;
	x2 = SCREEN_WIDTH/2;
	y2 = SCREEN_HEIGHT/2;
	
	angle = GetAngle(x1, y1, x2, y2);
	
	
	if (!avectx && !avecty) avtimer = 0;
	if (x1 > x2 && avx / CSFI < x2) k++;
	if (x1 < x2 && avx / CSFI > x2) k++;
	if (y1 > y2 && avy / CSFI < y2) k++;
	if (y1 < y2 && avy / CSFI > y2) k++;
	if (k >= 2) avtimer = 0;
	if (x1 != lx || y1 != ly) { lx=x1;ly=y1;avtimer=0; }
	avtimer--;
	if (avtimer < 0)
	{
		vector_from_angle(angle, 0xA0, &avectx, &avecty);
		avtimer = 650;
		avx = x1 * CSFI; avy = y1 * CSFI;
	}
	avx += avectx;
	avy += avecty;
	//angle *= 360;
	//angle /= 256;
	
	PlotCircle(x1, y1, 120, 250, 120, 0, 256, 7);
	PlotCircle(x2, y2, 200, 120, 0, 0, 256, 7);
	
	fratio = ((double)ydist / (double)xdist);
	if (fratio < 0) fratio = -fratio;
	
	DrawSDLLine(x1, y1, x2, y2, 200, 120, 0);
	DrawSDLLine(x1, y1, x1, y2, 200, 120, 0);
	DrawSDLLine(x1, y2, x2, y2, 200, 120, 0);
	sprintf(buf, "%d", xdist); font_draw((((x2-x1)/2)+x1)-(GetFontWidth(buf,0)/2), y2+8, buf, 0);
	sprintf(buf, "%d", ydist); font_draw(x1+8, (((y2-y1)/2)+y1)-4, buf, 0);
	sprintf(buf, "%d", angle); font_draw(x1-12-GetFontWidth(buf,0), y1-4, buf, 0);
	PlotCircle(x1-8-GetFontWidth(buf,0),y1,120,250,120,128,200,6);
	x = (x1+x2)/2; y = (y1+y2)/2; x+=4; y+=9;
	sprintf(buf, "%.2f", fratio); font_draw(x,y,buf,0);
	y+=8; sprintf(buf, "%04x", ratio); font_draw(x,y,buf,0);
	PlotCircle(avx / CSFI, avy / CSFI, 200, 120, 120, 0, 256, 6);
	
	flip();
}

void PlotCircle(int x, int y, uint8_t r, uint8_t g, uint8_t b, int start, int stop, int shift)
{
int ang;
int xa, ya;
uint8_t angle;
	for(ang=start;ang<stop;ang+=2)
	{
		angle = ang; xa = sin_table[angle] >> shift;
		angle += 64; ya = sin_table[angle] >> shift;
		
		PlotSDLPixel(x+xa, y+ya, r, g, b);
	}
}
*/


// given an angle and a speed, places the X and Y speeds in xs and ys.
// note: the output values _ARE_ CSF'd, despite the >>= CSF done on them at the end.
void vector_from_angle(uint8_t angle, int speed, int *xs, int *ys)
{
	if (ys)
	{
		*ys = sin_table[angle];
		*ys *= speed; *ys /= CSFI;
	}
	
	if (xs)
	{
		angle += 64;			// wraps at 255 because it's a char
		*xs = sin_table[angle];
		
		// what's going on here is that when we calculated sin_table, we could not hold the
		// fractional (0-1.00f) values outputted from sin(), so we scaled them from 0-0x200.
		// so now we basically are >>= CSFing the value back to it's original 0-1.00, then
		// multiplying by speed. We're just doing it backwards so as the precision will stay.
		// which is ok because multiplication and division are on the same level of OoO.
		*xs *= speed; *xs /= CSFI;
	}
}

int xinertia_from_angle(uint8_t angle, int speed)
{
	angle += 64;
	int xs = sin_table[angle];
	xs *= speed; xs /= CSFI;
	
	return xs;
}

int yinertia_from_angle(uint8_t angle, int speed)
{
	int ys = sin_table[angle];
	ys *= speed; ys /= CSFI;
	
	return ys;
}

// give it your position and a target position, and it tells you what angle you should travel at.
uint8_t GetAngle(int curx, int cury, int tgtx, int tgty)
{
int xdist, ydist;
int ratio;
int angle;

	xdist = (tgtx - curx);
	ydist = (tgty - cury);
	
	if (xdist==0)
	{	// fixup for undefined slope
		if (tgty > cury) return 0x40;		// straight down
		return 0xC0;						// straight up
	}
	
	// (ydist / xdist) * 512	[scale it for integer floating point]
	ratio = (abs(ydist) << 13) / abs(xdist);
	
	if (ratio > tan_table[63])
	{
		angle = 0x40;
	}
	else
	{
		for(angle=0;angle<64;angle++)
		{
			if (tan_table[angle] >= ratio) break;
		}
	}
	
	if (curx > tgtx) angle = 0x80 - angle;
	if (cury > tgty) angle = 0x100 - angle;
	return angle;
}

/*
void c------------------------------() {}
*/

// convenience function.
//  * spawn an object at o's action point.
//  * launch it at the player at speed.
//  * introduce "rand_variance" random error/variation into the launch angle.
void EmFireAngledShot(Object *o, int objtype, int rand_variance, int speed)
{
Object *shot;

	shot = SpawnObjectAtActionPoint(o, objtype);
	ThrowObjectAtPlayer(shot, rand_variance, speed);
}


// like EmFireAngledShot, but it's throws an already existing object
// instead of spawning a new one
void ThrowObjectAtPlayer(Object *o, int rand_variance, int speed)
{
	ThrowObject(o, player->x, player->y, rand_variance, speed);
}

// set the x&y inertia of object o so that it travels towards [destx,desty].
// rand_variance is a random amount of inaccuracy, in 0-255 degrees, to introduce
// into the toss.
// speed is how quickly to throw the object, in CSF'd coordinates.
void ThrowObject(Object *o, int destx, int desty, int rand_variance, int speed)
{
	uint8_t angle = GetAngle(o->x, o->y, destx, desty);
	
	if (rand_variance)
		angle += random(-rand_variance, rand_variance);
	
	ThrowObjectAtAngle(o, angle, speed);
}

// toss object o along angle angle at speed speed
void ThrowObjectAtAngle(Object *o, uint8_t angle, int speed)
{
	o->yinertia = (sin_table[angle] * speed) / CSFI;
	angle += 64;
	o->xinertia = (sin_table[angle] * speed) / CSFI;
}






