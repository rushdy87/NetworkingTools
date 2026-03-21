#pragma once

#include <string>
#include <map>

namespace NetworkingTools
{
    struct ConnectResult
    {
        bool success;
        bool timedOut;
        std::string host;
        int port;
        std::string resolvedIP;
        std::string errorMessage;
    };

    struct HttpResponse
    {
        bool success;
        std::string host;
        int port;
        std::string resolvedIP;
        std::string errorMessage;

        std::string rawResponse;
        std::string body;

        int statusCode;
        std::string statusText;
        std::map<std::string, std::string> headers;

        bool isRedirect;
        std::string location;
    };

    class TcpClient
    {
    public:
        ConnectResult connectToServer(const std::string& host, int port, int timeoutSeconds = 5) const;
        ConnectResult connectToIPAddress(const std::string& ip, int port, int timeoutSeconds = 5) const;

        HttpResponse sendHttpRequest(const std::string& host, int port, const std::string& path = "/") const;
        HttpResponse sendHttpRequestFollowRedirect(const std::string& host, int port, const std::string& path = "/") const;

    private:
        HttpResponse parseHttpResponse(
            const std::string& rawResponse,
            const std::string& host,
            int port,
            const std::string& resolvedIP
        ) const;
    };
}