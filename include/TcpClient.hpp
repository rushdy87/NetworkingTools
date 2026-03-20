#pragma once

#include <string>

namespace NetworkingTools
{
    struct ConnectResult
    {
        bool success;
        bool timedOut;
        std::string host;
        int port;
        std::string resolvedIP;
        std::string errorMessage;
    };

    class TcpClient
    {
    public:
        ConnectResult connectToServer(const std::string& host, int port, int timeoutSeconds = 5) const;
    };
}

/** NOTES:
 * - The `TcpClient` class is responsible for establishing TCP connections to a specified server.
 * - The `ConnectResult` struct encapsulates the result of a connection attempt, including 
 *   success status, host, port, resolved IP, and any error message.
 * - We will use "DNSResolver.hpp" for resolving hostnames to IP addresses.
 */