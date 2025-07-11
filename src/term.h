#pragma once
#include <signal.h>
#include <time.h>

void enable_raw_mode(void);
void disable_raw_mode(void);
void enable_altscreen(void);
void disable_altscreen(void);
void get_term_size(int *rows, int *cols);
int  was_resized(void);

// Timing utilities for acceleration
void get_current_time(struct timespec *ts);
long time_diff_ms(struct timespec *start, struct timespec *end);
