#include "nexusFile.h"
#include "processors.h"
#include <fmt/core.h>
#include <fstream>

namespace Processors
{
void DumpDetector(const std::vector<std::string> &inputNeXusFiles, int spectrumId, bool firstOnly)
{
    /*
     * Dump histograms for the specified detector spectrum
     */

    fmt::print("Dumping histogram for detector spectrum {}...\n", spectrumId);

    // Loop over input NeXuS files
    for (auto &nxsFileName : inputNeXusFiles)
    {
        // Open the NeXuS file and load in detector counts
        NeXuSFile nxs(nxsFileName);
        nxs.loadDetectorCounts();

        // Open the output file
        std::ofstream output(fmt::format("{}.det.{}", nxsFileName, spectrumId).c_str());
        output << "# TCB/usec   Counts\n";
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
