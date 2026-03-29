#pragma once

#include <string>

namespace NetworkingTools
{

    struct ServerResult
    {
        bool success = false;
        int port = 0;
        std::string errorMessage;
        std::string receivedMessage;
        std::string responseMessage;
    };

    class TcpServer
    {
    public:
        ServerResult start(int port) const;
    };
}