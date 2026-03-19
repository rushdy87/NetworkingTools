#include "../include/Utils.hpp"
#include <iostream>

namespace NetworkingTools
{
    namespace Utils
    {
        void printError(const std::string& message)
        {
            std::cerr << "Error: " << message << std::endl;
        }
    }
}