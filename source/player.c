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

static void *get_proc_address_mpv(void *fn_ctx, const char *name) {
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
    // ── STEP 1: Destroy the SDL2 Renderer (it owns an internal GL context) ──
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }

    // ── STEP 2: Create a raw OpenGL ES context on the existing SDL_Window ──
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    SDL_GLContext gl_ctx = SDL_GL_CreateContext(window);
    if (!gl_ctx) {
        goto restore_ui;
    }
    SDL_GL_MakeCurrent(window, gl_ctx);

    // ── STEP 3: Create MPV and its render context bound to our GL ──
    mpv = mpv_create();
    if (!mpv) goto cleanup_gl;

    mpv_set_option_string(mpv, "ytdl", "no");
    mpv_set_option_string(mpv, "hwdec", "auto");
    mpv_set_option_string(mpv, "terminal", "no");
    mpv_set_option_string(mpv, "input-default-bindings", "no");
    mpv_set_option_string(mpv, "input-vo-keyboard", "no");
    // Don't let mpv create its own window / vo:
    mpv_set_option_string(mpv, "vo", "libmpv");

    if (mpv_initialize(mpv) < 0) {
        mpv_terminate_destroy(mpv);
        mpv = NULL;
        goto cleanup_gl;
    }

    mpv_opengl_init_params gl_init_params = {
        .get_proc_address = get_proc_address_mpv,
        .get_proc_address_ctx = NULL,
    };
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_OPENGL},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
        {MPV_RENDER_PARAM_INVALID, NULL}
    };

    if (mpv_render_context_create(&mpv_gl, mpv, params) < 0) {
        mpv_terminate_destroy(mpv);
        mpv = NULL;
        goto cleanup_gl;
    }
    mpv_render_context_set_update_callback(mpv_gl, on_mpv_render_update, NULL);

    // ── STEP 4: Start playback ──
    const char *cmd[] = {"loadfile", stream_url, NULL};
    mpv_command(mpv, cmd);

    bool playing = true;
    bool paused = false;

    while (appletMainLoop() && playing) {
        // Handle MPV events
        while (1) {
            mpv_event *event = mpv_wait_event(mpv, 0);
            if (event->event_id == MPV_EVENT_NONE) break;
            if (event->event_id == MPV_EVENT_END_FILE) playing = false;
            if (event->event_id == MPV_EVENT_SHUTDOWN) playing = false;
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
        }
        if (kDown & HidNpadButton_Right) {
            mpv_command_string(mpv, "seek 10");
        }
        if (kDown & HidNpadButton_Left) {
            mpv_command_string(mpv, "seek -10");
        }

        // ── STEP 5: Render MPV frame into our GL context ──
        if (mpv_render_context_update(mpv_gl) & MPV_RENDER_UPDATE_FRAME) {
            int w = 1280, h = 720;
            SDL_GetWindowSize(window, &w, &h);

            mpv_opengl_fbo fbo = {
                .fbo = 0,       // default framebuffer
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
        } else {
            svcSleepThread(5000000); // 5ms idle when no new frame
        }

        // Pump SDL events
        SDL_Event e;
        while (SDL_PollEvent(&e)) { }
    }

    // ── STEP 6: Cleanup MPV ──
    mpv_render_context_free(mpv_gl);
    mpv_gl = NULL;
    mpv_terminate_destroy(mpv);
    mpv = NULL;

cleanup_gl:
    if (gl_ctx) {
        SDL_GL_DeleteContext(gl_ctx);
    }

restore_ui:
    // ── STEP 7: Recreate the SDL2 Renderer for the UI ──
    if (window) {
        renderer = SDL_CreateRenderer(window, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (renderer) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        }
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
