
#include "nx.h"
#include "floattext.h"
#include "object.h"
#include "graphics/graphics.h"
#include "graphics/sprites.h"
using namespace Graphics;
using namespace Sprites;
#include "map.h"

FloatText *FloatText::first = NULL;
FloatText *FloatText::last = NULL;

/*
void c------------------------------() {}
*/

FloatText::FloatText(int sprite)
{
	prev = NULL;
	next = first;
	
	if (first)
		first->prev = this;
	else
		last = this;
	
	first = this;
	this->sprite = sprite;
	
	Reset();
	ObjectDestroyed = false;
}

FloatText::~FloatText()
{
	if (this->next) this->next->prev = this->prev;
	if (this->prev) this->prev->next = this->next;
	
	if (this == first) first = first->next;
	if (this == last) last = last->prev;
}

void FloatText::Reset()
{
	this->state = FT_IDLE;
	this->shownAmount = 0;
}

/*
void c------------------------------() {}
*/

// adds the spec'd amount of damage/energy to the object's point display
void FloatText::AddQty(int amt)
{
	//stat("FloatText::AddQty(%d)", amt);
	if (amt == 0) return;
	
	// first add the damage to the total
	if (this->state == FT_IDLE)
	{
		this->state = FT_RISE;
		
		this->shownAmount = amt;
		this->yoff = FT_Y_START;
		this->timer = 0;
	}
	else
	{
		this->shownAmount += amt;
		
		// if we're scrolling away jerk back down
		if (this->state == FT_SCROLL_AWAY)
		{
			this->state = FT_HOLD;
			this->yoff = FT_Y_HOLD;
		}
		
		// reset the timer which counts how long we stay in hold state
		if (this->state != FT_RISE)
			this->timer = 0;
	}
	
	if (this->shownAmount > 9999)
		this->shownAmount = 9999;		// overrun protection for buffer
}

// updates the position of a floattext in respect to it's object
void FloatText::UpdatePos(Object *assoc_object)
{
	// get the center pixel of the object we're associated with
	this->objX = (assoc_object->x >> CSF) + (sprites[assoc_object->sprite].w / 2);
	this->objY = (assoc_object->y >> CSF) + (sprites[assoc_object->sprite].h / 2);
	
	// adjust for possible draw point
	SIFDir *dir = &sprites[assoc_object->sprite].frame[assoc_object->frame].dir[assoc_object->dir];
	this->objX -= dir->drawpoint.x;
	this->objY -= dir->drawpoint.y;
}


// moves and draws the given float text if need be
void FloatText::Draw()
{
FloatText *ft = this;
int x, y, i;

	switch(ft->state)
	{
		// rise to top point, moving once every other frame
		case FT_RISE:
		{
			ft->timer ^= 1;
			if (ft->timer)
			{
				if (--ft->yoff <= FT_Y_HOLD)
				{
					ft->state = FT_HOLD;
					ft->timer = 0;
				}
			}
		}
		break;
		
		// hold at top for a moment
		case FT_HOLD:
		{
			if (++ft->timer >= 42)
				ft->state = FT_SCROLL_AWAY;
		}
		break;
		
		// scroll away quickly and disappear
		case FT_SCROLL_AWAY:
		{
			if (--ft->yoff <= FT_Y_RISEAWAY)
			{
				ft->state = FT_IDLE;
				ft->shownAmount = 0;
				ft->timer = 0;
				return;
			}
		}
		break;
	}
	
	// set the SDL clipping region to just above the hold point
	// so it looks like it "rolls" away.
	if (ft->state == FT_SCROLL_AWAY)
	{
		// this formula is confusing until you realize that FT_Y_HOLD is a negative number
		int y = ((ft->objY - (map.displayed_yscroll >> CSF)) + FT_Y_HOLD);
		int h = (SCREEN_HEIGHT - y);
		
		set_clip_rect(0, y, SCREEN_WIDTH, h);
	}
	
	// render the damage amount into a string
	char text[6] = { 10 };
	sprintf(&text[1], "%d", ft->shownAmount);
	for(i=1;text[i];i++) text[i] -= '0';
	int textlen = i;
	
	x = ft->objX - (textlen * (8 / 2));			// center the string on the object
	y = ft->objY + ft->yoff;
	// adjust to object's onscreen position
	x -= (map.displayed_xscroll >> CSF);
	y -= (map.displayed_yscroll >> CSF);
	
	// draw the text char by char
	for(i=0;i<textlen;i++)
	{
		draw_sprite(x, y, ft->sprite, text[i], 0);
		x += 8;
	}
	
	if (ft->state == FT_SCROLL_AWAY)
		clear_clip_rect();
}

bool FloatText::IsScrollingAway()
{
	return (this->state == FT_SCROLL_AWAY);
}

/*
void c------------------------------() {}
*/

void FloatText::DrawAll(void)
{
	FloatText *ft = first;
	FloatText *nextft;
	int count = 0;
	
	while(ft)
	{
		nextft = ft->next;
		
		if (ft->state != FT_IDLE)
		{
			ft->Draw();
		}
		else
		{
			if (ft->ObjectDestroyed)
				delete ft;
		}
		
		ft = nextft;
		count++;
	}
}

// do NOT call this to remove all enemy's floattext from the map.
// for one thing, it could leave dangling invalid pointers.
// for another, it deletes the player->XPText, etc.
// instead, call ResetAll and let them clean themselves up.
void FloatText::DeleteAll(void)
{
	while(first)
		delete first;
}

void FloatText::ResetAll(void)
{
	FloatText *ft = first;
	while(ft)
	{
		ft->Reset();
		ft = ft->next;
	}
}



