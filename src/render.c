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
      default:  return 100; // grey
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
        // print ID with fixed width of 16 characters
        size_t idlen = strcspn(s->id, "\n");
        int id_width = 16;
        
        // Print ID with exactly 16 characters, replacing tabs/whitespace with spaces
        int chars_printed = 0;
        for (int i = 0; i < (int)idlen && chars_printed < id_width; i++) {
            char c = s->id[i];
            // Replace tabs and other whitespace with spaces
            if (c == '\t' || c == '\r' || c == '\v' || c == '\f') {
                putchar(' ');
            } else {
                putchar(c);
            }
            chars_printed++;
        }
        // Pad with spaces to reach exactly 16 characters
        for (int pad = chars_printed; pad < id_width; pad++) {
            putchar(' ');
        }
        printf("| ");

        // show sequence using remaining available space
        int avail = vs->cols - id_width - 2;  // subtract ID width and "| " separator
        if (avail < 0) avail = 0;  // ensure non-negative
        if (avail > 0) {
            int chars_printed = 0;
            for (int i = vs->col_offset; i < (int)s->len && chars_printed < avail; i++) {
                int bg = bg_for(s->seq[i]);
                printf("\x1b[%dm%c\x1b[0m", bg, s->seq[i]);
                chars_printed++;
            }
        }
        putchar('\n');
    }

    // draw underscores on the second-to-last line
    for (int i = 0; i < vs->cols; i++) putchar('_');
    putchar('\n');

    // draw status on the last line
    if (vs->jump_mode) {
        printf("Jump to position: %s", vs->jump_buffer);
    } else {
        printf("(Q) Quit (H) Help (A) About (J) Jump  (← ↑ ↓ →) Navigate");
    }

    fflush(stdout);
}