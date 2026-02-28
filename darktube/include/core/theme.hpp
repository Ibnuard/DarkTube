#pragma once

#include <borealis.hpp>

// Define YouTube TV Dark theme colors and constants
namespace DarkTube {
namespace Theme {

    // Backgrounds
    const NVGcolor BackgroundDark = nvgRGB(15, 15, 15);     // Main app background
    const NVGcolor SurfaceDark = nvgRGB(33, 33, 33);        // Cards/Items background
    const NVGcolor SurfaceHover = nvgRGB(61, 61, 61);       // Highlighted item

    // Text
    const NVGcolor TextPrimary = nvgRGB(255, 255, 255);     // Main text
    const NVGcolor TextSecondary = nvgRGB(170, 170, 170);   // Subtitles, hints

    // Accents
    const NVGcolor AccentRed = nvgRGB(255, 0, 0);           // Selection borders or highlights

    // Overlay (for Player)
    const NVGcolor OverlayBackground = nvgRGBA(0, 0, 0, 180);

    // Apply global theme overrides to Borealis if needed
    inline void applyTheme() {
        brls::Theme::getLightTheme().addColor("brls/background", BackgroundDark);
        brls::Theme::getDarkTheme().addColor("brls/background", BackgroundDark);
        
        brls::Theme::getLightTheme().addColor("brls/text", TextPrimary);
        brls::Theme::getDarkTheme().addColor("brls/text", TextPrimary);

        brls::Theme::getLightTheme().addColor("brls/list/item_bg", SurfaceDark);
        brls::Theme::getDarkTheme().addColor("brls/list/item_bg", SurfaceDark);

        // Focus Highlight color overrides (Red Outline, Dark Background)
        NVGcolor focusBackground = nvgRGB(5, 5, 5); // Darker than main
        
        brls::Theme::getLightTheme().addColor("brls/highlight/background", focusBackground);
        brls::Theme::getDarkTheme().addColor("brls/highlight/background", focusBackground);
        
        brls::Theme::getLightTheme().addColor("brls/highlight/color1", AccentRed);
        brls::Theme::getDarkTheme().addColor("brls/highlight/color1", AccentRed);
        
        brls::Theme::getLightTheme().addColor("brls/highlight/color2", AccentRed); // No gradient, solid red
        brls::Theme::getDarkTheme().addColor("brls/highlight/color2", AccentRed);
        
        // Pulse ripple effect matches highlight
        brls::Theme::getLightTheme().addColor("brls/click_pulse", nvgRGBA(255, 0, 0, 38));
        brls::Theme::getDarkTheme().addColor("brls/click_pulse", nvgRGBA(255, 0, 0, 38));
    }

} // namespace Theme
} // namespace DarkTube
