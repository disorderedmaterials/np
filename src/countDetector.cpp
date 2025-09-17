#include "nexusFile.h"
#include "processors.h"
#include <fmt/core.h>
#include <numeric>

namespace Processors
{
void countDetector(const std::vector<std::string> &inputNeXusFiles, int detectorIndex)
{
    /*
     * Count events in the specified detector histogram
     */

    fmt::print("Counting events for detector index {}...\n", detectorIndex);

    // Loop over input NeXuS files
    for (auto &nxsFileName : inputNeXusFiles)
    {
        // Open the NeXuS file and load in detector counts
        NeXuSFile nxs(nxsFileName);
        nxs.prepareSpectraSpace();
        nxs.loadDetectorCounts();

        const auto spectrumId = nxs.spectrumForDetector(detectorIndex);
        fmt::print("{}\n", nxs.detectorHistograms().at(spectrumId).sum());
    }
}

} // namespace Processors
