#pragma once
#include "view.h"

// Search mode handling
void view_start_search(ViewState *vs);
void view_add_search_char(ViewState *vs, char c);
void view_execute_search(ViewState *vs);
void view_cancel_search(ViewState *vs);

// Search navigation
void view_navigate_search_history(ViewState *vs, bool up);
void view_navigate_matches(ViewState *vs, bool next);

// Search match queries
bool view_is_search_match(ViewState *vs, int seq_idx, int pos);
bool view_is_current_search_match(ViewState *vs, int seq_idx, int pos); 