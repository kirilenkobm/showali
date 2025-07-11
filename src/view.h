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

// Include all the decomposed modules
#include "view_state.h"
#include "view_navigation.h"
#include "view_search.h"
#include "view_selection.h"
#include "view_modes.h"