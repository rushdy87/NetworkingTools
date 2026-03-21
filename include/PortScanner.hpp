#pragma once

#include <string>
#include <vector>

namespace NetworkingTools
{
    enum class PortStatus
    {
        Open,
        Closed,
        TimedOut,
        Invalid
    };

    struct PortScanEntry
    {
        int port;
        PortStatus status;
        std::string message;
    };

    struct PortScanResult
    {
        bool success;
        std::string host;
        std::string resolvedIP;
        int startPort;
        int endPort;
        std::string errorMessage;
        std::vector<PortScanEntry> entries;
    };

    class PortScanner
    {
    public:
        PortScanResult scan(const std::string& host, int startPort, int endPort, int timeoutSeconds = 2) const;
    };

    std::string portStatusToString(PortStatus status);
}