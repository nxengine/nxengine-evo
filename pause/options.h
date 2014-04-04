
#ifndef _OPTIONS_H
#define _OPTIONS_H
#include "../object.h"

namespace Options
{
	// class for something like a dialog box that can hold the focus.
	// there is a stack of them. everyone in the stack is drawn,
	// and the topmost one receives HandleKey events.
	class FocusHolder
	{
	public:
		virtual ~FocusHolder() { }
		
		virtual void Draw() = 0;
		virtual void RunInput() = 0;
	};
	
	
	void init_objects();
	void close_objects();
	void run_and_draw_objects(void);
	Object *create_object(int x, int y, int type);

};	// end namespace


bool options_init(int param);
void options_tick(void);
void options_close(void);

#endif
