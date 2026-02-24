#include "ui.h"
#include "network.h"
#include <switch.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
static TTF_Font *global_font = NULL;
static TTF_Font *small_font = NULL;
static TTF_Font *large_font = NULL;

int ui_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0) return -1;
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
    
    window = SDL_CreateWindow("DarkTube", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
    if (!window) return -1;
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return -1;
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_WEBP);
    if (TTF_Init() == -1) return -1;
    
    if (R_SUCCEEDED(plInitialize(PlServiceType_User))) {
        PlFontData font_data;
        if (R_SUCCEEDED(plGetSharedFontByType(&font_data, PlSharedFontType_Standard))) {
            global_font = TTF_OpenFontRW(SDL_RWFromConstMem(font_data.address, font_data.size), 1, 24);
            small_font = TTF_OpenFontRW(SDL_RWFromConstMem(font_data.address, font_data.size), 1, 18);
            large_font = TTF_OpenFontRW(SDL_RWFromConstMem(font_data.address, font_data.size), 1, 36);
        }
    }
    return 0;
}

void ui_exit(void) {
    if (global_font) TTF_CloseFont(global_font);
    if (small_font) TTF_CloseFont(small_font);
    if (large_font) TTF_CloseFont(large_font);
    plExit();
    TTF_Quit();
    IMG_Quit();
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

static void render_text(TTF_Font *font, const char *text, int x, int y, SDL_Color color, int align_center) {
    if (!font || !text || strlen(text) == 0) return;
    char safe_text[256];
    strncpy(safe_text, text, sizeof(safe_text) - 1);
    safe_text[sizeof(safe_text) - 1] = '\0';

    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, safe_text, color);
    if (!surf) return;
    
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (tex) {
        SDL_Rect rect = {x, y, surf->w, surf->h};
        if (align_center) {
            rect.x = x - (surf->w / 2);
            rect.y = y - (surf->h / 2);
        }
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

void ui_render_list(VideoItem *items, int count, int selected_index) {
    SDL_SetRenderDrawColor(renderer, 24, 24, 28, 255); // Dark gray bg
    SDL_RenderClear(renderer);
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {170, 170, 170, 255};
    SDL_Color red = {255, 50, 50, 255};
    SDL_Color selection_color = {0, 200, 255, 255}; // Nintendo Blue
    
    render_text(large_font, "DarkTube", 50, 40, red, 0);
    render_text(global_font, "Trending Videos", 210, 50, white, 0);
    
    if (count == 0) {
        render_text(global_font, "No videos loaded.", 640, 360, white, 1);
        SDL_RenderPresent(renderer);
        return;
    }
    
    int cols = 3;
    int margin_x = 90;
    int margin_y = 120;
    int thumb_w = 320;
    int thumb_h = 180;
    int spacing_x = 30 + thumb_w;
    int spacing_y = 80 + thumb_h;
    
    // Pagination / Scrolling
    int rows_visible = 2;
    int items_per_page = cols * rows_visible;
    int current_page = selected_index / items_per_page;
    int start_i = current_page * items_per_page;
    
    for (int i = start_i; i < count && i < start_i + items_per_page; i++) {
        int idx_on_page = i - start_i;
        int col = idx_on_page % cols;
        int row = idx_on_page / cols;
        
        int x = margin_x + (col * spacing_x);
        int y = margin_y + (row * spacing_y);
        
        // Selection outline
        if (i == selected_index) {
            SDL_SetRenderDrawColor(renderer, selection_color.r, selection_color.g, selection_color.b, 255);
            SDL_Rect outline = {x - 6, y - 6, thumb_w + 12, thumb_h + 12};
            // Draw thick border
            for (int r = 0; r < 4; r++) {
                SDL_Rect r_outline = {outline.x + r, outline.y + r, outline.w - r*2, outline.h - r*2};
                SDL_RenderDrawRect(renderer, &r_outline);
            }
        }
        
        // Draw Thumbnail
        SDL_Rect thumb_rect = {x, y, thumb_w, thumb_h};
        if (items[i].thumb_tex) {
            SDL_RenderCopy(renderer, (SDL_Texture*)items[i].thumb_tex, NULL, &thumb_rect);
        } else {
            SDL_SetRenderDrawColor(renderer, 40, 40, 45, 255);
            SDL_RenderFillRect(renderer, &thumb_rect);
        }
        
        // Draw Duration Bubble if available
        if (strlen(items[i].durationText) > 0) {
            SDL_Surface *d_surf = TTF_RenderUTF8_Blended(small_font, items[i].durationText, white);
            if (d_surf) {
                SDL_Texture *d_tex = SDL_CreateTextureFromSurface(renderer, d_surf);
                SDL_Rect bg_rect = {x + thumb_w - d_surf->w - 12, y + thumb_h - d_surf->h - 8, d_surf->w + 8, d_surf->h + 4};
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
                SDL_RenderFillRect(renderer, &bg_rect);
                SDL_Rect d_rect = {bg_rect.x + 4, bg_rect.y + 2, d_surf->w, d_surf->h};
                SDL_RenderCopy(renderer, d_tex, NULL, &d_rect);
                SDL_DestroyTexture(d_tex);
                SDL_FreeSurface(d_surf);
            }
        }
        
        // Draw Text (Title & Author)
        // Truncate long titles for UI
        char title_buf[64];
        strncpy(title_buf, items[i].title, sizeof(title_buf) - 1);
        title_buf[sizeof(title_buf)-1]='\0';
        if (strlen(title_buf) > 35) {
            strcpy(&title_buf[32], "...");
        }
        render_text(small_font, title_buf, x, y + thumb_h + 10, white, 0);
        render_text(small_font, items[i].author, x, y + thumb_h + 35, gray, 0);
    }
    
    // Page indicator
    char page_str[32];
    snprintf(page_str, sizeof(page_str), "Page %d / %d", current_page + 1, (count + items_per_page - 1) / items_per_page);
    render_text(small_font, page_str, 1280 - 150, 40, gray, 0);
    
    // Bottom controls
    SDL_Rect footer = {0, 720 - 40, 1280, 40};
    SDL_SetRenderDrawColor(renderer, 15, 15, 18, 255);
    SDL_RenderFillRect(renderer, &footer);
    render_text(small_font, "(A) Play   (B) Back   (+) Exit", 640, 720 - 20, gray, 1);
    
    SDL_RenderPresent(renderer);
}

void ui_render_loading(const char *message) {
    if (!renderer) return;
    SDL_SetRenderDrawColor(renderer, 24, 24, 28, 255);
    SDL_RenderClear(renderer);
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color red = {255, 50, 50, 255};
    render_text(large_font, "DarkTube", 640, 300, red, 1);
    render_text(global_font, message, 640, 360, white, 1);
    SDL_RenderPresent(renderer);
}

// IP Input Screen (Nintendo Profile Switch Style)
void ui_render_ip_input(const char *current_ip, const char *msg) {
    if (!renderer) return;
    SDL_SetRenderDrawColor(renderer, 235, 235, 235, 255); // Light theme for input dialog like OS
    SDL_RenderClear(renderer);
    
    SDL_Color dark_gray = {50, 50, 50, 255};
    SDL_Color light_gray = {100, 100, 100, 255};
    SDL_Color blue = {0, 150, 255, 255};
    
    // Draw a big rounded box representation
    SDL_Rect box = { 640 - 300, 360 - 200, 600, 400 };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(renderer, &box);
    
    render_text(global_font, "Configure API Server", 640, 360 - 140, dark_gray, 1);
    
    SDL_Rect ip_box = { 640 - 200, 360 - 50, 400, 60 };
    SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
    SDL_RenderFillRect(renderer, &ip_box);
    SDL_SetRenderDrawColor(renderer, blue.r, blue.g, blue.b, 255); // Highlight
    for(int r=0; r<3; r++) {
        SDL_Rect ip_box_r = {ip_box.x+r, ip_box.y+r, ip_box.w-r*2, ip_box.h-r*2};
        SDL_RenderDrawRect(renderer, &ip_box_r);
    }
    
    render_text(global_font, current_ip, 640, 360 - 20, dark_gray, 1);
    
    render_text(small_font, msg, 640, 360 + 60, dark_gray, 1);
    
    // Bottom Buttons
    render_text(small_font, "(A) Edit IP Address   (B) Connect", 640, 360 + 150, light_gray, 1);
    
    SDL_RenderPresent(renderer);
}

void ui_download_thumbnails(VideoItem *items, int count) {
    for (int i = 0; i < count; i++) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Loading thumbnails... (%d/%d)", i + 1, count);
        ui_render_loading(msg);
        
        items[i].thumb_tex = NULL;
        if (strlen(items[i].thumbUrl) > 5) {
            char *img_data = NULL;
            size_t img_size = 0;
            if (http_get(items[i].thumbUrl, &img_data, &img_size) == 0 && img_data && img_size > 0) {
                SDL_RWops *rw = SDL_RWFromMem(img_data, img_size);
                if (rw) {
                    SDL_Surface *surf = IMG_Load_RW(rw, 1);
                    if (surf) {
                        items[i].thumb_tex = (void *)SDL_CreateTextureFromSurface(renderer, surf);
                        SDL_FreeSurface(surf);
                    }
                }
                free(img_data);
            }
        }
    }
}

void ui_free_thumbnails(VideoItem *items, int count) {
    for (int i = 0; i < count; i++) {
        if (items[i].thumb_tex) {
            SDL_DestroyTexture((SDL_Texture *)items[i].thumb_tex);
            items[i].thumb_tex = NULL;
        }
    }
}
