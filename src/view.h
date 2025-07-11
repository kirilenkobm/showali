#pragma once
#include "parser_fasta.h"
#include <stdbool.h>
#include <time.h>

typedef struct {
    SeqList *seqs;      // all sequences
    int      row_offset; // index of the first sequence row shown
    int      col_offset; // index of the first base shown
    int      rows;       // terminal height
    int      cols;       // terminal width
    bool     jump_mode;  // true when collecting jump position
    char     jump_buffer[16]; // buffer for collecting jump digits
    int      jump_pos;   // current position in jump buffer
    
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

// half-screen movement functions for WASD navigation
void view_scroll_half_screen_up(ViewState *vs);
void view_scroll_half_screen_down(ViewState *vs);
void view_scroll_half_screen_left(ViewState *vs);
void view_scroll_half_screen_right(ViewState *vs);