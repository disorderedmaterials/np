#ifndef PERIOD_H
#define PERIOD_H

#include <pulse.hpp>
#include <vector>


class Period {

    public:
        double duration;
        std::vector<Pulse> pulses;

        Period(double duration_, std::vector<Pulse> pulses_) : duration(duration_), pulses(pulses_) {}  

};

#endif