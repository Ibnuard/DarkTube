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

} // namespace UIUtils
} // namespace Presentation
} // namespace DarkTube
