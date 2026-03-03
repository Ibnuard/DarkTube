#include <borealis.hpp>

#ifdef __SWITCH__
#include <switch.h>
#endif

#include <borealis.hpp>

#ifdef __SWITCH__
#include <switch.h>
#endif

#include "view/mpv_core.hpp"
#include "../include/core/theme.hpp"
#include "../include/presentation/server_list_activity.hpp"
#include "../include/presentation/home_activity.hpp"
#include "../include/data/ip_repository.hpp"

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

    // Apply custom YouTube TV Dark theme
    DarkTube::Theme::applyTheme();

    // Set language
    std::string lang = DarkTube::Data::IPRepository::getInstance().getLanguage();
    brls::Application::setLanguage(lang);

    // Check saved servers but always route to HomeActivity
    auto servers = DarkTube::Data::IPRepository::getInstance().getSavedServers();
    if (!servers.empty()) {
        DarkTube::Data::IPRepository::getInstance().setActiveServer(servers.front());
    }

    brls::Application::pushActivity(new DarkTube::Presentation::HomeActivity());
    brls::Logger::info("DarkTube: HomeActivity pushed");

    // Main loop
    while (brls::Application::mainLoop()) {
    }

    brls::Logger::info("DarkTube: Clean exit");

    return EXIT_SUCCESS;
}
