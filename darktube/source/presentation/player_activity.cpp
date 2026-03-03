#include "../include/presentation/player_activity.hpp"
#include "../include/core/theme.hpp"
#include "view/mpv_core.hpp"
#include "../include/presentation/ui_utils.hpp"
#include "../include/data/network_client.hpp"
#include <borealis.hpp>

namespace DarkTube {
namespace Presentation {

    // --- VideoPlayerView ---

    VideoPlayerView::VideoPlayerView() {
        this->setFocusable(false); // Disable focus so it doesn't steal from overlay
        this->setHideHighlight(true);
        brls::Logger::info("VideoPlayerView created. Focus disabled.");
    }

    void VideoPlayerView::draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx) {
        // Draw the video frame using MPV
        MPVCore::instance().draw(brls::Rect(x, y, width, height));

        // Draw overlay children
        Box::draw(vg, x, y, width, height, style, ctx);
    }

    // --- PlayerOverlayView ---

    PlayerOverlayView::PlayerOverlayView(const std::string& title) : videoTitle(title) {
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
            brls::Logger::info("Button A pressed on PlayerOverlayView");
            if (MPVCore::instance().isEOF()) {
                brls::Logger::info("Video EOF. Restarting.");
                MPVCore::instance().restart();
                this->toggleOSD(false);
            } else if (MPVCore::instance().isPaused()) {
                brls::Logger::info("Resuming video");
                MPVCore::instance().resume();
                this->toggleOSD(false);
            } else {
                brls::Logger::info("Pausing video");
                MPVCore::instance().pause();
                this->toggleOSD(true);
            }
            return true;
        });

        this->registerAction("Back", brls::BUTTON_B, [this](brls::View* view) {
            if (osdVisible) {
                this->toggleOSD(false);
            } else {
                brls::Application::popActivity();
            }
            return true;
        });

        this->registerAction("Seek Forward", brls::BUTTON_RIGHT, [this](brls::View* view) {
            brls::Logger::info("Seek +10s");
            MPVCore::instance().seek(10);
            this->toggleOSD(true); // Show OSD when seeking
            return true;
        });
        
        this->registerAction("Seek Backward", brls::BUTTON_LEFT, [this](brls::View* view) {
            brls::Logger::info("Seek -10s");
            MPVCore::instance().seek(-10);
            this->toggleOSD(true); // Show OSD when seeking
            return true;
        });

        this->registerAction("Show OSD", brls::BUTTON_UP, [this](brls::View* view) {
            this->toggleOSD(true);
            return true;
        });

        this->registerAction("Hide OSD", brls::BUTTON_DOWN, [this](brls::View* view) {
            this->toggleOSD(false);
            return true;
        });
    }

    void PlayerOverlayView::toggleOSD(bool show) {
        osdVisible = show;
        if (osdVisible) {
            // Show OSD
            topBar->setVisibility(brls::Visibility::VISIBLE);
            bottomBar->setVisibility(brls::Visibility::VISIBLE);
            this->setBackgroundColor(nvgRGBA(0, 0, 0, 160)); // Darker overlay for YouTube TV feel
            osdLastTick = brls::getCPUTimeUsec() / 1000;
        } else {
            // Hide OSD
            topBar->setVisibility(brls::Visibility::INVISIBLE);
            bottomBar->setVisibility(brls::Visibility::INVISIBLE);
            this->setBackgroundColor(nvgRGBA(0, 0, 0, 0));
        }
    }

    void PlayerOverlayView::frame(brls::FrameContext* ctx) {
        // Poll for buffering state and toggle the loader
        if (MPVCore::instance().isBuffering()) {
            if (bufferingLoader->getVisibility() != brls::Visibility::VISIBLE) {
                bufferingLoader->setVisibility(brls::Visibility::VISIBLE);
            }
        } else {
            if (bufferingLoader->getVisibility() == brls::Visibility::VISIBLE) {
                bufferingLoader->setVisibility(brls::Visibility::INVISIBLE);
            }
        }
        
        // Auto-hide OSD
        if (osdVisible && !MPVCore::instance().isPaused() && !MPVCore::instance().isEOF()) {
            if (brls::getCPUTimeUsec() / 1000 - osdLastTick > osdTimeout) {
                this->toggleOSD(false);
            }
        }

        // Show OSD when video ends
        if (MPVCore::instance().isEOF() && !osdVisible) {
            this->toggleOSD(true);
        }

        // Update playback info
        updatePlaybackInfo();
        
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

        brls::Label* videoTitleLabel = new brls::Label();
        videoTitleLabel->setText(videoTitle + " | DarkTube");
        videoTitleLabel->setFontSize(28);
        videoTitleLabel->setTextColor(Theme::TextPrimary);
        bar->addView(videoTitleLabel);

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

        // YouTube Style Red progress fill
        progressFill = new brls::Box();
        progressFill->setWidthPercentage(0); 
        progressFill->setHeight(8);
        progressFill->setBackgroundColor(Theme::AccentRed);
        progressFill->setCornerRadius(4);
        track->addView(progressFill);

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
        timeLabel->setText("0:00 / 0:00");
        timeLabel->setFontSize(20);
        timeLabel->setTextColor(Theme::TextSecondary);
        leftGroup->addView(timeLabel);

        bar->addView(leftGroup);

        brls::Box* rightGroup = new brls::Box();
        rightGroup->setAxis(brls::Axis::ROW);
        rightGroup->setAlignItems(brls::AlignItems::CENTER);

        rightGroup->addView(UIUtils::createHint(nvgRGB(50, 160, 60), "A", "Play/Pause"));
        rightGroup->addView(UIUtils::createHint(nvgRGB(220, 180, 0), "Left/Right", "Seek"));
        rightGroup->addView(UIUtils::createHint(nvgRGB(200, 40, 40), "B", "Back"));

        bar->addView(rightGroup);

        return bar;
    }

    void PlayerOverlayView::updatePlaybackInfo() {
        auto& mpv = MPVCore::instance();
        
        // Update status label
        if (mpv.isBuffering()) {
            statusLabel->setText("BUFFERING...");
        } else if (mpv.isEOF()) {
            statusLabel->setText("FINISHED");
        } else if (mpv.isPaused()) {
            statusLabel->setText("PAUSED");
        } else {
            statusLabel->setText("PLAYING");
        }

        // Update time label
        double currentTime = mpv.getPlaybackTime();
        double duration = mpv.getDuration();
        timeLabel->setText(formatTime(currentTime) + " / " + formatTime(duration));

        // Update progress bar
        float progress = mpv.getPlaybackProgress();
        progressFill->setWidthPercentage(progress * 100.0f);
    }

    std::string PlayerOverlayView::formatTime(double seconds) {
        if (seconds < 0) seconds = 0;
        int h = (int)(seconds / 3600);
        int m = (int)((seconds - h * 3600) / 60);
        int s = (int)(seconds - h * 3600 - m * 60);

        char buf[32];
        if (h > 0) {
            snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
        } else {
            snprintf(buf, sizeof(buf), "%d:%02d", m, s);
        }
        return std::string(buf);
    }

    void PlayerOverlayView::onFocusGained() {
        brls::Box::onFocusGained();
    }

    void PlayerOverlayView::onFocusLost() {
        // We don't want to hide OSD automatically on focus lost if it's supposed to be visible
        // But for TV interaction, usually focus stays on the overlay anyway
        brls::Box::onFocusLost();
    }

    // --- PlayerActivity ---

    PlayerActivity::PlayerActivity(const std::string& url, const std::string& title, const std::string& videoId) 
        : videoUrl(url), videoTitle(title), videoId(videoId) {
        brls::Logger::info("User pushed PlayerActivity: " + videoTitle);
        
        if (videoUrl.empty() && !videoId.empty()) {
            brls::Logger::info("URL empty, fetching stream for ID: {}", videoId);
            Data::NetworkClient::instance().getStream(videoId, [this](const std::string& url, const std::string& error) {
                if (!error.empty()) {
                    brls::Logger::error("Failed to fetch stream: {}", error);
                    // Show error in OSD or pop activity?
                    // For now just pop back
                    brls::Application::popActivity();
                    return;
                }
                this->videoUrl = url;
                MPVCore::instance().setUrl(this->videoUrl);
                MPVCore::instance().resume();
            });
        } else if (!videoUrl.empty()) {
            MPVCore::instance().setUrl(videoUrl);
            MPVCore::instance().resume();
        }
    }

    PlayerActivity::~PlayerActivity() {
        brls::Logger::info("PlayerActivity destroyed. Stopping media.");
        MPVCore::instance().stop();
    }

    brls::View* PlayerActivity::createContentView() {
        VideoPlayerView* videoView = new VideoPlayerView();
        
        // Add overlay that fades in/out on interaction
        PlayerOverlayView* overlay = new PlayerOverlayView(videoTitle);
        videoView->addView(overlay);

        // Ensure the overlay gets focus so A/B inputs are not swallowed by the VideoView
        brls::Application::giveFocus(overlay);

        return videoView;
    }

} // namespace Presentation
} // namespace DarkTube
