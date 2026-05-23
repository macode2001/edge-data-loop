#pragma once

#include "data_packet.h"

#include <random>
#include <string>

class SensorSimulator {
public:
    explicit SensorSimulator(std::string vehicle_id);

    SensorFrame next_frame();
    const std::string& vehicle_id() const;

private:
    std::string vehicle_id_;
    int sequence_ = 0;
    double speed_mps_ = 14.0;
    double latitude_ = 31.2360;
    double longitude_ = 121.4990;
    std::mt19937 rng_;
};
