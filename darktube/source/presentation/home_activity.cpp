#include "../include/presentation/home_activity.hpp"
#include "../include/presentation/server_list_activity.hpp"
#include "../include/presentation/player_activity.hpp"
#include "../include/core/theme.hpp"
#include "../include/data/ip_repository.hpp"
#include "../include/data/network_client.hpp"
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
        root->addView(createFooterHints(false));

        // Initial fetch
        if (!isServerEmpty() && currentVideos.empty()) {
            fetchTrending();
        }

        // Global actions for this screen to toggle sidebar
        root->registerAction("Toggle Sidebar", brls::BUTTON_B, [this](brls::View* view) {
            this->toggleSidebar();
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
        bar->setFocusable(false); // Container should not be focusable itself

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
                    this->updateFooter(false);
                } else if (item == "Settings") {
                    this->renderSettingsView();
                    this->updateFooter(true);
                } else if (item == "Trending") {
                    this->fetchTrending();
                    this->updateFooter(false);
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
            header->setJustifyContent(brls::JustifyContent::SPACE_BETWEEN);
            header->setMarginBottom(30);

            this->miniLogo = new brls::Image();
            this->miniLogo->setImageFromFile("romfs:/img/logo_horizontal.png");
            this->miniLogo->setDimensions(140, 35);
            this->miniLogo->setScalingType(brls::ImageScalingType::FIT);
            this->miniLogo->setVisibility(sidebarVisible ? brls::Visibility::GONE : brls::Visibility::VISIBLE);
            header->addView(this->miniLogo);

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

            gridWrapper->addView(createCategoryRow(currentTitle, currentVideos));

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

    brls::Box* HomeActivity::createCategoryRow(const std::string& title, const std::vector<Domain::VideoItem>& videos) {
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

        if (videos.empty()) {
            brls::Box* loadingRow = new brls::Box();
            loadingRow->setAxis(brls::Axis::ROW);
            for (int i = 0; i < 4; i++) {
                loadingRow->addView(UIUtils::createSkeletonVideoCard());
            }
            section->addView(loadingRow);
        } else {
            brls::Box* currentRow = nullptr;
            for (size_t i = 0; i < videos.size(); ++i) {
                // Create a new row every 4 items
                if (i % 4 == 0) {
                    currentRow = new brls::Box();
                    currentRow->setAxis(brls::Axis::ROW);
                    section->addView(currentRow);
                }

                const auto& video = videos[i];
                brls::Box* cardContainer = new brls::Box();
                cardContainer->setAxis(brls::Axis::COLUMN);
                cardContainer->setMarginRight(25);
                cardContainer->setMarginBottom(30);
                cardContainer->setMarginLeft(10);
                cardContainer->setWidth(260); // Fixed width for grid alignment

                // Video Thumbnail (16:9)
                brls::Image* thumbnail = new brls::Image();
                thumbnail->setDimensions(256, 144);
                thumbnail->setScalingType(brls::ImageScalingType::FILL); // Use FILL/COVER
                thumbnail->setBackgroundColor(Theme::SurfaceDark);
                thumbnail->setFocusable(true);
                thumbnail->setCornerRadius(12);

                // Asynchronously fetch medium thumbnail
                if (!video.thumbnailUrlMedium.empty()) {
                    Data::NetworkClient::instance().fetchImage(video.thumbnailUrlMedium, [thumbnail](const unsigned char* data, size_t size) {
                        if (data && size > 0) thumbnail->setImageFromMem(data, size);
                        else thumbnail->setImageFromFile("romfs:/img/video_placeholder.png");
                    });
                } else {
                    thumbnail->setImageFromFile("romfs:/img/video_placeholder.png");
                }

                thumbnail->registerAction("Play", brls::BUTTON_A, [video](brls::View* view) {
                    brls::Logger::info("Play Video clicked: " + video.title);
                    brls::Application::pushActivity(new PlayerActivity("", video.title, video.id));
                    return true;
                });
                cardContainer->addView(thumbnail);

                // Metadata
                brls::Box* metadata = new brls::Box();
                metadata->setAxis(brls::Axis::COLUMN);
                metadata->setMarginTop(12);
                metadata->setWidth(256);

                brls::Label* vidTitle = new brls::Label();
                vidTitle->setText(video.title);
                vidTitle->setFontSize(18);
                vidTitle->setTextColor(Theme::TextPrimary);
                vidTitle->setMarginBottom(4);
                vidTitle->setSingleLine(true);
                metadata->addView(vidTitle);

                brls::Label* vidChannel = new brls::Label();
                std::string channelText = video.author;
                if (video.views != "SEARCH_HIDDEN") {
                    channelText += " • " + UIUtils::formatViewCount(video.views);
                }
                vidChannel->setText(channelText);
                vidChannel->setFontSize(14);
                vidChannel->setTextColor(Theme::TextSecondary);
                metadata->addView(vidChannel);

                cardContainer->addView(metadata);

                currentRow->addView(cardContainer);
            }
        }

        return section;
    }

    void HomeActivity::fetchTrending() {
        this->currentVideos.clear();
        this->currentTitle = "Trending Entertainment";
        this->renderVideoGrid({}); // Show skeletons

        Data::NetworkClient::instance().getTrending([this](const std::vector<Domain::VideoItem>& videos, const std::string& error) {
            if (!error.empty()) {
                brls::Logger::error("Failed to fetch trending: {}", error);
                return;
            }
            this->currentVideos = videos;
            this->renderVideoGrid(videos);
        });
    }

    void HomeActivity::renderVideoGrid(const std::vector<Domain::VideoItem>& videos) {
        // Re-render main content
        if (mainContent) {
            brls::Box* split = dynamic_cast<brls::Box*>(mainContent->getParent());
            if (split) {
                split->removeView(mainContent, true);
                mainContent = createMainContent();
                split->addView(mainContent);
                // Try to restore focus if possible
            }
        }
    }

    void HomeActivity::promptForSearch() {
        brls::Logger::info("Prompting for Search via IME...");
        
        brls::Application::getPlatform()->getImeManager()->openForText(
            [this](std::string text) {
                if (!text.empty()) {
                    brls::Logger::info("Searching for: " + text);
                    
                    this->currentVideos.clear();
                    this->currentTitle = "Searching: " + text;
                    this->renderVideoGrid({}); // Show skeletons
                    Data::NetworkClient::instance().search(text, [this, text](const std::vector<Domain::VideoItem>& videos, const std::string& error) {
                         if (!error.empty()) {
                            brls::Logger::error("Search failed: {}", error);
                            return;
                        }
                        this->currentTitle = "Search: " + text;
                        this->currentVideos = videos;
                        this->renderVideoGrid(videos);
                    });
                }
            },
            "Search DarkTube",
            "",
            255,
            "",
            0
        );
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

                // Add Edit (X) and Delete (Y) actions
                sBtn->registerAction("Edit Server", brls::BUTTON_X, [this, s](brls::View* v) {
                    this->promptForEditIP(s);
                    return true;
                });

                sBtn->registerAction("Delete Server", brls::BUTTON_Y, [this, s](brls::View* v) {
                    Data::IPRepository::getInstance().removeServer(s.id);
                    this->renderSettingsView(); // Refresh settings view
                    return true;
                });

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

    brls::Box* HomeActivity::createFooterHints(bool isSettings) {
        brls::Box* footer = new brls::Box();
        footer->setAxis(brls::Axis::ROW);
        footer->setHeight(30); // Made more compact
        footer->setWidthPercentage(100);
        footer->setAlignItems(brls::AlignItems::CENTER);
        footer->setPadding(0, 20, 0, 20); // Reduced padding
        footer->setBackgroundColor(Theme::SurfaceDark);
        
        footer->addView(UIUtils::createHint(nvgRGB(50, 160, 60), "A", "Select"));
        footer->addView(UIUtils::createHint(nvgRGB(220, 180, 0), "B", "Toggle Sidebar"));
        
        if (isSettings) {
             footer->addView(UIUtils::createHint(nvgRGB(100, 100, 255), "X", "Edit"));
             footer->addView(UIUtils::createHint(nvgRGB(255, 60, 60), "Y", "Delete"));
        } else if (!isServerEmpty()) {
             footer->addView(UIUtils::createHint(nvgRGB(100, 100, 255), "X", "Change Server"));
        }

        return footer;
    }

    void HomeActivity::toggleSidebar() {
        sidebarVisible = !sidebarVisible;
        if (sidebarVisible) {
            sidebar->setVisibility(brls::Visibility::VISIBLE);
            sidebar->setWidth(320); // Restore width
            
            // Re-enable focus on children
            for (auto child : sidebar->getChildren()) {
                child->setFocusable(true);
            }

            if (miniLogo) miniLogo->setVisibility(brls::Visibility::GONE);
            
            // Re-focus the first item in sidebar 
            if (sidebar->getChildren().size() > 1) { // logo + nav items
                 brls::Application::giveFocus(sidebar->getChildren()[1]); 
            }
        } else {
            // Invisible still takes space occasionally in flex, but Width=0 solves that
            sidebar->setVisibility(brls::Visibility::GONE); 
            sidebar->setWidth(0); 

            // Disable focus on children to prevent accidental focus on "empty" area
            for (auto child : sidebar->getChildren()) {
                child->setFocusable(false);
            }

            if (miniLogo) miniLogo->setVisibility(brls::Visibility::VISIBLE);

            // Put focus on main content area
            brls::Application::giveFocus(mainContent);
        }
    }

    void HomeActivity::promptForNewIP() {
        brls::Logger::info("Prompting for New IP via IME...");
        
        brls::Application::getPlatform()->getImeManager()->openForText(
            [this](std::string text) {
                if (!text.empty()) {
                    auto servers = Data::IPRepository::getInstance().getSavedServers();
                    if (servers.size() >= 4) {
                        brls::Dialog* dialog = new brls::Dialog("Server limit reached. You can only have up to 4 servers.");
                        dialog->addButton("OK", []() {});
                        dialog->open();
                        return;
                    }

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

    void HomeActivity::promptForEditIP(const Domain::ServerIP& server) {
        brls::Logger::info("Prompting for Edit IP via IME...");
        
        brls::Application::getPlatform()->getImeManager()->openForText(
            [this, server](std::string text) {
                if (!text.empty()) {
                    Domain::ServerIP updatedServer = server;
                    updatedServer.address = text;

                    Data::IPRepository::getInstance().updateServer(updatedServer);
                    this->renderSettingsView(); // Refresh settings view
                }
            },
            "Edit Server IP",
            server.address,
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

                // Update footer
                this->updateFooter(true);

                brls::Application::giveFocus(mainContent);
            }
        }
    }

    void HomeActivity::updateFooter(bool isSettings) {
        brls::Box* rootBox = dynamic_cast<brls::Box*>(this->getContentView());
        if (rootBox && rootBox->getChildren().size() >= 2) {
            brls::View* oldFooter = rootBox->getChildren()[1];
            rootBox->removeView(oldFooter, true);
            rootBox->addView(createFooterHints(isSettings));
        }
    }
} // namespace Presentation
} // namespace DarkTube
