#pragma once

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

struct SensorFrame {
    int sequence = 0;
    double speed_mps = 0.0;
    double acceleration_mps2 = 0.0;
    double obstacle_distance_m = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    std::chrono::system_clock::time_point timestamp;
};

struct DataPacket {
    std::string packet_id;
    std::string vehicle_id;
    std::string trigger_reason;
    std::vector<SensorFrame> frames;
    std::chrono::system_clock::time_point created_at;
};

inline long long to_epoch_ms(std::chrono::system_clock::time_point ts) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               ts.time_since_epoch())
        .count();
}

inline std::string json_escape(const std::string& value) {
    std::ostringstream out;
    for (char ch : value) {
        switch (ch) {
            case '"':
                out << "\\\"";
                break;
            case '\\':
                out << "\\\\";
                break;
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\t':
                out << "\\t";
                break;
            default:
                out << ch;
                break;
        }
    }
    return out.str();
}

inline double anonymize_coord(double value) {
    return static_cast<long long>(value * 1000.0) / 1000.0;
}

inline std::string packet_to_json(const DataPacket& packet) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3);
    out << "{";
    out << "\"packet_id\":\"" << json_escape(packet.packet_id) << "\",";
    out << "\"vehicle_id\":\"" << json_escape(packet.vehicle_id) << "\",";
    out << "\"trigger_reason\":\"" << json_escape(packet.trigger_reason) << "\",";
    out << "\"created_at_ms\":" << to_epoch_ms(packet.created_at) << ",";
    out << "\"frame_count\":" << packet.frames.size() << ",";
    out << "\"frames\":[";
    for (size_t i = 0; i < packet.frames.size(); ++i) {
        const auto& frame = packet.frames[i];
        if (i > 0) {
            out << ",";
        }
        out << "{";
        out << "\"sequence\":" << frame.sequence << ",";
        out << "\"timestamp_ms\":" << to_epoch_ms(frame.timestamp) << ",";
        out << "\"speed_mps\":" << frame.speed_mps << ",";
        out << "\"acceleration_mps2\":" << frame.acceleration_mps2 << ",";
        out << "\"obstacle_distance_m\":" << frame.obstacle_distance_m << ",";
        out << "\"latitude_grid\":" << anonymize_coord(frame.latitude) << ",";
        out << "\"longitude_grid\":" << anonymize_coord(frame.longitude);
        out << "}";
    }
    out << "]";
    out << "}";
    return out.str();
}
