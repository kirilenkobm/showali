#include <stdio.h> 
#include <stdbool.h> 
#include "term.h"
#include "input.h"
#include "parser_fasta.h"
#include "view.h"
#include "render.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file.fasta>\n", argv[0]);
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
