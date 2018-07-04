
#ifndef _TEXTBOX_H
#define _TEXTBOX_H
#include <cstdint>
#include <string>
#include <array>
#include "YesNoPrompt.h"
#include "ItemImage.h"
#include "StageSelect.h"
#include "SaveSelect.h"
#include "../graphics/graphics.h"
using namespace Graphics;

#define MSG_W				244
#define MSG_H				64
#define MSG_UPPER_Y			24

#define MSG_NLINES			4
#define MSG_LINE_SPACING	16

enum TBFlags
{
	TB_DEFAULTS					= 0x00,
	
	TB_DRAW_AT_TOP				= 0x01,
	TB_NO_BORDER				= 0x02,
	
	TB_LINE_AT_ONCE				= 0x04,
	TB_VARIABLE_WIDTH_CHARS		= 0x08,
	TB_CURSOR_NEVER_SHOWN		= 0x10
};

class TextBox
{
public:
	bool Init();
	void Deinit();
	
	void SetVisible(bool enable, uint8_t flags = TB_DEFAULTS);
	void ResetState();
	
	void AddText(const std::string& str);
	void SetText(const std::string& str);
	void ClearText();
	void RecalculateOffsets();
	
	void SetFace(int newface);
	
	void SetFlags(uint8_t flags, bool enable);
	void SetFlags(uint8_t flags);
	void ShowCursor(bool enable);
	
	TB_YNJPrompt YesNoPrompt;
	TB_ItemImage ItemImage;
	TB_StageSelect StageSelect;
	TB_SaveSelect SaveSelect;
	
	bool IsVisible();
	bool IsBusy();
	
	void Draw();
	static void DrawFrame(int x, int y, int w, int h);
	
	uint8_t GetFlags() { return fFlags; }
	void SetCanSpeedUp(bool newstate);
	
private:
	void DrawTextBox();
	int GetMaxLineLen();
	void AddNextChar();
	
	bool fVisible;
	uint8_t fFlags;
	
	uint8_t fFace;			// current NPC face or 0 if none
	int fFaceXOffset;		// for face slide-in animation
	
	// currently visible lines
	std::array<std::string, MSG_NLINES> fLines;
	int fCurLine;
	int fCurLineLen;
	
	// handles scrolling lines off
	bool fScrolling;
	int fTextYOffset;
	
	// chars waiting to be added
	std::string fCharsWaiting;
	
	int fTextTimer;
	bool fCanSpeedUp;
	
	// blinking cursor control
	bool fCursorVisible;
	int fCursorTimer;
	
	struct
	{
		int x, y;
		int w, h;
	}
	fCoords;
};

#endif

