#pragma once

#include "histogram.h"
#include <H5Cpp.h>
#include <map>
#include <optional>
#include <string>
#include <vector>

class NeXuSFile
{
    public:
    NeXuSFile(std::string filename = "", bool printInfo = false);
    void operator=(NeXuSFile &source);
    NeXuSFile(const NeXuSFile &source);
    NeXuSFile(NeXuSFile &&source);
    ~NeXuSFile();
    // Clear all data and arrays
    void clear();
    // Copy data from specified source
    void copy(const NeXuSFile &source, bool deepCopyHistograms = true);

    /*
     * I/O
     */
    private:
    // Filename
    std::string filename_;

    public:
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
