#include "TcpClient.hpp"
#include "DNSResolver.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <sys/select.h>

namespace NetworkingTools
{
    ConnectResult TcpClient::connectToServer(const std::string& host, int port, int timeoutSeconds) const
    {
        ConnectResult result;
        result.success = false;
        result.timedOut = false;
        result.host = host;
        result.port = port;

        DNSResolver resolver;
        std::string ip = resolver.resolveFirstIPv4(host);

        if (ip.empty())
        {
            result.errorMessage = "Failed to resolve host to IPv4.";
            return result;
        }

        result.resolvedIP = ip;

        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0)
        {
            result.errorMessage = std::strerror(errno);
            return result;
        }

        int flags = fcntl(clientSocket, F_GETFL, 0);
        if (flags < 0)
        {
            result.errorMessage = std::strerror(errno);
            close(clientSocket);
            return result;
        }

        if (fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK) < 0)
        {
            result.errorMessage = std::strerror(errno);
            close(clientSocket);
            return result;
        }

        struct sockaddr_in serverAddress;
        std::memset(&serverAddress, 0, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);

        int conversionStatus = inet_pton(AF_INET, ip.c_str(), &serverAddress.sin_addr);
        if (conversionStatus <= 0)
        {
            result.errorMessage = "Invalid IPv4 address format.";
            close(clientSocket);
            return result;
        }

        int connectionStatus = connect(
            clientSocket,
            reinterpret_cast<struct sockaddr*>(&serverAddress),
            sizeof(serverAddress)
        );

        if (connectionStatus == 0)
        {
            close(clientSocket);
            result.success = true;
            return result;
        }

        if (connectionStatus < 0 && errno != EINPROGRESS)
        {
            result.errorMessage = std::strerror(errno);
            close(clientSocket);
            return result;
        }

        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(clientSocket, &writeSet);

        struct timeval timeout;
        timeout.tv_sec = timeoutSeconds;
        timeout.tv_usec = 0;

        int selectStatus = select(clientSocket + 1, nullptr, &writeSet, nullptr, &timeout);

        if (selectStatus == 0)
        {
            result.timedOut = true;
            result.errorMessage = "Connection timed out.";
            close(clientSocket);
            return result;
        }

        if (selectStatus < 0)
        {
            result.errorMessage = std::strerror(errno);
            close(clientSocket);
            return result;
        }

        int socketError = 0;
        socklen_t len = sizeof(socketError);

        if (getsockopt(clientSocket, SOL_SOCKET, SO_ERROR, &socketError, &len) < 0)
        {
            result.errorMessage = std::strerror(errno);
            close(clientSocket);
            return result;
        }

        if (socketError != 0)
        {
            result.errorMessage = std::strerror(socketError);
            close(clientSocket);
            return result;
        }

        close(clientSocket);
        result.success = true;
        return result;
    }
}