#pragma once

#include <string>

namespace NetworkingTools
{
    class DNSResolver
    {
    public:
        std::string resolve(const std::string& hostname) const;
    };
}

/**
 * NOTES:
 * - The DNSResolver class provides a method to 
 *   resolve a hostname to an IP address.
 * - The class is designed to be simple and focused on the DNS resolution
 *   functionality, allowing for future expansion if needed.
 * - The use of std::string allows for easy handling of hostnames and IP addresses 
 *   as strings, which is common in networking applications.
 */