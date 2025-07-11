#include <stdio.h> 
#include <stdbool.h> 
#include <string.h>
#include "term.h"
#include "input.h"
#include "parser_fasta.h"
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
        printf("Usage: %s [options] <file.fasta>\n", argv[0]);
        printf("Options:\n");
        printf("  -v, --version    Show version information\n");
        printf("  -h, --help       Show this help message\n");
        printf("  -n, --no-color   Disable ANSI color codes\n");
        printf("\nControls:\n");
        printf("  Arrow keys       Navigate (hold for acceleration)\n");
        printf("  WASD             Navigate (jump half-screen)\n");
        printf("  Q                Quit\n");
        printf("  J                Jump to position\n");
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
            fprintf(stderr, "Usage: %s [options] <file.fasta>\n", argv[0]);
            fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
            return 1;
        }
    }
    
    if (filename == NULL) {
        fprintf(stderr, "Error: No input file specified\n");
        fprintf(stderr, "Usage: %s [options] <file.fasta>\n", argv[0]);
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        return 1;
    }

    // 1) load sequences
    SeqList *seqs = parse_fasta(filename);
    if (!seqs) {
        fprintf(stderr, "Failed to parse FASTA file '%s'\n", filename);
        return 1;
    }

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
                } else {
                    // Normal mode - handle acceleration for arrow keys
                    if (ev.key == 'q' || ev.key == 'Q') {
                        running = false;
                    } else if (ev.key == 'j' || ev.key == 'J') {
                        view_start_jump(&vs);
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
                    } else if (ev.key == 'w') {
                        // W - up, half screen
                        view_scroll_half_screen_up(&vs);
                        view_reset_acceleration(&vs);
                    } else if (ev.key == 's') {
                        // S - down, half screen
                        view_scroll_half_screen_down(&vs);
                        view_reset_acceleration(&vs);
                    } else if (ev.key == 'a') {
                        // A - left, half screen
                        view_scroll_half_screen_left(&vs);
                        view_reset_acceleration(&vs);
                    } else if (ev.key == 'd') {
                        // D - right, half screen
                        view_scroll_half_screen_right(&vs);
                        view_reset_acceleration(&vs);
                    } else {
                        // Reset acceleration for non-arrow keys
                        view_reset_acceleration(&vs);
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
