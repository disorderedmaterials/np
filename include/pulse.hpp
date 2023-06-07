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
        int sliceIndex;
        Pulse(PulseDefinition &definition_, double start_, double end_,int sliceIndex_ = 0) : definition(definition_), start(start_), end(end_), sliceIndex(sliceIndex_) {}
        Pulse(double start_, double end_) : start(start_), end(end_) {}
        Pulse() = default;
};

#endif // PULSE_H