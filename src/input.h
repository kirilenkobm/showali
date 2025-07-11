#pragma once
#include <stdbool.h>

typedef enum { EVT_NONE, EVT_KEY, EVT_RESIZE, EVT_TIMEOUT, EVT_MOUSE } EvtType;

typedef enum {
  ARROW_UP    = 1000,
  ARROW_DOWN  = 1001,
  ARROW_LEFT  = 1002,
  ARROW_RIGHT = 1003,
  ENTER       = 1004
} Key;

typedef struct { 
    EvtType type; 
    int key;
    // Mouse event data
    int mouse_x;
    int mouse_y;
    int mouse_button;  // 0=left, 1=middle, 2=right
    bool mouse_pressed;
    bool mouse_released;
    bool mouse_drag;
} InputEvt;

InputEvt input_read(void);
InputEvt input_read_timeout(int timeout_ms);
