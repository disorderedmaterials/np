#include "pulse.h"

Pulse::Pulse(std::string_view id, double startTime, double duration) : id_(id), startTime_(startTime), duration_(duration) {}

// String ID
std::string_view Pulse::id() const { return id_; }

// Return start time (seconds since epoch) of the pulse
double Pulse::startTime() const { return startTime_; }

// Return end time (seconds since epoch) of the pulse
double Pulse::endTime() const { return startTime_ + duration_; }

// Shift start time by specified delta
void Pulse::shiftStartTime(double delta) { startTime_ += delta; }

// Return duration of the pulse
double Pulse::duration() const { return duration_; }

// Return frame counter
int Pulse::frameCounter() const { return frameCounter_; }

// Increase frame counter
void Pulse::incrementFrameCounter(int delta) { frameCounter_ += delta; }
