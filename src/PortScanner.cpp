#include "PortScanner.hpp"
#include "TcpClient.hpp"
#include "DNSResolver.hpp"

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
                return "TimedOut";
            default:
                return "Invalid";
        }
    }

    PortScanResult PortScanner::scan(
        const std::string& host,
        int startPort,
        int endPort,
        int timeoutSeconds
    ) const
    {
        PortScanResult result;
        result.success = false;
        result.host = host;
        result.startPort = startPort;
        result.endPort = endPort;

        if (startPort < 1 || endPort > 65535 || startPort > endPort)
        {
            result.errorMessage = "Invalid port range.";
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

            PortScanEntry entry;
            entry.port = port;

            if (connectResult.success)
            {
                entry.status = PortStatus::Open;
                entry.message = "Connection succeeded.";
            }
            else if (connectResult.timedOut)
            {
                entry.status = PortStatus::TimedOut;
                entry.message = connectResult.errorMessage;
            }
            else
            {
                entry.status = PortStatus::Closed;
                entry.message = connectResult.errorMessage;
            }

            result.entries.push_back(entry);
        }

        result.success = true;
        return result;
    }
}