#include "view.h"
#include "term.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

ViewState view_init(SeqList *s) {
    ViewState vs = { 
        .seqs = s, 
        .row_offset = 0, 
        .col_offset = 0, 
        .jump_mode = false, 
        .jump_pos = 0,
        .no_color = false,
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

// Mouse selection functions
void view_screen_to_sequence_pos(ViewState *vs, int screen_x, int screen_y, int *seq_row, int *seq_col) {
    // Convert screen coordinates to sequence row/col
    // Screen layout: 16 chars for ID + "| " + sequence data
    
    // Row calculation
    *seq_row = vs->row_offset + screen_y;
    
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
}

void view_start_mouse_selection(ViewState *vs, int row, int col) {
    vs->has_selection = true;
    vs->selecting = true;
    vs->select_start_row = row;
    vs->select_start_col = col;
    vs->select_end_row = row;
    vs->select_end_col = col;
}

void view_update_mouse_selection(ViewState *vs, int row, int col) {
    if (!vs->selecting) return;
    
    vs->select_end_row = row;
    vs->select_end_col = col;
}

void view_end_mouse_selection(ViewState *vs) {
    vs->selecting = false;
    // Keep has_selection = true to maintain the selection
}

void view_clear_selection(ViewState *vs) {
    vs->has_selection = false;
    vs->selecting = false;
    vs->select_start_row = 0;
    vs->select_start_col = 0;
    vs->select_end_row = 0;
    vs->select_end_col = 0;
}

void view_copy_selection(ViewState *vs) {
    if (!vs->has_selection) return;
    
    // Determine selection bounds (normalize start/end)
    int start_row = (vs->select_start_row < vs->select_end_row) ? vs->select_start_row : vs->select_end_row;
    int end_row = (vs->select_start_row < vs->select_end_row) ? vs->select_end_row : vs->select_start_row;
    int start_col = (vs->select_start_col < vs->select_end_col) ? vs->select_start_col : vs->select_end_col;
    int end_col = (vs->select_start_col < vs->select_end_col) ? vs->select_end_col : vs->select_start_col;
    
    // Clamp to valid ranges
    if (start_row < 0) start_row = 0;
    if (end_row >= (int)vs->seqs->count) end_row = (int)vs->seqs->count - 1;
    if (start_col < 0) start_col = 0;
    
    // Find maximum column
    int max_col = 0;
    for (size_t i = 0; i < vs->seqs->count; i++) {
        if ((int)vs->seqs->items[i].len > max_col) {
            max_col = (int)vs->seqs->items[i].len;
        }
    }
    if (end_col >= max_col) end_col = max_col - 1;
    
    // Create a temporary file for the selected text
    FILE *temp_file = tmpfile();
    if (!temp_file) return;
    
    // Extract rectangular selection
    for (int row = start_row; row <= end_row; row++) {
        if (row < (int)vs->seqs->count) {
            Sequence *seq = &vs->seqs->items[row];
            for (int col = start_col; col <= end_col; col++) {
                if (col < (int)seq->len) {
                    fputc(seq->seq[col], temp_file);
                } else {
                    fputc(' ', temp_file);  // Pad with spaces if sequence is shorter
                }
            }
        }
        fputc('\n', temp_file);
    }
    
    // Copy to clipboard using system command
    fflush(temp_file);
    rewind(temp_file);
    
    // Try different clipboard commands based on system
    char *clipboard_cmd = NULL;
    
    // Check for macOS first (most likely on your system)
    if (system("which pbcopy >/dev/null 2>&1") == 0) {
        clipboard_cmd = "pbcopy";  // macOS
    } 
    // Then check for Linux tools
    else if (system("which xclip >/dev/null 2>&1") == 0) {
        clipboard_cmd = "xclip -selection clipboard";  // Linux with xclip
    } else if (system("which xsel >/dev/null 2>&1") == 0) {
        clipboard_cmd = "xsel --clipboard --input";  // Linux with xsel
    }
    // Windows WSL might have clip.exe
    else if (system("which clip.exe >/dev/null 2>&1") == 0) {
        clipboard_cmd = "clip.exe";  // Windows WSL
    }
    
    if (clipboard_cmd) {
        FILE *clipboard = popen(clipboard_cmd, "w");
        if (clipboard) {
            char buffer[1024];
            size_t bytes;
            while ((bytes = fread(buffer, 1, sizeof(buffer), temp_file)) > 0) {
                fwrite(buffer, 1, bytes, clipboard);
            }
            int result = pclose(clipboard);
            if (result == 0) {
                // Success - clipboard copy worked
            }
        }
    }
    
    fclose(temp_file);
}

bool view_is_selected(ViewState *vs, int row, int col) {
    if (!vs->has_selection) return false;
    
    // Determine selection bounds
    int start_row = (vs->select_start_row < vs->select_end_row) ? vs->select_start_row : vs->select_end_row;
    int end_row = (vs->select_start_row < vs->select_end_row) ? vs->select_end_row : vs->select_start_row;
    int start_col = (vs->select_start_col < vs->select_end_col) ? vs->select_start_col : vs->select_end_col;
    int end_col = (vs->select_start_col < vs->select_end_col) ? vs->select_end_col : vs->select_start_col;
    
    return (row >= start_row && row <= end_row && col >= start_col && col <= end_col);
}

bool view_is_click_in_selection(ViewState *vs, int row, int col) {
    if (!vs->has_selection) return false;
    
    // Use the same logic as view_is_selected
    return view_is_selected(vs, row, col);
}