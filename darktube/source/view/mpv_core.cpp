// Simplified MPV Core based on TsVitch for standard OpenGL
#include <cstdlib>
#include <clocale>
#include <cmath>
#include <borealis/core/thread.hpp>
#include <borealis/core/application.hpp>
#include "view/mpv_core.hpp"

static inline void check_error(int status) {
    if (status < 0) {
        brls::Logger::error("MPV ERROR ====> {}", mpv_error_string(status));
    }
}

static void *get_proc_address(void *unused, const char *name) {
#ifdef __SDL2__
    SDL_GL_GetCurrentContext();
    return (void *)SDL_GL_GetProcAddress(name);
#else
    glfwGetCurrentContext();
    return (void *)glfwGetProcAddress(name);
#endif
}

void MPVCore::on_update(void *self) {
    brls::sync([]() {
        uint64_t flags = mpv_render_context_update(MPVCore::instance().getContext());
        MPVCore::instance().redraw = (flags & MPV_RENDER_UPDATE_FRAME) != 0;
    });
}

void MPVCore::on_wakeup(void *self) {
    brls::sync([]() { MPVCore::instance().eventMainLoop(); });
}

MPVCore::MPVCore() {
    this->init();

    exitDoneEventSubscription = brls::Application::getExitDoneEvent()->subscribe([this]() {
        this->clean();
    });
}

MPVCore::~MPVCore() = default;

void MPVCore::init() {
    brls::Logger::info("MPVCore::init started");
    setlocale(LC_NUMERIC, "C");
    this->mpv = mpv_create();
    if (!mpv) {
        brls::fatal("Error Create mpv Handle");
    }

    mpv_set_option_string(mpv, "ytdl", "no");
    mpv_set_option_string(mpv, "audio-channels", "stereo");
    mpv_set_option_string(mpv, "idle", "yes");
    mpv_set_option_string(mpv, "loop-file", "no");
    mpv_set_option_string(mpv, "osd-level", "0");
    mpv_set_option_string(mpv, "video-timing-offset", "0");
    mpv_set_option_string(mpv, "keep-open", "yes");
    mpv_set_option_string(mpv, "hr-seek", "yes");
    mpv_set_option_string(mpv, "reset-on-next-file", "speed,pause");
    mpv_set_option_string(mpv, "vo", "libmpv");
    mpv_set_option_string(mpv, "tls-verify", "no");

#if defined(__SWITCH__)
    mpv_set_option_string(mpv, "vd-lavc-dr", "no");
    mpv_set_option_string(mpv, "vd-lavc-threads", "4");
    mpv_set_option_string(mpv, "opengl-glfinish", "yes");
    mpv_set_option_string(mpv, "hwdec", "no");
#endif

    mpv_set_option_string(mpv, "cache", "yes");
    mpv_set_option_string(mpv, "demuxer-max-bytes", "12MiB");
    mpv_set_option_string(mpv, "demuxer-max-back-bytes", "5MiB");
    mpv_set_option_string(mpv, "demuxer-lavf-analyzeduration", "0.4");
    mpv_set_option_string(mpv, "demuxer-lavf-probescore", "24");

    mpv_set_option_string(mpv, "terminal", "yes");
    mpv_set_option_string(mpv, "msg-level", "all=v");

    if (mpv_initialize(mpv) < 0) {
        mpv_terminate_destroy(mpv);
        brls::fatal("Could not initialize mpv context");
    }

    check_error(mpv_request_log_messages(mpv, "debug"));
    check_error(mpv_observe_property(mpv, 1, "core-idle", MPV_FORMAT_FLAG));
    check_error(mpv_observe_property(mpv, 2, "eof-reached", MPV_FORMAT_FLAG));
    check_error(mpv_observe_property(mpv, 3, "duration", MPV_FORMAT_INT64));
    check_error(mpv_observe_property(mpv, 4, "playback-time", MPV_FORMAT_DOUBLE));
    check_error(mpv_observe_property(mpv, 12, "pause", MPV_FORMAT_FLAG));

    // Create render context for OpenGL
    int advanced_control{1};
    mpv_opengl_init_params gl_init_params{get_proc_address, nullptr};
    mpv_render_param params[]{{MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
                              {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
                              {MPV_RENDER_PARAM_ADVANCED_CONTROL, &advanced_control},
                              {MPV_RENDER_PARAM_INVALID, nullptr}};

    if (mpv_render_context_create(&mpv_context, mpv, params) < 0) {
        mpv_terminate_destroy(mpv);
        brls::fatal("failed to initialize mpv render context");
    }

    brls::Logger::info("MPV Version: {}", mpv_get_property_string(mpv, "mpv-version"));
    brls::Logger::info("FFMPEG Version: {}", mpv_get_property_string(mpv, "ffmpeg-version"));

    setVolume(100);
    mpv_set_wakeup_callback(mpv, on_wakeup, this);
    mpv_render_context_set_update_callback(mpv_context, on_update, this);

    focusSubscription = brls::Application::getWindowFocusChangedEvent()->subscribe([this](bool focus) {
        if (focus) {
            if (isPlaying()) resume();
        } else {
            pause();
        }
    });
    
    // Get default framebuffer for drawing
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &default_framebuffer);
    mpv_fbo.fbo = default_framebuffer;
}

void MPVCore::clean() {
    mpv_command_string(this->mpv, "quit");
    brls::Application::getWindowFocusChangedEvent()->unsubscribe(focusSubscription);
    if (this->mpv_context) {
        mpv_render_context_free(this->mpv_context);
        this->mpv_context = nullptr;
    }
    if (this->mpv) {
        mpv_terminate_destroy(this->mpv);
        this->mpv = nullptr;
    }
}

void MPVCore::setUrl(const std::string &url) {
    if (!mpv) return;
    const char *cmd[] = {"loadfile", url.c_str(), NULL};
    check_error(mpv_command_async(mpv, 0, cmd));
}

void MPVCore::setFrameSize(brls::Rect r) {
    rect = r;
    if (std::isnan(rect.getWidth()) || std::isnan(rect.getHeight())) return;

    this->mpv_fbo.w = brls::Application::windowWidth;
    this->mpv_fbo.h = brls::Application::windowHeight;

    const char *cmd_r_right[] = {"set", "video-margin-ratio-right", std::to_string((float)(brls::Application::contentWidth - rect.getMaxX()) / brls::Application::contentWidth).c_str(), NULL};
    mpv_command_async(mpv, 0, cmd_r_right);
    const char *cmd_r_bot[] = {"set", "video-margin-ratio-bottom", std::to_string((float)(brls::Application::contentHeight - rect.getMaxY()) / brls::Application::contentHeight).c_str(), NULL};
    mpv_command_async(mpv, 0, cmd_r_bot);
    const char *cmd_r_top[] = {"set", "video-margin-ratio-top", std::to_string((float)rect.getMinY() / brls::Application::contentHeight).c_str(), NULL};
    mpv_command_async(mpv, 0, cmd_r_top);
    const char *cmd_r_left[] = {"set", "video-margin-ratio-left", std::to_string((float)rect.getMinX() / brls::Application::contentWidth).c_str(), NULL};
    mpv_command_async(mpv, 0, cmd_r_left);
}

void MPVCore::draw(brls::Rect area, float alpha) {
    if (mpv_context == nullptr) return;
    if (!(this->rect == area)) setFrameSize(area);

    if (alpha >= 1) {
        mpv_render_context_render(this->mpv_context, mpv_params);
        glBindFramebuffer(GL_FRAMEBUFFER, default_framebuffer);
        glViewport(0, 0, brls::Application::windowWidth, brls::Application::windowHeight);
        mpv_render_context_report_swap(this->mpv_context);

        if (area.getWidth() < brls::Application::contentWidth) {
            auto *vg = brls::Application::getNVGContext();
            nvgBeginPath(vg);
            nvgFillColor(vg, brls::Application::getTheme().getColor("brls/background"));
            nvgRect(vg, 0, 0, area.getMinX(), brls::Application::contentHeight);
            nvgRect(vg, area.getMaxX(), 0, brls::Application::contentWidth - area.getMaxX(),
                    brls::Application::contentHeight);
            nvgRect(vg, area.getMinX() - 1, 0, area.getWidth() + 2, area.getMinY());
            nvgRect(vg, area.getMinX() - 1, area.getMaxY(), area.getWidth() + 2,
                    brls::Application::contentHeight - area.getMaxY());
            nvgFill(vg);
        }
    }
}

void MPVCore::eventMainLoop() {
    while (true) {
        auto event = mpv_wait_event(this->mpv, 0);
        switch (event->event_id) {
            case MPV_EVENT_NONE:
                return;
            case MPV_EVENT_LOG_MESSAGE: {
                auto *msg = (mpv_event_log_message *)event->data;
                brls::Logger::info("[MPV] [{}] {}: {}", msg->prefix, msg->level, msg->text);
                break;
            }
            case MPV_EVENT_PROPERTY_CHANGE: {
                auto *prop = (mpv_event_property *)event->data;
                if (prop->format == MPV_FORMAT_FLAG) {
                    if (strcmp(prop->name, "core-idle") == 0) {
                        int idle = *(int *)prop->data;
                        video_playing = !idle;
                    }
                }
                break;
            }
            case MPV_EVENT_FILE_LOADED:
                video_stopped = false;
                break;
            case MPV_EVENT_END_FILE: {
                video_stopped = true;
                video_playing = false;
                break;
            }
            default:
                break;
        }
    }
}

mpv_render_context *MPVCore::getContext() { return this->mpv_context; }
mpv_handle *MPVCore::getHandle() { return this->mpv; }

bool MPVCore::isStopped() const { return video_stopped; }
bool MPVCore::isPlaying() const { return video_playing; }
bool MPVCore::isPaused() const { return !video_playing && !video_stopped; }

void MPVCore::resume() { mpv_command_string(mpv, "set pause no"); }
void MPVCore::pause() { mpv_command_string(mpv, "set pause yes"); }
void MPVCore::stop() { mpv_command_string(mpv, "stop"); }
void MPVCore::seek(int64_t p) {
    std::string cmd = "seek " + std::to_string(p) + " absolute";
    mpv_command_string(mpv, cmd.c_str());
}
void MPVCore::setVolume(int64_t value) {
    std::string cmd = "set volume " + std::to_string(value);
    mpv_command_string(mpv, cmd.c_str());
}
