#pragma once

#include <string>
#include <cstdlib>
#include <borealis/core/geometry.hpp>
#include <borealis/core/singleton.hpp>
#include <borealis/core/logger.hpp>
#include <mpv/client.h>
#include <mpv/render.h>
#include <mpv/render_gl.h>

#ifdef __SDL2__
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#else
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#endif

class MPVCore : public brls::Singleton<MPVCore> {
public:
    MPVCore();
    ~MPVCore();

    void setUrl(const std::string &url);
    void draw(brls::Rect rect, float alpha = 1.0);

    bool isStopped() const;
    bool isPlaying() const;
    bool isPaused() const;

    void resume();
    void pause();
    void stop();
    void seek(int64_t p);
    void setVolume(int64_t value);

    mpv_render_context *getContext();
    mpv_handle *getHandle();

    bool video_stopped = true;
    bool video_playing = false;
    bool redraw = false;

private:
    mpv_handle *mpv = nullptr;
    mpv_render_context *mpv_context = nullptr;
    brls::Rect rect = {0, 0, 1920, 1080};

    int default_framebuffer = 0;
    
    mpv_opengl_fbo mpv_fbo{0, 1280, 720, 0};
    mpv_render_param mpv_params[3] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &mpv_fbo},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };

    void init();
    void clean();
    void setFrameSize(brls::Rect rect);
    void eventMainLoop();

    static void on_update(void *self);
    static void on_wakeup(void *self);

    brls::Event<bool>::Subscription focusSubscription;
    brls::Event<>::Subscription exitDoneEventSubscription;
};
