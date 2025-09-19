#include "eventChunker.h"
#include <fmt/core.h>
#include <array>

EventChunker::EventChunker(NeXuSFile &source) : neXuSFile_(source)
{
    // Initialise frame chunk storage
    frameData_.reserve(frameChunkSize_);
    nextFrameIndex_ = 0;

    // Open input NeXuS file in read only mode.
    fileHandle_ = H5::H5File(neXuSFile_.filename(), H5F_ACC_RDONLY);

    // Get event counts per frame
    auto &&[eventsPerFrameID, eventsPerFrameDimension] =
        NeXuSFile::get1DDataset(fileHandle_, "raw_data_1/framelog/events_log", "value");
    eventsPerFrame_.resize(eventsPerFrameDimension);
    H5Dread(eventsPerFrameID.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, eventsPerFrame_.data());

    // Get first event indices per frame
    auto &&[frameFirstIndices, frameFirstIndicesDimension] =
    NeXuSFile::get1DDataset(fileHandle_, "raw_data_1/detector_1_events", "event_index");
    frameFirstIndices_.resize(frameFirstIndicesDimension);
    H5Dread(frameFirstIndices.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, frameFirstIndices_.data());

    // Get total number of events
    auto &&[totalCountsID, totalCountsDimension] = NeXuSFile::get1DDataset(fileHandle_, "raw_data_1/detector_1_events", "total_counts");
    std::array<long long, 1> totalCountsBuffer;
    H5Dread(totalCountsID.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, totalCountsBuffer.data());
    totalEvents_ = totalCountsBuffer[0];

    // Read in good frames
    auto &&[goodFramesID, goodFramesDimension] = NeXuSFile::get1DDataset(fileHandle_, "raw_data_1", "good_frames");
    std::array<int, 1> goodFramesTemp;
    H5Dread(goodFramesID.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, goodFramesTemp.data());
    totalFrames_ = goodFramesTemp[0];

    fmt::print("There are {} total events.\n", totalEvents_);
}

// // Load event data
// void NeXuSFile::loadEventData()
// {
//     printf("Loading event data...\n");
//
//     // Open our NeXuS file in read only mode.
//     H5::H5File input = H5::H5File(filename_, H5F_ACC_RDONLY);
//
//     // Read in event indices.
//     auto &&[eventIndicesID, eventIndicesDimension] = NeXuSFile::get1DDataset(input, "raw_data_1/detector_1_events", "event_id");
//     eventIndices_.resize(eventIndicesDimension);
//     H5Dread(eventIndicesID.getId(), H5T_STD_I64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, eventIndices_.data());
//
//     // Read in events.
//     auto &&[eventTimesID, eventTimesDimension] =
//         NeXuSFile::get1DDataset(input, "raw_data_1/detector_1_events", "event_time_offset");
//     eventTimes_.resize(eventTimesDimension);
//     H5Dread(eventTimesID.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, eventTimes_.data());
//
//     input.close();
// }

/*
 * Current Frame Chunk
 */

// Move to next frame data
bool EventChunker::getNextFrameData()
{
    // If our next frame index is out of range, return false as we are done
    if (nextFrameIndex_ >= totalFrames_)
        return false;

    // Clear current data
    frameData_.clear();

    // Read in events for the current frame
    auto nextFrameLimit = std::min(nextFrameIndex_ + frameChunkSize_, totalFrames_);


    return true;
}

// Return current frame data
std::vector<FrameData> &frameData();