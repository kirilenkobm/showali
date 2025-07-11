#include "app.h"
#include "parser.h"
#include "view.h"
#include "view_search.h"
#include "render.h"
#include "input.h"
#include "term.h"
#include <stdio.h>
#include <stdbool.h>

int run_app(Args *args) {
    // 1) load sequences with auto-detection
    ParseResult *result = parse_alignment(args->filename);
    if (!result || !result->sequences) {
        if (result && result->error_message) {
            fprintf(stderr, "Failed to parse alignment file '%s': %s\n", args->filename, result->error_message);
        } else {
            fprintf(stderr, "Failed to parse alignment file '%s'\n", args->filename);
        }
        if (result) {
            free_parse_result(result);
        }
        return 1;
    }
    
    SeqList *seqs = result->sequences;
    AlignmentFormat format = result->format;
    
    // Print detected format info
    printf("Detected format: %s\n", format_to_string(format));
    printf("Loaded %zu sequences\n", seqs->count);
    
    // Don't free result->sequences since we're using it
    result->sequences = NULL;
    free_parse_result(result);

    // 2) init terminal (alt-screen, raw mode, SIGWINCH)
    enable_raw_mode();
    enable_altscreen();

    // 3) init view state
    ViewState vs = view_init(seqs);
    vs.no_color = args->no_color;

    // 4) main loop
    bool running = true;
    while (running) {
        render_frame(&vs);
        
        // Use timeout-based input reading (30ms timeout for acceleration reset)
        InputEvt ev = input_read_timeout(30);
        
        switch (ev.type) {
            case EVT_KEY:
                if (vs.jump_mode) {
                    // In jump mode, handle digits and Enter
                    if (ev.key == ENTER) {
                        view_execute_jump(&vs);
                    } else if (ev.key >= '0' && ev.key <= '9') {
                        view_add_jump_digit(&vs, ev.key);
                    } else if (ev.key == 'q' || ev.key == 'Q') {
                        running = false;
                    } else {
                        // Any other key cancels jump mode
                        view_cancel_jump(&vs);
                    }
                } else if (vs.search_mode) {
                    // In search mode, handle text input and navigation
                    if (ev.key == ENTER) {
                        view_execute_search(&vs);
                    } else if (ev.key == 27) { // ESC key
                        view_cancel_search(&vs);
                    } else if (ev.key == 'q' || ev.key == 'Q') {
                        // In search mode, Q doesn't quit - user needs to ESC first
                        view_add_search_char(&vs, ev.key);
                    } else if (ev.key == 8 || ev.key == 127) { // Backspace
                        if (vs.search_pos > 0) {
                            vs.search_pos--;
                            vs.search_buffer[vs.search_pos] = '\0';
                            
                            // Live search: update results after backspace
                            view_find_matches(&vs, vs.search_buffer);
                            
                            if (vs.search_matches > 0) {
                                vs.search_current = 0;
                                // Jump to first match
                                view_jump_to_match(&vs, 0);
                            }
                        }
                    } else if (ev.key == ARROW_LEFT && vs.search_matches > 0) {
                        view_navigate_matches(&vs, false);
                    } else if (ev.key == ARROW_RIGHT && vs.search_matches > 0) {
                        view_navigate_matches(&vs, true);
                    } else if (ev.key >= 32 && ev.key <= 126) { // Printable characters
                        view_add_search_char(&vs, ev.key);
                    }
                } else {
                    // Normal mode - handle acceleration for arrow keys
                    if (ev.key == 'q' || ev.key == 'Q') {
                        running = false;
                    } else if (ev.key == 'j' || ev.key == 'J') {
                        view_start_jump(&vs);
                        view_reset_acceleration(&vs);
                    } else if (ev.key == 27) { // ESC key
                        if (vs.has_selection) {
                            view_clear_selection(&vs);
                        }
                        view_reset_acceleration(&vs);
                    } else if (ev.key == 3) { // Ctrl+C
                        if (vs.has_selection) {
                            view_copy_selection(&vs);
                        }
                        view_reset_acceleration(&vs);
                    } else if ((ev.key == 'c' || ev.key == 'C') && vs.has_selection) {
                        // 'c' key to copy when selection is active
                        view_copy_selection(&vs);
                        view_reset_acceleration(&vs);
                    } else if (ev.key == ARROW_UP) {
                        view_update_acceleration(&vs, ARROW_UP);
                        view_scroll_up(&vs);
                    } else if (ev.key == ARROW_DOWN) {
                        view_update_acceleration(&vs, ARROW_DOWN);
                        view_scroll_down(&vs);
                    } else if (ev.key == ARROW_LEFT) {
                        view_update_acceleration(&vs, ARROW_LEFT);
                        view_scroll_left(&vs);
                    } else if (ev.key == ARROW_RIGHT) {
                        view_update_acceleration(&vs, ARROW_RIGHT);
                        view_scroll_right(&vs);
                    } else if (ev.key == 'w' || ev.key == 'W') {
                        // W - up, half screen
                        view_scroll_half_screen_up(&vs);
                        view_reset_acceleration(&vs);
                    } else if (ev.key == 's' || ev.key == 'S') {
                        // S - down, half screen
                        view_scroll_half_screen_down(&vs);
                        view_reset_acceleration(&vs);
                    } else if (ev.key == 'f' || ev.key == 'F') {
                        // F - find/search mode
                        view_start_search(&vs);
                        view_reset_acceleration(&vs);
                    } else if (ev.key == 'a' || ev.key == 'A') {
                        // A - left, half screen
                        view_scroll_half_screen_left(&vs);
                        view_reset_acceleration(&vs);
                    } else if (ev.key == 'd' || ev.key == 'D') {
                        // D - right, half screen
                        view_scroll_half_screen_right(&vs);
                        view_reset_acceleration(&vs);
                    } else {
                        // Reset acceleration for non-arrow keys
                        view_reset_acceleration(&vs);
                    }
                }
                break;
            case EVT_MOUSE:
                // Handle mouse events for rectangular selection
                if (ev.mouse_button == 0) {  // Left mouse button
                    int seq_row, seq_col;
                    view_screen_to_sequence_pos(&vs, ev.mouse_x, ev.mouse_y, &seq_row, &seq_col);
                    
                    if (ev.mouse_drag) {
                        // Drag event - update existing selection
                        if (vs.selecting) {
                            view_update_mouse_selection(&vs, seq_row, seq_col);
                        }
                    } else if (ev.mouse_pressed) {
                        // Initial press - start new selection
                        if (vs.has_selection && !view_is_click_in_selection(&vs, seq_row, seq_col)) {
                            // Clicking outside existing selection - clear it first
                            view_clear_selection(&vs);
                        }
                        // Start new selection
                        view_start_mouse_selection(&vs, seq_row, seq_col);
                    } else if (ev.mouse_released) {
                        if (vs.selecting) {
                            // End selection
                            view_end_mouse_selection(&vs);
                        }
                    }
                } else if (ev.mouse_button == 2 && ev.mouse_pressed) {  // Right mouse button
                    if (vs.has_selection) {
                        // Copy selection to clipboard
                        view_copy_selection(&vs);
                    }
                }
                break;
            case EVT_RESIZE:
                view_resize(&vs);
                break;
            case EVT_TIMEOUT:
                // Reset acceleration on timeout (user stopped pressing keys)
                view_reset_acceleration(&vs);
                break;
            default:
                break;
        }
    }

    // 5) restore terminal
    disable_altscreen();
    disable_raw_mode();
    return 0;
} 