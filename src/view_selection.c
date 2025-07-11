#include "view_selection.h"
#include <stdio.h>
#include <stdlib.h>

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