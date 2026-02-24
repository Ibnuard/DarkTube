#include "ui.h"
#include <switch.h>
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
static TTF_Font *global_font = NULL;

int ui_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("SDL_Init fail\n");
        return -1;
    }
    
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
    
    window = SDL_CreateWindow("DarkTube", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        1280, 720, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
    if (!window) return -1;
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return -1;
    
    if (TTF_Init() == -1) return -1;
    
    if (R_SUCCEEDED(plInitialize(PlServiceType_User))) {
        PlFontData font_data;
        if (R_SUCCEEDED(plGetSharedFontByType(&font_data, PlSharedFontType_Standard))) {
            global_font = TTF_OpenFontRW(SDL_RWFromConstMem(font_data.address, font_data.size), 1, 24);
        }
    }
    return 0;
}

void ui_exit(void) {
    if (global_font) TTF_CloseFont(global_font);
    plExit();
    TTF_Quit();
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

static void render_text(const char *text, int x, int y, SDL_Color color) {
    if (!global_font || !text || strlen(text) == 0) return;
    
    // We truncate text internally so it doesn't crash TTF due to excessively long strings
    char safe_text[256];
    strncpy(safe_text, text, sizeof(safe_text) - 1);
    safe_text[sizeof(safe_text) - 1] = '\0';

    SDL_Surface *surf = TTF_RenderUTF8_Blended(global_font, safe_text, color);
    if (!surf) return;
    
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (tex) {
        SDL_Rect rect = {x, y, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

void ui_render_list(VideoItem *items, int count, int selected_index) {
    SDL_SetRenderDrawColor(renderer, 20, 20, 24, 255);
    SDL_RenderClear(renderer);
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color red = {255, 50, 50, 255};
    SDL_Color gray = {150, 150, 150, 255};
    
    char header[64];
    snprintf(header, sizeof(header), "DarkTube Framework - Rendering (%d videos)", count);
    render_text(header, 50, 20, red);
    
    if (count == 0) {
        render_text("No videos loaded.", 50, 100, white);
        SDL_RenderPresent(renderer);
        return;
    }
    
    int start_y = 80;
    int line_h = 60;
    
    int visible_items = 9;
    int start_i = selected_index - (visible_items / 2);
    if (start_i < 0) start_i = 0;
    if (start_i + visible_items > count) start_i = count - visible_items;
    if (start_i < 0) start_i = 0;
    
    for (int i = start_i; i < count && i < start_i + visible_items; i++) {
        int y = start_y + ((i - start_i) * line_h);
        
        if (i == selected_index) {
            SDL_SetRenderDrawColor(renderer, 60, 60, 70, 255);
            SDL_Rect bg = {40, y - 5, 1200, 55};
            SDL_RenderFillRect(renderer, &bg);
        }
        
        char time_str[16];
        int h = items[i].lengthSeconds / 3600;
        int m = (items[i].lengthSeconds % 3600) / 60;
        int s = items[i].lengthSeconds % 60;
        if (h > 0) snprintf(time_str, sizeof(time_str), "%d:%02d:%02d", h, m, s);
        else snprintf(time_str, sizeof(time_str), "%02d:%02d", m, s);
        
        char line[512];
        snprintf(line, sizeof(line), "%d. %s [%s]", i + 1, items[i].title, time_str);
        
        render_text(line, 50, y, white);
        render_text(items[i].author, 50, y + 25, gray);
    }
    
    render_text("Controls: UP/DOWN: Navigate | A: Play | +: Exit", 50, 670, gray);
    
    SDL_RenderPresent(renderer);
}

void ui_render_loading(const char *message) {
    if (!renderer) return;
    SDL_SetRenderDrawColor(renderer, 20, 20, 24, 255);
    SDL_RenderClear(renderer);
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color red = {255, 50, 50, 255};
    render_text("DarkTube Initialize...", 50, 20, red);
    render_text(message, 50, 100, white);
    SDL_RenderPresent(renderer);
}
