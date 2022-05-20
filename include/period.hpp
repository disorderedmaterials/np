#ifndef PERIOD_H
#define PERIOD_H

#include <pulse.hpp>
#include "period_definition.hpp"
#include <vector>


class Period {

    public:
        PeriodDefinition definition;
        double start;
        double end;
        std::vector<Pulse> pulses;
        Period(PeriodDefinition &definition_, double start_, double end_, std::vector<Pulse> pulses_) : definition(definition_), start(start_), end(end_), pulses(pulses_) {}
        Period() = default;
        bool isComplete() {return true;}

};

#endif // PERIOD_H