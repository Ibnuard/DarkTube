#include "player.h"
#include "ui.h"
#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpv/client.h>
#include <SDL2/SDL.h>

extern PadState pad;
static mpv_handle *mpv = NULL;

#define LOG(fmt, ...) printf("[Player] " fmt "\n", ##__VA_ARGS__)

#define CHECK_MPV(expr) do { \
    int _err = (expr); \
    if (_err < 0) { \
        LOG("MPV Error at %s:%d: %s", __FILE__, __LINE__, mpv_error_string(_err)); \
        goto cleanup; \
    } \
} while(0)

int player_init(void) {
    return 0;
}

void player_play_stream(const char *stream_url) {
    LOG("Starting playback: %s", stream_url);

    // ── STEP 1: FULL SDL teardown ──
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    SDL_Quit();

    // Let the system fully release GPU resources
    svcSleepThread(100000000); // 100ms

    // ── STEP 2: Create mpv with Tegra-optimized config ──
    mpv = mpv_create();
    if (!mpv) {
        LOG("Failed to create mpv instance");
        goto restore_ui;
    }

    // All options in a clean array
    const char *options[] = {
        "vo",                       "gpu",
        "gpu-api",                  "opengl",
        "opengl-es",                "yes",
        "hwdec",                    "auto",
        "hwdec-codecs",             "all",
        "ytdl",                     "no",
        "terminal",                 "no",
        "msg-level",                "all=no",
        "input-default-bindings",   "no",
        "input-vo-keyboard",        "no",
        "keep-open",                "no",
        "cache",                    "yes",
        "cache-secs",               "120",
        "demuxer-max-bytes",        "50M",
        "demuxer-readahead-secs",   "10",
        "profile",                  "fast",
        NULL
    };

    for (int i = 0; options[i]; i += 2) {
        mpv_set_option_string(mpv, options[i], options[i + 1]);
    }

    CHECK_MPV(mpv_initialize(mpv));

    // ── STEP 3: Load and play ──
    {
        const char *cmd[] = {"loadfile", stream_url, NULL};
        CHECK_MPV(mpv_command(mpv, cmd));
    }

    LOG("Playback started, entering event loop");

    // ── STEP 4: Playback loop with complete event handling ──
    {
        bool playing = true;
        bool paused = false;

        while (appletMainLoop() && playing) {
            // Drain mpv events with 10ms timeout (no busy-wait)
            while (1) {
                mpv_event *event = mpv_wait_event(mpv, 0.01);
                if (event->event_id == MPV_EVENT_NONE) break;

                switch (event->event_id) {
                    case MPV_EVENT_END_FILE:
                        LOG("Playback ended (END_FILE)");
                        playing = false;
                        break;
                    case MPV_EVENT_SHUTDOWN:
                        LOG("MPV shutdown event");
                        playing = false;
                        break;
                    default:
                        break;
                }
            }

            // Controller input
            padUpdate(&pad);
            u64 kDown = padGetButtonsDown(&pad);

            if (kDown & HidNpadButton_B) {
                LOG("User pressed B - stopping");
                mpv_command_string(mpv, "stop");
                playing = false;
            }
            if (kDown & HidNpadButton_A) {
                paused = !paused;
                int err = mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &paused);
                if (err < 0) LOG("Warning: Failed to set pause: %s", mpv_error_string(err));
            }
            if (kDown & HidNpadButton_Right) {
                mpv_command_string(mpv, "seek 10");
            }
            if (kDown & HidNpadButton_Left) {
                mpv_command_string(mpv, "seek -10");
            }

            svcSleepThread(16000000); // ~16ms
        }
    }

cleanup:
    // ── STEP 5: Always destroy mpv before restoring UI ──
    if (mpv) {
        LOG("Destroying mpv");
        mpv_terminate_destroy(mpv);
        mpv = NULL;
    }

    // Let GPU resources release
    svcSleepThread(200000000); // 200ms

restore_ui:
    // ── STEP 6: FULL SDL reinit ──
    LOG("Restoring SDL2 UI");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0) {
        LOG("Fatal: SDL_Init failed: %s", SDL_GetError());
        return;
    }
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");

    window = SDL_CreateWindow("DarkTube",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
    if (!window) {
        LOG("Fatal: SDL_CreateWindow failed: %s", SDL_GetError());
        return;
    }

    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        LOG("Fatal: SDL_CreateRenderer failed: %s", SDL_GetError());
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    LOG("UI restored successfully");
}

void player_exit(void) {
    if (mpv) {
        mpv_terminate_destroy(mpv);
        mpv = NULL;
    }
}
