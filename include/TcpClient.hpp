#pragma once

#include <string>
#include <map>

namespace NetworkingTools
{
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
        std::string location; // For storing the redirect location if the response is a redirect
    };

    class TcpClient
    {
    public:
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