
#ifndef _NX_H
#define _NX_H

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "config.h"
#include "version.h"
#include "common/basics.h"

#define CSF				9 // eh, okay. seems like it for more smooth per-pixel moving. fuck if i know, really. go ask Kate

const char *strhex(int value);

#endif
