#include "../include/presentation/player_activity.hpp"
#include "../include/core/theme.hpp"
#include "view/mpv_core.hpp"
#include "../include/presentation/ui_utils.hpp"
#include <borealis.hpp>

namespace DarkTube {
namespace Presentation {

    // --- VideoPlayerView ---

    VideoPlayerView::VideoPlayerView() {
        this->setFocusable(true);
        this->setHideHighlight(true);
        brls::Logger::info("VideoPlayerView created. Setting focusable.");
    }

    void VideoPlayerView::draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx) {
        // Draw the video frame using MPV
        MPVCore::instance().draw(brls::Rect(x, y, width, height));

        // Draw overlay children
        Box::draw(vg, x, y, width, height, style, ctx);
    }

    // --- PlayerOverlayView ---

    PlayerOverlayView::PlayerOverlayView() {
        this->setWidthPercentage(100);
        this->setHeightPercentage(100);
        this->setFocusable(true);
        this->setHideHighlight(true);
        this->setAxis(brls::Axis::COLUMN);
        this->setJustifyContent(brls::JustifyContent::SPACE_BETWEEN); // Top and Bottom

        topBar = createTopBar();
        bottomBar = createBottomBar();

        this->addView(topBar);
        
        // Add buffering loader to the center of the screen
        brls::Box* centerContainer = new brls::Box();
        centerContainer->setGrow(1.0f);
        centerContainer->setAlignItems(brls::AlignItems::CENTER);
        centerContainer->setJustifyContent(brls::JustifyContent::CENTER);
        
        bufferingLoader = new brls::ProgressSpinner();
        bufferingLoader->setVisibility(brls::Visibility::INVISIBLE);
        centerContainer->addView(bufferingLoader);
        
        this->addView(centerContainer);
        this->addView(bottomBar);

        // Hide initially since it starts playing
        topBar->setVisibility(brls::Visibility::INVISIBLE);
        bottomBar->setVisibility(brls::Visibility::INVISIBLE);
        this->setBackgroundColor(nvgRGBA(0, 0, 0, 0));

        // Actions
        this->registerAction("Toggle Play", brls::BUTTON_A, [this](brls::View* view) {
            this->toggleOSD();
            return true;
        });

        this->registerAction("Seek Forward", brls::BUTTON_RIGHT, [](brls::View* view) {
            brls::Logger::info("Seek +10s");
            // Implement seek interaction later
            return true;
        });
        
        this->registerAction("Seek Backward", brls::BUTTON_LEFT, [](brls::View* view) {
            brls::Logger::info("Seek -10s");
            // Implement seek interaction later
            return true;
        });
    }

    void PlayerOverlayView::toggleOSD() {
        if (osdVisible) {
            // Hide OSD and resume
            osdVisible = false;
            isPlaying = true;
            statusLabel->setText("PLAYING");
            topBar->setVisibility(brls::Visibility::INVISIBLE);
            bottomBar->setVisibility(brls::Visibility::INVISIBLE);
            this->setBackgroundColor(nvgRGBA(0, 0, 0, 0));
            MPVCore::instance().resume();
        } else {
            // Show OSD and pause
            osdVisible = true;
            isPlaying = false;
            statusLabel->setText("PAUSED");
            topBar->setVisibility(brls::Visibility::VISIBLE);
            bottomBar->setVisibility(brls::Visibility::VISIBLE);
            this->setBackgroundColor(Theme::OverlayBackground);
            MPVCore::instance().pause();
        }
    }

    void PlayerOverlayView::frame(brls::FrameContext* ctx) {
        // Poll for buffering state and toggle the loader
        if (MPVCore::instance().isBuffering()) {
            if (bufferingLoader->getVisibility() != brls::Visibility::VISIBLE) {
                bufferingLoader->setVisibility(brls::Visibility::VISIBLE);
                statusLabel->setText("BUFFERING...");
            }
        } else {
            if (bufferingLoader->getVisibility() == brls::Visibility::VISIBLE) {
                bufferingLoader->setVisibility(brls::Visibility::INVISIBLE);
                statusLabel->setText(isPlaying ? "PLAYING" : "PAUSED");
            }
        }
        
        // Pass to base class for rendering
        brls::Box::frame(ctx);
    }

    brls::Box* PlayerOverlayView::createTopBar() {
        brls::Box* bar = new brls::Box();
        bar->setAxis(brls::Axis::ROW);
        bar->setWidthPercentage(100);
        bar->setHeight(80);
        bar->setAlignItems(brls::AlignItems::CENTER);
        bar->setPadding(40, 60, 0, 60); // Top margin

        brls::Button* backBtn = new brls::Button();
        backBtn->setStyle(&brls::BUTTONSTYLE_BORDERLESS);
        backBtn->setText("<  Back");
        backBtn->registerAction("Back", brls::BUTTON_B, [](brls::View* view) {
            brls::Application::popActivity();
            return true;
        });
        bar->addView(backBtn);

        brls::Label* videoTitle = new brls::Label();
        videoTitle->setText("Sample Video Title - YouTube | DarkTube");
        videoTitle->setFontSize(28);
        videoTitle->setTextColor(Theme::TextPrimary);
        videoTitle->setMarginLeft(30);
        bar->addView(videoTitle);

        return bar;
    }

    brls::Box* PlayerOverlayView::createBottomBar() {
        brls::Box* bar = new brls::Box();
        bar->setAxis(brls::Axis::COLUMN);
        bar->setWidthPercentage(100);
        bar->setPadding(0, 60, 50, 60); // Bottom margin

        bar->addView(createProgressBar());
        bar->addView(createControlBar());

        return bar;
    }

    brls::Box* PlayerOverlayView::createProgressBar() {
        brls::Box* container = new brls::Box();
        container->setAxis(brls::Axis::ROW);
        container->setWidthPercentage(100);
        container->setHeight(30);
        container->setAlignItems(brls::AlignItems::CENTER);
        container->setMarginBottom(15);

        // Simulated progress bar background
        brls::Box* track = new brls::Box();
        track->setAxis(brls::Axis::ROW);
        track->setGrow(1.0f);
        track->setHeight(8);
        track->setBackgroundColor(nvgRGBA(255, 255, 255, 100));
        track->setCornerRadius(4);

        // Simulated filled progress (YouTube Red)
        brls::Box* fill = new brls::Box();
        fill->setWidthPercentage(30); // 30% complete
        fill->setHeight(8);
        fill->setBackgroundColor(Theme::AccentRed);
        fill->setCornerRadius(4);
        track->addView(fill);

        container->addView(track);
        return container;
    }

    brls::Box* PlayerOverlayView::createControlBar() {
        brls::Box* bar = new brls::Box();
        bar->setAxis(brls::Axis::ROW);
        bar->setWidthPercentage(100);
        bar->setAlignItems(brls::AlignItems::CENTER);
        bar->setJustifyContent(brls::JustifyContent::SPACE_BETWEEN);

        brls::Box* leftGroup = new brls::Box();
        leftGroup->setAxis(brls::Axis::ROW);
        leftGroup->setAlignItems(brls::AlignItems::CENTER);

        statusLabel = new brls::Label();
        statusLabel->setText("PLAYING");
        statusLabel->setFontSize(24);
        statusLabel->setTextColor(Theme::TextPrimary);
        statusLabel->setMarginRight(30);
        leftGroup->addView(statusLabel);

        timeLabel = new brls::Label();
        timeLabel->setText("1:23 / 4:56");
        timeLabel->setFontSize(20);
        timeLabel->setTextColor(Theme::TextSecondary);
        leftGroup->addView(timeLabel);

        bar->addView(leftGroup);

        brls::Box* rightGroup = new brls::Box();
        rightGroup->setAxis(brls::Axis::ROW);
        rightGroup->setAlignItems(brls::AlignItems::CENTER);

        rightGroup->addView(UIUtils::createHint(nvgRGB(50, 160, 60), "A", "Play/Pause"));
        rightGroup->addView(UIUtils::createHint(nvgRGB(220, 180, 0), "L/R", "Seek"));
        rightGroup->addView(UIUtils::createHint(nvgRGB(200, 40, 40), "B", "Back"));

        bar->addView(rightGroup);

        return bar;
    }

    void PlayerOverlayView::onFocusGained() {
        brls::Box::onFocusGained();
    }

    void PlayerOverlayView::onFocusLost() {
        // If we lose focus, hide the UI to prevent it from getting stuck on screen
        if (osdVisible) {
             this->toggleOSD();
        }
        brls::Box::onFocusLost();
    }

    // --- PlayerActivity ---

    PlayerActivity::PlayerActivity() {
        brls::Logger::info("User pushed PlayerActivity");
        MPVCore::instance().setUrl("https://cdn.brid.tv/live/partners/6205/sd/69838.mp4");
        MPVCore::instance().resume();
    }

    brls::View* PlayerActivity::createContentView() {
        VideoPlayerView* videoView = new VideoPlayerView();
        
        // Add overlay that fades in/out on interaction
        PlayerOverlayView* overlay = new PlayerOverlayView();
        videoView->addView(overlay);

        // Ensure the overlay gets focus so A/B inputs are not swallowed by the VideoView
        brls::Application::giveFocus(overlay);

        return videoView;
    }

} // namespace Presentation
} // namespace DarkTube
