#include <iostream>
#include <string>

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

    printUsage();
    return 1;
}