#include "DNSResolver.hpp"

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>
#include <iostream>

namespace NetworkingTools
{
    std::vector<ResolvedAddress> DNSResolver::resolveAll(const std::string& hostname) const
    {
        struct addrinfo hints;
        struct addrinfo* results = nullptr;
        std::vector<ResolvedAddress> addresses;

        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;       // IPv4 or IPv6, for IPv4-only use AF_INET, for IPv6-only use AF_INET6
        hints.ai_socktype = SOCK_STREAM;   // TCP-style results

        int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &results);

        if (status != 0)
        {
            std::cerr << "getaddrinfo failed: " << gai_strerror(status) << '\n';
            return addresses;
        }

        // results is a linked list of addrinfo structures, we need to iterate through it to extract the IP addresses

        for (struct addrinfo* current = results; current != nullptr; current = current->ai_next)
        {
            // std::cout << "ai_family = " << (current->ai_family == AF_INET ? "AF_INET" : (current->ai_family == AF_INET6 ? "AF_INET6" : "Unknown")) << '\n';

            char ipStr[INET6_ADDRSTRLEN];
            void* addrPtr = nullptr;
            std::string family;

            if (current->ai_family == AF_INET)
            {
                struct sockaddr_in* ipv4 = reinterpret_cast<struct sockaddr_in*>(current->ai_addr);
                addrPtr = &(ipv4->sin_addr);
                family = "IPv4";
            }
            else if (current->ai_family == AF_INET6)
            {
                struct sockaddr_in6* ipv6 = reinterpret_cast<struct sockaddr_in6*>(current->ai_addr);
                addrPtr = &(ipv6->sin6_addr);
                family = "IPv6";
            }
            else
            {
                continue;
            }

            const char* converted = inet_ntop(current->ai_family, addrPtr, ipStr, sizeof(ipStr));
            if (converted != nullptr)
            {
                addresses.push_back({ipStr, family});
            }
        }

        freeaddrinfo(results);
        return addresses;
    }
}