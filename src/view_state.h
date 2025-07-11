#pragma once
#include "view.h"

// Core ViewState management functions
ViewState view_init(SeqList *s);
void view_resize(ViewState *vs); 