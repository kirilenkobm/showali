#pragma once
#include "view.h"

// Search mode handling
void view_start_search(ViewState *vs);
void view_add_search_char(ViewState *vs, char c);
void view_execute_search(ViewState *vs);
void view_cancel_search(ViewState *vs);

// Search navigation
void view_navigate_matches(ViewState *vs, bool next);

// Search internal functions (made public for live search)
void view_find_matches(ViewState *vs, const char *query);
void view_jump_to_match(ViewState *vs, int match_idx);

// Search match queries
bool view_is_search_match(ViewState *vs, int seq_idx, int pos);
bool view_is_current_search_match(ViewState *vs, int seq_idx, int pos); 