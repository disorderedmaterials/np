#pragma once

#include <string>

class Pulse
{
    public:
    Pulse(std::string_view id, double startTime, double duration);
    Pulse() = default;

    private:
    // String ID
    std::string id_;
    // Start time (seconds since epoch) pulse
    double startTime_{0};
    // Duration of the pulse
    double duration_{0};
    // Frame counter
    int frameCounter_{0};

    public:
    // Return string ID
    [[nodiscard]] std::string_view id() const;
    // Return start time (seconds since epoch) of the pulse
    [[nodiscard]] double startTime() const;
    // Return end time (seconds since epoch) of the pulse
    [[nodiscard]] double endTime() const;
    // Shift start time by specified delta
    void shiftStartTime(double delta);
    // Return duration of the pulse
    [[nodiscard]] double duration() const;
    // Return frame counter
    int frameCounter() const;
    // Increase frame counter
    void incrementFrameCounter(int delta = 1);
};
