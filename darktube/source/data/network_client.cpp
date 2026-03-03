#include "../include/data/network_client.hpp"
#include "../include/data/ip_repository.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/core/thread.hpp>

using json = nlohmann::json;

namespace DarkTube {
namespace Data {

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    NetworkClient::NetworkClient() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    NetworkClient::~NetworkClient() {
        curl_global_cleanup();
    }

    std::string NetworkClient::getBaseUrl() {
        auto server = IPRepository::getInstance().getActiveServer();
        if (server.address.empty()) return "";
        
        // Ensure protocol
        if (server.address.find("http") == std::string::npos) {
            return "http://" + server.address + ":3000";
        }
        return server.address;
    }

    std::string NetworkClient::performGet(const std::string& url) {
        CURL* curl;
        CURLcode res;
        std::string readBuffer;

        curl = curl_easy_init();
        if (curl) {
            brls::Logger::info("Network: GET {}", url);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // 10s timeout
            
            // Disable SSL verify for local servers if needed
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                brls::Logger::error("Network: curl_easy_perform() failed: {}", curl_easy_strerror(res));
                readBuffer = "";
            }
            curl_easy_cleanup(curl);
        }
        return readBuffer;
    }

    void NetworkClient::getTrending(Callback cb) {
        brls::async([this, cb]() {
            std::string baseUrl = getBaseUrl();
            if (baseUrl.empty()) {
                brls::sync([cb]() { cb({}, "No server configured"); });
                return;
            }

            std::string response = performGet(baseUrl + "/api/trending?maxResults=20");
            std::vector<Domain::VideoItem> videos;
            std::string error = "";

            try {
                if (response.empty()) throw std::runtime_error("Empty response");
                
                json j = json::parse(response);
                if (j.contains("videos")) {
                    for (auto& item : j["videos"]) {
                        Domain::VideoItem video;
                        video.id = item.value("id", "");
                        video.title = item.value("title", "No Title");
                        video.author = item.value("channelTitle", "Unknown");
                        
                        if (item.contains("thumbnails") && item["thumbnails"].contains("medium")) {
                            video.thumbnailUrlMedium = item["thumbnails"]["medium"].value("url", "");
                        }
                        
                        video.views = item.contains("statistics") ? item["statistics"].value("viewCount", "0") : "0";
                        video.date = item.value("publishedAt", "");
                        
                        videos.push_back(video);
                    }
                }
            } catch (const std::exception& e) {
                error = e.what();
                brls::Logger::error("Network: getTrending failed: {}", error);
            }

            brls::sync([cb, videos, error]() { cb(videos, error); });
        });
    }

    void NetworkClient::search(const std::string& query, Callback cb) {
        brls::async([this, query, cb]() {
            std::string baseUrl = getBaseUrl();
            if (baseUrl.empty()) {
                brls::sync([cb]() { cb({}, "No server configured"); });
                return;
            }

            char* encoded = curl_easy_escape(nullptr, query.c_str(), query.length());
            std::string url = baseUrl + "/api/search?q=" + std::string(encoded) + "&maxResults=20&type=video";
            curl_free(encoded);

            std::string response = performGet(url);
            std::vector<Domain::VideoItem> videos;
            std::string error = "";

            try {
                if (response.empty()) throw std::runtime_error("Empty response");
                
                json j = json::parse(response);
                if (j.contains("videos")) {
                    for (auto& item : j["videos"]) {
                        std::string id = item.value("id", "");
                        if (id.empty()) continue;

                        Domain::VideoItem video;
                        video.id = id;
                        video.title = item.value("title", "No Title");
                        
                        // New model: channel is an object with "name"
                        if (item.contains("channel")) {
                            video.author = item["channel"].value("name", "Unknown");
                        } else {
                            video.author = item.value("channelTitle", "Unknown");
                        }
                        
                        if (item.contains("thumbnails") && item["thumbnails"].contains("medium")) {
                            video.thumbnailUrlMedium = item["thumbnails"]["medium"].value("url", "");
                        }
                        
                        video.date = item.value("publishedAt", "");
                        video.views = "SEARCH_HIDDEN"; // Marker to hide views in HomeActivity
                        videos.push_back(video);
                    }
                }
            } catch (const std::exception& e) {
                error = e.what();
            }

            brls::sync([cb, videos, error]() { cb(videos, error); });
        });
    }

    void NetworkClient::getStream(const std::string& videoId, StreamCallback cb) {
        brls::async([this, videoId, cb]() {
            std::string baseUrl = getBaseUrl();
            if (baseUrl.empty()) {
                brls::sync([cb]() { cb("", "No server configured"); });
                return;
            }

            std::string response = performGet(baseUrl + "/api/stream?id=" + videoId);
            std::string streamUrl = "";
            std::string error = "";

            try {
                if (response.empty()) throw std::runtime_error("Empty response");
                
                json j = json::parse(response);
                if (j.contains("url")) {
                    streamUrl = j["url"];
                } else if (j.contains("error")) {
                    error = j["error"];
                } else {
                    error = "Unknown error extracting stream";
                }
            } catch (const std::exception& e) {
                error = e.what();
            }

            brls::sync([cb, streamUrl, error]() { cb(streamUrl, error); });
        });
    }

    void NetworkClient::fetchImage(const std::string& url, std::function<void(const unsigned char* data, size_t size)> cb) {
        brls::async([this, url, cb]() {
            std::string response = performGet(url);
            if (!response.empty()) {
                // Return data to main thread
                // Copy data to a vector to safely pass it
                std::vector<unsigned char> data(response.begin(), response.end());
                brls::sync([cb, data]() {
                    cb(data.data(), data.size());
                });
            } else {
                brls::sync([cb]() { cb(nullptr, 0); });
            }
        });
    }

} // namespace Data
} // namespace DarkTube
