#ifndef UI_H
#define UI_H

#include "api.h"
#include <SDL2/SDL.h>

extern SDL_Window *window;
extern SDL_Renderer *renderer;

// Initialize the SDL2 UI
int ui_init(void);

// Clean up the UI
void ui_exit(void);

// Render the list of trending videos and highlight the selected one
void ui_render_list(VideoItem *items, int count, int selected_index);

// Render a simple loading/status message
void ui_render_loading(const char *message);

// Render the IP Configuration screen
void ui_render_ip_input(const char *current_ip, const char *msg);

// Download and allocate thumbnails into video items
void ui_download_thumbnails(VideoItem *items, int count);

// Free allocated thumbnails
void ui_free_thumbnails(VideoItem *items, int count);

#endif // UI_H
