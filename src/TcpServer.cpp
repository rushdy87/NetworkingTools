#include "TcpServer.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>
#include <cerrno>

namespace NetworkingTools
{
    ServerResult TcpServer::start(int port) const
    {
        ServerResult result;
        result.port = port;
        result.responseMessage = "Hello from TcpServer";

        if (port < 1 || port > 65535)
        {
            result.errorMessage = "Invalid port number. Must be between 1 and 65535.";
            return result;
        }

        int serverSocket = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket, AF_INET for IPv4, SOCK_STREAM for TCP
        if (serverSocket < 0)
        {
            result.errorMessage = "Failed to create socket: " + std::string(strerror(errno));
            return result;
        }

        int opt = 1; // Allow reuse of the address
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        // Set socket options to allow reuse of the address, which helps avoid "Address already in use"
        // errors when restarting the server
        {
            result.errorMessage = "Failed to set socket options: " + std::string(strerror(errno));
            close(serverSocket);
            return result;
        }

        // Bind the socket to the specified port
        struct sockaddr_in serverAddress; // Define the server address structure
        std::memset(&serverAddress, 0, sizeof(serverAddress)); // Zero out the structure to ensure all fields are initialized
        serverAddress.sin_family = AF_INET; // Set the address family to AF_INET for IPv4
        serverAddress.sin_addr.s_addr = INADDR_ANY; // Bind to all available interfaces (INADDR_ANY)
        serverAddress.sin_port = htons(port); // Convert the port number to network byte order using htons (host to network short)

        // Bind the socket to the specified address and port. If the bind fails, return an error message and close the socket.
        if(bind(
            serverSocket,
            reinterpret_cast<struct sockaddr*>(&serverAddress),
            sizeof(serverAddress)
            ) < 0)
        {
            result.errorMessage = "Failed to bind socket: " + std::string(strerror(errno));
            close(serverSocket);
            return result;
        }

        // Start listening for incoming connections. The second argument (5) specifies 
        // the maximum number of pending connections in the queue.
        if (listen(serverSocket, 5) < 0)
        {
            result.errorMessage = "Failed to listen on socket: " + std::string(strerror(errno));
            close(serverSocket);
            return result;
        }

        struct sockaddr_in clientAddress;
        socklen_t clientLength = sizeof(clientAddress);

        int clientSocket = accept(
            serverSocket,
            reinterpret_cast<struct sockaddr*>(&clientAddress),
            &clientLength
        );

        if (clientSocket < 0)
        {
            result.errorMessage = "Failed to accept connection: " + std::string(strerror(errno));
            close(serverSocket);
            return result;
        }

        char buffer[4096];
        std::memset(buffer, 0, sizeof(buffer));

        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived < 0)
        {
            result.errorMessage = "Failed to receive data: " + std::string(strerror(errno));
            close(clientSocket);
            close(serverSocket);
            return result;
        }

        result.receivedMessage = std::string(buffer, bytesReceived);

        ssize_t bytesSent = send(
            clientSocket,
            result.responseMessage.c_str(),
            result.responseMessage.size(),
            0
        );

        if (bytesSent < 0)
        {
            result.errorMessage = "Failed to send data: " + std::string(strerror(errno));
            close(clientSocket);
            close(serverSocket);
            return result;
        }

        close(clientSocket);
        close(serverSocket);

        result.success = true;
        return result;
    }
}