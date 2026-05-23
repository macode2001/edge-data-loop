#include "uploader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

Uploader::Uploader(CacheStore& store, std::string host, int port, std::string path)
    : store_(store), host_(std::move(host)), port_(port), path_(std::move(path)) {}

int Uploader::upload_once() const {
    int uploaded = 0;

    for (const auto& file : store_.list_cached_packets()) {
        std::ifstream in(file, std::ios::binary);
        std::ostringstream body;
        body << in.rdbuf();
        in.close();

        const auto response = client_.post_json(host_, port_, path_, body.str());
        if (response.ok) {
            store_.mark_sent(file);
            ++uploaded;
            std::cout << "[upload] sent " << file.filename().string() << "\n";
        } else {
            std::cout << "[upload] retry later " << file.filename().string()
                      << " status=" << response.status_code
                      << " error=" << response.error << "\n";
        }
    }
    return uploaded;
}
