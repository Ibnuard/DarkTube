#include "../include/presentation/home_activity.hpp"
#include "../include/presentation/server_list_activity.hpp"
#include "../include/presentation/player_activity.hpp"
#include "../include/core/theme.hpp"
#include "../include/data/ip_repository.hpp"
#include <borealis.hpp>
#include "../include/presentation/ui_utils.hpp"

namespace {
    class SidebarItem : public brls::Box {
    private:
        brls::Label* label;
    public:
        SidebarItem(const std::string& title) {
            this->setFocusable(true);
            this->setPadding(15, 20, 15, 20);
            this->setCornerRadius(8);
            this->setMarginBottom(10);
            
            label = new brls::Label();
            label->setText(title);
            label->setFontSize(22);
            label->setTextColor(DarkTube::Theme::TextSecondary); 
            
            this->addView(label);
        }

        void onFocusGained() override {
            brls::Box::onFocusGained();
            label->setTextColor(nvgRGB(255, 60, 60)); // Red text when focused
        }

        void onFocusLost() override {
            brls::Box::onFocusLost();
            label->setTextColor(DarkTube::Theme::TextSecondary); // Revert when lost focus
        }
    };
}

namespace DarkTube {
namespace Presentation {

    HomeActivity::HomeActivity() {
        brls::Logger::info("HomeActivity created");
    }

    bool HomeActivity::isServerEmpty() {
        return Data::IPRepository::getInstance().getSavedServers().empty();
    }

    brls::View* HomeActivity::createContentView() {
        brls::Box* root = new brls::Box();
        root->setAxis(brls::Axis::COLUMN);
        root->setWidthPercentage(100);
        root->setHeightPercentage(100);
        root->setBackgroundColor(Theme::BackgroundDark); // Main background (navgRGB(15, 15, 15)) to contrast with sidebar
        
        // Main split: Sidebar + Content
        brls::Box* split = new brls::Box();
        split->setAxis(brls::Axis::ROW);
        split->setGrow(1.0f);
        split->setWidthPercentage(100);

        sidebar = createSidebar();
        mainContent = createMainContent();

        split->addView(sidebar);
        split->addView(mainContent);

        root->addView(split);
        root->addView(createFooterHints());

        // Global actions for this screen to toggle sidebar
        root->registerAction("Toggle Sidebar", brls::BUTTON_START, [this](brls::View* view) {
            this->toggleSidebar();
            return true;
        });

        root->registerAction("Back / Hide", brls::BUTTON_B, [this](brls::View* view) {
            if (this->sidebarVisible) {
                this->toggleSidebar();
            }
            return true;
        });

        // X Button to Change Server
        root->registerAction("Change Server", brls::BUTTON_X, [this](brls::View* view) {
            if (!this->isServerEmpty()) {
                this->promptForNewIP();
            }
            return true;
        });

        return root;
    }

    brls::Box* HomeActivity::createSidebarItem(const std::string& title, std::function<bool(brls::View*)> onClick) {
        SidebarItem* item = new SidebarItem(title);
        item->registerAction("Select", brls::BUTTON_A, onClick);

        // Highlight visual using background color change on focus is native
        item->registerAction("Focus", brls::BUTTON_NAV_UP, [](brls::View* v){return false;}); 

        return item;
    }

    brls::Box* HomeActivity::createSidebar() {
        brls::Box* bar = new brls::Box();
        bar->setAxis(brls::Axis::COLUMN);
        bar->setWidth(320);
        bar->setHeightPercentage(100);
        bar->setBackgroundColor(nvgRGB(0x12, 0x12, 0x16)); // #121216 Sidebar background
        bar->setPadding(40, 20, 40, 20);

        brls::Image* logo = new brls::Image();
        logo->setImageFromFile("romfs:/img/logo_horizontal.png"); // Using PNG instead of JPG
        logo->setWidth(200);
        logo->setHeight(50);
        logo->setScalingType(brls::ImageScalingType::FIT);
        logo->setMarginBottom(40);
        logo->setMarginLeft(20);
        bar->addView(logo);

        std::vector<std::string> navItems;
        navItems.push_back("Trending");
        // Only show search if there is a connected IP
        if (!isServerEmpty()) {
            navItems.push_back("Search");
        }
        navItems.push_back("Settings");

        for (const auto& item : navItems) {
            bar->addView(createSidebarItem(item, [item, this](brls::View* view) {
                brls::Logger::info("Navigated to " + item);
                if (item == "Search") {
                    this->promptForSearch();
                } else if (item == "Settings") {
                    this->renderSettingsView();
                } else if (item == "Trending") {
                    // Re-render main content
                    brls::Application::popActivity();
                    brls::Application::pushActivity(new HomeActivity());
                }
                return true;
            }));
        }

        return bar;
    }

    brls::Box* HomeActivity::createMainContent() {
        brls::Box* content = new brls::Box();
        content->setAxis(brls::Axis::COLUMN);
        content->setGrow(1.0f);
        content->setPadding(40, 40, 0, 40);

        if (isServerEmpty()) {
            content->addView(createEmptyStateView());
        } else {
            // Header (Network/Account status)
            brls::Box* header = new brls::Box();
            header->setAxis(brls::Axis::ROW);
            header->setWidthPercentage(100);
            header->setJustifyContent(brls::JustifyContent::FLEX_END);
            header->setMarginBottom(30);

            brls::Label* ipLabel = new brls::Label();
            auto activeServer = Data::IPRepository::getInstance().getActiveServer();
            ipLabel->setText("SERVER: " + activeServer.address);
            ipLabel->setFontSize(18);
            ipLabel->setTextColor(Theme::TextSecondary);
            header->addView(ipLabel);
            content->addView(header);

            // Scrolling Grid area
            brls::ScrollingFrame* gridContainer = new brls::ScrollingFrame();
            gridContainer->setGrow(1.0f);
            
            brls::Box* gridWrapper = new brls::Box();
            gridWrapper->setAxis(brls::Axis::COLUMN);

            gridWrapper->addView(createCategoryRow("Trending Entertainment"));

            gridContainer->setContentView(gridWrapper);
            content->addView(gridContainer);
        }

        return content;
    }

    brls::Box* HomeActivity::createEmptyStateView() {
        brls::Box* state = new brls::Box();
        state->setAxis(brls::Axis::COLUMN);
        state->setGrow(1.0f);
        state->setAlignItems(brls::AlignItems::CENTER);
        state->setJustifyContent(brls::JustifyContent::CENTER);

        brls::Image* logo = new brls::Image();
        logo->setImageFromFile("romfs:/img/logo_horizontal.png"); 
        logo->setWidth(300);
        logo->setHeight(80);
        logo->setScalingType(brls::ImageScalingType::FIT);
        logo->setMarginBottom(30);
        state->addView(logo);

        brls::Label* title = new brls::Label();
        title->setText("Welcome to DarkTube");
        title->setFontSize(36);
        title->setTextColor(Theme::TextPrimary);
        title->setMarginBottom(10);
        state->addView(title);

        brls::Label* desc = new brls::Label();
        desc->setText("Please add your server IP to view videos.");
        desc->setFontSize(22);
        desc->setTextColor(Theme::TextSecondary);
        desc->setMarginBottom(40);
        state->addView(desc);

        brls::Box* btn = createSidebarItem("Add Server IP", [this](brls::View* v) {
            this->promptForNewIP();
            return true;
        });
        btn->setBackgroundColor(nvgRGB(40, 40, 45)); 
        btn->setWidth(brls::View::AUTO); // Allow padding to dictate size naturally
        btn->setPadding(20, 60, 20, 60); // Rectangular but padded
        btn->setCornerRadius(10); // Nice rounded corners without being pill
        btn->setJustifyContent(brls::JustifyContent::CENTER);
        
        state->addView(btn);

        return state;
    }

    brls::Box* HomeActivity::createCategoryRow(const std::string& title) {
        brls::Box* section = new brls::Box();
        section->setAxis(brls::Axis::COLUMN);
        section->setMarginBottom(40);

        brls::Label* titleLabel = new brls::Label();
        titleLabel->setText(title);
        titleLabel->setFontSize(28);
        titleLabel->setTextColor(Theme::TextPrimary);
        titleLabel->setMarginBottom(20);
        titleLabel->setMarginLeft(10);
        section->addView(titleLabel);

        brls::Box* row = new brls::Box();
        row->setAxis(brls::Axis::ROW);

        for (int c = 0; c < 3; ++c) {
            brls::Box* cardContainer = new brls::Box();
            cardContainer->setAxis(brls::Axis::COLUMN);
            cardContainer->setMarginRight(25); // Added gap
            cardContainer->setMarginLeft(10); // Spacing for focus ring

            // Video Thumbnail (16:9)
            brls::Box* thumbnail = new brls::Box();
            thumbnail->setWidth(256); 
            thumbnail->setHeight(144);
            thumbnail->setBackgroundColor(Theme::SurfaceDark); // Placeholder
            thumbnail->setFocusable(true);
            thumbnail->setCornerRadius(12);
            
            // Action to play
            std::string url = "https://cdn.brid.tv/live/partners/6205/sd/69838.mp4";
            std::string title = "Trending Video Title " + std::to_string(c+1);

            if (c == 0) {
                title = "Live Stream";
                url = "https://cdn.brid.tv/live/partners/6205/sd/69838.mp4";
            } else if (c == 1) {
                title = "SD Card Test";
                url = "sdmc:/test.mp4";
            }

            thumbnail->registerAction("Play", brls::BUTTON_A, [url, title](brls::View* view) {
                brls::Logger::info("Play Video clicked: " + title);
                brls::Application::pushActivity(new PlayerActivity(url, title));
                return true;
            });
            cardContainer->addView(thumbnail);

            // Metadata Row (Avatar + Text)
            brls::Box* metadata = new brls::Box();
            metadata->setAxis(brls::Axis::ROW);
            metadata->setMarginTop(12);

            // Channel Avatar placeholder
            brls::Box* avatar = new brls::Box();
            avatar->setWidth(40);
            avatar->setHeight(40);
            avatar->setCornerRadius(20);
            avatar->setBackgroundColor(nvgRGB(80, 80, 80));
            avatar->setMarginRight(15);
            metadata->addView(avatar);

            // Text Info
            brls::Box* textGroup = new brls::Box();
            textGroup->setAxis(brls::Axis::COLUMN);
            
            brls::Label* vidTitle = new brls::Label();
            vidTitle->setText(title);
            vidTitle->setFontSize(18);
            vidTitle->setTextColor(Theme::TextPrimary);
            vidTitle->setMarginBottom(4);
            textGroup->addView(vidTitle);

            brls::Label* vidChannel = new brls::Label();
            vidChannel->setText("DarkTube Trending • 1.2M views");
            vidChannel->setFontSize(14);
            vidChannel->setTextColor(Theme::TextSecondary);
            textGroup->addView(vidChannel);

            metadata->addView(textGroup);
            cardContainer->addView(metadata);

            row->addView(cardContainer);
        }

        section->addView(row);
        return section;
    }

    brls::Box* HomeActivity::createSettingsView() {
        brls::Box* content = new brls::Box();
        content->setAxis(brls::Axis::COLUMN);
        content->setGrow(1.0f);
        content->setPadding(40, 40, 0, 40);

        brls::Label* title = new brls::Label();
        title->setText("Settings & Info");
        title->setFontSize(36);
        title->setTextColor(Theme::TextPrimary);
        title->setMarginBottom(30);
        content->addView(title);

        brls::ScrollingFrame* scroll = new brls::ScrollingFrame();
        scroll->setGrow(1.0f);
        
        brls::Box* inner = new brls::Box();
        inner->setAxis(brls::Axis::COLUMN);

        auto addSection = [&inner](const std::string& headerText, const std::string& bodyText) {
            brls::Label* header = new brls::Label();
            header->setText(headerText);
            header->setFontSize(24);
            header->setTextColor(Theme::TextPrimary);
            header->setMarginTop(20);
            header->setMarginBottom(10);
            inner->addView(header);

            brls::Label* body = new brls::Label();
            body->setText(bodyText);
            body->setFontSize(18);
            body->setTextColor(Theme::TextSecondary);
            body->setMarginBottom(20);
            inner->addView(body);
        };

        // --- IP LIST SECTION ---
        brls::Label* ipHeader = new brls::Label();
        ipHeader->setText("Server Configuration");
        ipHeader->setFontSize(24);
        ipHeader->setTextColor(Theme::TextPrimary);
        ipHeader->setMarginTop(10);
        ipHeader->setMarginBottom(10);
        inner->addView(ipHeader);

        brls::Label* activeIpLabel = new brls::Label();
        if (isServerEmpty()) {
            activeIpLabel->setText("Active Server: None");
        } else {
            auto server = Data::IPRepository::getInstance().getActiveServer();
            activeIpLabel->setText("Active Server: " + server.address);
        }
        activeIpLabel->setFontSize(18);
        activeIpLabel->setTextColor(Theme::TextSecondary);
        activeIpLabel->setMarginBottom(20);
        inner->addView(activeIpLabel);

        // List servers
        auto servers = Data::IPRepository::getInstance().getSavedServers();
        if (!servers.empty()) {
            for (const auto& s : servers) {
                brls::Box* sBtn = createSidebarItem("Server: " + s.address, [s](brls::View* v) {
                    Data::IPRepository::getInstance().setActiveServer(s);
                    brls::Application::popActivity();
                    brls::Application::pushActivity(new HomeActivity());
                    return true;
                });
                sBtn->setMarginBottom(5);
                inner->addView(sBtn);
            }
        }

        // Add new server button
        brls::Box* addBtn = createSidebarItem("Add New Server", [this](brls::View* v) {
            this->promptForNewIP();
            return true;
        });
        addBtn->setMarginTop(5);
        addBtn->setMarginBottom(30);
        inner->addView(addBtn);

        // --- INFO SECTIONS ---

        addSection("Developer Info", "Developed by the DarkTube Team.\nProviding a custom YouTube client experience on Nintendo Switch.");
        addSection("Instructions", "1. Start the DarkTube Backend Server on your PC/NAS.\n2. Input the local IP Address (e.g., 192.168.1.10) in the app.\n3. Browse Trending or Search for videos directly.");
        addSection("GitHub", "https://github.com/DarkTube");
        addSection("Changelog (v1.0)", "- Initial Release\n- Custom YouTube TV Grid UI\n- MPV Hardware Accelerated Player\n- Settings Panel added");

        scroll->setContentView(inner);
        content->addView(scroll);

        return content;
    }

    brls::Box* HomeActivity::createFooterHints() {
        brls::Box* footer = new brls::Box();
        footer->setAxis(brls::Axis::ROW);
        footer->setHeight(30); // Made more compact
        footer->setWidthPercentage(100);
        footer->setAlignItems(brls::AlignItems::CENTER);
        footer->setPadding(0, 20, 0, 20); // Reduced padding
        footer->setBackgroundColor(Theme::SurfaceDark);
        
        footer->addView(UIUtils::createHint(nvgRGB(50, 160, 60), "A", "Select"));
        footer->addView(UIUtils::createHint(nvgRGB(220, 180, 0), "+", "Toggle Sidebar"));
        
        if (!isServerEmpty()) {
             footer->addView(UIUtils::createHint(nvgRGB(100, 100, 255), "X", "Change Server"));
        }

        return footer;
    }

    void HomeActivity::toggleSidebar() {
        sidebarVisible = !sidebarVisible;
        if (sidebarVisible) {
            sidebar->setVisibility(brls::Visibility::VISIBLE);
            sidebar->setWidth(320); // Restore width
            
            // Re-focus the first item in sidebar 
            if (sidebar->getChildren().size() > 1) { // logo + nav items
                 brls::Application::giveFocus(sidebar->getChildren()[1]); 
            }
        } else {
            // Invisible still takes space occasionally in flex, but Width=0 solves that
            sidebar->setVisibility(brls::Visibility::INVISIBLE); 
            sidebar->setWidth(0); 

            // Put focus on main content area
            brls::Application::giveFocus(mainContent);
        }
    }

    void HomeActivity::promptForNewIP() {
        brls::Logger::info("Prompting for New IP via IME...");
        
        brls::Application::getPlatform()->getImeManager()->openForText(
            [this](std::string text) {
                if (!text.empty()) {
                    Domain::ServerIP newIp;
                    newIp.id = std::to_string(brls::getCPUTimeUsec());
                    newIp.name = "My Server";
                    newIp.address = text;

                    Data::IPRepository::getInstance().addServer(newIp);
                    Data::IPRepository::getInstance().setActiveServer(newIp);

                    brls::Logger::info("IP added. Pushing HomeActivity again to reload layout.");
                    brls::Application::popActivity(); // Pop self
                    brls::Application::pushActivity(new HomeActivity()); // Reload
                }
            },
            "Enter Server IP",
            "", // Default empty
            255,
            "",
            0
        );
    }

    void HomeActivity::promptForSearch() {
        brls::Logger::info("Prompting for Search via IME...");
        
        brls::Application::getPlatform()->getImeManager()->openForText(
            [this](std::string text) {
                if (!text.empty()) {
                    brls::Logger::info("Searching for: " + text);
                    // For now, reload home activity to reset focus, later will load search results
                    brls::Application::popActivity();
                    brls::Application::pushActivity(new HomeActivity());
                }
            },
            "Search DarkTube",
            "",
            255,
            "",
            0
        );
    }

    void HomeActivity::renderSettingsView() {
        // Replace main content with settings view
        brls::Box* rootBox = dynamic_cast<brls::Box*>(this->getContentView());
        if (rootBox && rootBox->getChildren().size() > 0) {
            brls::Box* split = dynamic_cast<brls::Box*>(rootBox->getChildren()[0]);
            if (split && split->getChildren().size() >= 2) {
                split->removeView(mainContent, true); // true to free memory
                mainContent = createSettingsView();
                split->addView(mainContent);
                brls::Application::giveFocus(mainContent);
            }
        }
    }

} // namespace Presentation
} // namespace DarkTube
