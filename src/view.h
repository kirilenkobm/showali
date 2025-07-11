#pragma once
#include "parser_fasta.h"
#include <stdbool.h>
#include <time.h>

typedef struct {
    int seq_idx;    // which sequence
    int pos;        // position in sequence
} SearchMatch;

typedef struct {
    SeqList *seqs;      // all sequences
    int      row_offset; // index of the first sequence row shown
    int      col_offset; // index of the first base shown
    int      rows;       // terminal height
    int      cols;       // terminal width
    bool     jump_mode;  // true when collecting jump position
    char     jump_buffer[16]; // buffer for collecting jump digits
    int      jump_pos;   // current position in jump buffer
    bool     no_color;   // true when colors should be disabled
    
    // Search state
    bool     search_mode;        // true when in search mode
    char     search_buffer[64];  // current search query
    int      search_pos;         // position in search buffer
    
    // Search results
    int      search_matches;     // total number of matches
    int      search_current;     // current match (0-based)
    SearchMatch *search_results; // array of match positions
    int      search_capacity;    // capacity of search_results array
    
    // Search history (simple circular buffer)
    char     search_history[10][64];  // last 10 searches
    int      search_history_pos;      // current position in history
    int      search_history_count;    // number of items in history
    bool     search_browsing_history; // true when browsing history with arrows
    int      search_history_idx;      // current index when browsing
    
    // Mouse selection state
    bool     has_selection;      // true when there's an active selection
    int      select_start_row;   // start row of selection (sequence index)
    int      select_start_col;   // start column of selection (base index)
    int      select_end_row;     // end row of selection (sequence index)
    int      select_end_col;     // end column of selection (base index)
    bool     selecting;          // true when actively selecting (mouse drag)
    
    // Acceleration state
    int      last_key;   // last key pressed (for detecting repeats)
    struct timespec last_key_time;  // timestamp of last key press
    int      repeat_count; // number of consecutive repeats of the same key
    int      accel_step;  // current step size for scrolling
} ViewState;

// initialize view state from parsed sequences
ViewState view_init(SeqList *s);

// adjust to a terminal resize
void view_resize(ViewState *vs);

// vertical scrolling
void view_scroll_up(ViewState *vs);
void view_scroll_down(ViewState *vs);

// horizontal scrolling
void view_scroll_left(ViewState *vs);
void view_scroll_right(ViewState *vs);

// vertical scrolling with step size
void view_scroll_up_steps(ViewState *vs, int steps);
void view_scroll_down_steps(ViewState *vs, int steps);

// horizontal scrolling with step size
void view_scroll_left_steps(ViewState *vs, int steps);
void view_scroll_right_steps(ViewState *vs, int steps);

// acceleration handling
void view_update_acceleration(ViewState *vs, int key);
void view_reset_acceleration(ViewState *vs);

// jump mode handling
void view_start_jump(ViewState *vs);
void view_add_jump_digit(ViewState *vs, char digit);
void view_execute_jump(ViewState *vs);
void view_cancel_jump(ViewState *vs);

// search mode handling
void view_start_search(ViewState *vs);
void view_add_search_char(ViewState *vs, char c);
void view_execute_search(ViewState *vs);
void view_cancel_search(ViewState *vs);
void view_navigate_search_history(ViewState *vs, bool up);
void view_navigate_matches(ViewState *vs, bool next);
bool view_is_search_match(ViewState *vs, int seq_idx, int pos);
bool view_is_current_search_match(ViewState *vs, int seq_idx, int pos);

// half-screen movement functions for WASD navigation
void view_scroll_half_screen_up(ViewState *vs);
void view_scroll_half_screen_down(ViewState *vs);
void view_scroll_half_screen_left(ViewState *vs);
void view_scroll_half_screen_right(ViewState *vs);

// mouse selection functions
void view_start_mouse_selection(ViewState *vs, int row, int col);
void view_update_mouse_selection(ViewState *vs, int row, int col);
void view_end_mouse_selection(ViewState *vs);
void view_clear_selection(ViewState *vs);
void view_copy_selection(ViewState *vs);
bool view_is_selected(ViewState *vs, int row, int col);
bool view_is_click_in_selection(ViewState *vs, int row, int col);
void view_screen_to_sequence_pos(ViewState *vs, int screen_x, int screen_y, int *seq_row, int *seq_col);