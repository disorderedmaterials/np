#ifndef PULSE_H
#define PULSE_H

#include "pulse_definition.hpp"

class Pulse {

    public:
        PulseDefinition definition;
        double start;
        double end;
        std::string startRun;
        std::string endRun;
        Pulse(PulseDefinition &definition_, double start_, double end_) : start(start_), end(end_) {}
        Pulse(double start_, double end_) : start(start_), end(end_) {}
        Pulse() = default;

        bool inBounds(double lower, double upper) {return ((start >= lower) && (end <= upper));}
};

#endif // PULSE_H