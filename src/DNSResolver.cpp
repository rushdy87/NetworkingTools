#include "../include/DNSResolver.hpp"

#include <cstring> // For memset
#include <netdb.h> // For getaddrinfo, freeaddrinfo, and gai_strerror
#include <arpa/inet.h> // For inet_ntop
#include <sys/socket.h> // For AF_INET and SOCK_STREAM

namespace NetworkingTools
{
    std::string DNSResolver::resolve(const std::string& hostname) const
    {
        // 1) Set up hints and result structures for getaddrinfo.
        struct addrinfo hints; // Hints structure to specify criteria for selecting socket address structures.
        struct addrinfo* result = nullptr;

        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET; // Use IPv4 addresses, not taking into account IPv6 for now.
        hints.ai_socktype = SOCK_STREAM; // Use TCP socket type, which is common for most applications.

        int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
        if (status != 0)
        {
            return "Resolution failed: " + std::string(gai_strerror(status));
        }

        if (result == nullptr)
        {
            return "Resolution failed: no results found.";
        }

        char ipStr[INET_ADDRSTRLEN];
        struct sockaddr_in* ipv4 = reinterpret_cast<struct sockaddr_in*>(result->ai_addr);
        void* addrPtr = &(ipv4->sin_addr);

        const char* converted = inet_ntop(AF_INET, addrPtr, ipStr, sizeof(ipStr));
        if (converted == nullptr)
        {
            freeaddrinfo(result);
            return "Resolution failed: could not convert address to string.";
        }

        std::string resolvedIP(ipStr);
        freeaddrinfo(result); // Free the memory allocated by getaddrinfo.

        return resolvedIP;
    }
}


/** NOTES:
 * - The DNSResolver class provides a method to 
 *   resolve a hostname to an IP address.
 * - The class is designed to be simple and focused on the DNS resolution
 *   functionality, allowing for future expansion if needed.
 * - The use of std::string allows for easy handling of hostnames and IP addresses 
 *   as strings, which is common in networking applications.
 * 
 *  - memset method is used to zero out the hints structure before 
 *    setting its fields, ensuring that all fields are initialized to 
 *    a known state.
 * 
 * - The getaddrinfo function is called with the hostname and hints, and
 *  the result is checked for errors. If there is an error, a descriptive
 *  error message is returned using gai_strerror to convert the error code 
 *  to a string.
 * 
 * - If the resolution is successful, the first result is processed to extract
 *  the IP address, which is then converted to a string format using inet_ntop.
 */