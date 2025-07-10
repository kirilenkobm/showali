#include "term.h"
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <signal.h>    // for SIGWINCH & signal()

static struct termios orig;
static volatile sig_atomic_t resized = 0;

// catch SIGWINCH
static void handle_sigwinch(int signo) {
    (void)signo;
    resized = 1;
}

// called by input_read to see if a SIGWINCH arrived
int was_resized(void) {
    if (resized) { resized = 0; return 1; }
    return 0;
}

void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig);
    struct termios raw = orig;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // catch window resize
    signal(SIGWINCH, handle_sigwinch);
}

void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

void enable_altscreen(void)  {
    write(STDOUT_FILENO, "\x1b[?1049h\x1b[?25l", 12);
}
void disable_altscreen(void) {
    write(STDOUT_FILENO, "\x1b[?25h\x1b[?1049l", 12);
}

void get_term_size(int *rows, int *cols) {
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *rows = ws.ws_row; 
    *cols = ws.ws_col;
    
    // Enforce minimum terminal width of 24
    if (*cols < 24) *cols = 24;
}