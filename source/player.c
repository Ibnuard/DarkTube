#include "player.h"
#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpv/client.h>

extern PadState pad;
static mpv_handle *mpv = NULL;

int player_init(void) {
    mpv = mpv_create();
    if (!mpv) {
        printf("Failed to create mpv handle\n");
        return -1;
    }

    // Set some basic options
    int val = 1;
    mpv_set_option(mpv, "osc", MPV_FORMAT_FLAG, &val); // Enable on-screen controller
    mpv_set_option_string(mpv, "vo", "gpu");
    mpv_set_option_string(mpv, "hwdec", "auto");
    mpv_set_option_string(mpv, "ytdl", "no"); // We resolve URLs on the server

    // Initialize mpv
    if (mpv_initialize(mpv) < 0) {
        printf("Failed to initialize mpv\n");
        mpv_terminate_destroy(mpv);
        mpv = NULL;
        return -1;
    }

    return 0;
}

void player_play_stream(const char *stream_url) {
    if (!mpv) {
        if (player_init() != 0) return;
    }

    const char *cmd[] = {"loadfile", stream_url, NULL};
    mpv_command(mpv, cmd);

    bool playing = true;
    bool paused = false;

    consoleClear();
    printf("\x1b[31;1m=== DarkTube Player ===\x1b[0m\n\n");
    printf("Playing stream...\n");
    printf("Controls:\n");
    printf("  A: Play/Pause\n");
    printf("  B: Stop and Return\n");
    consoleUpdate(NULL);

    while (appletMainLoop() && playing) {
        // Handle MPV events
        while (1) {
            mpv_event *event = mpv_wait_event(mpv, 0);
            if (event->event_id == MPV_EVENT_NONE) break;
            
            if (event->event_id == MPV_EVENT_END_FILE) {
                playing = false;
            }
        }

        // Handle Input
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_B) {
            mpv_command_string(mpv, "stop");
            playing = false;
        }

        if (kDown & HidNpadButton_A) {
            paused = !paused;
            mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &paused);
            printf(paused ? "Paused\n" : "Resumed\n");
            consoleUpdate(NULL);
        }

        // yield
        svcSleepThread(10000000); // 10ms
    }
}

void player_exit(void) {
    if (mpv) {
        mpv_terminate_destroy(mpv);
        mpv = NULL;
    }
}
