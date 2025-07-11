#pragma once
#include "view.h"

// Basic scrolling functions
void view_scroll_up(ViewState *vs);
void view_scroll_down(ViewState *vs);
void view_scroll_left(ViewState *vs);
void view_scroll_right(ViewState *vs);

// Scrolling with step size
void view_scroll_up_steps(ViewState *vs, int steps);
void view_scroll_down_steps(ViewState *vs, int steps);
void view_scroll_left_steps(ViewState *vs, int steps);
void view_scroll_right_steps(ViewState *vs, int steps);

// Half-screen movement functions for WASD navigation
void view_scroll_half_screen_up(ViewState *vs);
void view_scroll_half_screen_down(ViewState *vs);
void view_scroll_half_screen_left(ViewState *vs);
void view_scroll_half_screen_right(ViewState *vs);

// Acceleration handling
void view_update_acceleration(ViewState *vs, int key);
void view_reset_acceleration(ViewState *vs);

// Coordinate transformation
void view_screen_to_sequence_pos(ViewState *vs, int screen_x, int screen_y, int *seq_row, int *seq_col); 