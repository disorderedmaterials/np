#include "nexusFile.h"
#include "processors.h"
#include <fmt/core.h>
#include <fstream>

namespace Processors
{
void DumpMonitor(const std::vector<std::string> &inputNeXusFiles, int spectrumId, bool firstOnly)
{
    /*
     * Dump histograms for the specified monitor spectrum
     */

    fmt::print("Dumping histogram for monitor spectrum {}...\n", spectrumId);

    // Loop over input NeXuS files
    for (auto &nxsFileName : inputNeXusFiles)
    {
        // Open the NeXuS file and load in monitor counts
        NeXuSFile nxs(nxsFileName);
        nxs.loadMonitorCounts();

        // Open the output file
        std::ofstream output(fmt::format("{}.mon.{}", nxsFileName, spectrumId).c_str());
        output << "# TCB/usec   Counts\n";
        auto bin = 0;
        const auto &counts = nxs.monitorCounts().at(spectrumId);
        for (auto tof : nxs.tofBins())
        {
            output << fmt::format("{}  {}\n", tof, counts[bin++]);
        }
        output.close();
    }
}

} // namespace Processors
