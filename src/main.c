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
        printf("\nControls:\n");
        printf("  Arrow keys       Navigate\n");
        printf("  Q                Quit\n");
        printf("                       \n");
        printf("Made in Berlin by Krlnk\n");
        return 0;
    }
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file.fasta>\n", argv[0]);
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        return 1;
    }

    // 1) load sequences
    SeqList *seqs = parse_fasta(argv[1]);

    // 2) init terminal (alt-screen, raw mode, SIGWINCH)
    enable_raw_mode();
    enable_altscreen();

    // 3) init view state
    ViewState vs = view_init(seqs);

    // 4) main loop
    bool running = true;
    while (running) {
        render_frame(&vs);
        InputEvt ev = input_read();
        switch (ev.type) {
            case EVT_KEY:
                if (ev.key == 'q' || ev.key == 'Q') running = false;
                else if (ev.key == ARROW_UP)          view_scroll_up(&vs);
                else if (ev.key == ARROW_DOWN)        view_scroll_down(&vs);
                else if (ev.key == ARROW_LEFT)        view_scroll_left(&vs);
                else if (ev.key == ARROW_RIGHT)       view_scroll_right(&vs);
                // H/A do nothing for now
                break;
            case EVT_RESIZE:
                view_resize(&vs);
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
