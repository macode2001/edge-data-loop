#pragma once

#include <string>

struct HttpResponse {
    bool ok = false;
    int status_code = 0;
    std::string body;
    std::string error;
};

class HttpClient {
public:
    HttpResponse post_json(const std::string& host,
                           int port,
                           const std::string& path,
                           const std::string& json_body) const;
};
