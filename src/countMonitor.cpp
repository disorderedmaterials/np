#include "nexusFile.h"
#include "processors.h"
#include <fmt/core.h>
#include <numeric>

namespace Processors
{
void countMonitor(const std::vector<std::string> &inputNeXusFiles, int monitorIndex)
{
    /*
     * Count events in the specified monitor histogram
     */

    fmt::print("Counting events for monitor index {}...\n", monitorIndex);

    // Loop over input NeXuS files
    for (auto &nxsFileName : inputNeXusFiles)
    {
        // Open the NeXuS file and load in monitor counts
        NeXuSFile nxs(nxsFileName);
        nxs.prepareSpectraSpace();
        nxs.loadMonitorCounts();

        const auto &counts = nxs.monitorCounts().at(monitorIndex);
        auto sum = std::accumulate(counts.begin(), counts.end(), 0);
        fmt::print("{}\n", sum);
    }
}

} // namespace Processors
