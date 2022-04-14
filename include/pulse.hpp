#ifndef PULSE_H
#define PULSE_H

class Pulse {

    public:
        std::string label;
        double start;
        double end;
        Pulse(std::string label_, double start_, double end_) : label(label_), start(start_), end(end_) {}
        Pulse() = default;

};

#endif