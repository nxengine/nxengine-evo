#ifndef __AIDOCTORCOM_H_
#define __AIDOCTORCOM_H_
#include "../../object.h"

Object *dr_create_red_crystal(int x, int y);
void dr_tp_out_init(Object *o);
bool dr_tp_out(Object *o);
void dr_tp_in_init(Object *o);
bool dr_tp_in(Object *o);

#endif
