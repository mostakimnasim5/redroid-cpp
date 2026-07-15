#include "HttpClient.hpp"
#include "Logger.hpp"
#include <sstream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#endif

namespace AntiDetect {

HttpClient::HttpClient()
    : m_timeout(30)
    , m_followRedirects(true)
{
}

HttpClient::~HttpClient() {
}

void HttpClient::setTimeout(int seconds) {
    m_timeout = seconds;
}

void HttpClient::setFollowRedirects(bool follow) {
    m_followRedirects = follow;
}

void HttpClient::setUserAgent(const std::string& userAgent) {
    m_userAgent = userAgent;
}

void HttpClient::setProxy(const std::string& proxy) {
    m_proxy = proxy;
}

std::string HttpClient::getLastError() const {
    return m_lastError;
}

std::string parseUrl(const std::string& url, std::string& host, std::string& path, int& port) {
    std::string protocol;
    size_t protocolEnd = url.find("://");
    
    if (protocolEnd != std::string::npos) {
        protocol = url.substr(0, protocolEnd);
        host = url.substr(protocolEnd + 3);
    } else {
        host = url;
    }
    
    size_t pathStart = host.find('/');
    if (pathStart != std::string::npos) {
        path = host.substr(pathStart);
        host = host.substr(0, pathStart);
    } else {
        path = "/";
    }
    
    size_t portStart = host.find(':');
    if (portStart != std::string::npos) {
        std::string portStr = host.substr(portStart + 1);
        port = std::stoi(portStr);
        host = host.substr(0, portStart);
    } else {
        port = (protocol == "https") ? 443 : 80;
    }
    
    return protocol;
}

HttpResponse HttpClient::get(const std::string& url, const std::map<std::string, std::string>& headers) {
    return executeRequest("GET", url, "", headers);
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body,
                              const std::map<std::string, std::string>& headers) {
    return executeRequest("POST", url, body, headers);
}

HttpResponse HttpClient::put(const std::string& url, const std::string& body,
                              const std::map<std::string, std::string>& headers) {
    return executeRequest("PUT", url, body, headers);
}

HttpResponse HttpClient::del(const std::string& url, const std::map<std::string, std::string>& headers) {
    return executeRequest("DELETE", url, "", headers);
}

HttpResponse HttpClient::request(const HttpRequest& request) {
    std::map<std::string, std::string> headers = request.headers;
    if (!request.body.empty()) {
        headers["Content-Length"] = std::to_string(request.body.length());
    }
    
    return executeRequest(request.method, request.url, request.body, headers);
}

HttpResponse HttpClient::executeRequest(const std::string& method, const std::string& url,
                                        const std::string& body,
                                        const std::map<std::string, std::string>& headers) {
    HttpResponse response;
    response.statusCode = 0;
    
    std::string host, path;
    int port;
    std::string protocol = parseUrl(url, host, path, port);
    
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        response.error = "WSAStartup failed";
        return response;
    }
#endif
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        response.error = "Failed to create socket";
#ifdef _WIN32
        WSACleanup();
#endif
        return response;
    }
    
    struct hostent* server = gethostbyname(host.c_str());
    if (server == nullptr) {
        response.error = "Could not resolve host: " + host;
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return response;
    }
    
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);
    
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        response.error = "Connection failed";
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return response;
    }
    
    std::stringstream requestStream;
    requestStream << method << " " << path << " HTTP/1.1\r\n";
    requestStream << "Host: " << host << "\r\n";
    
    if (!m_userAgent.empty()) {
        requestStream << "User-Agent: " << m_userAgent << "\r\n";
    } else {
        requestStream << "User-Agent: AntiDetectPro/1.0\r\n";
    }
    
    for (const auto& header : headers) {
        requestStream << header.first << ": " << header.second << "\r\n";
    }
    
    if (!body.empty()) {
        requestStream << "Content-Type: application/x-www-form-urlencoded\r\n";
        requestStream << "Content-Length: " << body.length() << "\r\n";
    }
    
    requestStream << "Connection: close\r\n";
    requestStream << "\r\n";
    
    if (!body.empty()) {
        requestStream << body;
    }
    
    std::string requestStr = requestStream.str();
    
    int sent = send(sock, requestStr.c_str(), requestStr.length(), 0);
    if (sent < 0) {
        response.error = "Failed to send request";
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return response;
    }
    
    std::string responseData;
    char buffer[4096];
    
    while (true) {
        int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            break;
        }
        buffer[bytesReceived] = '\0';
        responseData += buffer;
    }
    
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    
    size_t headerEnd = responseData.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        response.error = "Invalid HTTP response";
        return response;
    }
    
    std::string headerSection = responseData.substr(0, headerEnd);
    std::string bodySection = responseData.substr(headerEnd + 4);
    
    std::istringstream headerStream(headerSection);
    std::string statusLine;
    std::getline(headerStream, statusLine);
    
    if (statusLine.length() >= 12) {
        std::string statusCodeStr = statusLine.substr(9, 3);
        try {
            response.statusCode = std::stoi(statusCodeStr);
        } catch (...) {
            response.statusCode = 0;
        }
    }
    
    std::string headerLine;
    while (std::getline(headerStream, headerLine)) {
        size_t colonPos = headerLine.find(':');
        if (colonPos != std::string::npos) {
            std::string key = headerLine.substr(0, colonPos);
            std::string value = headerLine.substr(colonPos + 1);
            
            while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
                value = value.substr(1);
            }
            
            while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) {
                value.pop_back();
            }
            
            response.headers[key] = value;
        }
    }
    
    response.body = bodySection;
    
    if (m_followRedirects && (response.statusCode == 301 || response.statusCode == 302)) {
        auto it = response.headers.find("Location");
        if (it != response.headers.end()) {
            return get(it->second, headers);
        }
    }
    
    return response;
}

}
