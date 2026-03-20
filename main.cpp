#include <iostream>
#include <string>
#include <cstdlib>

#include "DNSResolver.hpp"
#include "TcpClient.hpp"

void printUsage()
{
    std::cout << "Usage:\n";
    std::cout << "  ./nettools resolve <hostname>\n";
    std::cout << "  ./nettools connect <hostname> <port>\n";
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

    if (command == "connect")
    {
        if (argc != 4)
        {
            printUsage();
            return 1;
        }

        std::string host = argv[2];
        int port = std::atoi(argv[3]);

        if (port <= 0 || port > 65535)
        {
            std::cout << "Invalid port number.\n";
            return 1;
        }

        NetworkingTools::TcpClient client;
        NetworkingTools::ConnectResult result = client.connectToServer(host, port, 5);
        if (!result.success)
        {
            std::cout << "Failed to connect to " << result.host
                      << ":" << result.port
                      << " (" << result.resolvedIP << ")"
                      << " - " << result.errorMessage << '\n';
            return 1;
        }

        std::cout << "Successfully connected to "
                  << result.host << ":" << result.port
                  << " (" << result.resolvedIP << ")\n";

        return 0;
    }

    printUsage();
    return 1;
}