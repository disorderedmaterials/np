#include "nexusFile.h"
#include "processors.h"
#include <fmt/core.h>
#include <fstream>

namespace Processors
{
// Dump histogram from specified spectrum
void DumpDetector(const std::vector<std::string> &inputNeXusFiles, int spectrumId, bool firstOnly)
{
    /*
     * Dump all events for the specified detector spectrum
     */

    fmt::print("Dumping histogram for detector spectrum {}...\n", spectrumId);

    // Loop over input NeXuS files
    for (auto &nxsFileName : inputNeXusFiles)
    {
        // Open the NeXuS file and load in detector counts
        NeXuSFile nxs(nxsFileName);
        nxs.loadDetectorCounts();

        // Open the output file
        std::ofstream output(fmt::format("{}.{}", nxsFileName, spectrumId).c_str());
        auto bin = 0;
        const auto &counts = nxs.detectorCounts().at(spectrumId);
        for (auto tof : nxs.tofBins())
        {
            output << fmt::format("{}  {}\n", tof, counts[bin++]);
        }
        output.close();
    }
}

} // namespace Processors
