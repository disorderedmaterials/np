#pragma once

#include "period_definition.h"
#include "pulse.h"
#include <vector>

class Period
{
    public:
    Period(PeriodDefinition &definition, double startTime, double endTime, std::vector<Pulse> pulses)
        : definition_(definition), startTime_(startTime), endTime_(endTime), pulses_(pulses)
    {
    }
    Period() = default;

    private:
    PeriodDefinition definition_;
    double startTime_;
    double endTime_;
    std::vector<Pulse> pulses_;

    public:
    bool isComplete();
};
