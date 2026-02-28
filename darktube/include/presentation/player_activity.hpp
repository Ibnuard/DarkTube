#pragma once

#include <borealis.hpp>

namespace DarkTube {
namespace Presentation {

    class VideoPlayerView : public brls::Box {
    public:
        VideoPlayerView();
        void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx) override;
    };

    class PlayerOverlayView : public brls::Box {
    public:
        PlayerOverlayView();
        
        void onFocusGained() override;
        void onFocusLost() override;
        void toggleOSD();
        void frame(brls::FrameContext* ctx) override;

    private:
        brls::Box* topBar;
        brls::Box* bottomBar;
        brls::Label* statusLabel;
        brls::Label* timeLabel;
        brls::ProgressSpinner* bufferingLoader;
        bool isPlaying = true;
        bool osdVisible = false;

        brls::Box* createTopBar();
        brls::Box* createBottomBar();
        brls::Box* createProgressBar();
        brls::Box* createControlBar();
    };

    class PlayerActivity : public brls::Activity {
    public:
        PlayerActivity();
        ~PlayerActivity() override = default;

        brls::View* createContentView() override;
    };

} // namespace Presentation
} // namespace DarkTube
