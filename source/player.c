#include "player.h"
#include "ui.h"
#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <SDL2/SDL.h>

extern PadState pad;
static mpv_handle *mpv = NULL;
static mpv_render_context *mpv_gl = NULL;

#define LOG(fmt, ...) printf("[Player] " fmt "\n", ##__VA_ARGS__)

static void *get_proc_address(void *fn_ctx, const char *name) {
    (void)fn_ctx;
    return SDL_GL_GetProcAddress(name);
}

static void on_mpv_render_update(void *ctx) {
    (void)ctx;
}

int player_init(void) {
    return 0;
}

void player_play_stream(const char *stream_url) {
    LOG("=== Starting playback ===");
    LOG("URL: %.80s...", stream_url);

    SDL_GLContext gl_ctx = NULL;

    // ── STEP 1: Destroy SDL Renderer only (keep the window!) ──
    // The window holds the display layer on Switch.
    // We only need to release the internal GL context that SDL_Renderer owns.
    if (renderer) {
        LOG("Destroying SDL_Renderer");
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }

    // ── STEP 2: Create a raw OpenGL ES context on the existing window ──
    LOG("Setting GL attributes for GLES 2.0");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    gl_ctx = SDL_GL_CreateContext(window);
    if (!gl_ctx) {
        LOG("ERROR: SDL_GL_CreateContext failed: %s", SDL_GetError());
        goto restore_ui;
    }
    LOG("GL context created successfully");

    if (SDL_GL_MakeCurrent(window, gl_ctx) != 0) {
        LOG("ERROR: SDL_GL_MakeCurrent failed: %s", SDL_GetError());
        goto cleanup_gl;
    }
    LOG("GL context made current");

    // ── STEP 3: Create mpv with libmpv render API ──
    mpv = mpv_create();
    if (!mpv) {
        LOG("ERROR: mpv_create failed");
        goto cleanup_gl;
    }

    mpv_set_option_string(mpv, "vo", "libmpv");
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
    mpv_set_option_string(mpv, "demuxer-readahead-secs", "10");

    int err = mpv_initialize(mpv);
    if (err < 0) {
        LOG("ERROR: mpv_initialize failed: %s", mpv_error_string(err));
        mpv_terminate_destroy(mpv);
        mpv = NULL;
        goto cleanup_gl;
    }
    LOG("mpv initialized");

    // ── STEP 4: Create mpv render context bound to our GL ──
    {
        mpv_opengl_init_params gl_init = {
            .get_proc_address = get_proc_address,
            .get_proc_address_ctx = NULL,
        };
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_OPENGL},
            {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init},
            {MPV_RENDER_PARAM_INVALID, NULL}
        };

        err = mpv_render_context_create(&mpv_gl, mpv, params);
        if (err < 0) {
            LOG("ERROR: mpv_render_context_create failed: %s", mpv_error_string(err));
            mpv_terminate_destroy(mpv);
            mpv = NULL;
            goto cleanup_gl;
        }
    }
    mpv_render_context_set_update_callback(mpv_gl, on_mpv_render_update, NULL);
    LOG("mpv render context created");

    // ── STEP 5: Start playback ──
    {
        const char *cmd[] = {"loadfile", stream_url, NULL};
        err = mpv_command(mpv, cmd);
        if (err < 0) {
            LOG("ERROR: loadfile failed: %s", mpv_error_string(err));
            goto cleanup_mpv;
        }
    }
    LOG("loadfile command sent, entering render loop");

    // ── STEP 6: Render loop ──
    {
        bool playing = true;
        bool paused = false;

        while (appletMainLoop() && playing) {
            // Drain mpv events
            while (1) {
                mpv_event *event = mpv_wait_event(mpv, 0);
                if (event->event_id == MPV_EVENT_NONE) break;
                if (event->event_id == MPV_EVENT_END_FILE) {
                    LOG("END_FILE event");
                    playing = false;
                }
                if (event->event_id == MPV_EVENT_SHUTDOWN) {
                    LOG("SHUTDOWN event");
                    playing = false;
                }
            }

            // Controller input
            padUpdate(&pad);
            u64 kDown = padGetButtonsDown(&pad);

            if (kDown & HidNpadButton_B) {
                LOG("User stop");
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

            // Render mpv frame
            if (mpv_render_context_update(mpv_gl) & MPV_RENDER_UPDATE_FRAME) {
                int w = 1280, h = 720;

                mpv_opengl_fbo fbo = {
                    .fbo = 0,
                    .w = w,
                    .h = h,
                    .internal_format = 0
                };
                int flip_y = 1;

                mpv_render_param render_params[] = {
                    {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
                    {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
                    {MPV_RENDER_PARAM_INVALID, NULL}
                };

                mpv_render_context_render(mpv_gl, render_params);
                SDL_GL_SwapWindow(window);
            }

            // Pump SDL events
            SDL_Event e;
            while (SDL_PollEvent(&e)) { }

            svcSleepThread(8000000); // ~8ms
        }
    }

cleanup_mpv:
    LOG("Cleaning up mpv");
    if (mpv_gl) {
        mpv_render_context_free(mpv_gl);
        mpv_gl = NULL;
    }
    if (mpv) {
        mpv_terminate_destroy(mpv);
        mpv = NULL;
    }

cleanup_gl:
    if (gl_ctx) {
        LOG("Deleting GL context");
        SDL_GL_DeleteContext(gl_ctx);
        gl_ctx = NULL;
    }

restore_ui:
    // ── STEP 7: Recreate SDL Renderer ──
    LOG("Restoring SDL Renderer");
    if (window) {
        renderer = SDL_CreateRenderer(window, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (renderer) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            LOG("SDL Renderer restored OK");
        } else {
            LOG("ERROR: Failed to recreate renderer: %s", SDL_GetError());
        }
    } else {
        LOG("ERROR: Window is NULL, cannot restore renderer");
    }
}

void player_exit(void) {
    if (mpv_gl) {
        mpv_render_context_free(mpv_gl);
        mpv_gl = NULL;
    }
    if (mpv) {
        mpv_terminate_destroy(mpv);
        mpv = NULL;
    }
}
