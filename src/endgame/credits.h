
#ifndef _CREDITS_H
#define _CREDITS_H

#define MAX_BIGIMAGES		20
#define CRED_MAX_TEXT		80
#include "CredReader.h"
#include "../graphics/nxsurface.h"

// a currently displayed line of text
struct CredLine
{
	char text[CRED_MAX_TEXT];
	int image;
	int x, y;
	
	CredLine *next, *prev;
};

class BigImage
{
public:
	bool Init();
	~BigImage();
	
	void Set(int num);
	void Clear();
	void Draw();
	
private:
	int imagex, state;
	int imgno;
	NXSurface *images[MAX_BIGIMAGES];
};


class Credits
{
public:
	bool Init();
	void Tick();
	~Credits();
	
	BigImage bigimage;	// current "SIL" big left-hand image
	
private:
	void RunNextCommand();
	bool Jump(int label);
	
	CredLine *NewLine();
	CredLine *AddLine(CredLine *line);
	void RemoveLine(CredLine *line);
	
	void Draw();
	bool DrawLine(CredLine *line);
	
	
	int spawn_y;		// position of next line relative to top of roll
	int scroll_y;		// CSFd roll position
	
	int xoffset;		// x position of next line
	
	// turns off scrolling and further script execution when "/" command hit at end
	bool roll_running;
	
	
	int lines_out;		// debug...
	int lines_vis;		// ...counters
	
	CredReader script;
	CredLine *firstline, *lastline;
};


bool credit_init(int parameter);
void credit_tick();
void credit_set_image(int imgno);
void credit_clear_image();
void credit_close();

#endif



