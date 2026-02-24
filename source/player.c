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

// Error checking macro for mpv calls
#define CHECK_MPV(expr) do { \
    int _err = (expr); \
    if (_err < 0) { \
        goto cleanup; \
    } \
} while(0)

int player_init(void) {
    return 0;
}

void player_play_stream(const char *stream_url) {
    // ── STEP 1: FULL SDL teardown ──
    // On Switch, mpv and SDL2 CANNOT share the GPU at all.
    // We must fully release everything including the SDL subsystem.
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    SDL_Quit();  // FULL quit - release all GPU/display resources

    // Small delay to let the system fully release GPU resources
    svcSleepThread(100000000); // 100ms

    // ── STEP 2: Create mpv with Tegra-optimized config ──
    mpv = mpv_create();
    if (!mpv) goto restore_ui;

    // Switch-specific: OpenGL ES on Tegra
    mpv_set_option_string(mpv, "vo", "gpu");
    mpv_set_option_string(mpv, "gpu-api", "opengl");
    mpv_set_option_string(mpv, "opengl-es", "yes");
    mpv_set_option_string(mpv, "hwdec", "auto");
    mpv_set_option_string(mpv, "hwdec-codecs", "all");
    mpv_set_option_string(mpv, "ytdl", "no");
    mpv_set_option_string(mpv, "terminal", "no");
    mpv_set_option_string(mpv, "msg-level", "all=no");
    mpv_set_option_string(mpv, "input-default-bindings", "no");
    mpv_set_option_string(mpv, "input-vo-keyboard", "no");
    mpv_set_option_string(mpv, "keep-open", "no");
    mpv_set_option_string(mpv, "cache", "yes");
    mpv_set_option_string(mpv, "demuxer-max-bytes", "50M");

    if (mpv_initialize(mpv) < 0) {
        mpv_terminate_destroy(mpv);
        mpv = NULL;
        goto restore_ui;
    }

    // ── STEP 3: Load and play ──
    {
        const char *cmd[] = {"loadfile", stream_url, NULL};
        int err = mpv_command(mpv, cmd);
        if (err < 0) {
            mpv_terminate_destroy(mpv);
            mpv = NULL;
            goto restore_ui;
        }
    }

    // ── STEP 4: Playback loop with proper event handling ──
    {
        bool playing = true;
        bool paused = false;

        while (appletMainLoop() && playing) {
            // Drain mpv events with a small timeout to avoid busy-wait
            while (1) {
                mpv_event *event = mpv_wait_event(mpv, 0.01); // 10ms timeout
                if (event->event_id == MPV_EVENT_NONE) break;

                switch (event->event_id) {
                    case MPV_EVENT_END_FILE:
                    case MPV_EVENT_SHUTDOWN:
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
                mpv_command_string(mpv, "stop");
                playing = false;
            }
            if (kDown & HidNpadButton_A) {
                paused = !paused;
                mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &paused);
            }
            if (kDown & HidNpadButton_Right) {
                mpv_command_string(mpv, "seek 10");
            }
            if (kDown & HidNpadButton_Left) {
                mpv_command_string(mpv, "seek -10");
            }

            svcSleepThread(16000000); // ~16ms for ~60fps
        }
    }

    // ── STEP 5: Destroy mpv ──
    mpv_terminate_destroy(mpv);
    mpv = NULL;

    // Delay to let GPU resources fully release
    svcSleepThread(200000000); // 200ms

restore_ui:
    // ── STEP 6: FULL SDL reinit ──
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0) {
        return; // Fatal: can't restore UI
    }
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");

    window = SDL_CreateWindow("DarkTube",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
    if (!window) return;

    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

void player_exit(void) {
    if (mpv) {
        mpv_terminate_destroy(mpv);
        mpv = NULL;
    }
}
