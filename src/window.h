#pragma once

#include <string>

class Window
{
    public:
    Window(std::string_view id, double startTime, double duration);
    ~Window() = default;

    private:
    // String ID
    std::string id_;
    // Start time (seconds since epoch) pulse
    double startTime_{0};
    // Duration of the pulse
    double duration_{0};

    public:
    // Return string ID
    [[nodiscard]] std::string_view id() const;
    // Return start time (seconds since epoch) of the window
    [[nodiscard]] double startTime() const;
    // Return end time (seconds since epoch) of the window
    [[nodiscard]] double endTime() const;
    // Shift start time by specified delta
    void shiftStartTime(double delta);
    // Return duration of the window
    [[nodiscard]] double duration() const;
};
