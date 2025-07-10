#include <stdio.h>
#include <string.h>
#include "render.h"

// simple DNA -> ANSI bg color mapper
static int bg_for(char c) {
    switch (c) {
      case 'A': return 42;  // green
      case 'T': return 41;  // red
      case 'C': return 44;  // blue
      case 'G': return 43;  // yellow
      default:  return 40;  // black
    }
}

void render_frame(ViewState *vs) {
    // full clear + home
    printf("\x1b[2J\x1b[H");

    int content = vs->rows - 2;
    for (int line = 0; line < content; line++) {
        size_t idx = vs->row_offset + line;
        if (idx < vs->seqs->count) {
            Sequence *s = &vs->seqs->items[idx];
            // strip newline from ID
            size_t idlen = strcspn(s->id, "\n");
            printf("%.*s| ", (int)idlen, s->id);

            // how many cols left?
            int avail = vs->cols - idlen - 3;
            for (int i = vs->col_offset; i < (int)s->len && i < vs->col_offset + avail; i++) {
                int bg = bg_for(s->seq[i]);
                printf("\x1b[%dm%c\x1b[0m", bg, s->seq[i]);
            }
        }
        putchar('\n');
    }

    // status bar
    for (int i = 0; i < vs->cols; i++) putchar('_');
    printf("\n(Q) Quit (H) Help (A) About  (← ↑ ↓ →) Navigate\n");
    fflush(stdout);
}