#pragma once
#include "view.h"

// Mouse selection functions
void view_start_mouse_selection(ViewState *vs, int row, int col);
void view_update_mouse_selection(ViewState *vs, int row, int col);
void view_end_mouse_selection(ViewState *vs);
void view_clear_selection(ViewState *vs);
void view_copy_selection(ViewState *vs);

// Selection queries
bool view_is_selected(ViewState *vs, int row, int col);
bool view_is_click_in_selection(ViewState *vs, int row, int col); 