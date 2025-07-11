#include "view.h"
#include "term.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

ViewState view_init(SeqList *s) {
    ViewState vs = { 
        .seqs = s, 
        .row_offset = 0, 
        .col_offset = 0, 
        .jump_mode = false, 
        .jump_pos = 0,
        .no_color = false,
        .last_key = 0,
        .repeat_count = 0,
        .accel_step = 1
    };
    vs.jump_buffer[0] = '\0';
    get_term_size(&vs.rows, &vs.cols);
    get_current_time(&vs.last_key_time);
    return vs;
}

void view_resize(ViewState *vs) {
    get_term_size(&vs->rows, &vs->cols);
    // clamp row_offset so we never drop the first line
    int content = vs->rows - 2;  // underscores + status
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

// vertical scroll (single step)
void view_scroll_down(ViewState *vs) {
    view_scroll_down_steps(vs, vs->accel_step);
}

void view_scroll_up(ViewState *vs) {
    view_scroll_up_steps(vs, vs->accel_step);
}

// horizontal scroll (single step)
void view_scroll_right(ViewState *vs) {
    view_scroll_right_steps(vs, vs->accel_step);
}

void view_scroll_left(ViewState *vs) {
    view_scroll_left_steps(vs, vs->accel_step);
}

// vertical scroll with step size
void view_scroll_down_steps(ViewState *vs, int steps) {
    int content = vs->rows - 2;  // underscores + status
    int max_off = vs->seqs->count - content;
    if (max_off < 0) max_off = 0;
    
    vs->row_offset += steps;
    if (vs->row_offset > max_off) vs->row_offset = max_off;
}

void view_scroll_up_steps(ViewState *vs, int steps) {
    vs->row_offset -= steps;
    if (vs->row_offset < 0) vs->row_offset = 0;
}

// horizontal scroll with step size
void view_scroll_right_steps(ViewState *vs, int steps) {
    // Calculate the new position after the movement
    int new_col_offset = vs->col_offset + steps;
    
    // Find the maximum sequence length to determine bounds
    int max_seq_len = 0;
    for (size_t i = 0; i < vs->seqs->count; i++) {
        if ((int)vs->seqs->items[i].len > max_seq_len) {
            max_seq_len = (int)vs->seqs->items[i].len;
        }
    }
    
    // Allow scrolling until the last character position (more permissive)
    // This allows reaching the very end of sequences
    int max_col_offset = max_seq_len - 1;  // 0-based indexing
    if (max_col_offset < 0) max_col_offset = 0;
    
    // Apply the movement but clamp to maximum if it would go too far
    if (new_col_offset > max_col_offset) {
        vs->col_offset = max_col_offset;
    } else {
        vs->col_offset = new_col_offset;
    }
}

void view_scroll_left_steps(ViewState *vs, int steps) {
    vs->col_offset -= steps;
    if (vs->col_offset < 0) vs->col_offset = 0;
}

// acceleration handling
void view_update_acceleration(ViewState *vs, int key) {
    struct timespec current_time;
    get_current_time(&current_time);
    
    if (vs->last_key == key) {
        // Same key pressed again, increment repeat count
        vs->repeat_count++;
        
        // Calculate step size based on repeat count - more aggressive acceleration
        if (vs->repeat_count <= 2) {
            vs->accel_step = 1;           // Normal speed
        } else if (vs->repeat_count <= 5) {
            vs->accel_step = 16;           // Noticeable acceleration
        } else if (vs->repeat_count <= 10) {
            vs->accel_step = 32;           // Fast
        } else if (vs->repeat_count <= 20) {
            vs->accel_step = 64;          // Very fast
        } else {
            vs->accel_step = 256;          // Maximum speed
        }
    } else {
        // Different key pressed, reset acceleration
        vs->repeat_count = 1;
        vs->accel_step = 1;
    }
    
    vs->last_key = key;
    vs->last_key_time = current_time;
}

void view_reset_acceleration(ViewState *vs) {
    vs->repeat_count = 0;
    vs->accel_step = 1;
    vs->last_key = 0;
}

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

// Half-screen movement functions for WASD navigation
void view_scroll_half_screen_up(ViewState *vs) {
    int content = vs->rows - 2;  // underscores + status
    int half_screen = content / 2;
    if (half_screen < 1) half_screen = 1;
    view_scroll_up_steps(vs, half_screen);
}

void view_scroll_half_screen_down(ViewState *vs) {
    int content = vs->rows - 2;  // underscores + status
    int half_screen = content / 2;
    if (half_screen < 1) half_screen = 1;
    view_scroll_down_steps(vs, half_screen);
}

void view_scroll_half_screen_left(ViewState *vs) {
    int avail_width = vs->cols - 18;  // subtract ID width and separator
    int half_screen = avail_width / 2;
    if (half_screen < 1) half_screen = 1;
    view_scroll_left_steps(vs, half_screen);
}

void view_scroll_half_screen_right(ViewState *vs) {
    int avail_width = vs->cols - 18;  // subtract ID width and separator
    int half_screen = avail_width / 2;
    if (half_screen < 1) half_screen = 1;
    view_scroll_right_steps(vs, half_screen);
}