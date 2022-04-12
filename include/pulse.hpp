#ifndef PULSE_H
#define PULSE_H

#include <string>

class Pulse {

    public:
        std::string label;
        double periodOffset;
        double duration;
        double pulseStart;
        double pulseEnd;
        Pulse(std::string label_, double periodOffset_, double duration_, bool isDefinition) {
            label = label_;
            if (isDefinition) {
                periodOffset = periodOffset_;
                duration = duration_;
            } else {
                pulseStart = periodOffset_;
                pulseEnd = duration_;
            }
        };
        Pulse() = default;

};

#endif