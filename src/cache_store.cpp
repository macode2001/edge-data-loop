#include "cache_store.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <utility>

CacheStore::CacheStore(std::filesystem::path cache_dir, std::filesystem::path sent_dir)
    : cache_dir_(std::move(cache_dir)), sent_dir_(std::move(sent_dir)) {}

void CacheStore::ensure_directories() const {
    std::filesystem::create_directories(cache_dir_);
    std::filesystem::create_directories(sent_dir_);
}

std::filesystem::path CacheStore::save_packet(const DataPacket& packet) const {
    ensure_directories();
    const auto target = cache_dir_ / (packet.packet_id + ".json");
    const auto temp = cache_dir_ / (packet.packet_id + ".tmp");

    // Write a temp file first, then rename it to .json after the write succeeds.
    std::filesystem::remove(temp);

    std::ofstream file(temp, std::ios::binary);
    if (!file) {
        throw std::runtime_error("failed to open cache file: " + temp.string());
    }
    file << packet_to_json(packet);
    file.close();

    std::filesystem::rename(temp, target);
    return target;
}

std::vector<std::filesystem::path> CacheStore::list_cached_packets() const {
    ensure_directories();
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(cache_dir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

void CacheStore::mark_sent(const std::filesystem::path& cached_file) const {
    ensure_directories();
    auto target = sent_dir_ / cached_file.filename();
    int duplicate_index = 1;

    while (std::filesystem::exists(target)) {
        target = sent_dir_ /
            (cached_file.stem().string() + "-dup-" + std::to_string(duplicate_index) +
             cached_file.extension().string());
        ++duplicate_index;
    }

    std::filesystem::rename(cached_file, target);
}
