#include <period_definition.hpp>

#include <iostream>

bool PeriodDefinition::isValid() {

    for (const auto &pulse : pulses) {
        if (pulse.periodOffset > duration) {
            std::cerr << "ERROR: Pulse with label " << pulse.label << " has period offset (" << pulse.periodOffset << ")greater than period duration (" << duration << ")." << std::endl;
            return false;
        }
        if ((pulse.periodOffset + pulse.duration) > duration) {
            std::cerr << "ERROR: Pulse with label " << pulse.label << " has end time (" << pulse.periodOffset + pulse.duration << ")greater than period duration (" << duration << ")." << std::endl;
            return false;
        }
    }
    return true;
}