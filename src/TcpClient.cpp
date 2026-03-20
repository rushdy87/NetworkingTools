#include "TcpClient.hpp"
#include "DNSResolver.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>
#include <cerrno>
#include <sstream>

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
}