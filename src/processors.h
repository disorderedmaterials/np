#pragma once

#include <string>
#include <vector>

// Forward Declarations
class Pulse;

namespace Processors
{
// Available Processing Modes
enum class ProcessingMode
{
    //    FORWARDS,
    //    BACKWARDS,
    //    BI_DIRECTIONAL,
    FORWARDS_SUMMED
    // NONE
};

// Perform forwards-summation processing
bool processForwardsSummed(const std::vector<std::string> &inputNeXusFiles, std::string_view outputFilePath,
                           const Pulse &pulseDefinition, int nSlices, double pulseDelta);
}; // namespace Processors
