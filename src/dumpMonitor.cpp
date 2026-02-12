#include "nexusFile.h"
#include "processors.h"
#include <filesystem>
#include <fmt/core.h>
#include <fstream>

namespace Processors
{
void dumpMonitor(const std::vector<std::string> &inputNeXusFiles, int monitorIndex)
{
    /*
     * Dump histograms for the specified monitor index
     */

    fmt::print("Dumping histogram for monitor index {}...\n", monitorIndex);

    // Loop over input NeXuS files
    for (auto &nxsFileName : inputNeXusFiles)
    {
        // Open the NeXuS file and load in monitor counts
        NeXuSFile nxs(nxsFileName);
        nxs.prepareSpectraSpace();
        nxs.loadMonitorCounts();

        // Open the output file
        auto filename = std::string(std::filesystem::path(nxsFileName).filename().c_str());
        std::ofstream output(fmt::format("{}.mon.{}", filename, monitorIndex).c_str());
        output << "# TCB/us   Counts\n";

        const auto &counts = nxs.monitorCounts().at(monitorIndex);
        auto bin = 0;
        for (auto tof : nxs.tofBoundaries())
        {
            output << fmt::format("{}  {}\n", tof, counts[bin++]);
        }
        output.close();
    }
}

} // namespace Processors
