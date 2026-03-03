#pragma once

#include "../domain/models.hpp"
#include <vector>
#include <string>

namespace DarkTube {
namespace Data {

    class IPRepository {
    public:
        // Singleton access
        static IPRepository& getInstance() {
            static IPRepository instance;
            return instance;
        }

        // Methods
        std::vector<Domain::ServerIP> getSavedServers();
        void addServer(const Domain::ServerIP& server);
        void removeServer(const std::string& id);
        
        bool loadFromFile();
        void saveToFile();

        // Currently active server
        void setActiveServer(const Domain::ServerIP& server);
        void updateServer(const Domain::ServerIP& server);
        Domain::ServerIP getActiveServer() const;

    private:
        IPRepository(); // Initialize with mock data
        std::vector<Domain::ServerIP> m_servers;
        Domain::ServerIP m_activeServer;
    };

} // namespace Data
} // namespace DarkTube
