#include "view_navigation.h"
#include "term.h"

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
    int content = vs->rows - 3;  // ruler + underscores + status
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

// Half-screen movement functions for WASD navigation
void view_scroll_half_screen_up(ViewState *vs) {
    int content = vs->rows - 3;  // ruler + underscores + status
    int half_screen = content / 2;
    if (half_screen < 1) half_screen = 1;
    view_scroll_up_steps(vs, half_screen);
}

void view_scroll_half_screen_down(ViewState *vs) {
    int content = vs->rows - 3;  // ruler + underscores + status
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

// Mouse selection functions
void view_screen_to_sequence_pos(ViewState *vs, int screen_x, int screen_y, int *seq_row, int *seq_col) {
    // Convert screen coordinates to sequence row/col
    // Screen layout: ruler line + 16 chars for ID + "| " + sequence data
    
    // Row calculation - account for ruler line at top
    *seq_row = vs->row_offset + screen_y - 1;
    
    // Column calculation - account for ID width and separator
    int id_width = 16;
    int separator_width = 2; // "| "
    
    if (screen_x < id_width + separator_width) {
        *seq_col = vs->col_offset; // Click was on ID, use current column offset
    } else {
        *seq_col = vs->col_offset + (screen_x - id_width - separator_width);
    }
    
    // Clamp to valid ranges
    if (*seq_row < 0) *seq_row = 0;
    if (*seq_row >= (int)vs->seqs->count) *seq_row = (int)vs->seqs->count - 1;
    if (*seq_col < 0) *seq_col = 0;
    
    // Find max sequence length and clamp column
    int max_seq_len = 0;
    for (size_t i = 0; i < vs->seqs->count; i++) {
        if ((int)vs->seqs->items[i].len > max_seq_len) {
            max_seq_len = (int)vs->seqs->items[i].len;
        }
    }
    if (*seq_col >= max_seq_len) *seq_col = max_seq_len - 1;
    if (max_seq_len == 0) *seq_col = 0;  // Handle empty sequences
} 