#pragma once

#include "nexusFile.h"
#include <vector>

struct FrameData
{
    // Event data:
    // Detector index(detector_1_events/event_id) - detector indices (M+1 -> N+M+1) of events
    // Event time (detector_1_events/event_time_offset) - times, in us relative to frame start, of events
    using EventData = std::tuple<long long, double>;

    // Index of this frame
    int frameIndex;
    std::vector<EventData> events;
};

class EventChunker
{
    public:
    EventChunker(NeXuSFile &source);
    ~EventChunker() = default;

    /*
     * File and Basic Info
     */
    private:
    // Source NeXuS file
    NeXuSFile &neXuSFile_;
    // HDF5 file handle
    H5::H5File fileHandle_;
    // Total number of events
    long long totalEvents_;
    // First event index of each frame (detector_1_events/event_index)
    std::vector<int> frameFirstIndices_;
    // Number of events per frame (framelog/events_log/value)
    std::vector<int> eventsPerFrame_;
    // Total number of frames
    int totalFrames_{0};
    // Frame chunk size
    const int frameChunkSize_{1024};

    /*
     * Current Frame Chunk
     */
    private:
    // Next frame chunk to distil into frame data
    int nextFrameIndex_{0};
    // Current chunk of frames
    std::vector<FrameData> frameData_;

    public:
    // Move to next frame data
    bool getNextFrameData();
    // Return current frame data
    std::vector<FrameData> &frameData();
};
