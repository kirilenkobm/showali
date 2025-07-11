// All view functionality has been decomposed into focused modules:
// - view_state.c/h: Core ViewState management
// - view_navigation.c/h: Scrolling and navigation
// - view_search.c/h: Search functionality
// - view_selection.c/h: Mouse selection and clipboard
// - view_modes.c/h: Jump mode and other special modes

#include "view.h"

// This file serves as a compatibility layer for the decomposed view system.
// All actual functionality is now implemented in the focused modules above. 