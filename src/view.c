#include "view.h"
#include "term.h"

ViewState view_init(SeqList *s) {
    ViewState vs = { .seqs = s, .row_offset = 0, .col_offset = 0 };
    get_term_size(&vs.rows, &vs.cols);
    return vs;
}

void view_resize(ViewState *vs) {
    get_term_size(&vs->rows, &vs->cols);
    // clamp row_offset so we never drop the first line
    int content = vs->rows - 2;
    int max_row_off = vs->seqs->count - content;
    if (max_row_off < 0) max_row_off = 0;
    if (vs->row_offset > max_row_off) vs->row_offset = max_row_off;
    // optionally clamp col_offset too (so we don't scroll past the end of the longest seq)
    // you could track max_seq_len in your model, but a quick hack is:
    for (size_t i = 0; i < vs->seqs->count; i++) {
        if ((int)vs->seqs->items[i].len - (vs->cols - 4) < vs->col_offset)
            vs->col_offset = vs->seqs->items[i].len - (vs->cols - 4);
    }
    if (vs->col_offset < 0) vs->col_offset = 0;
}

// vertical scroll
void view_scroll_down(ViewState *vs) {
    int content = vs->rows - 2; 
    int max_off = vs->seqs->count - content;
    if (vs->row_offset < max_off) vs->row_offset++;
}
void view_scroll_up(ViewState *vs) {
    if (vs->row_offset > 0) vs->row_offset--;
}

// horizontal scroll
void view_scroll_right(ViewState *vs) {
    vs->col_offset++;
}
void view_scroll_left(ViewState *vs) {
    if (vs->col_offset > 0) vs->col_offset--;
}