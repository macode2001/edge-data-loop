#pragma once

#include "cache_store.h"
#include "http_client.h"

#include <string>

class Uploader {
public:
    Uploader(CacheStore& store, std::string host, int port, std::string path);

    int upload_once() const;

private:
    CacheStore& store_;
    HttpClient client_;
    std::string host_;
    int port_ = 0;
    std::string path_;
};
