#include <unistd.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "input.h"
#include "term.h"

// Parse mouse escape sequence
static InputEvt parse_mouse_event(void) {
    InputEvt evt = {.type = EVT_NONE};
    char buffer[32];
    int i = 0;
    char c;
    
    // Read the mouse sequence: \x1b[<button;x;yM/m
    while (i < (int)sizeof(buffer) - 1 && read(STDIN_FILENO, &c, 1) == 1) {
        buffer[i++] = c;
        if (c == 'M' || c == 'm') {
            buffer[i] = '\0';
            break;
        }
    }
    
    if (i == 0) {
        return evt;
    }
    
    // Parse the sequence
    int button, x, y;
    char end_char = buffer[i-1];
    
    if (sscanf(buffer, "%d;%d;%d", &button, &x, &y) == 3) {
        evt.type = EVT_MOUSE;
        evt.mouse_x = x - 1;  // Convert to 0-based
        evt.mouse_y = y - 1;  // Convert to 0-based
        evt.mouse_button = button & 3;  // Extract button number
        evt.mouse_pressed = (end_char == 'M');
        evt.mouse_released = (end_char == 'm');
        evt.mouse_drag = ((button & 32) != 0);  // Drag bit is set
    }
    
    return evt;
}

InputEvt input_read(void) {
    return input_read_timeout(-1);  // blocking read (no timeout)
}

InputEvt input_read_timeout(int timeout_ms) {
    if (was_resized()) {
        return (InputEvt){ .type = EVT_RESIZE };
    }
    
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    
    struct timeval timeout;
    struct timeval *timeout_ptr = NULL;
    
    if (timeout_ms >= 0) {
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        timeout_ptr = &timeout;
    }
    
    int n = select(STDIN_FILENO+1, &rfds, NULL, NULL, timeout_ptr);
    
    if (n == 0) {
        // Timeout occurred
        return (InputEvt){ .type = EVT_TIMEOUT };
    }

    if (n > 0 && FD_ISSET(STDIN_FILENO, &rfds)) {
        char c;
        read(STDIN_FILENO, &c, 1);
        switch (c) {
            case 'q': case 'Q': return (InputEvt){.type=EVT_KEY, .key=c};
            case 3: // Ctrl+C
                return (InputEvt){.type=EVT_KEY, .key=3};
            case 'c': case 'C': // Plain C key (we'll check for modifiers in main)
                return (InputEvt){.type=EVT_KEY, .key=c};
            case '\r': case '\n': return (InputEvt){.type=EVT_KEY, .key=ENTER};
            case '\x1b': {
                // Set a small timeout to distinguish between ESC key and escape sequences
                fd_set esc_fds;
                FD_ZERO(&esc_fds);
                FD_SET(STDIN_FILENO, &esc_fds);
                
                struct timeval esc_timeout;
                esc_timeout.tv_sec = 0;
                esc_timeout.tv_usec = 100000; // 100ms timeout
                
                int esc_ready = select(STDIN_FILENO+1, &esc_fds, NULL, NULL, &esc_timeout);
                
                if (esc_ready == 0) {
                    // Timeout - treat as plain ESC key
                    return (InputEvt){.type=EVT_KEY, .key=27};
                } else if (esc_ready > 0 && FD_ISSET(STDIN_FILENO, &esc_fds)) {
                    // More data available - likely escape sequence
                    char seq[3];
                    if (read(STDIN_FILENO, &seq[0], 1) == 1) {
                        if (seq[0] == '[') {
                            if (read(STDIN_FILENO, &seq[1], 1) == 1) {
                                if (seq[1] == '<') {
                                    // Mouse event
                                    return parse_mouse_event();
                                }
                                // Arrow keys
                                switch(seq[1]) {
                                    case 'A': return (InputEvt){.type=EVT_KEY,.key=ARROW_UP};
                                    case 'B': return (InputEvt){.type=EVT_KEY,.key=ARROW_DOWN};
                                    case 'C': return (InputEvt){.type=EVT_KEY,.key=ARROW_RIGHT};
                                    case 'D': return (InputEvt){.type=EVT_KEY,.key=ARROW_LEFT};
                                }
                            }
                            // Check for other sequences that might be Cmd+C
                            if (seq[1] >= '0' && seq[1] <= '9') {
                                // Could be a numeric escape sequence, read more
                                char num_seq[16];
                                num_seq[0] = seq[1];
                                int pos = 1;
                                char next_c;
                                
                                // Read until we get a letter (like ~, m, etc.)
                                while (pos < 15 && read(STDIN_FILENO, &next_c, 1) == 1) {
                                    num_seq[pos++] = next_c;
                                    if ((next_c >= 'A' && next_c <= 'Z') || 
                                        (next_c >= 'a' && next_c <= 'z') || 
                                        next_c == '~') {
                                        num_seq[pos] = '\0';
                                        break;
                                    }
                                }
                                
                                // Check for Cmd+C sequences (these are just examples, might vary)
                                if (strcmp(num_seq, "200~") == 0 || strcmp(num_seq, "201~") == 0) {
                                    // Bracketed paste mode - could be Cmd+C related
                                    return (InputEvt){.type=EVT_KEY, .key=3}; // Treat as copy
                                }
                            }
                        }
                    }
                }
                // Fallback to ESC key
                return (InputEvt){.type=EVT_KEY, .key=27};
            }
            default: return (InputEvt){.type=EVT_KEY, .key=c};
        }
    }
    return (InputEvt){.type=EVT_NONE};
}