#ifndef PULSE_DEFINITION_H
#define PULSE_DEFINITION_H

#include <string>

class PulseDefinition {

    public:
        std::string label;
        double periodOffset;
        double duration;
        PulseDefinition(std::string label_, double periodOffset_, double duration_) : label(label_), periodOffset(periodOffset_), duration(duration_) {}
        PulseDefinition() = default;

};

#endif // PULSE_DEFINITION_H