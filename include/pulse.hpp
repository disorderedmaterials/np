#ifndef PULSE_H
#define PULSE_H

#include <string>

class Pulse {

    public:
        std::string label;
        double periodOffset;
        double duration;
        Pulse(std::string label_, double periodOffset_, double duration_) : label(label_), periodOffset(periodOffset_), duration(duration_) {}
        Pulse() = default;
};

#endif