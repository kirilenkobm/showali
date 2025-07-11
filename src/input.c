#include <unistd.h>
#include <sys/select.h>
#include "input.h"
#include "term.h"

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
            case '\r': case '\n': return (InputEvt){.type=EVT_KEY, .key=ENTER};
            case '\x1b': {
                char seq[2];
                if (read(STDIN_FILENO, &seq[0], 1)==1 &&
                    read(STDIN_FILENO, &seq[1], 1)==1) {
                    if (seq[0]=='[') {
                        switch(seq[1]) {
                            case 'A': return (InputEvt){.type=EVT_KEY,.key=ARROW_UP};
                            case 'B': return (InputEvt){.type=EVT_KEY,.key=ARROW_DOWN};
                            case 'C': return (InputEvt){.type=EVT_KEY,.key=ARROW_RIGHT};
                            case 'D': return (InputEvt){.type=EVT_KEY,.key=ARROW_LEFT};
                        }
                    }
                }
            }
            default: return (InputEvt){.type=EVT_KEY, .key=c};
        }
    }
    return (InputEvt){.type=EVT_NONE};
}