#include "DNSResolver.hpp"

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>
#include <set>

namespace NetworkingTools
{
    std::string addressFamilyToString(AddressFamily family)
    {
        switch (family)
        {
            case AddressFamily::IPv4:
                return "IPv4";
            case AddressFamily::IPv6:
                return "IPv6";
            default:
                return "Unknown";
        }
    }

    ResolveResult DNSResolver::resolveAll(const std::string& hostname) const
    {
        ResolveResult output;
        output.success = false;
        output.hostname = hostname;

        struct addrinfo hints;
        struct addrinfo* results = nullptr;
        std::set<std::string> uniqueAddresses;

        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;

        int status = getaddrinfo(hostname.c_str(), "80", &hints, &results);
        if (status != 0)
        {
            output.errorMessage = gai_strerror(status);
            return output;
        }

        if (results == nullptr)
        {
            output.errorMessage = "No address information returned.";
            return output;
        }

        for (struct addrinfo* current = results; current != nullptr; current = current->ai_next)
        {
            char ipStr[INET6_ADDRSTRLEN];
            void* addrPtr = nullptr;
            AddressFamily family = AddressFamily::Unknown;

            if (current->ai_family == AF_INET)
            {
                struct sockaddr_in* ipv4 = reinterpret_cast<struct sockaddr_in*>(current->ai_addr);
                addrPtr = &(ipv4->sin_addr);
                family = AddressFamily::IPv4;
            }
            else if (current->ai_family == AF_INET6)
            {
                struct sockaddr_in6* ipv6 = reinterpret_cast<struct sockaddr_in6*>(current->ai_addr);
                addrPtr = &(ipv6->sin6_addr);
                family = AddressFamily::IPv6;
            }
            else
            {
                continue;
            }

            const char* converted = inet_ntop(current->ai_family, addrPtr, ipStr, sizeof(ipStr));
            if (converted != nullptr)
            {
                std::string ip = ipStr;
                std::string uniqueKey = addressFamilyToString(family) + ":" + ip;

                if (uniqueAddresses.insert(uniqueKey).second)
                {
                    output.addresses.push_back({ip, family});
                }
            }
        }

        freeaddrinfo(results);

        if (output.addresses.empty())
        {
            output.errorMessage = "No usable IP addresses found.";
            return output;
        }

        output.success = true;
        return output;
    }

    std::string DNSResolver::resolveFirstIPv4(const std::string& hostname) const
    {
        ResolveResult result = resolveAll(hostname);

        if (!result.success)
        {
            return "";
        }

        for (const auto& address : result.addresses)
        {
            if (address.family == AddressFamily::IPv4)
            {
                return address.ip;
            }
        }

        return "";
    }
}