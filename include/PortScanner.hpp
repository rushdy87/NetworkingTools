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

    enum class ScanOutputMode
    {
        OpenOnly,
        All
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
        PortScanResult scan(
            const std::string& host,
            int startPort = 1,
            int endPort = 65535,
            int timeoutSeconds = 2,
            ScanOutputMode outputMode = ScanOutputMode::OpenOnly
        ) const;
    };

    std::string portStatusToString(PortStatus status);
}