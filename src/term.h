#pragma once
#include <signal.h>

void enable_raw_mode(void);
void disable_raw_mode(void);
void enable_altscreen(void);
void disable_altscreen(void);
void get_term_size(int *rows, int *cols);
int  was_resized(void);
