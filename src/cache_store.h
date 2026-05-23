#pragma once

#include "data_packet.h"

#include <filesystem>
#include <string>
#include <vector>

class CacheStore {
public:
    CacheStore(std::filesystem::path cache_dir, std::filesystem::path sent_dir);

    void ensure_directories() const;
    std::filesystem::path save_packet(const DataPacket& packet) const;
    std::vector<std::filesystem::path> list_cached_packets() const;
    void mark_sent(const std::filesystem::path& cached_file) const;

private:
    std::filesystem::path cache_dir_;
    std::filesystem::path sent_dir_;
};
