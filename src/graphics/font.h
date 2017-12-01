
#ifndef _FONT_H
#define _FONT_H

// I don't want to needlessly include <SDL_ttf.h> in every file that
// includes this one so this forward declaration makes those modules
// that don't what a TTF_Font is shut up about InitChars(); however
// technically TTF_Font is a typedef, so if the including file knows
// the real declaration, it would bawk at this.
#ifndef SDL_TTF_VERSION
	struct TTF_Font;
#endif

#define NUM_FONT_LETTERS		256
#define NUM_LETTERS_RENDERED	128
#define FONT_DEFAULT_SPACING	5

class NXFont
{
public:
	NXFont();
	~NXFont();
	
	bool InitChars(TTF_Font *font, uint32_t color);
	bool InitCharsShadowed(TTF_Font *top, uint32_t color, uint32_t shadowcolor);
	
	bool InitBitmapChars(SDL_Surface *sheet, uint32_t fgcolor, uint32_t color);
	bool InitBitmapCharsShadowed(SDL_Surface *sheet, uint32_t fgcolor, uint32_t color, uint32_t shadowcolor);
	
	bool InitTextures();

	void free();
	
	SDL_Surface *letters[NUM_FONT_LETTERS];
	SDL_Texture *tletters[NUM_FONT_LETTERS];

private:
	void ReplaceColor(SDL_Surface *sfc, uint32_t oldcolor, uint32_t newcolor);
};


extern NXFont whitefont;
extern NXFont greenfont;
extern NXFont shadowfont;	// white letters w/ drop shadow

int font_draw(int x, int y, const char *text, int spacing=0, NXFont *font=&whitefont);

int GetFontWidth(const char *text, int spacing=0, bool is_shaded=false);
int GetFontHeight();
bool font_init(void);
void font_close(void);
bool font_reload();
void direct_text_draw(int x, int y, const char *text);
int GetFontWidth(const char *text, int spacing, bool is_shaded);
int GetFontHeight();
#endif
