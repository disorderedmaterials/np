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
    H5Dread(totalCountsID.getId(), H5T_STD_I64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, totalCountsBuffer.data());
    totalEvents_ = totalCountsBuffer[0];

    // Read in good frames
    auto &&[goodFramesID, goodFramesDimension] = NeXuSFile::get1DDataset(fileHandle_, "raw_data_1", "good_frames");
    std::array<int, 1> goodFramesTemp;
    H5Dread(goodFramesID.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, goodFramesTemp.data());
    totalFrames_ = goodFramesTemp[0];


    // Read in frame offsets.
    auto &&[frameZeroID, frameZerosDimension] =
    NeXuSFile::get1DDataset(fileHandle_, "raw_data_1/detector_1_events", "event_time_zero");
    frameZero_.resize(frameZerosDimension);
    H5Dread(frameZeroID.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, frameZero_.data());

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

    // Get dataset handles
    auto &&[indicesDataset, indicesDimension] = NeXuSFile::get1DDataset(fileHandle_, "raw_data_1/detector_1_events", "event_id");
    auto &&[timesDataset, timesDimension] = NeXuSFile::get1DDataset(fileHandle_, "raw_data_1/detector_1_events", "event_time_offset");

    // Read in events for the current frame
    auto nextFrameLimit = std::min(nextFrameIndex_ + frameChunkSize_, totalFrames_);
    for (auto i = nextFrameIndex_; i < nextFrameLimit; ++i)
    {
        // fmt::print("Reading frame {}...\n", i);
        // Push a new frame data
        auto &frame = frameData_.emplace_back();
        frame.index = i;
        frame.timeZero = frameZero_[i];

        // Set vector sizes
        frame.detectorIndices.resize(eventsPerFrame_[i]);
        frame.times.resize(eventsPerFrame_[i]);

        // Select and read in only events for this frame
        H5::DataSpace indicesSpace = indicesDataset.getSpace();
        // fmt::print("NSELECTED POINTS IN FILE DATASET AT START = {}\n", space.getSelectNpoints());
        std::array<hsize_t, 1> start = {(hsize_t)frameFirstIndices_[i] }, stride = {1}, block  = { (hsize_t)eventsPerFrame_[i]}, count = {1};
        // std::array<hsize_t, 1> start = {0}, stride = {1}, count  = {1}, block = {20};
        indicesSpace.selectHyperslab(H5S_SELECT_SET, count.data(), start.data(), stride.data(), block.data());
            H5::DataSpace timesSpace = timesDataset.getSpace();
        // fmt::print("NSELECTED POINTS IN FILE DATASET AT START = {}\n", space.getSelectNpoints());
        // std::array<hsize_t, 1> start = {0}, stride = {1}, count  = {1}, block = {20};
        timesSpace.selectHyperslab(H5S_SELECT_SET, count.data(), start.data(), stride.data(), block.data());
        // fmt::print("NSELECTED POINTS IN FILE DATASET = {}\n", space.getSelectNpoints());
        auto memSpace = H5Screate_simple(1, block.data(), block.data());

        // fmt::print("NSELECTED POINTS IN MEMSPACE = {}\n", H5Sget_select_npoints(memSpace));
        H5Dread(indicesDataset.getId(), H5T_STD_I32LE, memSpace, indicesSpace.getId(), H5P_DEFAULT, frame.detectorIndices.data());
        H5Dread(timesDataset.getId(), H5T_IEEE_F64LE, memSpace, timesSpace.getId(), H5P_DEFAULT, frame.times.data());

        // for (auto idx : frame.detectorIndices)
            // fmt::print("{}\n", idx);

        //
        //     // Read in events.

        //     eventTimes_.resize(eventTimesDimension);
        //     H5Dread(eventTimesID.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, eventTimes_.data());
    }

    // Set frame index for next call
    nextFrameIndex_ = nextFrameLimit;

    return true;
}

// Return current frame data
std::vector<FrameData> &EventChunker::frameData() { return frameData_; }