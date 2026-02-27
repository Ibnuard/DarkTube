#include <borealis.hpp>

#ifdef __SWITCH__
#include <switch.h>
#endif

#include "view/mpv_core.hpp"

// Simple video player view using MPVCore
class VideoPlayerView : public brls::Box {
public:
    VideoPlayerView() {
        this->setFocusable(true);
        this->setHideHighlight(true);
        brls::Logger::info("VideoPlayerView created");
    }

    ~VideoPlayerView() override = default;

    void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style,
              brls::FrameContext* ctx) override {
        // Draw the video frame
        MPVCore::instance().draw(brls::Rect(x, y, width, height));

        // Draw children on top of mpv
        Box::draw(vg, x, y, width, height, style, ctx);
    }

    brls::View* getDefaultFocus() override { return this; }
};

// Simple activity that holds the video player
class PlayerActivity : public brls::Activity {
public:
    brls::View* createContentView() override {
        return new VideoPlayerView();
    }
};

int main(int argc, char* argv[]) {
    // Redirect stdout and stderr to a file on the SD card so we can read it after crash
#ifdef __SWITCH__
    freopen("sdmc:/darktube_crash.log", "w", stdout);
    freopen("sdmc:/darktube_crash.log", "a", stderr);
#else
    freopen("darktube_crash.log", "w", stdout);
    freopen("darktube_crash.log", "a", stderr);
#endif
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    brls::Logger::setLogLevel(brls::LogLevel::LOG_DEBUG);

    brls::Logger::info("DarkTube: Starting...");

    // Init borealis
    if (!brls::Application::init()) {
        brls::Logger::error("Unable to init application");
        return EXIT_FAILURE;
    }

    brls::Application::createWindow("DarkTube");

    // DO NOT init MPV here! MPV must be initialized from INSIDE the main loop
    // (just like TsVitch does). Initializing before the loop causes
    // on_wakeup/on_update callbacks to fire before Borealis is ready = memory corruption!

    // Create a simple blank view for the "Home Screen"
    brls::Box* homeBox = new brls::Box();
    homeBox->setFocusable(true);

    // Register action A to play local file
    homeBox->registerAction("Play Local (A)", brls::BUTTON_A, [](brls::View* view) {
        brls::Logger::info("User pressed A: Playing local video");
        // MPVCore::instance() will lazily init MPV on first call (inside main loop = safe!)
        brls::Logger::info("Initializing MPVCore...");
        MPVCore::instance().setUrl("sdmc:/test.mp4");
        brls::Logger::info("setUrl done, pushing PlayerActivity...");
        brls::Application::pushActivity(new PlayerActivity(), brls::TransitionAnimation::NONE);
        return true;
    });

    // Register action B to play URL
    homeBox->registerAction("Play URL (B)", brls::BUTTON_B, [](brls::View* view) {
        brls::Logger::info("User pressed B: Playing stream URL");
        brls::Logger::info("Initializing MPVCore...");
        MPVCore::instance().setUrl("http://cdn.brid.tv/live/partners/6205/sd/69838.mp4");
        brls::Logger::info("setUrl done, pushing PlayerActivity...");
        brls::Application::pushActivity(new PlayerActivity(), brls::TransitionAnimation::NONE);
        return true;
    });

    // Add a simple label so the user knows what to do
    brls::Label* helpLabel = new brls::Label();
    helpLabel->setText("Press A for Local (test.mp4),\nPress B for URL Stream");
    helpLabel->setFontSize(24);
    helpLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    helpLabel->setVerticalAlign(brls::VerticalAlign::CENTER);
    homeBox->addView(helpLabel);

    brls::Application::pushActivity(new brls::Activity(homeBox));
    brls::Logger::info("DarkTube: Home screen pushed");

    // Main loop
    while (brls::Application::mainLoop()) {
    }

    brls::Logger::info("DarkTube: Clean exit");

    return EXIT_SUCCESS;
}
