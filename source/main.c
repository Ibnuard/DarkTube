#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network.h"
#include "api.h"
#include "ui.h"
#include "player.h"

// Set heap size to 64MB for large JSON parsing
u32 __nx_applet_type = AppletType_Default;
size_t __nx_heap_size = 64 * 1024 * 1024;

// Define pad globally so player.c can use it
PadState pad;

#define MAX_VIDEOS 20

int main(int argc, char **argv) {
    ui_init();
    
    ui_render_loading("Initializing network and APIs...");
    
    if (init_network() != 0) {
        ui_render_loading("Failed to initialize network.\nPress + to exit.");
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&pad);
        while (appletMainLoop()) {
            padUpdate(&pad);
            u64 kDown = padGetButtonsDown(&pad);
            if (kDown & HidNpadButton_Plus) break;
            consoleUpdate(NULL);
        }
        ui_exit();
        return 0;
    }
    
    player_init();
    
    // Configure input
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);
    
    // API IP Input Flow
    char api_url[256] = "http://192.168.1.10:3000";
    bool api_ready = false;
    
    while (appletMainLoop() && !api_ready) {
        ui_render_loading("Press A to enter API Server IP\nPress + to exit.");
        
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
        
        if (kDown & HidNpadButton_Plus) {
            player_exit();
            cleanup_network();
            ui_exit();
            return 0;
        }
        
        if (kDown & HidNpadButton_A) {
            SwkbdConfig swkbd;
            Result rc = swkbdCreate(&swkbd, 0);
            if (R_SUCCEEDED(rc)) {
                swkbdConfigSetHeaderText(&swkbd, "Enter API Server URL (e.g. http://ip:3000)");
                swkbdConfigSetInitialText(&swkbd, api_url);
                rc = swkbdShow(&swkbd, api_url, sizeof(api_url));
                swkbdClose(&swkbd);
                
                if (R_SUCCEEDED(rc)) {
                    api_set_base_url(api_url);
                    ui_render_loading("Testing connection...");
                    if (api_test_connection() == 0) {
                        api_ready = true;
                    } else {
                        ui_render_loading("Connection failed! Press A to try again.");
                        while (appletMainLoop()) {
                            padUpdate(&pad);
                            if (padGetButtonsDown(&pad) & HidNpadButton_A) break;
                            consoleUpdate(NULL);
                        }
                    }
                }
            } else {
                ui_render_loading("Failed to create keyboard. Using default.");
                api_set_base_url(api_url);
                api_ready = true;
            }
        }
        
        consoleUpdate(NULL);
    }
    
    // Allocate VideoItem array on the heap instead of the stack
    VideoItem *items = malloc(sizeof(VideoItem) * MAX_VIDEOS);
    if (!items) {
        ui_render_loading("Failed to allocate memory for video list.\nPress + to exit.");
        while (appletMainLoop()) {
            padUpdate(&pad);
            u64 kDown = padGetButtonsDown(&pad);
            if (kDown & HidNpadButton_Plus) break;
            consoleUpdate(NULL);
        }
        player_exit();
        cleanup_network();
        ui_exit();
        return 0;
    }

    int video_count = 0;
    int selected_index = 0;
    
    ui_render_loading("Fetching trending videos from API Server...");
    video_count = fetch_trending(items, MAX_VIDEOS);
    
    if (video_count <= 0) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Failed to fetch videos. Error: %d\nPress + to exit.", video_count);
        ui_render_loading(err_msg);
        while (appletMainLoop()) {
            padUpdate(&pad);
            u64 kDown = padGetButtonsDown(&pad);
            if (kDown & HidNpadButton_Plus) break;
            consoleUpdate(NULL);
        }
    } else {
        // Main UI Loop
        while (appletMainLoop()) {
            // Scan pad
            padUpdate(&pad);
            u64 kDown = padGetButtonsDown(&pad);
            
            if (kDown & HidNpadButton_Plus) {
                break; // Exit app
            }
            
            if (kDown & HidNpadButton_Down) {
                selected_index++;
                if (selected_index >= video_count) selected_index = 0;
            }
            
            if (kDown & HidNpadButton_Up) {
                selected_index--;
                if (selected_index < 0) selected_index = video_count - 1;
            }
            
            if (kDown & HidNpadButton_A) {
                // Play video selection
                char stream_url[1024] = {0};
                
                ui_render_loading("Resolving stream URL...");
                
                if (fetch_video_stream_url(items[selected_index].videoId, stream_url, sizeof(stream_url)) == 0) {
                    player_play_stream(stream_url);
                } else {
                    ui_render_loading("Failed to resolve stream.\nPress B to return.");
                    while (appletMainLoop()) {
                        padUpdate(&pad);
                        u64 innerDown = padGetButtonsDown(&pad);
                        if (innerDown & HidNpadButton_B) break;
                        consoleUpdate(NULL);
                    }
                }
            }
            
            ui_render_list(items, video_count, selected_index);
        }
    }
    
    // Cleanup
    free(items);
    player_exit();
    cleanup_network();
    ui_exit();
    
    return 0;
}
