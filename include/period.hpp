#ifndef PERIOD_H
#define PERIOD_H

#include <pulse_definition.hpp>
#include <vector>


class Period {

    public:
        double duration;
        std::vector<PulseDefinition> pulses;
        Period(double duration_, std::vector<PulseDefinition> pulses_) : duration(duration_), pulses(pulses_) {}  
        Period() = default;
        bool isValid();
};

#endif // PERIOD_H