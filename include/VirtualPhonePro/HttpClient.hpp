#pragma once

#include <string>
#include <map>
#include <vector>

namespace AntiDetect {

struct HttpResponse {
    int statusCode;
    std::string body;
    std::map<std::string, std::string> headers;
    std::string error;
};

struct HttpRequest {
    std::string url;
    std::string method;
    std::map<std::string, std::string> headers;
    std::string body;
    int timeout;
    bool followRedirects;
};

class HttpClient {
public:
    HttpClient();
    ~HttpClient();
    
    void setTimeout(int seconds);
    void setFollowRedirects(bool follow);
    void setUserAgent(const std::string& userAgent);
    void setProxy(const std::string& proxy);
    
    HttpResponse get(const std::string& url, const std::map<std::string, std::string>& headers = {});
    HttpResponse post(const std::string& url, const std::string& body, 
                     const std::map<std::string, std::string>& headers = {});
    HttpResponse put(const std::string& url, const std::string& body,
                     const std::map<std::string, std::string>& headers = {});
    HttpResponse del(const std::string& url, const std::map<std::string, std::string>& headers = {});
    
    HttpResponse request(const HttpRequest& request);
    
    std::string getLastError() const;

private:
    HttpResponse executeRequest(const std::string& method, const std::string& url,
                               const std::string& body,
                               const std::map<std::string, std::string>& headers);
    
    int m_timeout;
    bool m_followRedirects;
    std::string m_userAgent;
    std::string m_proxy;
    std::string m_lastError;
};

}
