#ifndef PERIOD_DEFINITION_H
#define PERIOD_DEFINITION_H

#include <pulse_definition.hpp>
#include <vector>


class PeriodDefinition {

    public:
        double duration;
        std::vector<PulseDefinition> pulses;
        PeriodDefinition(double duration_, std::vector<PulseDefinition> pulses_) : duration(duration_), pulses(pulses_) {}  
        PeriodDefinition() = default;
};

#endif