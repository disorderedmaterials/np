#ifndef NEXUS_H
#define NEXUS_H

#include <string>
#include <vector>
#include <map>
#include <H5Cpp.h>

class Nexus {

    private:
        const H5std_string path;
        const H5std_string outpath;
        H5::H5File file;

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
        std::vector<std::pair<double, double>> bins;
        std::map<unsigned int, std::vector<unsigned int>> histogram;
        std::map<unsigned int, std::vector<double>> partitions;

        bool parseNexusFile();
        bool loadBasicData();
        bool loadEventModeData();



        bool getLeafDataset(H5::H5File file, std::vector<H5std_string> terminals, H5std_string dataset, H5::DataSet &out);
        bool copy();
        bool writeCountsHistogram();

        bool partitionEvents();
        bool partitionEvents(std::vector<int> &spectra_);

        bool createHistogram();
        bool createHistogram(std::vector<std::pair<double, double>> bounds);
        bool createHistogram(std::pair<double, double> bounds);

        bool extractPulseTimes(int spectrum, std::vector<double> &pulses);
        bool extrapolatePulseTimes(double start, bool backwards, bool forwards, double step, double duration, std::vector<std::pair<double, double>> &pulses);

};


#endif