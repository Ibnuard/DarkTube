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
    return SDL_GL_GetProcAddress(name);
}

static void on_mpv_render_update(void *ctx) {
    // mpv wants us to wake up the render loop
}

int player_init(void) {
    mpv = mpv_create();
    if (!mpv) {
        printf("Failed to create mpv handle\n");
        return -1;
    }

    int val = 1;
    mpv_set_option(mpv, "osc", MPV_FORMAT_FLAG, &val); // Enable on-screen controller
    mpv_set_option_string(mpv, "ytdl", "no");          // URLs resolved on server
    mpv_set_option_string(mpv, "hwdec", "auto");

    if (mpv_initialize(mpv) < 0) {
        printf("Failed to initialize mpv\n");
        mpv_terminate_destroy(mpv);
        mpv = NULL;
        return -1;
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
        printf("Failed to init mpv GL context\n");
        mpv_terminate_destroy(mpv);
        mpv = NULL;
        return -1;
    }

    mpv_render_context_set_update_callback(mpv_gl, on_mpv_render_update, NULL);

    return 0;
}

void player_play_stream(const char *stream_url) {
    if (!mpv || !mpv_gl) {
        if (player_init() != 0) return;
    }

    const char *cmd[] = {"loadfile", stream_url, NULL};
    mpv_command(mpv, cmd);

    bool playing = true;
    bool paused = false;

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
        }

        // MPV Rendering!
        int w = 1280, h = 720;
        SDL_GetWindowSize(window, &w, &h);
        
        mpv_opengl_fbo fbo = {
            .fbo = 0,
            .w = w,
            .h = h,
            .internal_format = 0
        };
        int flip_y = 0; // SDL often draws with Y down, but Switch usually expects standard GL Y up, so 0 is usually fine. Let's see.

        mpv_render_param render_params[] = {
            {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
            {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
            {MPV_RENDER_PARAM_INVALID, NULL}
        };

        // Render straight into the SDL window framebuffer
        mpv_render_context_render(mpv_gl, render_params);

        // Tell SDL to swap buffers
        SDL_GL_SwapWindow(window);

        // Poll SDL events so it doesn't freeze the application event loop internally
        SDL_Event e;
        while (SDL_PollEvent(&e)) { }
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
