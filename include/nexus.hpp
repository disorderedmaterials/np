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
        H5::H5File file;
        bool getLeafDataset(H5::H5File file, std::vector<H5std_string> terminals, H5std_string dataset, H5::DataSet &out);

    public:
        Nexus(std::string path_, std::string outpath_) : path(path_), outpath(outpath_) {}
        Nexus(std::string path_) : path(path_) {}
        Nexus() = default;

        std::vector<int> spectra;
        int* rawFrames;
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
        bool output(std::vector<std::string> paths);
        bool output(std::vector<std::string> paths, int rawFrames, std::map<int, std::vector<int>> monitors);
        bool copy(H5::H5File in, H5::H5File out, std::vector<std::string> paths);
        bool writeCounts(H5::H5File output);
        bool writeGoodFrames(H5::H5File output, int goodFrames);
        bool writeMonitors(H5::H5File output, std::map<int, std::vector<int>> monitors);
        int countGoodFrames(Pulse &pulse, int epochOffset=0);

};


#endif // NEXUS_H