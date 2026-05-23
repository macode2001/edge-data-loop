#include "sensor_simulator.h"

#include <algorithm>
#include <chrono>
#include <utility>

SensorSimulator::SensorSimulator(std::string vehicle_id)
    : vehicle_id_(std::move(vehicle_id)), rng_(std::random_device{}()) {}

SensorFrame SensorSimulator::next_frame() {
    std::normal_distribution<double> accel_noise(0.0, 1.2);
    std::uniform_real_distribution<double> obstacle_dist(3.0, 80.0);
    std::bernoulli_distribution hard_brake_event(0.08);
    std::bernoulli_distribution obstacle_event(0.10);

    double acceleration = accel_noise(rng_);
    if (hard_brake_event(rng_)) {
        acceleration -= 6.0;
    }

    speed_mps_ = std::clamp(speed_mps_ + acceleration * 0.2, 0.0, 33.0);
    latitude_ += 0.00001 * speed_mps_;
    longitude_ += 0.000006 * speed_mps_;

    SensorFrame frame;
    frame.sequence = ++sequence_;
    frame.speed_mps = speed_mps_;
    frame.acceleration_mps2 = acceleration;
    frame.obstacle_distance_m = obstacle_event(rng_) ? obstacle_dist(rng_) / 10.0 : obstacle_dist(rng_);
    frame.latitude = latitude_;
    frame.longitude = longitude_;
    frame.timestamp = std::chrono::system_clock::now();
    return frame;
}

const std::string& SensorSimulator::vehicle_id() const {
    return vehicle_id_;
}
