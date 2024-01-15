#include "window.h"

Window::Window(std::string_view id, double startTime, double duration) : id_(id), startTime_(startTime), duration_(duration) {}

// String ID
std::string_view Window::id() const { return id_; }

// Return start time (seconds since epoch) of the window
double Window::startTime() const { return startTime_; }

// Return end time (seconds since epoch) of the window
double Window::endTime() const { return startTime_ + duration_; }

// Shift start time by specified delta
void Window::shiftStartTime(double delta) { startTime_ += delta; }

// Return duration of the pulse
double Window::duration() const { return duration_; }
