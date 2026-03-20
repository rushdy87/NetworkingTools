#pragma once

#include <string>
#include <vector>

namespace NetworkingTools
{
    struct ResolvedAddress
    {
        std::string ip;
        std::string family;
    };

    class DNSResolver
    {
    public:
        std::vector<ResolvedAddress> resolveAll(const std::string& hostname) const;
    };
}

