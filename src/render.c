#include <stdio.h>
#include <string.h>
#include "render.h"

// map bases -> ANSI background codes for DNA/RNA
static int bg_for_nucleotide(char c) {
    switch (c) {
      case 'A': return 42;  // green
      case 'T': return 41;  // red
      case 'U': return 45;  // magenta (for RNA)
      case 'C': return 44;  // blue
      case 'G': return 43;  // yellow
      case 'N': return 100; // grey
      case '-': return 47;  // white (gaps)
      default:  return 100; // grey
    }
}

// map amino acids -> ANSI background codes for proteins
// Based on standard biochemical properties and Clustal coloring scheme
static int bg_for_amino_acid(char c) {
    switch (c) {
      // Small nonpolar (Ala, Gly, Pro, Val) - light colors
      case 'A': return 47;   // white
      case 'G': return 102;  // bright green
      case 'P': return 103;  // bright yellow  
      case 'V': return 47;   // white
      
      // Hydrophobic (Ile, Leu, Met, Phe, Trp) - warm colors
      case 'I': return 43;   // yellow
      case 'L': return 43;   // yellow
      case 'M': return 43;   // yellow
      case 'F': return 44;   // blue
      case 'W': return 44;   // blue
      
      // Polar uncharged (Ser, Thr, Asn, Gln, Tyr, Cys) - greens
      case 'S': return 42;   // green
      case 'T': return 42;   // green
      case 'N': return 46;   // cyan
      case 'Q': return 46;   // cyan
      case 'Y': return 46;   // cyan
      case 'C': return 105;  // bright magenta
      
      // Positively charged (Lys, Arg, His) - red/magenta
      case 'K': return 41;   // red
      case 'R': return 41;   // red
      case 'H': return 45;   // magenta
      
      // Negatively charged (Asp, Glu) - blue
      case 'D': return 104;  // bright blue
      case 'E': return 104;  // bright blue
      
      // Gaps and unknown
      case '-': return 100;  // dark grey
      case 'X': return 100;  // dark grey
      case '*': return 101;  // bright red (stop codon)
      
      default:  return 100;  // dark grey
    }
}

// Helper function to get background color based on sequence type
static int bg_for_sequence(char c, SequenceType type) {
    switch (type) {
        case SEQ_DNA:
        case SEQ_RNA:
            return bg_for_nucleotide(c);
        case SEQ_PROTEIN:
            return bg_for_amino_acid(c);
        default:
            return 100; // grey for unknown
    }
}

void render_frame(ViewState *vs) {
    // re-hide cursor in case anything unhid it
    printf("\x1b[?25l");
    // move cursor home without clearing screen
    printf("\x1b[H");

    int content = vs->rows - 2;  // leave 2 lines for status
    for (int line = 0; line < content; line++) {
        int idx = vs->row_offset + line;
        if (idx >= (int)vs->seqs->count) {
            printf("\x1b[K\n");  // clear to end of line for empty rows
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
            int current_bg = -1;  // track current background color
            for (int i = vs->col_offset; i < (int)s->len && chars_printed < avail; i++) {
                bool is_selected = view_is_selected(vs, idx, i);
                
                if (!vs->no_color) {
                    int bg = bg_for_sequence(s->seq[i], s->type);
                    if (is_selected) {
                        // Use inverse video for selected characters
                        printf("\x1b[7m");  // inverse video
                        if (bg != current_bg) {
                            printf("\x1b[%dm", bg);
                            current_bg = bg;
                        }
                    } else {
                        if (current_bg == -1 || bg != current_bg) {
                            printf("\x1b[0m");  // reset first
                            printf("\x1b[%dm", bg);
                            current_bg = bg;
                        }
                    }
                }
                putchar(s->seq[i]);
                
                if (is_selected && !vs->no_color) {
                    printf("\x1b[0m");  // reset after selected character
                    current_bg = -1;
                }
                chars_printed++;
            }
            if (current_bg != -1 && !vs->no_color) {
                printf("\x1b[0m");  // reset color at end of sequence
            }
        }
        printf("\x1b[K\n");  // clear to end of line and newline
    }

    // draw underscores on the second-to-last line
    for (int i = 0; i < vs->cols; i++) putchar('_');
    printf("\x1b[K\n");  // clear to end of line

    // draw status on the last line
    printf("\x1b[K");  // clear entire line first
    if (vs->jump_mode) {
        printf("Jump to position: %s", vs->jump_buffer);
    } else if (vs->has_selection) {
        // find the maximum sequence length for position info
        int max_seq_len = 0;
        for (size_t i = 0; i < vs->seqs->count; i++) {
            if ((int)vs->seqs->items[i].len > max_seq_len) {
                max_seq_len = (int)vs->seqs->items[i].len;
            }
        }
        
        // Left side: selection status
        char left_info[100];
        if (vs->selecting) {
            snprintf(left_info, sizeof(left_info), "SELECTING - 'c'/right-click to copy, ESC to cancel");
        } else {
            snprintf(left_info, sizeof(left_info), "SELECTED - 'c'/right-click to copy, click outside/ESC to clear");
        }
        
        // Right side: position info (same as normal mode)
        char right_info[100];
        int first_visible_seq = vs->row_offset + 1;  // 1-based
        snprintf(right_info, sizeof(right_info), "Pos:%d/%d %d/%zu seqs", 
                 vs->col_offset + 1, max_seq_len, first_visible_seq, vs->seqs->count);
        
        // Calculate spacing for full-width right-alignment
        int left_len = strlen(left_info);
        int right_len = strlen(right_info);
        
        // For some reason, we need to subtract 1 from the spacing, otherwise it's misaligned
        // Found with natural stupidity, not artificial intelligence.
        int spacing = vs->cols - left_len - right_len - 1;
        if (spacing < 0) spacing = 0;  // Don't allow negative spacing
        
        // Print status line with both selection status and position
        printf("%s%*s%s", left_info, spacing, "", right_info);
    } else {
        // find the maximum sequence length
        int max_seq_len = 0;
        for (size_t i = 0; i < vs->seqs->count; i++) {
            if ((int)vs->seqs->items[i].len > max_seq_len) {
                max_seq_len = (int)vs->seqs->items[i].len;
            }
        }
        
        // Left side: navigation info
        char left_info[] = "(Q) Quit (J) Jump (Mouse) Select (←↑↓→/WASD) Navigate";
        
        // Right side: position info with first visible sequence
        char right_info[100];
        int first_visible_seq = vs->row_offset + 1;  // 1-based
        snprintf(right_info, sizeof(right_info), "Pos:%d/%d %d/%zu seqs", 
                 vs->col_offset + 1, max_seq_len, first_visible_seq, vs->seqs->count);
        
        // Calculate spacing for full-width right-alignment
        int left_len = strlen(left_info);
        int right_len = strlen(right_info);
        
        // Adjust for Unicode arrows: each arrow is 3 bytes but 1 visual char
        // 4 arrows: 4×3=12 bytes, 4×1=4 visual chars, difference = 8
        int left_visual_len = left_len - 7;  // Correct for Unicode arrows
        
        int spacing = vs->cols - left_visual_len - right_len;
        if (spacing < 0) spacing = 0;  // Don't allow negative spacing
        
        // Print status line with full-width spacing
        printf("%s%*s%s", left_info, spacing, "", right_info);
    }

    fflush(stdout);
}