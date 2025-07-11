#include "view_modes.h"
#include <stdlib.h>
#include <ctype.h>

// jump mode handling
void view_start_jump(ViewState *vs) {
    vs->jump_mode = true;
    vs->jump_pos = 0;
    vs->jump_buffer[0] = '\0';
}

void view_add_jump_digit(ViewState *vs, char digit) {
    if (!vs->jump_mode) return;
    
    // ignore minus sign and non-digits
    if (digit == '-' || !isdigit(digit)) return;
    
    // don't overflow buffer
    if (vs->jump_pos >= 15) return;
    
    vs->jump_buffer[vs->jump_pos++] = digit;
    vs->jump_buffer[vs->jump_pos] = '\0';
}

void view_execute_jump(ViewState *vs) {
    if (!vs->jump_mode) return;
    
    // if empty buffer, just exit jump mode
    if (vs->jump_pos == 0) {
        view_cancel_jump(vs);
        return;
    }
    
    int target_pos = atoi(vs->jump_buffer);
    
    // if 0, do nothing (as requested)
    if (target_pos == 0) {
        view_cancel_jump(vs);
        return;
    }
    
    // convert to 0-based indexing (user enters 1-based)
    target_pos--;
    
    // find the maximum sequence length to clamp to
    int max_seq_len = 0;
    for (size_t i = 0; i < vs->seqs->count; i++) {
        if ((int)vs->seqs->items[i].len > max_seq_len) {
            max_seq_len = (int)vs->seqs->items[i].len;
        }
    }
    
    // clamp to valid range
    if (target_pos < 0) target_pos = 0;
    if (target_pos >= max_seq_len) target_pos = max_seq_len - 1;
    
    vs->col_offset = target_pos;
    view_cancel_jump(vs);
}

void view_cancel_jump(ViewState *vs) {
    vs->jump_mode = false;
    vs->jump_pos = 0;
    vs->jump_buffer[0] = '\0';
} 