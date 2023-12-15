#include "nexusFile.h"
#include "period.h"
#include "processors.h"
#include <iomanip>
#include <sstream>

namespace Processors
{
// Perform forwards-summation processing
bool processForwardsSummed(const std::vector<std::string> &inputNeXusFiles, std::string_view outputFilePath,
                           const Pulse &pulseDefinition, int nSlices, double pulseDelta)
{
    /*
     * From our main pulseDefinition we will continually propagate it forwards in time (by pulseDelta) splitting it into
     * nSlices and until we go over the end time of the current file.
     */

    printf("Processing FORWARDS_SUMMED...\n");
    printf("Pulse start time is %16.2f\n", pulseDefinition.startTime());

    // Generate a new set of Pulse "slices" and associated output NeXuS files to sum data in to
    const auto sliceDuration = pulseDefinition.duration() / nSlices;
    auto sliceStartTime = pulseDefinition.startTime();
    std::vector<std::pair<Pulse, NeXuSFile>> slices;
    for (auto i = 0; i < nSlices; ++i)
    {
        std::stringstream outputFileName;
        outputFileName << outputFilePath << pulseDefinition.id() << "-" << std::to_string((int)pulseDefinition.startTime());
        if (nSlices > 1)
            outputFileName << "-" << std::setw(3) << std::setfill('0') << (i + 1);

        std::stringstream sliceName;
        sliceName << pulseDefinition.id() << i + 1;

        auto &[pulse, nexus] = slices.emplace_back(Pulse(sliceName.str(), sliceStartTime, sliceDuration), NeXuSFile());

        nexus.templateFile(inputNeXusFiles[0], outputFileName.str());

        sliceStartTime += sliceDuration;
    }

    // Initialise the slice iterator and Pulse / NeXuSFile references
    auto sliceIt = slices.begin();

    // Loop over input Nexus files
    for (auto &nxsFileName : inputNeXusFiles)
    {
        // Open the Nexus file ready for use
        NeXuSFile nxs(nxsFileName);
        printf("Load frame counts...\n");
        nxs.loadFrameCounts();
        printf("Load event data...\n");
        nxs.loadEventData();
        printf("Load times....\n");
        nxs.loadTimes();
        printf("... file '%s' has %i goodframes and %li events...\n", nxsFileName.c_str(), nxs.totalGoodFrames(),
               nxs.eventTimes().size());

        auto eventStart = 0, eventEnd = 0;
        const auto &eventsPerFrame = nxs.eventsPerFrame();
        const auto &eventIndices = nxs.eventIndices();
        const auto &eventTimes = nxs.eventTimes();
        const auto &frameOffsets = nxs.frameOffsets();

        // Loop over frames in the Nexus file
        for (auto frameIndex = 0; frameIndex < nxs.eventsPerFrame().size(); ++frameIndex)
        {
            // Set new end event index and get zero for frame
            eventEnd += eventsPerFrame[frameIndex];
            auto frameZero = frameOffsets[frameIndex] + nxs.startSinceEpoch();

            // If the current slice end time is less than the frame zero, iterate the slices.
            while (sliceIt->first.endTime() < frameZero)
            {
                sliceIt++;

                // If we have run out of slices propagate the set forward.
                if (sliceIt == slices.end())
                {
                    sliceIt = slices.begin();
                    for (auto &&[slice, _unused] : slices)
                        slice.shiftStartTime(pulseDelta);
                    printf("Propagated pulse forwards... new start time is %16.2f\n", sliceIt->first.startTime());
                }
            }

            // If the frame zero is less than the start time of the current pulse, move on to the next frame
            if (frameZero < sliceIt->first.startTime())
                continue;

            // Sanity check!
            if (frameZero > sliceIt->first.endTime())
                throw(std::runtime_error("Somebody's done something wrong here....\n"));

            // Grab the destination datafile for this pulse and bin events
            auto &destinationHistograms = sliceIt->second.detectorHistograms();
            for (int k = eventStart; k < eventEnd; ++k)
            {
                auto id = eventIndices[k];
                if (id > 0)
                    gsl_histogram_accumulate(destinationHistograms[id], eventTimes[k], 1.0);
            }

            // Increment the frame counter for this pulse
            sliceIt->first.incrementFrameCounter(1);

            // Update start event index
            eventStart = eventEnd;
        }
    }

    // Renormalise counts in slices to match the full monitor counts templated / transferred from the first file
    for (auto &&[slice, outpuNeXuSFile] : slices)
    {
        printf("Slice '%s' (%16.2f -> %16.2f) has %i good frames in total.\n", std::string(slice.id()).c_str(),
               slice.startTime(), slice.endTime(), slice.frameCounter());
    }

    // Save output files
    for (auto &&[slice, outputNeXuSFile] : slices)
    {
        printf("Writing data to output NeXuS file '%s' for slice '%s'...\n", outputNeXuSFile.filename().c_str(),
               std::string(slice.id()).c_str());

        if (!outputNeXuSFile.saveDetectorHistograms())
            return false;
        //        diagnosticFile << nexus.getOutpath() << " " << nexus.nProcessedGoodFrames() << std::endl;
        //        std::cout << "Finished processing: " << nexus.getOutpath() << std::endl;
    }

    return true;
}

} // namespace Processors
