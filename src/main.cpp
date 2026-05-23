#include "cache_store.h"
#include "sensor_simulator.h"
#include "uploader.h"

#include <chrono>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>

namespace {

struct Config {
    std::string server_host = "127.0.0.1";
    int server_port = 8080;
    std::string upload_path = "/api/v1/packets";
    std::string cache_dir = "data/cache";
    std::string sent_dir = "data/sent";
    std::string vehicle_id = "car-001";
    int sample_interval_ms = 200;
    int periodic_trigger_every = 15;
    int max_packets = 60;
};

std::map<std::string, std::string> read_kv_file(const std::filesystem::path& path) {
    std::map<std::string, std::string> values;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        values[line.substr(0, pos)] = line.substr(pos + 1);
    }
    return values;
}

int to_int(const std::map<std::string, std::string>& values, const std::string& key, int fallback) {
    const auto it = values.find(key);
    if (it == values.end()) {
        return fallback;
    }
    return std::stoi(it->second);
}

std::string to_string_value(const std::map<std::string, std::string>& values,
                            const std::string& key,
                            const std::string& fallback) {
    const auto it = values.find(key);
    return it == values.end() ? fallback : it->second;
}

Config load_config() {
    Config config;
    const auto values = read_kv_file("config/agent.conf");
    config.server_host = to_string_value(values, "server_host", config.server_host);
    config.server_port = to_int(values, "server_port", config.server_port);
    config.upload_path = to_string_value(values, "upload_path", config.upload_path);
    config.cache_dir = to_string_value(values, "cache_dir", config.cache_dir);
    config.sent_dir = to_string_value(values, "sent_dir", config.sent_dir);
    config.vehicle_id = to_string_value(values, "vehicle_id", config.vehicle_id);
    config.sample_interval_ms = to_int(values, "sample_interval_ms", config.sample_interval_ms);
    config.periodic_trigger_every = to_int(values, "periodic_trigger_every", config.periodic_trigger_every);
    config.max_packets = to_int(values, "max_packets", config.max_packets);
    return config;
}

std::string detect_trigger(const SensorFrame& frame, int periodic_trigger_every) {
    if (frame.acceleration_mps2 < -5.0) {
        return "hard_brake";
    }
    if (frame.obstacle_distance_m < 6.0) {
        return "obstacle_close";
    }
    if (frame.sequence % periodic_trigger_every == 0) {
        return "periodic_sample";
    }
    return {};
}

std::string make_packet_id(const std::string& vehicle_id, const SensorFrame& frame) {
    std::ostringstream out;
    out << vehicle_id << "-" << to_epoch_ms(frame.timestamp) << "-" << frame.sequence;
    return out.str();
}

DataPacket build_packet(const std::string& vehicle_id,
                        const std::string& reason,
                        const std::deque<SensorFrame>& ring_buffer,
                        const SensorFrame& current) {
    DataPacket packet;
    packet.packet_id = make_packet_id(vehicle_id, current);
    packet.vehicle_id = vehicle_id;
    packet.trigger_reason = reason;
    packet.created_at = std::chrono::system_clock::now();
    packet.frames.assign(ring_buffer.begin(), ring_buffer.end());
    return packet;
}

}  // namespace

int run_agent() {
    const auto config = load_config();
    CacheStore store(config.cache_dir, config.sent_dir);
    SensorSimulator simulator(config.vehicle_id);
    Uploader uploader(store, config.server_host, config.server_port, config.upload_path);

    std::deque<SensorFrame> ring_buffer;
    constexpr size_t kRingBufferSize = 12;

    std::cout << "[agent] vehicle=" << config.vehicle_id
              << " server=" << config.server_host << ":" << config.server_port
              << " max_packets=" << config.max_packets << "\n";

    for (int i = 0; i < config.max_packets; ++i) {
        auto frame = simulator.next_frame();
        ring_buffer.push_back(frame);
        if (ring_buffer.size() > kRingBufferSize) {
            ring_buffer.pop_front();
        }

        const auto reason = detect_trigger(frame, config.periodic_trigger_every);
        if (!reason.empty()) {
            const auto packet = build_packet(config.vehicle_id, reason, ring_buffer, frame);
            const auto file = store.save_packet(packet);
            std::cout << "[cache] " << reason << " -> " << file.string() << "\n";
        }

        if (frame.sequence % 5 == 0) {
            uploader.upload_once();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(config.sample_interval_ms));
    }

    uploader.upload_once();
    std::cout << "[agent] finished\n";
    return 0;
}

int main() {
    try {
        return run_agent();
    } catch (const std::exception& error) {
        std::cerr << "[fatal] " << error.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "[fatal] unknown error\n";
        return 1;
    }
}
