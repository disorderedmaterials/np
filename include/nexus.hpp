#ifndef NEXUS_H
#define NEXUS_H

#include <string>
#include <vector>
#include <map>
#include <H5Cpp.h>
#include <gsl/gsl_histogram.h>

#include "pulse.hpp"

class Nexus {

    private:
        const H5std_string path;
        const H5std_string outpath;
        bool getLeafDataset(H5::H5File file, std::vector<H5std_string> terminals, H5std_string dataset, H5::DataSet &out);

    public:
        Nexus(std::string path_, std::string outpath_) : path(path_), outpath(outpath_) {}
        Nexus(std::string path_) : path(path_) {}
        Nexus() = default;

        std::vector<int> spectra;
        int* rawFrames;
        int goodFrames;
        int startSinceEpoch;
        int endSinceEpoch;

        std::vector<int> eventIndices;
        std::vector<double> events;
        std::vector<int> frameIndices;
        std::vector<double> frameOffsets;
        std::vector<double> ranges;
        std::map<int, std::vector<int>> monitors;
        std::map<unsigned int, gsl_histogram*> histogram;
        std::map<unsigned int, std::vector<double>> partitions;

        bool load(bool advanced = false);
        bool createHistogram(Pulse &pulse, int epochOffset=0);
        bool createHistogram(Pulse &pulse, std::map<unsigned int, gsl_histogram*> &mask, int epochOffset=0);
        void binPulseEvents(Pulse &pulse, int epochOffset, Nexus &destination);
        void addMonitors(int nGoodFrames, Nexus &destination);
        bool output(std::vector<std::string> paths);
        bool copy();
        bool copy(H5::H5File in, H5::H5File out, std::vector<std::string> paths);
        bool createEmpty(std::vector<std::string> pathss);
        bool writeCounts(H5::H5File output);
        bool writeTotalFrames(H5::H5File output, int frames);
        bool writeGoodFrames(H5::H5File output, int goodFrames);
        bool reduceMonitors(double scale);
        bool writeMonitors(H5::H5File output, std::map<int, std::vector<int>> monitors);
        bool writePartitionsWithRelativeTimes(unsigned int lowerSpec, unsigned int upperSpec);

};


#endif // NEXUS_H