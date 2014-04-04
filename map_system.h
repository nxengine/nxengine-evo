
#ifndef _MAPSYSTEM_H
#define _MAPSYSTEM_H


bool ms_init(int param);
void ms_tick(void);
void ms_close(void);

static void draw_expand(void);
static void draw_banner(void);
static void draw_row(int y);
static int get_color(int tilecode);


#endif
