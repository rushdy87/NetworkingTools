#pragma once

#include "TcpClient.hpp"

#include <string>
#include <vector>

namespace NetworkingTools
{
    // Port status for scan results
    enum class PortStatus
    {
        Open,
        Closed,
        TimedOut,
        Invalid
    };

    // Output mode for scan results
    enum class ScanOutputMode
    {
        OpenOnly,
        All
    };

    // Represents the result of scanning a single port
    struct PortScanEntry
    {
        int port;
        PortStatus status;
        std::string message;
    };

    // Represents the overall result of a port scan
    struct PortScanResult
    {
        bool success = false;
        std::string host;
        std::string resolvedIP;
        int startPort = 0;
        int endPort = 0;
        std::string errorMessage;
        std::vector<PortScanEntry> entries;

        int openCount = 0;
        int closedCount = 0;
        int timedOutCount = 0;
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

        PortScanResult scanMultiThreaded(
            const std::string& host,
            int startPort = 1,
            int endPort = 65535,
            int timeoutSeconds = 2,
            ScanOutputMode outputMode = ScanOutputMode::OpenOnly,
            int threadCount = 20
        ) const;

    private:
        // Helper function to validate port range
        bool isValidPortRange(int startPort, int endPort) const;

        // Helper function to perform the actual scanning logic for a given port range
        PortScanResult prepareScanResult(
            const std::string& host,
            int startPort,
            int endPort
        ) const;

        // Helper function to build a PortScanEntry from a ConnectResult
        PortScanEntry buildEntryFromConnectResult(
            int port,
            const ConnectResult& connectResult
        ) const;

        // Helper function to determine if a PortScanEntry should be stored based on the output mode
        bool shouldStoreEntry(
            const PortScanEntry& entry,
            ScanOutputMode outputMode
        ) const;

        // Helper function to update the counters in a PortScanResult based on a PortScanEntry
        void updateCounters(
            PortScanResult& result,
            const PortScanEntry& entry
        ) const;
    };

    // Utility function to convert PortStatus to string for display purposes
    std::string portStatusToString(PortStatus status);
}