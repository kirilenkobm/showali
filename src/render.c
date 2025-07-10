#include <stdio.h>
#include <string.h>
#include "render.h"

// map bases -> ANSI background codes
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
    // re-hide cursor in case anything unhid it
    printf("\x1b[?25l");
    // clear screen + move cursor home
    printf("\x1b[2J\x1b[H");

    int content = vs->rows - 2;  // leave 2 lines for status
    for (int line = 0; line < content; line++) {
        int idx = vs->row_offset + line;
        if (idx >= (int)vs->seqs->count) {
            putchar('\n');
            continue;
        }

        Sequence *s = &vs->seqs->items[idx];
        // print ID (up to newline)
        size_t idlen = strcspn(s->id, "\n");
        printf("%.*s| ", (int)idlen, s->id);

        // how many bases fit after “ID| ”
        int avail = vs->cols - (int)idlen - 3;
        for (int i = vs->col_offset; i < (int)s->len && i < vs->col_offset + avail; i++) {
            int bg = bg_for(s->seq[i]);
            printf("\x1b[%dm%c\x1b[0m", bg, s->seq[i]);
        }
        putchar('\n');
    }

    // draw underscores on the second-to-last line
    for (int i = 0; i < vs->cols; i++) putchar('_');
    putchar('\n');

    // draw status on the last line
    printf("(Q) Quit (H) Help (A) About  (← ↑ ↓ →) Navigate");

    fflush(stdout);
}