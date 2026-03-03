#pragma once

#include <string>

namespace DarkTube {
namespace Domain {

    struct ServerIP {
        std::string id;
        std::string name;
        std::string address;
    };

    struct VideoItem {
        std::string id;
        std::string title;
        std::string author;
        std::string thumbnailUrl;
        std::string thumbnailUrlMedium;
        std::string streamUrl;
        std::string description;
        std::string views;
        std::string date;
    };

} // namespace Domain
} // namespace DarkTube
