#pragma once

#include <H5Cpp.h>
#include <gsl/gsl_histogram.h>
#include <map>
#include <string>
#include <vector>

class NeXuSFile
{
    public:
    NeXuSFile(std::string filename = "");
    ~NeXuSFile() = default;

    /*
     * I/O
     */
    private:
    // Filename
    std::string filename_;

    private:
    // Return handle and (simple) dimension for named leaf dataset
    static std::pair<H5::DataSet, long int> find1DDataset(H5::H5File file, H5std_string terminal, H5std_string datasetName);

    public:
    // Return filename
    std::string filename() const;
    // Template basic paths from the referenceFile, and make ready for histogram binning
    void templateFile(std::string referenceFile, std::string outputFile);
    // Load frame counts
    void loadFrameCounts();
    // Load event data
    void loadEventData();
    // Load start/end times
    void loadTimes();
    // Save detector histograms back to the file
    bool saveDetectorHistograms();

    /*
     * Data
     */
    private:
    std::vector<int> spectra_;
    int nMonitorFrames_{0};
    int nDetectorFrames_{0};
    int nGoodFrames_{0};
    int startSinceEpoch_{0};
    int endSinceEpoch_{0};
    std::vector<int> eventIndices_;
    std::vector<double> eventTimes_;
    std::vector<int> eventsPerFrame_;
    std::vector<double> frameOffsets_;
    std::vector<double> tofBins_;
    std::map<int, std::vector<int>> monitorCounts_;
    std::map<unsigned int, gsl_histogram *> detectorHistograms_;
    std::map<unsigned int, std::vector<double>> partitions_;

    public:
    [[nodiscard]] int nGoodFrames() const;
    [[nodiscard]] int nMonitorFrames() const;
    [[nodiscard]] int nDetectorFrames() const;
    void incrementDetectorFrameCount(int delta = 1);
    [[nodiscard]] int startSinceEpoch() const;
    [[nodiscard]] int endSinceEpoch() const;
    [[nodiscard]] const std::vector<int> &eventIndices() const;
    [[nodiscard]] const std::vector<double> &eventTimes() const;
    [[nodiscard]] const std::vector<int> &eventsPerFrame() const;
    [[nodiscard]] const std::vector<double> &frameOffsets() const;
    [[nodiscard]] const std::vector<double> &tofBins() const;
    [[nodiscard]] const std::map<int, std::vector<int>> &monitorCounts() const;
    std::map<unsigned int, gsl_histogram *> &detectorHistograms();
    [[nodiscard]] const std::map<unsigned int, std::vector<double>> &partitions() const;

    /*
     * Manipulation
     */
    public:
    // Scale monitors by specified factor
    void scaleMonitors(double factor);
    // Scale detectors by specified factor
    void scaleDetectors(double factor);
};
