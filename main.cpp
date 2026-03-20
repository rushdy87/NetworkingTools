#include <iostream>
#include "DNSResolver.hpp"

int main()
{
    NetworkingTools::DNSResolver resolver;

    std::string ip = resolver.resolve("google.com");

    std::cout << "Resolved IP: " << ip << std::endl;

    return 0;
}