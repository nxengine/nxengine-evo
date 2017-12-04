
#ifndef _FONT_H
#define _FONT_H

#define FONT_DEFAULT_SPACING	5

int font_draw(int x, int y, const char *text, uint32_t color=0xFFFFFF, bool is_shaded=false);

int GetFontWidth(const char *text, bool is_shaded=false);
int GetFontHeight();
int GetFontBase();
bool font_init(void);
void font_close(void);
bool font_reload();
#endif
