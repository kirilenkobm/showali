#pragma once
#include "view.h"

// Jump mode handling
void view_start_jump(ViewState *vs);
void view_add_jump_digit(ViewState *vs, char digit);
void view_execute_jump(ViewState *vs);
void view_cancel_jump(ViewState *vs); 