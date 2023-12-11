#ifndef PERIOD_H
#define PERIOD_H

#include "period_definition.h"
#include "pulse.h"
#include <vector>

class Period
{

    public:
    PeriodDefinition definition;
    double start;
    double end;
    std::vector<Pulse> pulses;
    Period(PeriodDefinition &definition_, double start_, double end_, std::vector<Pulse> pulses_)
        : definition(definition_), start(start_), end(end_), pulses(pulses_)
    {
    }
    Period() = default;
    bool isComplete();
};

#endif // PERIOD_H
