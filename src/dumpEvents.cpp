#include "nexusFile.h"
#include "processors.h"
#include "window.h"
#include <fmt/core.h>
#include <optional>

namespace Processors
{
// Dump events from specified spectrum, returning seconds since epoch for each
std::map<int, std::vector<double>> dumpEvents(const std::vector<std::string> &inputNeXusFiles, int detectorIndex,
                                              bool firstOnly)
{
    /*
     * Dump all events for the specified detector spectrum
     */

    fmt::print("Dumping all events from detector index {}...\n", detectorIndex);

    std::map<int, std::vector<double>> eventMap;
    std::optional<double> lastSecondsSinceEpoch;

    // Loop over input NeXuS files
    for (auto &nxsFileName : inputNeXusFiles)
    {
        // Open the Nexus file ready for use
        NeXuSFile nxs(nxsFileName);
        nxs.loadEventData();
        nxs.loadTimes();
        fmt::print("... file '{}' has {} events...\n", nxsFileName, nxs.eventTimes().size());

        auto eventStart = 0, eventEnd = 0;
        const auto &eventsPerFrame = nxs.eventsPerFrame();
        const auto &eventIndices = nxs.eventIndices();
        const auto &eventTimes = nxs.eventTimes();
        const auto &frameOffsets = nxs.frameOffsets();
        const auto spectrumId = nxs.spectrumForDetector(detectorIndex);

        // Loop over frames in the Nexus file
        for (auto frameIndex = 0; frameIndex < nxs.eventsPerFrame().size(); ++frameIndex)
        {
            // Set new end event index and get zero for frame
            eventEnd += eventsPerFrame[frameIndex];
            auto frameZero = frameOffsets[frameIndex];

            for (auto k = eventStart; k < eventEnd; ++k)
            {
                if (eventIndices[k] == spectrumId)
                {
                    auto eMicroSeconds = eventTimes[k];
                    auto eSeconds = eMicroSeconds * 0.000001;
                    auto eSecondsSinceEpoch = eSeconds + frameZero + nxs.startSinceEpoch();
                    if (lastSecondsSinceEpoch)
                        fmt::print("{:20.6f}  {:20.10f}  {:20.5f}  {}\n", eMicroSeconds, eSeconds + frameZero,
                                   eSecondsSinceEpoch, eSecondsSinceEpoch - *lastSecondsSinceEpoch);
                    else
                        fmt::print("{:20.6f}  {:20.10f}  {:20.5f}\n", eMicroSeconds, eSeconds + frameZero, eSecondsSinceEpoch);
                    eventMap[spectrumId].push_back(eSecondsSinceEpoch);
                    lastSecondsSinceEpoch = eSecondsSinceEpoch;
                    if (firstOnly)
                        return eventMap;
                }
            }

            // Update start event index
            eventStart = eventEnd;
        }
    }

    return eventMap;
}

} // namespace Processors
