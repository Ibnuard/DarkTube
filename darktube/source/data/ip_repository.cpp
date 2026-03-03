#include "../include/data/ip_repository.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <borealis/core/logger.hpp>

using json = nlohmann::json;

#ifdef __SWITCH__
#define CONFIG_PATH "sdmc:/darktube_config.json"
#else
#define CONFIG_PATH "darktube_config.json"
#endif

namespace DarkTube {
namespace Data {

    IPRepository::IPRepository() {
        m_activeServer = {"", "", ""};
        loadFromFile();
    }

    std::vector<Domain::ServerIP> IPRepository::getSavedServers() {
        return m_servers;
    }

    void IPRepository::addServer(const Domain::ServerIP& server) {
        m_servers.push_back(server);
        saveToFile();
    }

    void IPRepository::removeServer(const std::string& id) {
        for (auto it = m_servers.begin(); it != m_servers.end(); ++it) {
            if (it->id == id) {
                m_servers.erase(it);
                saveToFile();
                break;
            }
        }
    }

    void IPRepository::setActiveServer(const Domain::ServerIP& server) {
        m_activeServer = server;
        saveToFile();
    }

    void IPRepository::setLanguage(const std::string& lang) {
        m_language = lang;
        saveToFile();
    }

    void IPRepository::updateServer(const Domain::ServerIP& server) {
        for (auto& s : m_servers) {
            if (s.id == server.id) {
                s.name = server.name;
                s.address = server.address;

                // Also update active server if this was it
                if (m_activeServer.id == server.id) {
                    m_activeServer = s;
                }
                
                saveToFile();
                break;
            }
        }
    }

    Domain::ServerIP IPRepository::getActiveServer() const {
        return m_activeServer;
    }

    bool IPRepository::loadFromFile() {
        std::ifstream file(CONFIG_PATH);
        if (!file.is_open()) return false;

        try {
            json j;
            file >> j;

            m_servers.clear();
            if (j.contains("servers")) {
                for (auto& item : j["servers"]) {
                    m_servers.push_back({
                        item.value("id", ""),
                        item.value("name", ""),
                        item.value("address", "")
                    });
                }
            }

            if (j.contains("activeServer")) {
                auto& active = j["activeServer"];
                m_activeServer = {
                    active.value("id", ""),
                    active.value("name", ""),
                    active.value("address", "")
                };
            }

            if (j.contains("language")) {
                m_language = j.value("language", "en-US");
            }
            
            brls::Logger::info("IPRepository: Loaded {} servers", m_servers.size());
            return true;
        } catch (...) {
            brls::Logger::error("IPRepository: Failed to parse config");
            return false;
        }
    }

    void IPRepository::saveToFile() {
        json j;
        json servers = json::array();
        for (const auto& s : m_servers) {
            servers.push_back({
                {"id", s.id},
                {"name", s.name},
                {"address", s.address}
            });
        }
        j["servers"] = servers;
        j["activeServer"] = {
            {"id", m_activeServer.id},
            {"name", m_activeServer.name},
            {"address", m_activeServer.address}
        };
        j["language"] = m_language;

        std::ofstream file(CONFIG_PATH);
        if (file.is_open()) {
            file << j.dump(4);
            brls::Logger::info("IPRepository: Saved config");
        }
    }

} // namespace Data
} // namespace DarkTube
