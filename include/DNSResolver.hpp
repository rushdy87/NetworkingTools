#pragma once

#include <string>
#include <vector>

namespace NetworkingTools
{
    enum class AddressFamily
    {
        IPv4,
        IPv6,
        Unknown
    };

    struct ResolvedAddress
    {
        std::string ip;
        AddressFamily family;
    };

    struct ResolveResult
    {
        bool success;
        std::string hostname;
        std::string errorMessage;
        std::vector<ResolvedAddress> addresses;
    };

    class DNSResolver
    {
    public:
        ResolveResult resolveAll(const std::string& hostname) const;
        std::string resolveFirstIPv4(const std::string& hostname) const;
    };

    std::string addressFamilyToString(AddressFamily family);
}