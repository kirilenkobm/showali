#include "view.h"
#include "term.h"

ViewState view_init(SeqList *s) {
    ViewState vs = { .seqs = s, .row_offset = 0, .col_offset = 0 };
    get_term_size(&vs.rows, &vs.cols);
    return vs;
}

void view_resize(ViewState *vs) {
    get_term_size(&vs->rows, &vs->cols);
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