#pragma once

#include <string>

namespace NetworkingTools
{
    class PortScanner
    {
        void scan(const std::string& host, int startPort, int endPort) const;
    };
}