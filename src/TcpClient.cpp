#include "TcpClient.hpp"
#include "DNSResolver.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <fcntl.h>

namespace NetworkingTools
{
    HttpResponse TcpClient::sendHttpRequest(const std::string& host, int port, const std::string& path) const
    {
        HttpResponse result;
        result.success = false;
        result.host = host;
        result.port = port;
        result.statusCode = 0;
        result.isRedirect = false;

        DNSResolver resolver;
        std::string ip = resolver.resolveFirstIPv4(host);

        if (ip.empty())
        {
            result.errorMessage = "Failed to resolve host.";
            return result;
        }

        result.resolvedIP = ip;

        int sock = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket, its work with operating system to choose the right protocol (TCP in this case)
        if (sock < 0)
        {
            result.errorMessage = std::strerror(errno);
            return result;
        }

        struct sockaddr_in serverAddr; // This structure is used to specify the address and port of the server we want to connect to
        std::memset(&serverAddr, 0, sizeof(serverAddr)); // Zero out the structure to ensure there are no garbage values
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);

        if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0)
        // Convert the resolved IP address from string format to binary format 
        // and store it in the serverAddr structure. If this conversion fails, 
        // we handle the error.
        {
            result.errorMessage = "Invalid IP format.";
            close(sock);
            return result;
        }

        if (connect(sock, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0)
        // Attempt to connect to the server using the specified address and port.
        // If the connection fails, we handle the error.
        {
            result.errorMessage = std::strerror(errno);
            close(sock);
            return result;
        }

        // Construct the HTTP GET request to be sent to the server. This includes the request line,
        // the Host header, and a Connection header to indicate that we want to close the connection
        // after the response is received.
        std::string request =
            "GET " + path + " HTTP/1.1\r\n"
            "Host: " + host + "\r\n"
            "Connection: close\r\n"
            "\r\n";

        // Send the HTTP request to the server. If the send operation fails, we handle the error.
        ssize_t sent = send(sock, request.c_str(), request.size(), 0);
        if (sent < 0)
        {
            result.errorMessage = std::strerror(errno);
            close(sock);
            return result;
        }

        char buffer[4096];
        std::string response;

        // Receive the HTTP response from the server. We read data in chunks and append it to the response string.
        while (true)
        {
            ssize_t bytesReceived = recv(sock, buffer, sizeof(buffer), 0);

            if (bytesReceived > 0)
            {
                response.append(buffer, bytesReceived);
            }
            else if (bytesReceived == 0)
            {
                break;
            }
            else
            {
                result.errorMessage = std::strerror(errno);
                close(sock);
                return result;
            }
        }

        close(sock); // Close the socket after the response is received

        // Parse the raw HTTP response and populate the HttpResponse structure with the relevant information.
        return parseHttpResponse(response, host, port, ip);
    }

    HttpResponse TcpClient::parseHttpResponse(
        const std::string& rawResponse,
        const std::string& host,
        int port,
        const std::string& resolvedIP
    ) const
    {
        HttpResponse result;
        result.success = true;
        result.host = host;
        result.port = port;
        result.resolvedIP = resolvedIP;
        result.rawResponse = rawResponse;
        result.statusCode = 0;
        result.isRedirect = false;

        std::size_t headerEnd = rawResponse.find("\r\n\r\n");
        if (headerEnd == std::string::npos)
        {
            result.success = false;
            result.errorMessage = "Invalid HTTP response: header/body separator not found.";
            return result;
        }

        std::string headerSection = rawResponse.substr(0, headerEnd);
        result.body = rawResponse.substr(headerEnd + 4);

        std::istringstream headerStream(headerSection);
        std::string line;

        if (!std::getline(headerStream, line))
        {
            result.success = false;
            result.errorMessage = "Invalid HTTP response: missing status line.";
            return result;
        }

        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        std::istringstream statusLineStream(line);
        std::string httpVersion;
        statusLineStream >> httpVersion;
        statusLineStream >> result.statusCode;
        std::getline(statusLineStream, result.statusText);

        if (!result.statusText.empty() && result.statusText.front() == ' ')
        {
            result.statusText.erase(0, 1);
        }

        while (std::getline(headerStream, line))
        {
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            std::size_t colonPos = line.find(':');
            if (colonPos == std::string::npos)
            {
                continue;
            }

            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);

            if (!value.empty() && value.front() == ' ')
            {
                value.erase(0, 1);
            }

            result.headers[key] = value;
        }

        if (result.statusCode >= 300 && result.statusCode < 400)
        {
            result.isRedirect = true;

            auto it = result.headers.find("Location");
            if (it != result.headers.end())
            {
                result.location = it->second;
            }
        }

        return result;
    }

    HttpResponse TcpClient::sendHttpRequestFollowRedirect(
        const std::string& host,
        int port,
        const std::string& path
    ) const
    {
        HttpResponse firstResponse = sendHttpRequest(host, port, path);

        if (!firstResponse.success)
        {
            return firstResponse;
        }

        if (!firstResponse.isRedirect || firstResponse.location.empty())
        {
            return firstResponse;
        }

        const std::string& location = firstResponse.location;

        if (location.rfind("https://", 0) == 0)
        {
            firstResponse.success = false;
            firstResponse.errorMessage =
                "Redirect target uses HTTPS, which is not supported yet by this client: " + location;
            return firstResponse;
        }

        if (location.rfind("http://", 0) == 0)
        {
            std::string remaining = location.substr(7);

            std::size_t slashPos = remaining.find('/');
            std::string newHost;
            std::string newPath;

            if (slashPos == std::string::npos)
            {
                newHost = remaining;
                newPath = "/";
            }
            else
            {
                newHost = remaining.substr(0, slashPos);
                newPath = remaining.substr(slashPos);
            }

            return sendHttpRequest(newHost, 80, newPath);
        }

        if (!location.empty() && location.front() == '/')
        {
            return sendHttpRequest(host, port, location);
        }

        firstResponse.success = false;
        firstResponse.errorMessage = "Unsupported redirect format: " + location;
        return firstResponse;
    }

    ConnectResult TcpClient::connectToIPAddress(const std::string& ip, int port, int timeoutSeconds) const
    {
        ConnectResult result;
        result.success = false;
        result.timedOut = false;
        result.host = ip;
        result.port = port;
        result.resolvedIP = ip;

        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0)
        {
            result.errorMessage = std::strerror(errno);
            return result;
        }

        int flags = fcntl(clientSocket, F_GETFL, 0); // Get the current file status flags for the socket
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

    ConnectResult TcpClient::connectToServer(const std::string& host, int port, int timeoutSeconds) const
    {
        DNSResolver resolver;
        std::string ip = resolver.resolveFirstIPv4(host);

        ConnectResult result;
        result.success = false;
        result.timedOut = false;
        result.host = host;
        result.port = port;

        if (ip.empty())
        {
            result.errorMessage = "Failed to resolve host to IPv4.";
            return result;
        }

        result = connectToIPAddress(ip, port, timeoutSeconds);
        result.host = host;
        return result;
    }
}