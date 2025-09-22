#include "frameChunker.h"
#include "nexusFile.h"
#include "processors.h"
#include "window.h"
#include <ctime>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <optional>

namespace Processors
{
void dumpEventTimesEpoch(const std::vector<std::string> &inputNeXusFiles, int detectorIndex, bool toStdOut)
{
    /*
     * Get all events for the specified detector spectrum, returning seconds since epoch for each
     */

    char timeBuffer[20];

    fmt::print("Retrieving all events from detector index {}...\n", detectorIndex);

    // Loop over input NeXuS files
    for (auto &nxsFileName : inputNeXusFiles)
    {
        // Open the NeXuS file ready for use
        NeXuSFile nxs(nxsFileName);
        nxs.prepareSpectraSpace();

        std::optional<double> lastSecondsSinceEpoch;

        const auto spectrumId = nxs.spectrumForDetector(detectorIndex);
        fmt::print("NeXuS file spectrum ID for detector index {} is {}.\n", detectorIndex, spectrumId);

        std::ofstream fileOutput;
        if (!toStdOut)
            fileOutput.open(fmt::format("{}.events.{}", nxsFileName, detectorIndex).c_str());
        std::ostream &output = toStdOut ? std::cout : fileOutput;

        // Prepare the frame chunker
        FrameChunker frameChunker(nxs);

        // Write header
        output << fmt::format("# {:20s}  {:20s}  {:20s}  {:20s}  {}\n", "frame_offset(us)", "start_time_offset(s)",
                              "epoch_offset(s)", "local time", "delta(s)");

        // Read in chunked frames
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
                    if (eventIndices[i] != spectrumId)
                        continue;

                    auto eSeconds = eventTimes[i] * 0.000001;
                    auto eSecondsSinceEpoch = eSeconds + frame.timeZero + nxs.startSinceEpoch();
                    auto convertedSeconds = time_t(eSecondsSinceEpoch);
                    strftime(timeBuffer, 20, "%d/%m/%y  %H:%M:%S", std::localtime(&convertedSeconds));
                    if (lastSecondsSinceEpoch)
                        output << fmt::format("{:20.6f}  {:20.10f}  {:20.5f}  {:20s}  {}\n", eventTimes[i],
                                              eSeconds + frame.timeZero, eSecondsSinceEpoch, timeBuffer,
                                              eSecondsSinceEpoch - *lastSecondsSinceEpoch);
                    else
                        output << fmt::format("{:20.6f}  {:20.10f}  {:20.5f}  {:20s}\n", eventTimes[i],
                                              eSeconds + frame.timeZero, eSecondsSinceEpoch, timeBuffer);

                    lastSecondsSinceEpoch = eSecondsSinceEpoch;
                }
            }
        }

        if (!toStdOut)
            fileOutput.close();
    }
}

} // namespace Processors
