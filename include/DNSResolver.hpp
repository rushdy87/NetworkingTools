#pragma once

#include <string>

namespace NetworkingTools
{
    class DNSResolver
    {
        std::string resolve(const std::string& hostname) const;
    };
}