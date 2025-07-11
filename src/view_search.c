#include "view_search.h"
#include <string.h>
#include <stdlib.h>

// Forward declarations for static functions

// search mode handling
void view_start_search(ViewState *vs) {
    vs->search_mode = true;
    vs->search_pos = 0;
    vs->search_buffer[0] = '\0';
    // Clear previous results
    vs->search_matches = 0;
    vs->search_current = 0;
}

void view_add_search_char(ViewState *vs, char c) {
    if (!vs->search_mode) return;
    
    // don't overflow buffer
    if (vs->search_pos >= 63) {
        // Buffer is full, ignore additional characters
        return;
    }
    
    vs->search_buffer[vs->search_pos++] = c;
    vs->search_buffer[vs->search_pos] = '\0';
    
    // Live search: automatically update results as user types
    view_find_matches(vs, vs->search_buffer);
    
    if (vs->search_matches > 0) {
        vs->search_current = 0;
        // Jump to first match
        view_jump_to_match(vs, 0);
    }
}



void view_find_matches(ViewState *vs, const char *query) {
    if (strlen(query) == 0) {
        vs->search_matches = 0;
        return;
    }
    
    // Safety check: prevent overly broad wildcard searches
    int query_len = strlen(query);
    int non_wildcard_chars = 0;
    for (int i = 0; i < query_len; i++) {
        if (query[i] != '*') {
            non_wildcard_chars++;
        }
    }
    
    // Require at least one non-wildcard character to prevent explosion
    if (non_wildcard_chars == 0) {
        vs->search_matches = 0;
        return;
    }
    
    // Allocate initial capacity
    if (vs->search_results == NULL) {
        vs->search_capacity = 100;
        vs->search_results = malloc(vs->search_capacity * sizeof(SearchMatch));
    }
    
    vs->search_matches = 0;
    
    // For very long searches, use a more aggressive limit to maintain performance
    int max_matches = (query_len > 40) ? 1000 : 10000;
    
    // Search through all sequences
    for (size_t seq_idx = 0; seq_idx < vs->seqs->count; seq_idx++) {
        Sequence *seq = &vs->seqs->items[seq_idx];
        
        // Search through the sequence
        for (int pos = 0; pos <= (int)seq->len - query_len; pos++) {
            bool match = true;
            
            // Check if query matches at this position (case-insensitive with wildcard support)
            for (int i = 0; i < query_len; i++) {
                char seq_char = seq->seq[pos + i];
                char query_char = query[i];
                
                // Wildcard '*' matches any character
                if (query_char == '*') {
                    continue;  // Always matches
                }
                
                // Convert to uppercase for comparison
                if (seq_char >= 'a' && seq_char <= 'z') seq_char -= 32;
                if (query_char >= 'a' && query_char <= 'z') query_char -= 32;
                
                if (seq_char != query_char) {
                    match = false;
                    break;
                }
            }
            
            if (match) {
                // Expand array if needed
                if (vs->search_matches >= vs->search_capacity) {
                    vs->search_capacity *= 2;
                    vs->search_results = realloc(vs->search_results, vs->search_capacity * sizeof(SearchMatch));
                }
                
                vs->search_results[vs->search_matches].seq_idx = seq_idx;
                vs->search_results[vs->search_matches].pos = pos;
                vs->search_matches++;
                
                // Safety limit to prevent excessive matches (lower limit for very long searches)
                if (vs->search_matches >= max_matches) {
                    return;
                }
            }
        }
    }
}

void view_jump_to_match(ViewState *vs, int match_idx) {
    if (match_idx < 0 || match_idx >= vs->search_matches) return;
    
    SearchMatch *match = &vs->search_results[match_idx];
    
    // Update viewport to show this match
    vs->row_offset = match->seq_idx;
    vs->col_offset = match->pos;
    
    // Ensure the match is visible by centering it
    int content_height = vs->rows - 3;  // ruler + underscores + status
    int half_screen = content_height / 2;
    
    // Center vertically
    vs->row_offset = match->seq_idx - half_screen;
    if (vs->row_offset < 0) vs->row_offset = 0;
    
    int max_row_off = vs->seqs->count - content_height;
    if (max_row_off < 0) max_row_off = 0;
    if (vs->row_offset > max_row_off) vs->row_offset = max_row_off;
    
    // Center horizontally
    int avail_width = vs->cols - 18;  // subtract ID width and separator
    int half_width = avail_width / 2;
    
    vs->col_offset = match->pos - half_width;
    if (vs->col_offset < 0) vs->col_offset = 0;
    
    // Find max sequence length for clamping
    int max_seq_len = 0;
    for (size_t i = 0; i < vs->seqs->count; i++) {
        if ((int)vs->seqs->items[i].len > max_seq_len) {
            max_seq_len = (int)vs->seqs->items[i].len;
        }
    }
    int max_col_offset = max_seq_len - 1;
    if (max_col_offset < 0) max_col_offset = 0;
    if (vs->col_offset > max_col_offset) vs->col_offset = max_col_offset;
}

void view_execute_search(ViewState *vs) {
    if (vs->search_pos == 0) {
        view_cancel_search(vs);
        return;
    }
    
    // Find all matches across all sequences
    view_find_matches(vs, vs->search_buffer);
    
    if (vs->search_matches > 0) {
        vs->search_current = 0;
        // Jump to first match
        view_jump_to_match(vs, 0);
    }
    
    // Stay in search mode to allow navigation
}

void view_cancel_search(ViewState *vs) {
    vs->search_mode = false;
    vs->search_pos = 0;
    vs->search_buffer[0] = '\0';
    vs->search_matches = 0;
    vs->search_current = 0;
}



void view_navigate_matches(ViewState *vs, bool next) {
    if (vs->search_matches == 0) return;
    
    if (next) {
        vs->search_current = (vs->search_current + 1) % vs->search_matches;
    } else {
        vs->search_current = (vs->search_current - 1 + vs->search_matches) % vs->search_matches;
    }
    
    view_jump_to_match(vs, vs->search_current);
}

bool view_is_search_match(ViewState *vs, int seq_idx, int pos) {
    if (vs->search_matches == 0) return false;
    
    // Don't highlight if too many matches (>100)
    if (vs->search_matches > 100) return false;
    
    int query_len = strlen(vs->search_buffer);
    if (query_len == 0) return false;
    
    // Check if this position starts a match
    for (int i = 0; i < vs->search_matches; i++) {
        SearchMatch *match = &vs->search_results[i];
        if (match->seq_idx == seq_idx && pos >= match->pos && pos < match->pos + query_len) {
            return true;
        }
    }
    return false;
}

bool view_is_current_search_match(ViewState *vs, int seq_idx, int pos) {
    if (vs->search_matches == 0 || vs->search_current >= vs->search_matches) return false;
    
    int query_len = strlen(vs->search_buffer);
    if (query_len == 0) return false;
    
    SearchMatch *current_match = &vs->search_results[vs->search_current];
    return (current_match->seq_idx == seq_idx && 
            pos >= current_match->pos && 
            pos < current_match->pos + query_len);
} 