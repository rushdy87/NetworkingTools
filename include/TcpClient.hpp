#pragma once

#include <string>

namespace NetworkingTools
{
    class TcpClient
    {
        bool connectToServer(const std::string& host, int port) const;
    };
}