#include <iostream>
#include <string>
#include <vector>

#include "DNSResolver.hpp"

void printUsage()
{
    std::cout << "Usage:\n";
    std::cout << "  ./nettools resolve <hostname>\n";
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printUsage();
        return 1;
    }

    std::string command = argv[1];

    if (command == "resolve")
    {
        std::string hostname = argv[2];

        NetworkingTools::DNSResolver resolver;
        std::vector<NetworkingTools::ResolvedAddress> addresses = resolver.resolveAll(hostname);

        if (addresses.empty())
        {
            std::cout << "No addresses found for: " << hostname << '\n';
            return 1;
        }

        std::cout << "Resolved addresses for " << hostname << ":\n";

        for (const auto& address : addresses)
        {
            std::cout << address.family << ": " << address.ip << '\n';
        }

        return 0;
    }

    printUsage();
    return 1;
}