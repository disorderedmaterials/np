#pragma once

#include <string>
#include <vector>

// Forward Declarations
class NeXuSFile;
class Window;

namespace Processors
{
// Processing Direction
enum class ProcessingDirection
{
    Forwards
};
// Available Processing Modes
enum class ProcessingMode
{
    None,
    // Individual,
    Summed
};
// Available Post-Processing Types
enum class PostProcessingMode
{
    None,
    ScaleDetectors,
    ScaleMonitors
};

// Get Events
std::map<int, std::vector<double>> getEvents(const std::vector<std::string> &inputNeXusFiles, int detectorId,
                                             bool firstOnly = false);
// Perform summed processing
[[nodiscard]] std::vector<std::pair<Window, NeXuSFile>> processSummed(const std::vector<std::string> &inputNeXusFiles,
                                                                      std::string_view outputFilePath,
                                                                      const Window &windowDefinition, int nSlices,
                                                                      double windowDelta);
}; // namespace Processors
