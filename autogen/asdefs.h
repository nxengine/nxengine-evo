
// this is a small file that tells AssignSprites.cpp how to do it's job,
// like glue that links it with the rest of the project in a project-independent way.
#include "../nx.h"

#define ASSIGN_SPRITE(OBJECT, SPRITE)	\
	objprop[OBJECT].sprite = SPRITE;
