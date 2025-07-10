#pragma once

typedef enum { EVT_NONE, EVT_KEY, EVT_RESIZE } EvtType;

typedef enum {
  ARROW_UP    = 1000,
  ARROW_DOWN  = 1001,
  ARROW_LEFT  = 1002,
  ARROW_RIGHT = 1003,
  ENTER       = 1004
} Key;

typedef struct { EvtType type; int key; } InputEvt;

InputEvt input_read(void);
