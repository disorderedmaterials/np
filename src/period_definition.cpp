#include "period_definition.h"

#include <iostream>

bool PeriodDefinition::isValid()
{

    for (const auto &pd : pulseDefinitions)
    {
        if (pd.periodOffset > duration)
        {
            std::cerr << "ERROR: Pulse definition with label " << pd.label << " has period offset (" << pd.periodOffset
                      << ")greater than period duration (" << duration << ")." << std::endl;
            return false;
        }
        if ((pd.periodOffset + pd.duration) > duration)
        {
            std::cerr << "ERROR: Pulse definition with label " << pd.label << " has end time (" << pd.periodOffset + pd.duration
                      << ")greater than period duration (" << duration << ")." << std::endl;
            return false;
        }
    }
    return true;
}
