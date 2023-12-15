#pragma once

#include "pulse.h"
#include <H5Cpp.h>
#include <gsl/gsl_histogram.h>
#include <map>
#include <string>
#include <vector>

class NeXuSFile
{
    public:
    NeXuSFile(std::string path_, std::string outpath_);
    NeXuSFile(std::string path_);
    NeXuSFile(std::string referenceFileName, std::string newFileName, const std::vector<std::string> &pathsToCopy);
    NeXuSFile() = default;

    private:
    const H5std_string path;
    const H5std_string outpath;
    bool getLeafDataset(H5::H5File file, std::vector<H5std_string> terminals, H5std_string dataset, H5::DataSet &out);

    std::vector<int> spectra_;
    int nRawFrames_{0};
    int totalGoodFrames_{0};
    int nProcessedGoodFrames_{0};
    int startSinceEpoch_{0};
    int endSinceEpoch_{0};
    std::vector<int> eventIndices_;
    std::vector<double> events_;
    std::vector<int> eventsPerFrame_;
    std::vector<double> frameOffsets_;
    std::vector<double> ranges_;
    std::map<int, std::vector<int>> monitorCounts_;
    std::map<unsigned int, gsl_histogram *> detectorHistograms_;
    std::map<unsigned int, std::vector<double>> partitions_;

    public:
    [[nodiscard]] int totalGoodFrames() const;
    [[nodiscard]] int &nProcessedGoodFrames();
    [[nodiscard]] int startSinceEpoch() const;
    [[nodiscard]] int endSinceEpoch() const;
    [[nodiscard]] const std::vector<int> &eventIndices() const;
    [[nodiscard]] const std::vector<double> &events() const;
    [[nodiscard]] const std::vector<int> &eventsPerFrame() const;
    [[nodiscard]] const std::vector<double> &frameOffsets() const;
    [[nodiscard]] const std::vector<double> &ranges() const;
    [[nodiscard]] const std::map<int, std::vector<int>> &monitorCounts() const;
    std::map<unsigned int, gsl_histogram *> &detectorHistograms();
    [[nodiscard]] const std::map<unsigned int, std::vector<double>> &partitions() const;

    bool load(bool advanced = false);
    int createHistogram(Pulse &pulse, int epochOffset = 0);
    int createHistogram(Pulse &pulse, std::map<unsigned int, gsl_histogram *> &mask, int epochOffset = 0);
    int binPulseEvents(Pulse &pulse, int epochOffset, NeXuSFile &destination);
    void addMonitors(double scale, NeXuSFile &destination);
    std::string getOutpath();
    bool output(std::vector<std::string> paths);
    bool copy();
    bool copy(H5::H5File in, H5::H5File out, std::vector<std::string> paths);
    bool createEmpty(std::vector<std::string> pathss);
    bool reduceMonitors(double scale);
    bool writePartitionsWithRelativeTimes(unsigned int lowerSpec, unsigned int upperSpec);
};
