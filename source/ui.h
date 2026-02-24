#ifndef UI_H
#define UI_H

#include "api.h"

// Initialize the console UI
void ui_init(void);

// Clean up the UI
void ui_exit(void);

// Render the list of trending videos and highlight the selected one
void ui_render_list(VideoItem *items, int count, int selected_index);

// Render a simple loading/status message
void ui_render_loading(const char *message);

#endif // UI_H
