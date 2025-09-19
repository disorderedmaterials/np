#pragma once

#include <H5Cpp.h>
#include <vector>

class EventParser
{
    public:
    EventParser(NeXuSFile &source);
    ~EventParser();

    /*
     * I/O
     */
    private:
    // Source NeXuS file
    NeXuSFile &neXuSFile_;

    private:
    // Return handle and (simple) dimension for named leaf dataset, resizing if requested
    static std::pair<H5::DataSet, long int> get1DDataset(H5::H5File file, H5std_string terminal, H5std_string datasetName);
    // Resize 1D dataset
    static void resize1DDataset(H5::DataSet dataset, std::vector<hsize_t> dimensions);

    public:
    // Whether verbose output is enabled
    static bool verbose;

    public:
    // Return filename
    std::string filename() const;
    // Load basic information from the NeXuS file
    void loadBasicData(bool printInfo = false);
    // Template a new NeXusFile from that specified
    static void templateTo(std::string sourceFilename, std::string newFilename);
    // Prepare spectra storage, including loading TOF boundaries etc.
    void prepareSpectraSpace(bool printInfo = false);
    // Load in monitor histograms
    void loadMonitorCounts();
    // Load event data
    void loadEventData();
    // Load detector counts from the file
    void loadDetectorCounts();
    // Save key modified data back to the file
    bool saveModifiedData();

    /*
     * Data
     */
    private:
    /*
     * Detector spectrum indices (detector_1/spectrum_index)
     * These are detector, not monitor, indices, with indices running from M+1 -> N+M+1 where M is the
     * number of monitors and N is the number of detectors. If there were no detectors the range would
     * therefore be 1 -> N, for NIMROD's 9 monitors the first detector is 10, etc. The ordering of the
     * indices (which is normally continuous?) reflects the order of data in the detector_1/counts
     * array, while the indices themselves are used as keys for per-detector lookup in the
     * detectorHistograms_ map.
     */
    std::vector<unsigned int> detectorSpectrumIndices_;
    // Number of monitor spectra, determined from the first index in detectorSpectrumIndices_
    int nMonitorSpectra_{0};
    // Number of frames contributing to monitor counts
    int nMonitorFrames_{0};
    // Number of frames contributing to detector counts
    int nGoodFrames_{0};
    // Start time in seconds since epoch (raw_data_1/start_time)
    int startSinceEpoch_{0};
    // End time in seconds since epoch (raw_data_1/end_time)
    int endSinceEpoch_{0};
    // Event indices (detector_1_events/event_id) - detector indices (M+1 -> N+M+1) of events
    std::vector<long long> eventIndices_;
    // Event times (detector_1_events/event_time_offset) - times, in us relative to frame start, of events
    std::vector<double> eventTimes_;
    // Number of events per frame (framelog/events_log/value)
    std::vector<int> eventsPerFrame_;
    // Frame start times (detector_1_events/event_time_zero) - seconds, relative to start time since epoch
    std::vector<double> frameOffsets_;
    // Time-of-flight bin boundaries in detector histograms (detector_1/time_of_flight) - us
    std::vector<double> tofBoundaries_;
    // Monitor counts (TOF bins), mapped by monitor index (monitor_M/data)
    std::map<unsigned int, std::vector<long int>> monitorCounts_;
    // Detector counts (TOF bins), mapped by detector index (M+1 -> N+M+1) (detector_1/counts)
    std::map<unsigned int, IntegerHistogram> detectorHistograms_;

    public:
    [[nodiscard]] int nGoodFrames() const;
    void zeroGoodFrames();
    void incrementGoodFrames(int delta = 1);
    [[nodiscard]] int nMonitorFrames() const;
    [[nodiscard]] int startSinceEpoch() const;
    [[nodiscard]] int endSinceEpoch() const;
    [[nodiscard]] const std::vector<long long> &eventIndices() const;
    [[nodiscard]] const std::vector<double> &eventTimes() const;
    [[nodiscard]] const std::vector<int> &eventsPerFrame() const;
    [[nodiscard]] const std::vector<double> &frameOffsets() const;
    [[nodiscard]] const std::vector<double> &tofBoundaries() const;
    [[nodiscard]] const int spectrumForDetector(int detectorId) const;
    [[nodiscard]] const int nDetectors() const;
    [[nodiscard]] const std::map<unsigned int, std::vector<long int>> &monitorCounts() const;
    [[nodiscard]] const std::map<unsigned int, std::vector<long int>> &detectorCounts() const;
    std::map<unsigned int, IntegerHistogram> &detectorHistograms();

    /*
     * Manipulation
     */
    public:
    // Remove the last detector spectrum, returning the index that was removed
    int removeLastDetector();
    // Append an empty detector
    int appendEmptyDetector(int specID = -1);
    // Scale monitors by specified factor
    void scaleMonitors(double factor);
    // Scale detectors by specified factor
    void scaleDetectors(double factor);
};
