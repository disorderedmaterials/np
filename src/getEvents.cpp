#include "nexusFile.h"
#include "processors.h"
#include "window.h"
#include <ctime>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <optional>
#include "eventChunker.h"

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
        EventChunker eventChunker(nxs);
        // nxs.loadEventData();

        std::optional<double> lastSecondsSinceEpoch;
        auto eventStart = 0, eventEnd = 0;
        // const auto &eventsPerFrame = nxs.eventsPerFrame();
        // const auto &eventIndices = nxs.eventIndices();
        // const auto &eventTimes = nxs.eventTimes();
        // const auto &frameOffsets = nxs.frameOffsets();
        const auto spectrumId = nxs.spectrumForDetector(detectorIndex);
        fmt::print("NeXuS file spectrum ID for detector index {} is {}.\n", detectorIndex, spectrumId);

        std::ofstream fileOutput;

        if (!toStdOut)
            fileOutput.open(fmt::format("{}.events.{}", nxsFileName, detectorIndex).c_str());

        std::ostream &output = toStdOut ? std::cout : fileOutput;
        output << fmt::format("# {:20s}  {:20s}  {:20s}  {:20s}  {}\n", "frame_offset(us)", "start_time_offset(s)",
                              "epoch_offset(s)", "local time", "delta(s)");

        while (eventChunker.getNextFrameData())
        {
            auto &frameData = eventChunker.frameData();
            for (const auto &frame : frameData)
            {
                const auto &eventIndices = frame.detectorIndices;
                const auto &eventTimes = frame.times;

                // Loop over events in this frame
                for (auto i = 0; i < eventIndices.size(); ++i)
                {
                    // printf("i = %i\n", i);
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
                        output << fmt::format("{:20.6f}  {:20.10f}  {:20.5f}  {:20s}\n", eventTimes[i], eSeconds + frame.timeZero,
                                              eSecondsSinceEpoch, timeBuffer);
        
                    lastSecondsSinceEpoch = eSecondsSinceEpoch;
                }
            }
        }
        // // Loop over frames in the NeXuS file
        // for (auto frameIndex = 0; frameIndex < nxs.eventsPerFrame().size(); ++frameIndex)
        // {
        //     // Set new end event index and get zero for frame
        //     eventEnd += eventsPerFrame[frameIndex];
        //     auto frameZero = frameOffsets[frameIndex];
        //
        //     for (auto k = eventStart; k < eventEnd; ++k)
        //     {
        //         if (eventIndices[k] == spectrumId)
        //         {
        //             auto eMicroSeconds = eventTimes[k];
        //             auto eSeconds = eMicroSeconds * 0.000001;
        //             auto eSecondsSinceEpoch = eSeconds + frameZero + nxs.startSinceEpoch();
        //             auto convertedSeconds = time_t(eSecondsSinceEpoch);
        //             strftime(timeBuffer, 20, "%d/%m/%y  %H:%M:%S", std::localtime(&convertedSeconds));
        //             if (lastSecondsSinceEpoch)
        //                 output << fmt::format("{:20.6f}  {:20.10f}  {:20.5f}  {:20s}  {}\n", eMicroSeconds,
        //                                       eSeconds + frameZero, eSecondsSinceEpoch, timeBuffer,
        //                                       eSecondsSinceEpoch - *lastSecondsSinceEpoch);
        //             else
        //                 output << fmt::format("{:20.6f}  {:20.10f}  {:20.5f}  {:20s}\n", eMicroSeconds, eSeconds + frameZero,
        //                                       eSecondsSinceEpoch, timeBuffer);
        //
        //             lastSecondsSinceEpoch = eSecondsSinceEpoch;
        //         }
        //     }
        //
        //     // Update start event index
        //     eventStart = eventEnd;
        // }

        if (!toStdOut)
            fileOutput.close();
    }
}

} // namespace Processors
