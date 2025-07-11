#include "view_state.h"
#include "term.h"
#include <string.h>

ViewState view_init(SeqList *s) {
    ViewState vs = { 
        .seqs = s, 
        .row_offset = 0, 
        .col_offset = 0, 
        .jump_mode = false, 
        .jump_pos = 0,
        .no_color = false,
        .search_mode = false,
        .search_pos = 0,
        .search_matches = 0,
        .search_current = 0,
        .search_results = NULL,
        .search_capacity = 0,
        .search_history_pos = 0,
        .search_history_count = 0,
        .search_browsing_history = false,
        .search_history_idx = 0,
        .has_selection = false,
        .select_start_row = 0,
        .select_start_col = 0,
        .select_end_row = 0,
        .select_end_col = 0,
        .selecting = false,
        .last_key = 0,
        .repeat_count = 0,
        .accel_step = 1
    };
    vs.jump_buffer[0] = '\0';
    vs.search_buffer[0] = '\0';
    get_term_size(&vs.rows, &vs.cols);
    get_current_time(&vs.last_key_time);
    return vs;
}

void view_resize(ViewState *vs) {
    get_term_size(&vs->rows, &vs->cols);
    // clamp row_offset so we never drop the first line
    int content = vs->rows - 3;  // ruler + underscores + status
    int max_row_off = vs->seqs->count - content;
    if (max_row_off < 0) max_row_off = 0;
    if (vs->row_offset > max_row_off) vs->row_offset = max_row_off;
    // clamp col_offset to not go past the end of the longest sequence
    int max_seq_len = 0;
    for (size_t i = 0; i < vs->seqs->count; i++) {
        if ((int)vs->seqs->items[i].len > max_seq_len) {
            max_seq_len = (int)vs->seqs->items[i].len;
        }
    }
    int max_col_offset = max_seq_len - 1;  // 0-based indexing
    if (max_col_offset < 0) max_col_offset = 0;
    if (vs->col_offset > max_col_offset) vs->col_offset = max_col_offset;
    if (vs->col_offset < 0) vs->col_offset = 0;
} 