#ifndef __NIKU_H_
#define __NIKU_H_
#include <stdint.h>

bool niku_load(uint32_t *value_out);
bool niku_save(uint32_t value);

#endif