#include <stdio.h> 
#include <stdbool.h> 
#include <string.h>
#include "term.h"
#include "input.h"
#include "parser.h"
#include "view.h"
#include "render.h"
#include "version.h"

int main(int argc, char **argv) {
    // Handle version flag
    if (argc == 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
        printf("%s %s\n", PROGRAM_NAME, VERSION_STRING);
        printf("%s\n", PROGRAM_DESCRIPTION);
        printf("Built: %s\n", VERSION_BUILD);
        return 0;
    }
    
    // Handle help flag
    if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("Usage: %s [options] <alignment_file>\n", argv[0]);
        printf("Options:\n");
        printf("  -v, --version    Show version information\n");
        printf("  -h, --help       Show this help message\n");
        printf("  -n, --no-color   Disable ANSI color codes\n");
        printf("\nControls:\n");
        printf("  Arrow keys       Navigate (hold for acceleration)\n");
        printf("  WASD             Navigate (jump half-screen)\n");
        printf("  Q                Quit\n");
        printf("  J                Jump to position\n");
        printf("  F                Find (beta)\n");
        printf("  Mouse            Drag to select rectangular area\n");
        printf("  Right-click      Copy selection to clipboard\n");
        printf("  ESC              Clear selection\n");
        printf("                       \n");
        printf("Made in Berlin by Krlnk\n");
        return 0;
    }
    
    // Parse arguments
    bool no_color = false;
    char *filename = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-color") == 0 || strcmp(argv[i], "-n") == 0) {
            no_color = true;
        } else if (filename == NULL) {
            filename = argv[i];
        } else {
            fprintf(stderr, "Error: Too many arguments\n");
            fprintf(stderr, "Usage: %s [options] <alignment_file>\n", argv[0]);
            fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
            return 1;
        }
    }
    
    if (filename == NULL) {
        fprintf(stderr, "Error: No input file specified\n");
        fprintf(stderr, "Usage: %s [options] <alignment_file>\n", argv[0]);
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        return 1;
    }

    // 1) load sequences with auto-detection
    ParseResult *result = parse_alignment(filename);
    if (!result || !result->sequences) {
        if (result && result->error_message) {
            fprintf(stderr, "Failed to parse alignment file '%s': %s\n", filename, result->error_message);
        } else {
            fprintf(stderr, "Failed to parse alignment file '%s'\n", filename);
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
    vs.no_color = no_color;

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
                            vs.search_browsing_history = false;
                        }
                    } else if (ev.key == ARROW_UP) {
                        view_navigate_search_history(&vs, true);
                    } else if (ev.key == ARROW_DOWN) {
                        view_navigate_search_history(&vs, false);
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
                    
                    if (ev.mouse_pressed && ev.mouse_drag) {
                        if (!vs.selecting) {
                            // Start new selection
                            view_start_mouse_selection(&vs, seq_row, seq_col);
                        } else {
                            // Update existing selection
                            view_update_mouse_selection(&vs, seq_row, seq_col);
                        }
                    } else if (ev.mouse_pressed && !ev.mouse_drag) {
                        if (vs.has_selection) {
                            if (view_is_click_in_selection(&vs, seq_row, seq_col)) {
                                // Clicking within selection - could be start of new drag
                                // Don't do anything yet, wait for drag or release
                            } else {
                                // Clicking outside selection - clear it
                                view_clear_selection(&vs);
                            }
                        } else {
                            // No existing selection - start new one
                            view_start_mouse_selection(&vs, seq_row, seq_col);
                        }
                    } else if (ev.mouse_released) {
                        if (vs.selecting) {
                            // End selection
                            view_end_mouse_selection(&vs);
                        }
                        // Note: We don't clear selection on release anymore
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
