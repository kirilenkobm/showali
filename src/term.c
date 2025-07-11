#include "term.h"
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <signal.h>    // for SIGWINCH & signal()
#include <time.h>      // for clock_gettime

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
    printf("\x1b[?1049h\x1b[?25l");
    // Enable mouse reporting
    printf("\x1b[?1000h");  // Enable mouse button press/release reporting
    printf("\x1b[?1002h");  // Enable mouse drag reporting
    printf("\x1b[?1015h");  // Enable urxvt mouse mode
    printf("\x1b[?1006h");  // Enable SGR mouse mode
    fflush(stdout);
}

void disable_altscreen(void) {
    // Disable mouse reporting
    printf("\x1b[?1006l");  // Disable SGR mouse mode
    printf("\x1b[?1015l");  // Disable urxvt mouse mode
    printf("\x1b[?1002l");  // Disable mouse drag reporting
    printf("\x1b[?1000l");  // Disable mouse button press/release reporting
    printf("\x1b[?25h\x1b[?1049l\x1b[2J\x1b[H");
    fflush(stdout);  // ensure cleanup commands are sent
}

void get_term_size(int *rows, int *cols) {
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *rows = ws.ws_row; 
    *cols = ws.ws_col;
    
    // Enforce minimum terminal width of 24
    if (*cols < 24) *cols = 24;
}

// Timing utilities for acceleration
void get_current_time(struct timespec *ts) {
    clock_gettime(CLOCK_MONOTONIC, ts);
}

long time_diff_ms(struct timespec *start, struct timespec *end) {
    long seconds = end->tv_sec - start->tv_sec;
    long nanoseconds = end->tv_nsec - start->tv_nsec;
    return seconds * 1000 + nanoseconds / 1000000;
}