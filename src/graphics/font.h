#ifndef _FONT_H
#define _FONT_H

#include <string>
#include <cstdint>

int font_draw(int x, int y, const std::string& text, uint32_t color=0xFFFFFF, bool is_shaded=false);

int GetFontWidth(const std::string& text, bool is_shaded=false);
int GetFontHeight();
int GetFontBase();
bool font_init(void);
void font_close(void);
bool font_reload();
#endif
