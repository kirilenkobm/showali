#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ioctl.h>
#include "term.h"

static struct termios orig;

void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig);
    struct termios raw = orig;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

void enable_altscreen(void)  { write(STDOUT_FILENO, "\x1b[?1049h", 8); }
void disable_altscreen(void) { write(STDOUT_FILENO, "\x1b[?1049l", 8); }

void get_term_size(int *rows, int *cols) {
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *rows = ws.ws_row; *cols = ws.ws_col;
}

void handle_sigwinch(int _) {
    // just raise a flag or call a callback in your view code
}