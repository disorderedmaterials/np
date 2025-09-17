#include "nexusFile.h"
#include "processors.h"
#include <filesystem>
#include <fmt/core.h>
#include <fstream>

namespace Processors
{
void dumpDetector(const std::vector<std::string> &inputNeXusFiles, int detectorIndex)
{
    /*
     * Dump histograms for the specified detector index
     */

    fmt::print("Dumping histogram for detector index {}...\n", detectorIndex);

    // Loop over input NeXuS files
    for (auto &nxsFileName : inputNeXusFiles)
    {
        // Open the NeXuS file and load in detector counts
        NeXuSFile nxs(nxsFileName);
        nxs.prepareSpectraSpace();
        nxs.loadDetectorCounts();

        const auto spectrumId = nxs.spectrumForDetector(detectorIndex);

        // Open the output file
        auto filename = std::string(std::filesystem::path(nxsFileName).filename().c_str());
        std::ofstream output(fmt::format("{}.det.{}", filename, detectorIndex).c_str());
        output << fmt::format("# TCB/us   Counts  [detector index {}, spectrum index = {}]\n", detectorIndex, spectrumId);
        auto bin = 0;
        const auto &histo = nxs.detectorHistograms().at(spectrumId);
        for (auto tof : nxs.tofBoundaries())
        {
            output << fmt::format("{}  {}\n", tof, histo.value(bin++));
        }
        output.close();
    }
}

} // namespace Processors
