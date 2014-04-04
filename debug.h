
#ifndef _DEBUG_H
#define _DEBUG_H

#define DM_PIXEL			0
#define DM_CROSSHAIR		1
#define DM_XLINE			2
#define DM_YLINE			3
#define DM_BOX				4

#include <stdint.h>
#include "siflib/sif.h"
#include "object.h"

void DrawDebug(void);
void DrawBoundingBoxes();
void DrawAttrPoints();
static void draw_pointlist(Object *o, SIFPointList *points);
void debug(const char *fmt, ...);
void debug_draw(void);
void debug_clear();
const char *DescribeObjectType(int type);
int ObjectNameToType(const char *name_in);
const char *DescribeDir(int dir);
const char *strhex(int value);
void DrawDebugMarks(void);
void AddDebugMark(int x, int y, int x2, int y2, char type, uint8_t r, uint8_t g, uint8_t b);
void DebugPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void DebugCrosshair(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void crosshair(int x, int y);
void DebugPixelNonCSF(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void DebugCrosshairNonCSF(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void debugVline(int x, uint8_t r, uint8_t g, uint8_t b);
void debugHline(int y, uint8_t r, uint8_t g, uint8_t b);
void debugbox(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b);
void debugtile(int x, int y, uint8_t r, uint8_t g, uint8_t b);



#endif
