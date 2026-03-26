#include <iostream>
#include <string>
#include <cstdlib>

#include "DNSResolver.hpp"
#include "TcpClient.hpp"
#include "PortScanner.hpp"


void printUsage()
{
    std::cout << "Usage:\n";
    std::cout << "  ./nettools resolve <hostname>\n";
    std::cout << "  ./nettools connect <hostname> <port>\n";
    std::cout << "  ./nettools http <hostname>\n";
    std::cout << "  ./nettools scan <hostname>\n";
    std::cout << "  ./nettools scan <hostname> <startPort> <endPort>\n";
    std::cout << "  ./nettools scan <hostname> <startPort> <endPort> <threadCount>\n";
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        printUsage();
        return 1;
    }

    std::string command = argv[1];

    if (command == "resolve")
    {
        if (argc != 3)
        {
            printUsage();
            return 1;
        }

        std::string hostname = argv[2];

        NetworkingTools::DNSResolver resolver;
        NetworkingTools::ResolveResult result = resolver.resolveAll(hostname);

        if (!result.success)
        {
            std::cout << "Failed to resolve " << result.hostname
                      << ": " << result.errorMessage << '\n';
            return 1;
        }

        std::cout << "Resolved addresses for " << result.hostname << ":\n";

        for (const auto& address : result.addresses)
        {
            std::cout << NetworkingTools::addressFamilyToString(address.family)
                      << ": " << address.ip << '\n';
        }

        return 0;
    }

    if (command == "http")
    {
        if (argc != 3)
        {
            printUsage();
            return 1;
        }

        std::string host = argv[2];

        std::cout << "Sending HTTP request to " << host << "...\n";

        NetworkingTools::TcpClient client;
        NetworkingTools::HttpResponse result = client.sendHttpRequestFollowRedirect(host, 80);

        if (!result.success)
        {
            std::cout << "HTTP request failed: " << result.errorMessage << "\n";

            if (!result.location.empty())
            {
                std::cout << "Redirect location: " << result.location << "\n";
            }

            return 1;
        }

        std::cout << "Status: " << result.statusCode << " " << result.statusText << "\n";
        std::cout << "Response body:\n\n";
        std::cout << result.body << "\n";

        return 0;
    }

    if (command == "scan")
    {
        if (argc != 3 && argc != 5 && argc != 6)
        {
            printUsage();
            return 1;
        }

        std::string host = argv[2];
        int startPort = 1;
        int endPort = 65535;
        int threadCount = 20;

        if (argc >= 5)
        {
            startPort = std::atoi(argv[3]);
            endPort = std::atoi(argv[4]);

            if (startPort < 1 || endPort > 65535 || startPort > endPort)
            {
                std::cout << "Invalid port range.\n";
                return 1;
            }
        }
        else
        {
            std::cout << "Warning: Full port scan may take a long time.\n";
        }

        if (argc == 6)
        {
            threadCount = std::atoi(argv[5]);

            if (threadCount < 1)
            {
                std::cout << "Invalid thread count.\n";
                return 1;
            }
        }

        std::cout << "Scanning " << host
                << " from port " << startPort
                << " to " << endPort
                << " using " << threadCount << " threads...\n";

        NetworkingTools::PortScanner scanner;
        NetworkingTools::PortScanResult result = scanner.scanMultiThreaded(
            host,
            startPort,
            endPort,
            2,
            NetworkingTools::ScanOutputMode::OpenOnly,
            threadCount
        );

        if (!result.success)
        {
            std::cout << "Scan failed: " << result.errorMessage << "\n";
            return 1;
        }

        std::cout << "Resolved IP: " << result.resolvedIP << "\n";
        std::cout << "Open: " << result.openCount
                << ", Closed: " << result.closedCount
                << ", TimedOut: " << result.timedOutCount << "\n";

        if (result.entries.empty())
        {
            std::cout << "No open ports found in the specified range.\n";
            return 0;
        }

        std::cout << "Open ports:\n";
        for (const auto& entry : result.entries)
        {
            std::cout << "Port " << entry.port << ": "
                    << NetworkingTools::portStatusToString(entry.status) << "\n";
        }

        return 0;
    }

    printUsage();
    return 1;
}