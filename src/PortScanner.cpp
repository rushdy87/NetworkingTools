#include "PortScanner.hpp"
#include "DNSResolver.hpp"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <thread>

namespace NetworkingTools
{
   std::string portStatusToString(PortStatus status)
   {
        switch (status)
        {
            case PortStatus::Open:
                return "Open";
            case PortStatus::Closed:
                return "Closed";
            case PortStatus::TimedOut:
                return "Timed Out";
            default:
                return "Invalid";
        }
   }

   bool PortScanner::isValidPortRange(int startPort, int endPort) const
   {
        return startPort >= 1 && endPort <= 65535 && startPort <= endPort;
   }

   PortScanResult PortScanner::prepareScanResult(
        const std::string& host,
        int startPort,
        int endPort
    ) const
    {
        PortScanResult result;
        result.host = host;
        result.startPort = startPort;
        result.endPort = endPort;
        return result;
    }

    PortScanEntry PortScanner::buildEntryFromConnectResult(
        int port,
        const ConnectResult& connectResult
    ) const
    {
        PortScanEntry entry;
        entry.port = port;

        if (connectResult.success)
        {
            entry.status = PortStatus::Open;
            entry.message = "Connection successful";
        }
        else if (connectResult.timedOut)
        {
            entry.status = PortStatus::TimedOut;
            entry.message = "Connection timed out";
        }
        else
        {
            entry.status = PortStatus::Closed;
            entry.message = "Connection failed: " + connectResult.errorMessage;
        }

        return entry;
    }

    bool PortScanner::shouldStoreEntry(
        const PortScanEntry& entry,
        ScanOutputMode outputMode
    ) const
    {
        return (outputMode == ScanOutputMode::All) ||
               (outputMode == ScanOutputMode::OpenOnly && entry.status == PortStatus::Open);
    }

    void PortScanner::updateCounters(
        PortScanResult& result,
        const PortScanEntry& entry
    ) const
    {
        if (entry.status == PortStatus::Open)
        {
            result.openCount++;
        }
        else if (entry.status == PortStatus::Closed)
        {
            result.closedCount++;
        }
        else if (entry.status == PortStatus::TimedOut)
        {
            result.timedOutCount++;
        }
    }

    PortScanResult PortScanner::scan(
        const std::string& host,
        int startPort,
        int endPort,
        int timeoutSeconds,
        ScanOutputMode outputMode
    ) const
    {
        PortScanResult result = prepareScanResult(host, startPort, endPort);

        if (!isValidPortRange(startPort, endPort))
        {
            result.errorMessage = "Invalid port range";
            return result;
        }

        DNSResolver resolver;
        std::string ip = resolver.resolveFirstIPv4(host);
        if (ip.empty())
        {
            result.errorMessage = "Failed to resolve host to IPv4.";
            return result;
        }

        result.resolvedIP = ip;

        TcpClient client;
        for (int port = startPort; port <= endPort; ++port)
        {
            ConnectResult connectResult = client.connectToIPAddress(ip, port, timeoutSeconds);

            PortScanEntry entry = buildEntryFromConnectResult(port, connectResult);

            updateCounters(result, entry);

            if (shouldStoreEntry(entry, outputMode))
            {
                result.entries.push_back(entry);
            }

        }
        result.success = true;
        return result;
    }

    PortScanResult PortScanner::scanMultiThreaded(
        const std::string& host,
        int startPort,
        int endPort,
        int timeoutSeconds,
        ScanOutputMode outputMode,
        int threadCount
    ) const
    {
        PortScanResult result = prepareScanResult(host, startPort, endPort);

        if (!isValidPortRange(startPort, endPort))
        {
            result.errorMessage = "Invalid port range.";
            return result;
        }

        if (threadCount < 1)
        {
            result.errorMessage = "Thread count must be at least 1.";
            return result;
        }

        DNSResolver resolver;
        std::string ip = resolver.resolveFirstIPv4(host);

        if (ip.empty())
        {
            result.errorMessage = "Failed to resolve host to IPv4.";
            return result;
        }

        result.resolvedIP = ip;

        std::atomic<int> nextPort(startPort);
        std::mutex resultMutex;
        std::vector<std::thread> workers;

        auto worker = [&]()
        {
            TcpClient client;

            while (true)
            {
                int port  = nextPort.fetch_add(1);

                if (port > endPort)
                {
                    break;
                }

                ConnectResult connectResult = client.connectToIPAddress(ip, port, timeoutSeconds);
                PortScanEntry entry = buildEntryFromConnectResult(port, connectResult);

                std::lock_guard<std::mutex> lock(resultMutex);

                updateCounters(result, entry);

                if (shouldStoreEntry(entry, outputMode))
                {
                    result.entries.push_back(entry);
                }
            }
        };

        workers.reserve(threadCount);

        // Launch worker threads, each will fetch the next port to scan until all ports are processed.
        for (int i = 0; i < threadCount; ++i)
        {
            workers.emplace_back(worker);
        }

        // Wait for all worker threads to finish before returning the result.
        for (auto& workerThread : workers)
        {
            workerThread.join();
        }

        std::sort (result.entries.begin(),
            result.entries.end(),
            [](const PortScanEntry& left, const PortScanEntry& right)
            {
                return left.port < right.port;
            }
        );

        result.success = true;
        return result;
    }
}