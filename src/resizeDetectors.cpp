#include "nexusFile.h"
#include "processors.h"
#include <fmt/core.h>
#include <numeric>

namespace Processors
{
void resizeDetectors(const std::vector<std::string> &inputNeXusFiles, int totalDetectorCount)
{
    /*
     * Resize detector space, deleting extra or adding empty as necessary
     */

    fmt::print("Resizing number of detector spectra to {}...\n", totalDetectorCount);

    // Loop over input NeXuS files
    for (auto &nxsFileName : inputNeXusFiles)
    {
        // Open the NeXuS file and load in detector counts
        NeXuSFile nxs(nxsFileName);
        nxs.prepareSpectraSpace();
        nxs.loadDetectorCounts();

        if (nxs.nDetectors() == totalDetectorCount)
        {
            fmt::print("Skipping - file already contains the correct number of detectors.\n");
            continue;
        }

        // Adjust detector space size
        while (nxs.nDetectors() != totalDetectorCount)
        {
            if (nxs.nDetectors() > totalDetectorCount)
            {
                // Remove the last (highest index) detector spectrum
                auto specID = nxs.removeLastDetector();
                fmt::print(" ... Removed detector {} (spectrum {})\n", specID, specID - 1);
            }
            else
            {
                // Add a new detector on the end
                auto specID = nxs.appendEmptyDetector();
                fmt::print(" ... Appended empty detector {} (spectrum {})\n", specID, specID - 1);
            }
        }

        // Write the new data
        nxs.saveModifiedData();
    }
}

} // namespace Processors
