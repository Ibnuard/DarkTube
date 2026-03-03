#pragma once

#include <borealis.hpp>
#include "../include/core/theme.hpp"

namespace DarkTube {
namespace Presentation {
namespace UIUtils {

    inline brls::Box* createHint(NVGcolor color, const std::string& key, const std::string& text) {
        brls::Box* hint = new brls::Box();
        hint->setAxis(brls::Axis::ROW);
        hint->setAlignItems(brls::AlignItems::CENTER);
        hint->setMarginRight(20);

        brls::Box* keyBox = new brls::Box();
        keyBox->setBackgroundColor(color);
        keyBox->setCornerRadius(8);
        keyBox->setPadding(2, 8, 2, 8);
        keyBox->setMarginRight(8);
        keyBox->setAlignItems(brls::AlignItems::CENTER);
        keyBox->setJustifyContent(brls::JustifyContent::CENTER);

        brls::Label* keyLabel = new brls::Label();
        keyLabel->setText(key);
        keyLabel->setFontSize(14);
        keyLabel->setTextColor(nvgRGB(255, 255, 255));
        
        keyBox->addView(keyLabel);
        hint->addView(keyBox);

        brls::Label* textLabel = new brls::Label();
        textLabel->setText(text);
        textLabel->setFontSize(18);
        textLabel->setTextColor(DarkTube::Theme::TextSecondary);
        hint->addView(textLabel);

        return hint;
    }

    inline std::string formatViewCount(const std::string& views) {
        if (views.empty()) return "0 views";
        try {
            long long count = std::stoll(views);
            if (count >= 1000000000) return std::to_string(count / 1000000000) + "B views";
            if (count >= 1000000) return std::to_string(count / 1000000) + "M views";
            if (count >= 1000) return std::to_string(count / 1000) + "K views";
            return views + " views";
        } catch (...) {
            return views + " views";
        }
    }

    // Helper for circular avatar
    inline void makeCircular(brls::Image* img, float size) {
        img->setDimensions(size, size);
        img->setCornerRadius(size / 2.0f);
    }

    // Simple pulse animation to simulate shimmer
    inline void pulseView(brls::View* view) {
        // Borealis views have an internal highlightAlpha or we can just use a custom animatable
        // But for simplicity let's just use a label or box and change its opacity if possible
        // Actually, let's just use a ProgressSpinner for now as it's built-in 
        // OR implement a very simple recurring timer
    }

    inline brls::Box* createSkeletonVideoCard() {
        brls::Box* card = new brls::Box();
        card->setAxis(brls::Axis::COLUMN);
        card->setMarginRight(25);
        card->setMarginBottom(30);
        card->setMarginLeft(10);
        card->setWidth(256);

        brls::Box* thumb = new brls::Box();
        thumb->setDimensions(256, 144);
        thumb->setBackgroundColor(DarkTube::Theme::SurfaceDark);
        thumb->setCornerRadius(12);
        card->addView(thumb);

        brls::Box* meta = new brls::Box();
        meta->setMarginTop(12);
        meta->setDimensions(200, 20);
        meta->setBackgroundColor(DarkTube::Theme::SurfaceDark);
        meta->setCornerRadius(4);
        card->addView(meta);

        return card;
    }

} // namespace UIUtils
} // namespace Presentation
} // namespace DarkTube
