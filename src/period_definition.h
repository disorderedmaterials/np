#ifndef PERIOD_DEFINITION_H
#define PERIOD_DEFINITION_H

#include "pulse_definition.h"
#include <vector>


class PeriodDefinition {

    public:
        double duration;
        std::vector<PulseDefinition> pulseDefinitions;
        PeriodDefinition(double duration_, std::vector<PulseDefinition> pulseDefinitions_) : duration(duration_), pulseDefinitions(pulseDefinitions_) {}  
        PeriodDefinition() = default;
        bool isValid();
};

#endif // PERIOD_DEFINITION_H
