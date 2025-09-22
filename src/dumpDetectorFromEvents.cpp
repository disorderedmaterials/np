#include "frameChunker.h"
#include "nexusFile.h"
#include "processors.h"
#include <filesystem>
#include <fmt/core.h>
#include <fstream>

namespace Processors
{
void dumpDetectorFromEvents(const std::vector<std::string> &inputNeXusFiles, int detectorIndex)
{
    /*
     * Dump histograms for the specified detector index after constructing it from event data
     */

    fmt::print("Dumping histogram for detector index {}...\n", detectorIndex);

    // Loop over input NeXuS files
    for (auto &nxsFileName : inputNeXusFiles)
    {
        // Open the NeXuS file and prepare spectra space for histograms
        NeXuSFile nxs(nxsFileName);
        nxs.prepareSpectraSpace();

        // Get spectrum index and histogram
        const auto spectrumId = nxs.spectrumForDetector(detectorIndex);
        auto &histo = nxs.detectorHistograms().at(spectrumId);

        // Read in chunked frames
        FrameChunker frameChunker(nxs);
        while (frameChunker.getNextFrameData())
        {
            auto &frameData = frameChunker.frameData();
            for (const auto &frame : frameData)
            {
                const auto &eventIndices = frame.detectorIndices;
                const auto &eventTimes = frame.times;

                // Loop over events in this frame
                for (auto i = 0; i < eventIndices.size(); ++i)
                {
                    if (eventIndices[i] == spectrumId)
                        histo.bin(eventTimes[i]);
                }
            }
        }

        // Open the output file
        auto filename = std::string(std::filesystem::path(nxsFileName).filename().c_str());
        std::ofstream output(fmt::format("{}.detev.{}", filename, detectorIndex).c_str());
        output << fmt::format("# TCB/us   Counts  [detector index {}, spectrum index = {}]\n", detectorIndex, spectrumId);
        auto bin = 0;
        for (auto tof : nxs.tofBoundaries())
        {
            output << fmt::format("{}  {}\n", tof, histo.value(bin++));
        }
        output.close();
    }
}

} // namespace Processors
